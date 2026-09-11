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
 * FlowProblemBlackoil and the BlackOilIntensiveQuantities. It re-uses the
 * generic two-phase material types from opm-common
 * (\c Opm::EclTwoPhaseMaterial and \c Opm::EclTwoPhaseMaterialParams) with
 * a value-storage policy (\c Opm::gpuistl::ValueAsPointer) so the per-cell
 * material law parameters are trivially copyable to the device.
 *
 * The chain of multiplexers used by the CPU manager
 * (\c EclMultiplexerMaterial \f$\to\f$ \c EclEpsTwoPhaseLaw \f$\to\f$
 * \c EclHysteresisTwoPhaseLaw \f$\to\f$ \c SatCurveMultiplexer) is bypassed:
 * the GPU manager unwraps the CPU multiplexer parameters down to the
 * underlying \c PiecewiseLinearTwoPhaseMaterialParams and uploads only the
 * piecewise-linear sample tables.
 */
#ifndef OPM_GPU_ECL_MATERIAL_LAW_MANAGER_HPP
#define OPM_GPU_ECL_MATERIAL_LAW_MANAGER_HPP

#include <opm/common/utility/VectorWithDefaultAllocator.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>
#include <opm/common/ErrorMacros.hpp>

#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclTwoPhaseMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/EclTwoPhaseMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/PiecewiseLinearTwoPhaseMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/SatCurveMultiplexerParams.hpp>

#include <cassert>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm::EclMaterialLaw
{

namespace detail
{

    /*!
     * \brief Walk down a CPU material-law parameter object until the enclosed
     *        \c PiecewiseLinearTwoPhaseMaterialParams is reached.
     *
     * Supports the standard layering used by the CPU \c EclMaterialLaw::Manager:
     *   \c EclHysteresisTwoPhaseLawParams \f$\to\f$ \c EclEpsTwoPhaseLawParams
     *   \f$\to\f$ \c SatCurveMultiplexerParams
     *   \f$\to\f$ \c PiecewiseLinearTwoPhaseMaterialParams.
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
            return params
                .template getRealParams<::Opm::SatCurveMultiplexerApproach::PiecewiseLinear>();
        } else {
            return params;
        }
    }

    /*!
     * \brief Optional host-side base used by \c GpuManager when its storage is
     *        owning device memory: keeps the per-cell sample buffers alive so
     *        that the \c GpuView pointers stored inside the cells'
     *        \c PiecewiseLinearTwoPhaseMaterialParams remain valid for as long
     *        as the manager exists. Empty for any non-owning storage so that
     *        \c GpuView based managers stay device-trivially-copyable.
     */
    template <class Scalar, bool Owning>
    struct GpuPiecewiseLinearSampleHolder {
    };

#if HAVE_CUDA
    template <class Scalar>
    struct GpuPiecewiseLinearSampleHolder<Scalar, true> {
        std::vector<::Opm::gpuistl::GpuBuffer<Scalar>> sampleBuffers {};
    };
#endif

} // namespace detail

/*!
 * \brief A minimal, GPU-compatible material-law manager.
 *
 * Only the EclDefaultMaterial three-phase law and the EclTwoPhaseMaterial
 * two-phase law are supported. The two-phase gas/oil and oil/water sub-law
 * types are template parameters so the caller can choose any GPU compatible
 * implementation (e.g. PiecewiseLinear).
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
          template <class> class Storage = ::Opm::VectorWithDefaultAllocator,
          class MaterialLawT = ::Opm::EclDefaultMaterial<TraitsT, GasOilLawT, OilWaterLawT>>
class GpuManager : private detail::GpuPiecewiseLinearSampleHolder<
                       typename TraitsT::Scalar,
                       std::is_same_v<Storage<int>, ::Opm::gpuistl::GpuBuffer<int>>>
{
private:
    #if HAVE_CUDA
    struct GpuData {
        std::vector<::Opm::gpuistl::GpuBuffer<typename TraitsT::Scalar>> sampleBuffers;
        std::vector<typename MaterialLawT::Params> materialLawParamsByRegion;
        std::vector<int> satnumRegionArray;
    };
    #endif

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

public:
    GpuManager() = default;

    GpuManager(Storage<MaterialLawParams> materialLawParamsByRegion,
               Storage<int> satnumRegionArray)
        : materialLawParamsByRegion_(std::move(materialLawParamsByRegion))
        , satnumRegionArray_(std::move(satnumRegionArray))
    {
    }

    /*! \brief Material-law parameters of an active cell. */
    OPM_HOST_DEVICE const MaterialLawParams& materialLawParams(unsigned elemIdx) const
    {
        return materialLawParamsByRegion_[satnumRegionArray_[elemIdx]];
    }

    OPM_HOST_DEVICE MaterialLawParams& materialLawParams(unsigned elemIdx)
    {
        return materialLawParamsByRegion_[satnumRegionArray_[elemIdx]];
    }

    /*! \brief Saturation-region index of an active cell. */
    OPM_HOST_DEVICE int satnumRegionIdx(unsigned elemIdx) const
    {
        return satnumRegionArray_[elemIdx];
    }

    std::size_t materialLawParamsRegionCount() const
    {
        return materialLawParamsByRegion_.size();
    }

    /*! \brief Direct access to the underlying storages
     *         (used by copy_to_gpu / make_view). */
    const Storage<MaterialLawParams>& materialLawParamsStorage() const
    {
        return materialLawParamsByRegion_;
    }
    Storage<MaterialLawParams>& materialLawParamsStorage()
    {
        return materialLawParamsByRegion_;
    }

    const Storage<int>& satnumRegionArrayStorage() const
    {
        return satnumRegionArray_;
    }
    Storage<int>& satnumRegionArrayStorage()
    {
        return satnumRegionArray_;
    }

#if HAVE_CUDA
    template <bool Enabled = isOwningGpu, std::enable_if_t<Enabled, int> = 0>
    GpuManager(GpuData data)
        : detail::GpuPiecewiseLinearSampleHolder<Scalar, Enabled>{std::move(data.sampleBuffers)}
        , materialLawParamsByRegion_(::Opm::gpuistl::GpuBuffer<MaterialLawParams>(
              data.materialLawParamsByRegion))
        , satnumRegionArray_(::Opm::gpuistl::GpuBuffer<int>(data.satnumRegionArray))
    {
    }
#endif

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
    static bool sameSupportedMaterialLawParams(const CpuMaterialParams& lhs,
                                               const CpuMaterialParams& rhs)
    {
        if (lhs.approach() != rhs.approach()) {
            return false;
        }

        if constexpr (isTwoPhase) {
            if (lhs.approach() != ::Opm::EclMultiplexerApproach::TwoPhase) {
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

        if (lhs.approach() != ::Opm::EclMultiplexerApproach::Default) {
            return false;
        }
        const auto& lhsDefault
            = lhs.template getRealParams<::Opm::EclMultiplexerApproach::Default>();
        const auto& rhsDefault
            = rhs.template getRealParams<::Opm::EclMultiplexerApproach::Default>();
        return lhsDefault.Swl() == rhsDefault.Swl()
            && samePiecewiseLinearParams(
                detail::extractCpuPlParams(lhsDefault.gasOilParams()),
                detail::extractCpuPlParams(rhsDefault.gasOilParams()))
            && samePiecewiseLinearParams(
                detail::extractCpuPlParams(lhsDefault.oilWaterParams()),
                detail::extractCpuPlParams(rhsDefault.oilWaterParams()));
    }

#if HAVE_CUDA
    template <class CpuManager>
    static GpuData buildGpuData(const CpuManager& cpu, std::size_t numElements)
    {
        GpuData data;
        data.satnumRegionArray = buildHostSatnumRegionArray(cpu, numElements);

        auto maxRegion = std::max_element(data.satnumRegionArray.begin(), data.satnumRegionArray.end());
        OPM_ERROR_IF(maxRegion == data.satnumRegionArray.end(),
                     "Failed to find maximum SATNUM region index in GPU material manager");

        data.materialLawParamsByRegion.resize(static_cast<std::size_t>(*maxRegion) + 1u);
        std::vector<bool> initialized(data.materialLawParamsByRegion.size(), false);
        std::vector<std::size_t> firstElement(data.materialLawParamsByRegion.size(), 0u);
        // reserve 6 or 12 sample buffers depending 2 or three-phase
        data.sampleBuffers.reserve(data.materialLawParamsByRegion.size() * (6u + 6u * static_cast<std::size_t>(isTwoPhase)));

        auto pushSampleBuffer = [&](const auto& sampleVector) {
            std::vector<Scalar> hostCopy(sampleVector.begin(), sampleVector.end());
            data.sampleBuffers.emplace_back(hostCopy);
            const auto& deviceBuffer = data.sampleBuffers.back();
            return ::Opm::gpuistl::GpuView<const Scalar>(deviceBuffer.data(), deviceBuffer.size());
        };

        for (std::size_t elemIdx = 0; elemIdx < numElements; ++elemIdx) {
            const int region = data.satnumRegionArray[elemIdx];
            if (region < 0 || static_cast<std::size_t>(region) >= initialized.size()) {
                OPM_THROW(std::logic_error, "Invalid SATNUM region index in GPU material manager");
            }

            const auto& cpuMaterialParams = cpu.materialLawParams(static_cast<unsigned>(elemIdx));
            if (!initialized[region]) {
                if constexpr (isTwoPhase) {
                    buildTwoPhaseCellParams(cpuMaterialParams,
                                            data.materialLawParamsByRegion[region],
                                            pushSampleBuffer);
                } else {
                    buildThreePhaseCellParams(cpuMaterialParams,
                                              data.materialLawParamsByRegion[region],
                                              pushSampleBuffer);
                }
                initialized[region] = true;
                firstElement[region] = elemIdx;
            } else {
                const auto& firstParams
                    = cpu.materialLawParams(static_cast<unsigned>(firstElement[region]));
                if (!sameSupportedMaterialLawParams(cpuMaterialParams, firstParams)) {
                    OPM_THROW(std::logic_error,
                              "GPU material-law tables vary within one SATNUM region");
                }
            }
        }
        return data;
    }

    template <class CpuTraits = TraitsT,
              class CpuManager = ::Opm::EclMaterialLaw::Manager<CpuTraits>,
              class GasOilParamsArg = GasOilParams,
              class OilWaterParamsArg = OilWaterParams,
              class IntegerStorage = Storage<int>,
              class MaterialLawParamsStorageT = Storage<MaterialLawParams>,
              std::enable_if_t<std::is_same_v<IntegerStorage, ::Opm::gpuistl::GpuBuffer<int>>
                                   && std::is_same_v<MaterialLawParamsStorageT,
                                                     ::Opm::gpuistl::GpuBuffer<MaterialLawParams>>
                                   && std::is_same_v<typename GasOilParamsArg::ValueVector,
                                                     ::Opm::gpuistl::GpuView<const Scalar>>
                                   && std::is_same_v<typename OilWaterParamsArg::ValueVector,
                                                     ::Opm::gpuistl::GpuView<const Scalar>>,
                               int>
              = 0>
    explicit GpuManager(const CpuManager& cpu, std::size_t numElements)
        : GpuManager(buildGpuData(cpu, numElements))
    {
    }
#endif

    /*!
     * \brief Build a single cell's two-phase \c EclTwoPhaseMaterialParams
     *        instance with GPU-resident piecewise-linear sample tables.
     *
     * Only the \c GasWater sub-approach is currently supported, since this
     * is the only configuration produced by CO2STORE-style decks.
     */
    template <class CpuMaterialParams, class PushBuffer>
    static void buildTwoPhaseCellParams(const CpuMaterialParams& cpuMaterialParams,
                                        MaterialLawParams& cellParams,
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
        const auto& cpuGasWaterPiecewiseLinear
            = detail::extractCpuPlParams(cpuTwoPhaseParams.gasWaterParams());
        GasWaterParams gasWaterParams(pushSampleBuffer(cpuGasWaterPiecewiseLinear.SwPcwnSamples()),
                                      pushSampleBuffer(cpuGasWaterPiecewiseLinear.pcwnSamples()),
                                      pushSampleBuffer(cpuGasWaterPiecewiseLinear.SwKrwSamples()),
                                      pushSampleBuffer(cpuGasWaterPiecewiseLinear.krwSamples()),
                                      pushSampleBuffer(cpuGasWaterPiecewiseLinear.SwKrnSamples()),
                                      pushSampleBuffer(cpuGasWaterPiecewiseLinear.krnSamples()));

        cellParams.setGasWaterParams(
            typename MaterialLawParams::GasWaterParamsStorage(std::move(gasWaterParams)));
        cellParams.setApproach(::Opm::EclTwoPhaseApproach::GasWater);
        cellParams.finalize();
    }

    /*!
     * \brief Build a single cell's three-phase \c EclDefaultMaterialParams
     *        instance with GPU-resident piecewise-linear sample tables.
     */
    template <class CpuMaterialParams, class PushBuffer>
    static void buildThreePhaseCellParams(const CpuMaterialParams& cpuMaterialParams,
                                          MaterialLawParams& cellParams,
                                          PushBuffer& pushSampleBuffer)
    {
        if (cpuMaterialParams.approach() != ::Opm::EclMultiplexerApproach::Default) {
            OPM_THROW(std::logic_error,
                      "GPU material manager only supports the Default three-phase approach");
        }
        const auto& cpuDefaultParams
            = cpuMaterialParams.template getRealParams<::Opm::EclMultiplexerApproach::Default>();
        const auto& cpuGasOilPiecewiseLinear
            = detail::extractCpuPlParams(cpuDefaultParams.gasOilParams());
        const auto& cpuOilWaterPiecewiseLinear
            = detail::extractCpuPlParams(cpuDefaultParams.oilWaterParams());

        GasOilParams gasOilParams(pushSampleBuffer(cpuGasOilPiecewiseLinear.SwPcwnSamples()),
                                  pushSampleBuffer(cpuGasOilPiecewiseLinear.pcwnSamples()),
                                  pushSampleBuffer(cpuGasOilPiecewiseLinear.SwKrwSamples()),
                                  pushSampleBuffer(cpuGasOilPiecewiseLinear.krwSamples()),
                                  pushSampleBuffer(cpuGasOilPiecewiseLinear.SwKrnSamples()),
                                  pushSampleBuffer(cpuGasOilPiecewiseLinear.krnSamples()));
        OilWaterParams oilWaterParams(pushSampleBuffer(cpuOilWaterPiecewiseLinear.SwPcwnSamples()),
                                      pushSampleBuffer(cpuOilWaterPiecewiseLinear.pcwnSamples()),
                                      pushSampleBuffer(cpuOilWaterPiecewiseLinear.SwKrwSamples()),
                                      pushSampleBuffer(cpuOilWaterPiecewiseLinear.krwSamples()),
                                      pushSampleBuffer(cpuOilWaterPiecewiseLinear.SwKrnSamples()),
                                      pushSampleBuffer(cpuOilWaterPiecewiseLinear.krnSamples()));

        cellParams.setGasOilParams(std::make_shared<GasOilParams>(std::move(gasOilParams)));
        cellParams.setOilWaterParams(std::make_shared<OilWaterParams>(std::move(oilWaterParams)));
        cellParams.setSwl(cpuDefaultParams.Swl());
        cellParams.finalize();
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

    Storage<MaterialLawParams> materialLawParamsByRegion_ {};
    Storage<int> satnumRegionArray_ {};
};

} // namespace Opm::EclMaterialLaw

namespace Opm::gpuistl
{

#if HAVE_CUDA

/*!
 * \brief Copy a CPU GpuManager to GPU-resident GpuBuffer storage.
 *
 * The MaterialLawParams element type is assumed to be the same on the CPU
 * and the GPU, i.e. the caller is responsible for making the GasOilLaw and
 * OilWaterLaw GPU-compatible (typically by templating their parameter type
 * on a GPU storage).
 */
template <class TraitsT, class GasOilLawT, class OilWaterLawT, class MaterialLawT>
::Opm::EclMaterialLaw::GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuBuffer, MaterialLawT>
copy_to_gpu(const ::Opm::EclMaterialLaw::GpuManager<TraitsT,
                                                    GasOilLawT,
                                                    OilWaterLawT,
                                                    ::Opm::VectorWithDefaultAllocator,
                                                    MaterialLawT>& cpu)
{
    using GpuManagerBuffer = ::Opm::EclMaterialLaw::
        GpuManager<TraitsT, GasOilLawT, OilWaterLawT, GpuBuffer, MaterialLawT>;
    return GpuManagerBuffer(cpu, cpu.satnumRegionArrayStorage().size());
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
    using MaterialLawParams = typename GpuManagerView::MaterialLawParams;
    return GpuManagerView(
        GpuView<MaterialLawParams>(buf.materialLawParamsStorage().data(),
                                   buf.materialLawParamsStorage().size()),
        GpuView<int>(buf.satnumRegionArrayStorage().data(), buf.satnumRegionArrayStorage().size()));
}

} // namespace Opm::gpuistl

#endif // HAVE_CUDA

#endif // OPM_GPU_ECL_MATERIAL_LAW_MANAGER_HPP
