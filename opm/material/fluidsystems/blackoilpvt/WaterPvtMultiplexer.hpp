// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::WaterPvtMultiplexer
 */
#ifndef OPM_WATER_PVT_MULTIPLEXER_HPP
#define OPM_WATER_PVT_MULTIPLEXER_HPP

#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/BrineH2Pvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityWaterPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityBrinePvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtThermal.hpp>
#include <functional>
#include <opm/common/utility/gpuDecorators.hpp>

#if OPM_IS_COMPILING_WITH_GPU_COMPILER
#define OPM_WATER_PVT_MULTIPLEXER_CALL(codeToCall, ...)                                \
    if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {              \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::BrineCo2>();                      \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    } else {                                                                           \
        auto& pvtImpl = realWaterPvt_;                                                 \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    }
#else
#define OPM_WATER_PVT_MULTIPLEXER_CALL(codeToCall, ...)                                \
    switch (approach_) {                                                               \
    case WaterPvtApproach::ConstantCompressibilityWater: {                             \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::ConstantCompressibilityWater>();  \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    }                                                                                  \
    case WaterPvtApproach::ConstantCompressibilityBrine: {                             \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::ConstantCompressibilityBrine>();  \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    }                                                                                  \
    case WaterPvtApproach::ThermalWater: {                                             \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::ThermalWater>();                  \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    }                                                                                  \
    case WaterPvtApproach::BrineCo2: {                                                 \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::BrineCo2>();                      \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    }                                                                                  \
    case WaterPvtApproach::BrineH2: {                                                  \
        auto& pvtImpl = getRealPvt<WaterPvtApproach::BrineH2>();                       \
        codeToCall;                                                                    \
        __VA_ARGS__;                                                                   \
    }                                                                                  \
    default:                                                                           \
    case WaterPvtApproach::NoWater:                                                    \
        throw std::logic_error("Not implemented: Water PVT of this deck!");            \
    }
#endif

namespace Opm {

enum class WaterPvtApproach {
    NoWater,
    ConstantCompressibilityBrine,
    ConstantCompressibilityWater,
    ThermalWater,
    BrineCo2,
    BrineH2
};

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the water
 *        phase in the black-oil model.
 */
template <class Scalar, bool enableThermal = true, bool enableBrine = true, class ParamsContainer = std::vector<double>, class ContainerT = std::vector<Scalar>, template <class...> class PtrType = std::unique_ptr>
class WaterPvtMultiplexer
{
public:
using ParamsT = CO2Tables<double, ParamsContainer>;
    using UniqueVoidPtrWithDeleter =
        std::conditional_t<
            std::is_same_v<PtrType<void>, std::unique_ptr<void>>,
            std::unique_ptr<void, std::function<void(void*)>>,
            BrineCo2Pvt<Scalar, ParamsT, ContainerT>
        >;

    WaterPvtMultiplexer()
        : approach_(WaterPvtApproach::NoWater)
        , realWaterPvt_(nullptr, [](void*){})
    {
    }

    WaterPvtMultiplexer(WaterPvtApproach approach, void* realWaterPvt)
        : approach_(approach)
        , realWaterPvt_(realWaterPvt, [this](void* ptr){ deleter(ptr); })
    { }

    template<class ConcretePvt>
    WaterPvtMultiplexer(WaterPvtApproach approach, const ConcretePvt& realWaterPvt)
    : approach_(approach)
    , realWaterPvt_(nullptr)
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            realWaterPvt_ = UniqueVoidPtrWithDeleter(new ConcretePvt(realWaterPvt), [this](void* ptr){ deleter(ptr); } );
        }
        else {
            realWaterPvt_ = realWaterPvt;
        }
    }

    template <class T = PtrType<void>, typename std::enable_if<!std::is_same_v<T, std::unique_ptr<void>>, int>::type = 0>
    WaterPvtMultiplexer(WaterPvtApproach approach, const BrineCo2Pvt<Scalar, ParamsT, ContainerT>& realWaterPvt)
        : approach_(approach)
        , realWaterPvt_(realWaterPvt)
    {
    }

    WaterPvtMultiplexer(const WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>& data)
    : approach_(data.approach_)
    , realWaterPvt_(initializeCopyConstructor(data))
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            *this = data;
        }
    }

    ~WaterPvtMultiplexer() = default;

    bool mixingEnergy() const
    {
        return approach_ == WaterPvtApproach::ThermalWater;
    }

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for water using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVDG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE unsigned numRegions() const;

    OPM_HOST_DEVICE void setVapPars(const Scalar par1, const Scalar par2);

    /*!
     * \brief Return the reference density which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE Scalar waterReferenceDensity(unsigned regionIdx) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.waterReferenceDensity(regionIdx)); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rsw,
                        const Evaluation& saltconcentration) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.internalEnergy(regionIdx, temperature, pressure, Rsw, saltconcentration)); }

    Scalar hVap(unsigned regionIdx) const;

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rsw,
                         const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.viscosity(regionIdx, temperature, pressure, Rsw, saltconcentration));
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure,
                                  const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedViscosity(regionIdx, temperature, pressure, saltconcentration));
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rsw,
                                            const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rsw, saltconcentration));
    }

        /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure,
                                                     const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure, saltconcentration));
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of saturated water.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& saltconcentration) const
    {
        OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturatedGasDissolutionFactor(regionIdx, temperature, pressure, saltconcentration));
    }

    /*!
     * \brief Returns the saturation pressure [Pa] of water given the mass fraction of the
     *        gas component in the water phase.
     *
     * Calling this method only makes sense for water that allows for dissolved gas. All other implementations of
     * the black-oil PVT interface will just throw an exception...
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& Rs,
                                  const Evaluation& saltconcentration) const
    { OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.saturationPressure(regionIdx, temperature, Rs, saltconcentration)); }

    /*!
     * \copydoc BaseFluidSystem::diffusionCoefficient
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
      OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.diffusionCoefficient(temperature, pressure, compIdx));
    }

    OPM_HOST_DEVICE void setApproach(WaterPvtApproach appr);

    /*!
     * \brief Returns the concrete approach for calculating the PVT relations.
     *
     * (This is only determined at runtime.)
     */
    OPM_HOST_DEVICE WaterPvtApproach approach() const
    { return approach_; }

    // get the concrete parameter object for the water phase
    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityWater, ConstantCompressibilityWaterPvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityWaterPvt<Scalar>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityWater, const ConstantCompressibilityWaterPvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityWaterPvt<Scalar>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityBrine, ConstantCompressibilityBrinePvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityBrinePvt<Scalar>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::ConstantCompressibilityBrine, const ConstantCompressibilityBrinePvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<ConstantCompressibilityBrinePvt<Scalar>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::ThermalWater, WaterPvtThermal<Scalar, enableBrine> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<WaterPvtThermal<Scalar, enableBrine>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::ThermalWater, const WaterPvtThermal<Scalar, enableBrine> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<WaterPvtThermal<Scalar, enableBrine>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::BrineCo2, BrineCo2Pvt<Scalar, ParamsT, ContainerT> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            return *static_cast<BrineCo2Pvt<Scalar, ParamsT, ContainerT>* >(realWaterPvt_.get());
        } else {
            return realWaterPvt_;
        }
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::BrineCo2, const BrineCo2Pvt<Scalar, ParamsT, ContainerT> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const BrineCo2Pvt<Scalar, ParamsT, ContainerT>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::BrineH2, BrineH2Pvt<Scalar> >::type& getRealPvt()
    {
        assert(approach() == approachV);
        return *static_cast<BrineH2Pvt<Scalar>* >(realWaterPvt_.get());
    }

    template <WaterPvtApproach approachV>
    OPM_HOST_DEVICE typename std::enable_if<approachV == WaterPvtApproach::BrineH2, const BrineH2Pvt<Scalar> >::type& getRealPvt() const
    {
        assert(approach() == approachV);
        return *static_cast<const BrineH2Pvt<Scalar>* >(realWaterPvt_.get());
    }

    const void* realWaterPvt() const { return realWaterPvt_.get(); }

    WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>&
    operator=(const WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>& data){
        approach_ = data.approach_;

        copyPointer(data.realWaterPvt_);
        return *this;
    }

private:
    template <class ConcreteGasPvt> UniqueVoidPtrWithDeleter makeWaterPvt();

    template <class ConcretePvt> UniqueVoidPtrWithDeleter copyPvt(const UniqueVoidPtrWithDeleter& sourcePvt){
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            return UniqueVoidPtrWithDeleter(
                new ConcretePvt(*static_cast<const ConcretePvt*>(sourcePvt.get())),
                [this](void* ptr) { deleter(ptr); }
            );
        }
        else {
            return UniqueVoidPtrWithDeleter(
                new ConcretePvt(*static_cast<const ConcretePvt*>(sourcePvt.get()))
            );
        }
    }

    void copyPointer(const UniqueVoidPtrWithDeleter& pointer) {
        switch (approach_) {
            case WaterPvtApproach::ConstantCompressibilityWater: {
                realWaterPvt_ = copyPvt<ConstantCompressibilityWaterPvt<Scalar>>(pointer);
                break;
            }
            case WaterPvtApproach::ConstantCompressibilityBrine: {
                realWaterPvt_ = copyPvt<ConstantCompressibilityBrinePvt<Scalar>>(pointer);
                break;
            }
            case WaterPvtApproach::ThermalWater: {
                realWaterPvt_ = copyPvt<WaterPvtThermal<Scalar, enableBrine>>(pointer);
                break;
            }
            case WaterPvtApproach::BrineCo2: {
                realWaterPvt_ = copyPvt<BrineCo2Pvt<Scalar, ParamsT, ContainerT>>(pointer);
                break;
            }
            case WaterPvtApproach::BrineH2: {
                realWaterPvt_ = copyPvt<BrineH2Pvt<Scalar>>(pointer);
                break;
            }
            case WaterPvtApproach::NoWater:
                break;
            }
    }

    void deleter(void* ptr){
        switch (approach_) {
            case WaterPvtApproach::ConstantCompressibilityWater: {
                delete static_cast<ConstantCompressibilityWaterPvt<Scalar>*>(ptr);
                break;
            }
            case WaterPvtApproach::ConstantCompressibilityBrine: {
                delete static_cast<ConstantCompressibilityBrinePvt<Scalar>*>(ptr);
                break;
            }
            case WaterPvtApproach::ThermalWater: {
                delete static_cast<WaterPvtThermal<Scalar, enableBrine>*>(ptr);
                break;
            }
            case WaterPvtApproach::BrineCo2: {
                delete static_cast<BrineCo2Pvt<Scalar, ParamsT, ContainerT>*>(ptr);
                break;
            }
            case WaterPvtApproach::BrineH2: {
                delete static_cast<BrineH2Pvt<Scalar>*>(ptr);
                break;
            }
            case WaterPvtApproach::NoWater:
                break;
            }
    }

    UniqueVoidPtrWithDeleter initializeCopyConstructor(
        const WaterPvtMultiplexer<Scalar, enableThermal, enableBrine, ParamsContainer, ContainerT, PtrType>& data)
    {
        if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
            if (data.realWaterPvt_.get() == nullptr) {
                if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
                    return UniqueVoidPtrWithDeleter(nullptr, [](void*){});
                } else {
                    return PtrType<void>(nullptr);
                }
            }
            switch (data.approach_) {
            case WaterPvtApproach::ConstantCompressibilityWater:
                return copyPvt<ConstantCompressibilityWaterPvt<Scalar>>(data.realWaterPvt_);
            case WaterPvtApproach::ConstantCompressibilityBrine:
                return copyPvt<ConstantCompressibilityBrinePvt<Scalar>>(data.realWaterPvt_);
            case WaterPvtApproach::ThermalWater:
                return copyPvt<WaterPvtThermal<Scalar, enableBrine>>(data.realWaterPvt_);
            case WaterPvtApproach::BrineCo2:
                return copyPvt<BrineCo2Pvt<Scalar, ParamsT, ContainerT>>(data.realWaterPvt_);
            case WaterPvtApproach::BrineH2:
                return copyPvt<BrineH2Pvt<Scalar>>(data.realWaterPvt_);
            default:
                if constexpr (std::is_same_v<PtrType<void>, std::unique_ptr<void>>) {
                    return UniqueVoidPtrWithDeleter(nullptr, [](void*){});
                } else {
                    return PtrType<void>(nullptr); // Assuming default constructor works
                }
            }
        }
        else {
            return data.realWaterPvt_;
        }
    }

    WaterPvtApproach approach_{WaterPvtApproach::NoWater};
    UniqueVoidPtrWithDeleter realWaterPvt_;
};

namespace gpuistl{
    template<class ParamsContainer, class GPUContainer, class Scalar>
    WaterPvtMultiplexer<Scalar, true, true, ParamsContainer, GPUContainer>
    copy_to_gpu(WaterPvtMultiplexer<Scalar>& waterMultiplexer)
    {
        using ParamsT = CO2Tables<double, ParamsContainer>;

        assert(waterMultiplexer.approach() == WaterPvtApproach::BrineCo2);

        auto gpuPvt = copy_to_gpu<ParamsT, GPUContainer>(waterMultiplexer.template getRealPvt<WaterPvtApproach::BrineCo2>());

        return WaterPvtMultiplexer<Scalar, true, true, ParamsContainer, GPUContainer>(WaterPvtApproach::BrineCo2, gpuPvt);
    }

    template <template <class> class ViewPtr, class ViewDouble, class ViewScalar, class GPUContainerDouble, class GPUContainerScalar, class Scalar>
    WaterPvtMultiplexer<Scalar, true, true, ViewDouble, ViewScalar, ViewPtr>
    make_view(WaterPvtMultiplexer<Scalar, true, true, GPUContainerDouble, GPUContainerScalar, std::unique_ptr>& waterMultiplexer)
    {
        using ParamsView = CO2Tables<Scalar, ViewDouble>;

        assert(waterMultiplexer.approach() == WaterPvtApproach::BrineCo2);

        auto gpuPvtView = make_view<ViewScalar, ParamsView>(waterMultiplexer.template getRealPvt<WaterPvtApproach::BrineCo2>());

        return WaterPvtMultiplexer<Scalar, true, true, ViewDouble, ViewScalar, ViewPtr>(WaterPvtApproach::BrineCo2, gpuPvtView);
    }
}

} // namespace Opm

#endif
