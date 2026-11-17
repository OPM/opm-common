// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
/*!
 * \file
 * \brief GPU-compatible, simplified version of Opm::EclMaterialLaw::Manager.
 *
 * This class only supports the actions needed by FlowProblem,
 * FlowProblemBlackoil and the BlackOilIntensiveQuantities. See the class
 * documentation of \c Opm::EclMaterialLaw::GpuManager for the supported
 * material laws and the data layout.
 */
#ifndef OPM_GPU_ECL_MATERIAL_LAW_MANAGER_HPP
#define OPM_GPU_ECL_MATERIAL_LAW_MANAGER_HPP

#include <opm/common/utility/VectorWithDefaultAllocator.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>
#include <opm/common/ErrorMacros.hpp>

#include <opm/material/fluidmatrixinteractions/EclDefaultMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/EclDefaultMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsScalingPoints.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsTwoPhaseLawParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclTwoPhaseMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/EclTwoPhaseMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/PiecewiseLinearTwoPhaseMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/SatCurveMultiplexerParams.hpp>

#include <cassert>
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm::EclMaterialLaw
{
template <class TraitsT,
          class GasOilLawT,
          class OilWaterLawT,
          template <class> class Storage = ::Opm::VectorWithDefaultAllocator,
          class MaterialLawT = ::Opm::EclDefaultMaterial<TraitsT, GasOilLawT, OilWaterLawT>>
class GpuManager;
} // namespace Opm::EclMaterialLaw

#if HAVE_CUDA
namespace Opm::gpuistl
{
template <class TraitsT, class GasOilLawT, class OilWaterLawT, class MaterialLawT>
::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuBuffer, MaterialLawT>
copy_to_gpu(const ::Opm::EclMaterialLaw::GpuManager<TraitsT,
                                                    GasOilLawT,
                                                    OilWaterLawT,
                                                    ::Opm::VectorWithDefaultAllocator,
                                                    MaterialLawT>& cpu);

template <class TraitsT, class GasOilLawT, class OilWaterLawT, class MaterialLawT>
::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuView, MaterialLawT>
make_view(
    ::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuBuffer, MaterialLawT>&
        buf);
} // namespace Opm::gpuistl
#endif // HAVE_CUDA

namespace Opm::EclMaterialLaw
{

namespace detail
{
    template <class T>
    struct IsGpuManager : std::false_type {
    };

    template <class TraitsT,
              class GasOilLawT,
              class OilWaterLawT,
              template <class> class Storage,
              class MaterialLawT>
    struct IsGpuManager<GpuManager<TraitsT, GasOilLawT, OilWaterLawT, Storage, MaterialLawT>>
        : std::true_type {
    };


    /*!
     * \brief Walk down a CPU material-law parameter object until the enclosed
     *        \c PiecewiseLinearTwoPhaseMaterialParams is reached.
     *
     * Supports the standard layering used by the CPU \c EclMaterialLaw::Manager:
     *   \c EclHysteresisTwoPhaseLawParams \f$\to\f$ \c EclEpsTwoPhaseLawParams
     *   \f$\to\f$ \c SatCurveMultiplexerParams
     *   \f$\to\f$ \c PiecewiseLinearTwoPhaseMaterialParams.
     *
     * The hysteresis and endpoint-scaling layers are skipped without looking
     * at their configuration. This is only correct after
     * \c GpuManager::validate() has accepted the CPU manager.
     */
    template <class CpuParams>
    const auto& extractCpuPlParams(const CpuParams& params)
    {
        if constexpr (requires { params.drainageParams(); }) {
            return extractCpuPlParams(params.drainageParams());
        } else if constexpr (requires { params.effectiveLawParams(); }) {
            return extractCpuPlParams(params.effectiveLawParams());
        } else if constexpr (requires {
                                 params.template getRealParams<
                                     ::Opm::SatCurveMultiplexerApproach::PiecewiseLinear>();
                             }) {
            OPM_ERROR_IF(params.approach()
                != ::Opm::SatCurveMultiplexerApproach::PiecewiseLinear,
                "GPU material manager only supports piecewise-linear saturation curves");
            return params
                .template getRealParams<::Opm::SatCurveMultiplexerApproach::PiecewiseLinear>();
        } else {
            return params;
        }
    }

    /*!
     * \brief Unwrap the hysteresis layer of a CPU two-phase parameter object,
     *        if there is one. The result is the endpoint-scaling parameter
     *        object when the CPU traits enable endpoint scaling.
     *
     * Only the drainage curve is used, so this assumes that
     * \c GpuManager::validate() has rejected hysteresis.
     */
    template <class CpuParams>
    const auto& extractCpuEpsParams(const CpuParams& params)
    {
        if constexpr (requires { params.drainageParams(); }) {
            return params.drainageParams();
        } else {
            return params;
        }
    }

    inline bool sameEpsConfig(const ::Opm::EclEpsConfig& lhs, const ::Opm::EclEpsConfig& rhs)
    {
        return lhs.enableSatScaling() == rhs.enableSatScaling()
            && lhs.enableThreePointKrSatScaling() == rhs.enableThreePointKrSatScaling()
            && lhs.enablePcScaling() == rhs.enablePcScaling()
            && lhs.enableLeverettScaling() == rhs.enableLeverettScaling()
            && lhs.enableKrwScaling() == rhs.enableKrwScaling()
            && lhs.enableKrnScaling() == rhs.enableKrnScaling()
            && lhs.enableThreePointKrwScaling() == rhs.enableThreePointKrwScaling()
            && lhs.enableThreePointKrnScaling() == rhs.enableThreePointKrnScaling();
    }

    /*!
     * \brief Whether a GPU two-phase law has the endpoint-scaling layer, and
     *        the law below it.
     *
     * Only \c EclEpsTwoPhaseLaw with its default parameter type counts, since
     * \c GpuManager assembles those parameters itself.
     */
    template <class Law>
    struct EpsLayer {
        static constexpr bool present = false;
        using EffectiveLaw = Law;
    };

    template <class EffLaw>
    struct EpsLayer<::Opm::EclEpsTwoPhaseLaw<EffLaw, ::Opm::EclEpsTwoPhaseLawParams<EffLaw>>> {
        static constexpr bool present = true;
        using EffectiveLaw = EffLaw;
    };

    /*!
     * \brief Per-cell scaled endpoints of one two-phase system, one array per
     *        value.
     *
     * Holds the values of \c EclEpsScalingPoints that the endpoint-scaling law
     * reads for saturation and capillary-pressure scaling. The kr value and
     * Leverett scaling values are not stored; \c GpuManager::validate()
     * rejects those options.
     */
    template <template <class> class Storage, class Scalar>
    struct EpsScaledPointsArrays {
        Storage<Scalar> saturationPcPoint0{};
        Storage<Scalar> saturationPcPoint1{};
        Storage<Scalar> saturationPcPoint2{};
        Storage<Scalar> saturationKrwPoint0{};
        Storage<Scalar> saturationKrwPoint1{};
        Storage<Scalar> saturationKrwPoint2{};
        Storage<Scalar> saturationKrnPoint0{};
        Storage<Scalar> saturationKrnPoint1{};
        Storage<Scalar> saturationKrnPoint2{};
        //! Empty unless capillary-pressure scaling is enabled.
        Storage<Scalar> maxPcnw{};

        /*!
         * \brief The scaled points of one cell. Fields the configuration does
         *        not use are left at zero.
         */
        OPM_HOST_DEVICE ::Opm::EclEpsScalingPoints<Scalar>
        load(unsigned elemIdx, const ::Opm::EclEpsConfig& config) const
        {
            ::Opm::EclEpsScalingPoints<Scalar> points;
            if (config.enableSatScaling()) {
                points.setSaturationPcPoint(0, saturationPcPoint0[elemIdx]);
                points.setSaturationPcPoint(1, saturationPcPoint1[elemIdx]);
                points.setSaturationPcPoint(2, saturationPcPoint2[elemIdx]);
                points.setSaturationKrwPoint(0, saturationKrwPoint0[elemIdx]);
                points.setSaturationKrwPoint(1, saturationKrwPoint1[elemIdx]);
                points.setSaturationKrwPoint(2, saturationKrwPoint2[elemIdx]);
                points.setSaturationKrnPoint(0, saturationKrnPoint0[elemIdx]);
                points.setSaturationKrnPoint(1, saturationKrnPoint1[elemIdx]);
                points.setSaturationKrnPoint(2, saturationKrnPoint2[elemIdx]);
            }
            if (config.enablePcScaling()) {
                points.setMaxPcnw(maxPcnw[elemIdx]);
            }
            return points;
        }

        /*!
         * \brief Append one cell's points (host vectors only).
         */
        void pushBack(const ::Opm::EclEpsScalingPoints<Scalar>& points,
                      const ::Opm::EclEpsConfig& config)
        {
            saturationPcPoint0.push_back(points.saturationPcPoints()[0]);
            saturationPcPoint1.push_back(points.saturationPcPoints()[1]);
            saturationPcPoint2.push_back(points.saturationPcPoints()[2]);
            saturationKrwPoint0.push_back(points.saturationKrwPoints()[0]);
            saturationKrwPoint1.push_back(points.saturationKrwPoints()[1]);
            saturationKrwPoint2.push_back(points.saturationKrwPoints()[2]);
            saturationKrnPoint0.push_back(points.saturationKrnPoints()[0]);
            saturationKrnPoint1.push_back(points.saturationKrnPoints()[1]);
            saturationKrnPoint2.push_back(points.saturationKrnPoints()[2]);
            if (config.enablePcScaling()) {
                maxPcnw.push_back(points.maxPcnw());
            }
        }
    };

    /*!
     * \brief Apply \p f to every array of \p src.
     */
    template <template <class> class ToStorage, class Scalar, class Src, class F>
    EpsScaledPointsArrays<ToStorage, Scalar> transformArrays(Src& src, F f)
    {
        EpsScaledPointsArrays<ToStorage, Scalar> dst;
        dst.saturationPcPoint0 = f(src.saturationPcPoint0);
        dst.saturationPcPoint1 = f(src.saturationPcPoint1);
        dst.saturationPcPoint2 = f(src.saturationPcPoint2);
        dst.saturationKrwPoint0 = f(src.saturationKrwPoint0);
        dst.saturationKrwPoint1 = f(src.saturationKrwPoint1);
        dst.saturationKrwPoint2 = f(src.saturationKrwPoint2);
        dst.saturationKrnPoint0 = f(src.saturationKrnPoint0);
        dst.saturationKrnPoint1 = f(src.saturationKrnPoint1);
        dst.saturationKrnPoint2 = f(src.saturationKrnPoint2);
        dst.maxPcnw = f(src.maxPcnw);
        return dst;
    }

    /*!
     * \brief Storage of the three-phase (\c EclDefaultMaterial) path.
     *
     * Tables and unscaled points per SATNUM region, scaled endpoints and Swl
     * per cell. The per-cell arrays are empty when the deck has no endpoint
     * scaling; Swl then comes from \c swlByRegion. Nothing here holds a
     * pointer: \c GpuManager::materialLawParams() takes the addresses of the
     * region entries from the current storage.
     */
    template <template <class> class Storage,
              class Scalar,
              class GasOilEffectiveParams,
              class OilWaterEffectiveParams>
    struct GpuThreePhaseStorage {
        Storage<int> satnumRegionArray{};

        Storage<GasOilEffectiveParams> gasOilEffectiveParams{};
        Storage<OilWaterEffectiveParams> oilWaterEffectiveParams{};
        Storage<::Opm::EclEpsScalingPoints<Scalar>> gasOilUnscaledPoints{};
        Storage<::Opm::EclEpsScalingPoints<Scalar>> oilWaterUnscaledPoints{};
        Storage<Scalar> swlByRegion{};

        EpsScaledPointsArrays<Storage, Scalar> gasOilScaledPoints{};
        EpsScaledPointsArrays<Storage, Scalar> oilWaterScaledPoints{};
        Storage<Scalar> swlByCell{};

        ::Opm::EclEpsConfig gasOilConfig{};
        ::Opm::EclEpsConfig oilWaterConfig{};
        bool hasPerCellEndpoints = false;

        std::size_t numRegions() const
        {
            return gasOilEffectiveParams.size();
        }
    };

    /*!
     * \brief Storage of the two-phase (\c EclTwoPhaseMaterial) path: one
     *        parameter object per SATNUM region.
     */
    template <template <class> class Storage, class MaterialLawParams>
    struct GpuTwoPhaseStorage {
        Storage<int> satnumRegionArray{};
        Storage<MaterialLawParams> materialLawParamsByRegion{};

        std::size_t numRegions() const
        {
            return materialLawParamsByRegion.size();
        }
    };

#if HAVE_CUDA
    template <class Scalar, class GasOilEffectiveParams, class OilWaterEffectiveParams>
    GpuThreePhaseStorage<::Opm::gpuistl::GpuView, Scalar, GasOilEffectiveParams, OilWaterEffectiveParams>
    makeStorageView(GpuThreePhaseStorage<::Opm::gpuistl::GpuBuffer,
                                         Scalar,
                                         GasOilEffectiveParams,
                                         OilWaterEffectiveParams>& buf)
    {
        auto toView = [](auto& buffer) { return ::Opm::gpuistl::make_view(buffer); };
        GpuThreePhaseStorage<::Opm::gpuistl::GpuView, Scalar, GasOilEffectiveParams, OilWaterEffectiveParams>
            view;
        view.satnumRegionArray = toView(buf.satnumRegionArray);
        view.gasOilEffectiveParams = toView(buf.gasOilEffectiveParams);
        view.oilWaterEffectiveParams = toView(buf.oilWaterEffectiveParams);
        view.gasOilUnscaledPoints = toView(buf.gasOilUnscaledPoints);
        view.oilWaterUnscaledPoints = toView(buf.oilWaterUnscaledPoints);
        view.swlByRegion = toView(buf.swlByRegion);
        view.gasOilScaledPoints
            = transformArrays<::Opm::gpuistl::GpuView, Scalar>(buf.gasOilScaledPoints, toView);
        view.oilWaterScaledPoints
            = transformArrays<::Opm::gpuistl::GpuView, Scalar>(buf.oilWaterScaledPoints, toView);
        view.swlByCell = toView(buf.swlByCell);
        view.gasOilConfig = buf.gasOilConfig;
        view.oilWaterConfig = buf.oilWaterConfig;
        view.hasPerCellEndpoints = buf.hasPerCellEndpoints;
        return view;
    }

    template <class MaterialLawParams>
    GpuTwoPhaseStorage<::Opm::gpuistl::GpuView, MaterialLawParams>
    makeStorageView(GpuTwoPhaseStorage<::Opm::gpuistl::GpuBuffer, MaterialLawParams>& buf)
    {
        GpuTwoPhaseStorage<::Opm::gpuistl::GpuView, MaterialLawParams> view;
        view.satnumRegionArray = ::Opm::gpuistl::make_view(buf.satnumRegionArray);
        view.materialLawParamsByRegion = ::Opm::gpuistl::make_view(buf.materialLawParamsByRegion);
        return view;
    }

    /*!
     * \brief Upload a host vector; an empty vector gives an empty buffer.
     */
    template <class T>
    ::Opm::gpuistl::GpuBuffer<T> toGpuBuffer(const std::vector<T>& host)
    {
        return host.empty() ? ::Opm::gpuistl::GpuBuffer<T>() : ::Opm::gpuistl::GpuBuffer<T>(host);
    }
#endif

    /*!
     * \brief Optional host-side base used by \c GpuManager when its storage is
     *        owning device memory: keeps the piecewise-linear sample buffers
     *        alive so that the \c GpuView pointers stored inside the region
     *        parameters remain valid for as long as the manager exists. Empty
     *        for any non-owning storage so that \c GpuView based managers stay
     *        device-trivially-copyable.
     */
    template <class Scalar, bool Owning>
    struct GpuMaterialLawSampleBuffers {
    };

#if HAVE_CUDA
    template <class Scalar>
    struct GpuMaterialLawSampleBuffers<Scalar, true> {
        std::vector<::Opm::gpuistl::GpuBuffer<Scalar>> sampleBuffers {};
    };
#endif

} // namespace detail

/*!
 * \brief A minimal, GPU-compatible material-law manager.
 *
 * Supports two material laws:
 * - the three-phase \c EclDefaultMaterial, with or without endpoint scaling.
 *   \c GasOilLawT and \c OilWaterLawT are either piecewise-linear laws, or
 *   \c EclEpsTwoPhaseLaw over piecewise-linear laws;
 * - the two-phase \c EclTwoPhaseMaterial (GasWater only, as used by
 *   CO2STORE), without endpoint scaling.
 *
 * The CPU manager's hysteresis layer and \c SatCurveMultiplexer are not
 * reproduced; \c validate() rejects the options that would need them.
 *
 * Data layout of the three-phase path (struct of arrays):
 * - per SATNUM region: the piecewise-linear parameters, the unscaled
 *   endpoints of each system, and Swl when the deck has no endpoint scaling;
 * - per cell: SATNUM, and with endpoint scaling the scaled saturation
 *   endpoints (and the scaled maximum capillary pressure with Pc scaling) of
 *   each system plus Swl, each value in its own array;
 * - the endpoint-scaling configuration of each system once.
 * \c materialLawParams() assembles the \c EclDefaultMaterialParams of a cell
 * from these and returns it by value. Without endpoint scaling the per-cell
 * arrays are empty, and a cell reads only SATNUM and its region's data.
 *
 * The manager is a snapshot of the CPU manager at construction. Later changes
 * to the CPU manager, such as \c applySwatinit() or
 * \c applyRestartSwatInit(), are not seen.
 *
 * \tparam TraitsT       Three-phase material traits.
 * \tparam GasOilLawT    Two-phase gas/oil law type (must implement the
 *                       saturation-only API).
 * \tparam OilWaterLawT  Two-phase oil/water law type (must implement the
 *                       saturation-only API).
 * \tparam Storage       Storage container template; defaults to a CPU vector.
 *                       Use \c Opm::gpuistl::GpuBuffer for owning GPU storage
 *                       and \c Opm::gpuistl::GpuView for non-owning GPU
 *                       storage.
 * \tparam MaterialLawT  The three-phase material-law type to use; defaults to
 *                       \c Opm::EclDefaultMaterial.
 */
template <class TraitsT,
          class GasOilLawT,
          class OilWaterLawT,
          template <class> class Storage,
          class MaterialLawT>
class GpuManager : private detail::GpuMaterialLawSampleBuffers<
                       typename TraitsT::Scalar,
                       std::is_same_v<Storage<int>, ::Opm::gpuistl::GpuBuffer<int>>>
{
public:
    using Traits = TraitsT;
    using Scalar = typename Traits::Scalar;
    using GasOilLaw = GasOilLawT;
    using OilWaterLaw = OilWaterLawT;
    using GasOilParams = typename GasOilLaw::Params;
    using OilWaterParams = typename OilWaterLaw::Params;

    /*!
     * \brief The actual material-law type used by the IQ.
     *
     * Defaults to the three-phase \c EclDefaultMaterial. Pass an
     * \c Opm::EclTwoPhaseMaterial<...> instantiation (with a value-storage
     * \c EclTwoPhaseMaterialParams, e.g. one using
     * \c Opm::gpuistl::ValueAsPointer) to instead use the two-phase law.
     * Only the GasWater sub-approach is currently supported by the
     * from-CPU constructors below; this matches the CO2STORE setup.
     */
    using MaterialLaw = MaterialLawT;
    using MaterialLawParams = typename MaterialLaw::Params;

    static constexpr bool isOwningGpu
        = std::is_same_v<Storage<int>, ::Opm::gpuistl::GpuBuffer<int>>;

    static constexpr int waterPhaseIdx = Traits::wettingPhaseIdx;
    static constexpr int oilPhaseIdx = Traits::nonWettingPhaseIdx;
    static constexpr int gasPhaseIdx = Traits::gasPhaseIdx;
    static constexpr int numPhases = Traits::numPhases;

private:
    /*! \brief Trait: true if the material-law parameters are an
     *         \c EclTwoPhaseMaterialParams (i.e. expose a nested
     *         \c GasWaterParams type). */
    template <class T, class = void>
    struct IsTwoPhaseMaterialLawParams : std::false_type {
    };

    template <class T>
    struct IsTwoPhaseMaterialLawParams<T, std::void_t<typename T::GasWaterParams>>
        : std::true_type {
    };

    static constexpr bool isTwoPhase = IsTwoPhaseMaterialLawParams<MaterialLawParams>::value;

    //! Whether the two-phase laws are \c EclEpsTwoPhaseLaw (three-phase path only).
    static constexpr bool hasEpsLayer = detail::EpsLayer<GasOilLawT>::present;
    static_assert(hasEpsLayer == detail::EpsLayer<OilWaterLawT>::present,
                  "GpuManager: the gas-oil and oil-water laws must either both or neither "
                  "have the endpoint-scaling layer");
    static_assert(!(isTwoPhase && hasEpsLayer),
                  "GpuManager: the two-phase path does not support endpoint scaling");

    using GasOilEffectiveParams = typename detail::EpsLayer<GasOilLawT>::EffectiveLaw::Params;
    using OilWaterEffectiveParams = typename detail::EpsLayer<OilWaterLawT>::EffectiveLaw::Params;

    template <template <class> class S>
    using StorageFor = std::conditional_t<
        isTwoPhase,
        detail::GpuTwoPhaseStorage<S, MaterialLawParams>,
        detail::GpuThreePhaseStorage<S, Scalar, GasOilEffectiveParams, OilWaterEffectiveParams>>;

    using StorageType = StorageFor<Storage>;

    explicit GpuManager(StorageType storage)
        : storage_(std::move(storage))
    {
    }

public:
    GpuManager() = default;

    /*! \brief Material-law parameters of an active cell. */
    OPM_HOST_DEVICE MaterialLawParams materialLawParams(unsigned elemIdx) const
    {
        const auto& s = storage_;
        const int region = s.satnumRegionArray[elemIdx];
        if constexpr (isTwoPhase) {
            return s.materialLawParamsByRegion[region];
        } else if constexpr (hasEpsLayer) {
            return MaterialLawParams(
                GasOilParams(&s.gasOilEffectiveParams[region],
                             &s.gasOilUnscaledPoints[region],
                             s.gasOilConfig,
                             s.gasOilScaledPoints.load(elemIdx, s.gasOilConfig)),
                OilWaterParams(&s.oilWaterEffectiveParams[region],
                               &s.oilWaterUnscaledPoints[region],
                               s.oilWaterConfig,
                               s.oilWaterScaledPoints.load(elemIdx, s.oilWaterConfig)),
                s.hasPerCellEndpoints ? s.swlByCell[elemIdx] : s.swlByRegion[region]);
        } else {
            return MaterialLawParams(s.gasOilEffectiveParams[region],
                                     s.oilWaterEffectiveParams[region],
                                     s.swlByRegion[region]);
        }
    }

    /*! \brief Saturation-region index of an active cell. */
    OPM_HOST_DEVICE int satnumRegionIdx(unsigned elemIdx) const
    {
        return storage_.satnumRegionArray[elemIdx];
    }

    /*! \brief Number of SATNUM regions with region data. */
    std::size_t numMaterialLawRegions() const
    {
        return storage_.numRegions();
    }

    /*!
     * \brief Whether the scaled endpoints and Swl are stored per cell, i.e.
     *        whether the deck uses endpoint scaling.
     */
    bool hasPerCellEndpoints() const
    {
        if constexpr (isTwoPhase) {
            return false;
        } else {
            return storage_.hasPerCellEndpoints;
        }
    }

    /*!
     * \brief Throw if the CPU manager uses a saturation-function option that
     *        this manager does not implement.
     *
     * Host only. Called by the converting constructor before anything is
     * built, so a GPU manager never silently drops an option.
     */
    template <class CpuManager>
    static void validate(const CpuManager& cpu)
    {
        if (cpu.enableHysteresis()) {
            OPM_THROW(std::logic_error, "GPU material manager does not support hysteresis");
        }
        if (cpu.hasDirectionalRelperms()) {
            OPM_THROW(std::logic_error,
                      "GPU material manager does not support directional relative "
                      "permeabilities (KRNUMX/Y/Z)");
        }
        if (cpu.hasDirectionalImbnum()) {
            OPM_THROW(std::logic_error,
                      "GPU material manager does not support directional imbibition "
                      "regions (IMBNUMX/Y/Z)");
        }
        if (!cpu.satCurveIsAllPiecewiseLinear()) {
            OPM_THROW(std::logic_error,
                      "GPU material manager only supports piecewise-linear saturation curves");
        }
        if (cpu.enableEndPointScaling()) {
            OPM_THROW(std::logic_error,
                      "GPU material manager does not support depth-dependent relative "
                      "permeability endpoints (ENKRVD)");
        }
        if (cpu.enablePpcwmax()) {
            OPM_THROW(std::logic_error, "GPU material manager does not support PPCWMAX");
        }

        bool anyScaling = false;
        for (const auto* config :
             {&cpu.gasOilConfig(), &cpu.oilWaterConfig(), &cpu.gasWaterConfig()}) {
            if (config->enableKrwScaling() || config->enableKrnScaling()
                || config->enableThreePointKrwScaling()
                || config->enableThreePointKrnScaling()) {
                OPM_THROW(std::logic_error,
                          "GPU material manager does not support relative permeability "
                          "value scaling (KRW, KRO, KRG, KRWR, KRORW, ...)");
            }
            if (config->enableLeverettScaling()) {
                OPM_THROW(std::logic_error,
                          "GPU material manager does not support Leverett J-function "
                          "capillary pressure scaling (JFUNC)");
            }
            anyScaling = anyScaling || config->enableSatScaling() || config->enablePcScaling();
        }
        if (anyScaling) {
            if constexpr (isTwoPhase) {
                OPM_THROW(std::logic_error,
                          "GPU material manager does not support endpoint scaling in the "
                          "two-phase material law");
            } else if constexpr (!hasEpsLayer) {
                OPM_THROW(std::logic_error,
                          "GPU material manager does not support endpoint scaling without "
                          "the endpoint-scaling layer (EclEpsTwoPhaseLaw) in its material law");
            }
        }
    }

#if HAVE_CUDA
private:
    struct GpuData {
        std::vector<::Opm::gpuistl::GpuBuffer<Scalar>> sampleBuffers;
        StorageFor<::Opm::gpuistl::GpuBuffer> storage;
    };

    template <bool Enabled = isOwningGpu, std::enable_if_t<Enabled, int> = 0>
    explicit GpuManager(GpuData data)
        : detail::GpuMaterialLawSampleBuffers<Scalar, Enabled>{std::move(data.sampleBuffers)}
        , storage_(std::move(data.storage))
    {
    }

public:
    template <class CpuTraits = TraitsT,
              class CpuManager = ::Opm::EclMaterialLaw::Manager<CpuTraits>,
              class GasOilParamsArg = GasOilEffectiveParams,
              class OilWaterParamsArg = OilWaterEffectiveParams,
              class IntegerStorage = Storage<int>,
              std::enable_if_t<std::is_same_v<IntegerStorage, ::Opm::gpuistl::GpuBuffer<int>>
                                   && std::is_same_v<typename GasOilParamsArg::ValueVector,
                                                     ::Opm::gpuistl::GpuView<const Scalar>>
                                   && std::is_same_v<typename OilWaterParamsArg::ValueVector,
                                                     ::Opm::gpuistl::GpuView<const Scalar>>,
                               int>
              = 0>
        requires(!detail::IsGpuManager<CpuManager>::value)
    explicit GpuManager(const CpuManager& cpu, std::size_t numElements)
        : GpuManager(buildGpuData(cpu, numElements))
    {
    }

private:
    template <class CpuManager>
    static GpuData buildGpuData(const CpuManager& cpu, std::size_t numElements)
    {
        validate(cpu);

        GpuData data;
        const std::vector<int> satnumRegionArray = buildHostSatnumRegionArray(cpu, numElements);

        const auto maxRegion = std::max_element(satnumRegionArray.begin(), satnumRegionArray.end());
        OPM_ERROR_IF(maxRegion == satnumRegionArray.end(),
                     "Failed to find maximum SATNUM region index in GPU material manager");
        const std::size_t numRegions = static_cast<std::size_t>(*maxRegion) + 1u;
        for (const int region : satnumRegionArray) {
            if (region < 0) {
                OPM_THROW(std::logic_error, "Invalid SATNUM region index in GPU material manager");
            }
        }

        // Each piecewise-linear two-phase law holds six sample tables; the two-phase
        // (gas-water) path builds one such law per region, the three-phase path two
        // (gas-oil and oil-water). Reserving only avoids reallocation: moving a
        // GpuBuffer keeps its device pointer, so GpuViews stay valid either way.
        constexpr std::size_t samplesPerLaw = 6u;
        data.sampleBuffers.reserve(numRegions * samplesPerLaw * (isTwoPhase ? 1u : 2u));

        auto pushSampleBuffer = [&](const auto& sampleVector) {
            std::vector<Scalar> hostCopy(sampleVector.begin(), sampleVector.end());
            data.sampleBuffers.emplace_back(hostCopy);
            const auto& deviceBuffer = data.sampleBuffers.back();
            return ::Opm::gpuistl::GpuView<const Scalar>(deviceBuffer.data(), deviceBuffer.size());
        };

        if constexpr (isTwoPhase) {
            buildTwoPhaseStorage(cpu, satnumRegionArray, numRegions, data.storage, pushSampleBuffer);
        } else {
            buildThreePhaseStorage(cpu, satnumRegionArray, numRegions, data.storage, pushSampleBuffer);
        }
        data.storage.satnumRegionArray = detail::toGpuBuffer(satnumRegionArray);
        return data;
    }

    template <class CpuManager, class PushBuffer>
    static void buildTwoPhaseStorage(const CpuManager& cpu,
                                     const std::vector<int>& satnumRegionArray,
                                     std::size_t numRegions,
                                     StorageFor<::Opm::gpuistl::GpuBuffer>& storage,
                                     PushBuffer& pushSampleBuffer)
    {
        std::vector<MaterialLawParams> materialLawParamsByRegion(numRegions);
        std::vector<bool> initialized(numRegions, false);
        std::vector<std::size_t> firstElement(numRegions, 0u);

        for (std::size_t elemIdx = 0; elemIdx < satnumRegionArray.size(); ++elemIdx) {
            const int region = satnumRegionArray[elemIdx];
            const auto& cpuMaterialParams = cpu.materialLawParams(static_cast<unsigned>(elemIdx));
            if (!initialized[region]) {
                buildTwoPhaseRegionParams(cpuMaterialParams,
                                          materialLawParamsByRegion[region],
                                          pushSampleBuffer);
                initialized[region] = true;
                firstElement[region] = elemIdx;
            } else {
                const auto& firstParams
                    = cpu.materialLawParams(static_cast<unsigned>(firstElement[region]));
                if (!sameTwoPhaseMaterialLawParams(cpuMaterialParams, firstParams)) {
                    OPM_THROW(std::logic_error,
                              "GPU material-law tables vary within one SATNUM region");
                }
            }
        }
        storage.materialLawParamsByRegion = detail::toGpuBuffer(materialLawParamsByRegion);
    }

    template <class CpuManager, class PushBuffer>
    static void buildThreePhaseStorage(const CpuManager& cpu,
                                       const std::vector<int>& satnumRegionArray,
                                       std::size_t numRegions,
                                       StorageFor<::Opm::gpuistl::GpuBuffer>& storage,
                                       PushBuffer& pushSampleBuffer)
    {
        using CpuMaterialParams = std::decay_t<decltype(cpu.materialLawParams(0u))>;
        using CpuDefaultParams = std::decay_t<decltype(std::declval<const CpuMaterialParams&>()
            .template getRealParams<::Opm::EclMultiplexerApproach::Default>())>;
        using CpuEpsParams = std::decay_t<decltype(detail::extractCpuEpsParams(
            std::declval<const CpuDefaultParams&>().gasOilParams()))>;
        constexpr bool cpuHasEpsLayer
            = requires(const CpuEpsParams& p) { p.scaledPoints(); p.unscaledPoints(); };

        // The configuration is the same in every cell (see the check below). If
        // the CPU law has no endpoint-scaling layer, it ignores the deck's
        // configuration, and so does the GPU law.
        ::Opm::EclEpsConfig gasOilConfig{};
        ::Opm::EclEpsConfig oilWaterConfig{};
        if constexpr (hasEpsLayer && cpuHasEpsLayer) {
            gasOilConfig = cpu.gasOilConfig();
            oilWaterConfig = cpu.oilWaterConfig();
        }
        const bool perCell = gasOilConfig.enableSatScaling() || oilWaterConfig.enableSatScaling();

        std::vector<GasOilEffectiveParams> gasOilEffectiveParams(numRegions);
        std::vector<OilWaterEffectiveParams> oilWaterEffectiveParams(numRegions);
        std::vector<::Opm::EclEpsScalingPoints<Scalar>> gasOilUnscaledPoints(numRegions);
        std::vector<::Opm::EclEpsScalingPoints<Scalar>> oilWaterUnscaledPoints(numRegions);
        std::vector<Scalar> swlByRegion(numRegions);
        std::vector<const CpuDefaultParams*> firstParams(numRegions, nullptr);

        detail::EpsScaledPointsArrays<::Opm::VectorWithDefaultAllocator, Scalar> gasOilScaledPoints;
        detail::EpsScaledPointsArrays<::Opm::VectorWithDefaultAllocator, Scalar> oilWaterScaledPoints;
        std::vector<Scalar> swlByCell;
        if (perCell) {
            swlByCell.reserve(satnumRegionArray.size());
        }

        for (std::size_t elemIdx = 0; elemIdx < satnumRegionArray.size(); ++elemIdx) {
            const int region = satnumRegionArray[elemIdx];
            const auto& cpuMaterialParams = cpu.materialLawParams(static_cast<unsigned>(elemIdx));
            if (cpuMaterialParams.approach() != ::Opm::EclMultiplexerApproach::Default) {
                OPM_THROW(std::logic_error,
                          "GPU material manager only supports the Default three-phase approach");
            }
            const auto& cpuDefault
                = cpuMaterialParams.template getRealParams<::Opm::EclMultiplexerApproach::Default>();
            [[maybe_unused]] const auto& cpuGasOil
                = detail::extractCpuEpsParams(cpuDefault.gasOilParams());
            [[maybe_unused]] const auto& cpuOilWater
                = detail::extractCpuEpsParams(cpuDefault.oilWaterParams());

            if (firstParams[region] == nullptr) {
                firstParams[region] = &cpuDefault;
                gasOilEffectiveParams[region] = buildEffectiveParams<GasOilEffectiveParams>(
                    detail::extractCpuPlParams(cpuDefault.gasOilParams()), pushSampleBuffer);
                oilWaterEffectiveParams[region] = buildEffectiveParams<OilWaterEffectiveParams>(
                    detail::extractCpuPlParams(cpuDefault.oilWaterParams()), pushSampleBuffer);
                if constexpr (hasEpsLayer && cpuHasEpsLayer) {
                    gasOilUnscaledPoints[region] = cpuGasOil.unscaledPoints();
                    oilWaterUnscaledPoints[region] = cpuOilWater.unscaledPoints();
                }
                swlByRegion[region] = cpuDefault.Swl();
            } else {
                const auto& first = *firstParams[region];
                bool sameTables = false;
                if constexpr (hasEpsLayer && cpuHasEpsLayer) {
                    // The CPU manager shares the tables and the unscaled points
                    // of a region between its cells.
                    const auto& firstGasOil = detail::extractCpuEpsParams(first.gasOilParams());
                    const auto& firstOilWater = detail::extractCpuEpsParams(first.oilWaterParams());
                    sameTables = &cpuGasOil.effectiveLawParams() == &firstGasOil.effectiveLawParams()
                        && &cpuGasOil.unscaledPoints() == &firstGasOil.unscaledPoints()
                        && &cpuOilWater.effectiveLawParams() == &firstOilWater.effectiveLawParams()
                        && &cpuOilWater.unscaledPoints() == &firstOilWater.unscaledPoints();
                } else {
                    sameTables = samePiecewiseLinearParams(
                                     detail::extractCpuPlParams(cpuDefault.gasOilParams()),
                                     detail::extractCpuPlParams(first.gasOilParams()))
                        && samePiecewiseLinearParams(
                                     detail::extractCpuPlParams(cpuDefault.oilWaterParams()),
                                     detail::extractCpuPlParams(first.oilWaterParams()));
                }
                if (!sameTables || (!perCell && cpuDefault.Swl() != first.Swl())) {
                    OPM_THROW(std::logic_error,
                              "GPU material-law tables vary within one SATNUM region");
                }
            }

            if constexpr (hasEpsLayer && cpuHasEpsLayer) {
                if (!detail::sameEpsConfig(cpuGasOil.config(), gasOilConfig)
                    || !detail::sameEpsConfig(cpuOilWater.config(), oilWaterConfig)) {
                    OPM_THROW(std::logic_error,
                              "GPU material manager requires the same endpoint-scaling "
                              "configuration in every cell");
                }
                if (perCell) {
                    gasOilScaledPoints.pushBack(cpuGasOil.scaledPoints(), gasOilConfig);
                    oilWaterScaledPoints.pushBack(cpuOilWater.scaledPoints(), oilWaterConfig);
                    // The Swl the CPU law reads, so that the GPU law sees
                    // exactly the CPU parameters.
                    swlByCell.push_back(cpuDefault.Swl());
                }
            }
        }

        auto upload = [](const auto& host) { return detail::toGpuBuffer(host); };
        storage.gasOilEffectiveParams = upload(gasOilEffectiveParams);
        storage.oilWaterEffectiveParams = upload(oilWaterEffectiveParams);
        if constexpr (hasEpsLayer) {
            storage.gasOilUnscaledPoints = upload(gasOilUnscaledPoints);
            storage.oilWaterUnscaledPoints = upload(oilWaterUnscaledPoints);
        }
        if (perCell) {
            storage.gasOilScaledPoints = detail::transformArrays<::Opm::gpuistl::GpuBuffer, Scalar>(
                gasOilScaledPoints, upload);
            storage.oilWaterScaledPoints
                = detail::transformArrays<::Opm::gpuistl::GpuBuffer, Scalar>(oilWaterScaledPoints,
                                                                           upload);
            storage.swlByCell = upload(swlByCell);
        } else {
            storage.swlByRegion = upload(swlByRegion);
        }
        storage.gasOilConfig = gasOilConfig;
        storage.oilWaterConfig = oilWaterConfig;
        storage.hasPerCellEndpoints = perCell;
    }

    /*!
     * \brief Build a region's two-phase \c EclTwoPhaseMaterialParams instance
     *        with GPU-resident piecewise-linear sample tables.
     *
     * Only the \c GasWater sub-approach is currently supported, since this
     * is the only configuration produced by CO2STORE-style decks.
     */
    template <class CpuMaterialParams, class PushBuffer>
    static void buildTwoPhaseRegionParams(const CpuMaterialParams& cpuMaterialParams,
                                          MaterialLawParams& regionParams,
                                          PushBuffer& pushSampleBuffer)
    {
        if (cpuMaterialParams.approach() != ::Opm::EclMultiplexerApproach::TwoPhase) {
            OPM_THROW(std::logic_error,
                      "GPU material manager requires EclMultiplexerApproach::TwoPhase");
        }
        const auto& cpuTwoPhaseParams
            = cpuMaterialParams.template getRealParams<::Opm::EclMultiplexerApproach::TwoPhase>();
        if (cpuTwoPhaseParams.approach() != ::Opm::EclTwoPhaseApproach::GasWater) {
            OPM_THROW(std::logic_error,
                      "GPU material manager only supports the GasWater two-phase approach");
        }

        using GasWaterParams = typename MaterialLawParams::GasWaterParams;
        regionParams.setGasWaterParams(
            typename MaterialLawParams::GasWaterParamsStorage(buildEffectiveParams<GasWaterParams>(
                detail::extractCpuPlParams(cpuTwoPhaseParams.gasWaterParams()),
                pushSampleBuffer)));
        regionParams.setApproach(::Opm::EclTwoPhaseApproach::GasWater);
        regionParams.finalize();
    }
#endif // HAVE_CUDA

private:
    /*!
     * \brief Piecewise-linear parameters with GPU-resident sample tables.
     */
    template <class EffectiveParams, class CpuPlParams, class PushBuffer>
    static EffectiveParams buildEffectiveParams(const CpuPlParams& cpuParams,
                                                PushBuffer& pushSampleBuffer)
    {
        return EffectiveParams(pushSampleBuffer(cpuParams.SwPcwnSamples()),
                               pushSampleBuffer(cpuParams.pcwnSamples()),
                               pushSampleBuffer(cpuParams.SwKrwSamples()),
                               pushSampleBuffer(cpuParams.krwSamples()),
                               pushSampleBuffer(cpuParams.SwKrnSamples()),
                               pushSampleBuffer(cpuParams.krnSamples()));
    }

    template <class CpuParams>
    static bool samePiecewiseLinearParams(const CpuParams& lhs, const CpuParams& rhs)
    {
        return lhs.SwPcwnSamples() == rhs.SwPcwnSamples()
            && lhs.pcwnSamples() == rhs.pcwnSamples()
            && lhs.SwKrwSamples() == rhs.SwKrwSamples()
            && lhs.krwSamples() == rhs.krwSamples()
            && lhs.SwKrnSamples() == rhs.SwKrnSamples()
            && lhs.krnSamples() == rhs.krnSamples();
    }

    template <class CpuMaterialParams>
    static bool sameTwoPhaseMaterialLawParams(const CpuMaterialParams& lhs,
                                              const CpuMaterialParams& rhs)
    {
        if (lhs.approach() != rhs.approach()
            || lhs.approach() != ::Opm::EclMultiplexerApproach::TwoPhase) {
            return false;
        }
        const auto& lhsTwoPhase
            = lhs.template getRealParams<::Opm::EclMultiplexerApproach::TwoPhase>();
        const auto& rhsTwoPhase
            = rhs.template getRealParams<::Opm::EclMultiplexerApproach::TwoPhase>();
        if (lhsTwoPhase.approach() != rhsTwoPhase.approach()
            || lhsTwoPhase.approach() != ::Opm::EclTwoPhaseApproach::GasWater) {
            return false;
        }
        return samePiecewiseLinearParams(
            detail::extractCpuPlParams(lhsTwoPhase.gasWaterParams()),
            detail::extractCpuPlParams(rhsTwoPhase.gasWaterParams()));
    }

    template <class CpuManager>
    static std::vector<int> buildHostSatnumRegionArray(const CpuManager& cpu,
                                                       std::size_t numElements)
    {
        std::vector<int> satnumRegionArray(numElements);
        for (std::size_t i = 0; i < numElements; ++i) {
            satnumRegionArray[i] = static_cast<int>(cpu.satnumRegionIdx(static_cast<unsigned>(i)));
        }
        return satnumRegionArray;
    }

#if HAVE_CUDA
    template <class T, class GO, class OW, class ML>
    friend GpuManager<T, GO, OW, ::Opm::gpuistl::GpuView, ML>
    (::Opm::gpuistl::make_view)(GpuManager<T, GO, OW, ::Opm::gpuistl::GpuBuffer, ML>& buf);
#endif

    StorageType storage_ {};
};

} // namespace Opm::EclMaterialLaw

namespace Opm::gpuistl
{

#if HAVE_CUDA

/*!
 * \brief Not supported: a CPU-storage GpuManager cannot be built.
 *
 * Defined only so that the return type can be named (\c GpuFlowProblem's
 * \c copy_to_gpu does). Instantiating it is a compile error.
 */
template <class TraitsT, class GasOilLawT, class OilWaterLawT, class MaterialLawT>
::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuBuffer, MaterialLawT>
copy_to_gpu(const ::Opm::EclMaterialLaw::GpuManager<TraitsT,
                                                    GasOilLawT,
                                                    OilWaterLawT,
                                                    ::Opm::VectorWithDefaultAllocator,
                                                    MaterialLawT>& /*cpu*/)
{
    static_assert(!std::is_same_v<TraitsT, TraitsT>,
                  "copy_to_gpu(GpuManager<..., VectorWithDefaultAllocator>) is not supported. "
                  "Build GpuManager<..., GpuBuffer> from the CPU EclMaterialLaw::Manager instead.");
    return {};
}

/*!
 * \brief Make a non-owning GpuView based GpuManager from an owning GpuBuffer
 *        based GpuManager.
 */
template <class TraitsT, class GasOilLawT, class OilWaterLawT, class MaterialLawT>
::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuView, MaterialLawT>
make_view(
    ::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuBuffer, MaterialLawT>&
        buf)
{
    using GpuManagerView = ::Opm::EclMaterialLaw::
        GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuView, MaterialLawT>;
    return GpuManagerView(::Opm::EclMaterialLaw::detail::makeStorageView(buf.storage_));
}

} // namespace Opm::gpuistl

#endif // HAVE_CUDA

#endif // OPM_GPU_ECL_MATERIAL_LAW_MANAGER_HPP
