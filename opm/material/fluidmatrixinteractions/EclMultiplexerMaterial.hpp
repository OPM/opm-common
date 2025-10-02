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
 * \copydoc Opm::EclMultiplexerMaterial
 */
#ifndef OPM_ECL_MULTIPLEXER_MATERIAL_HPP
#define OPM_ECL_MULTIPLEXER_MATERIAL_HPP

#include "EclMultiplexerMaterialParams.hpp"
#include "EclDefaultMaterial.hpp"
#include "EclStone1Material.hpp"
#include "EclStone2Material.hpp"
#include "EclTwoPhaseMaterial.hpp"

#include <opm/common/TimingMacros.hpp>

#include <algorithm>
#include <stdexcept>

namespace Opm {

#define OPM_ECL_MULTIPLEXER_MATERIAL_CALL(codeToCall, onePhaseCode)                                                    \
    switch (params.approach()) {                                                                                       \
    case EclMultiplexerApproach::Stone1: {                                                                             \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::Stone1;                   \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = Stone1Material;                                                                              \
        codeToCall;                                                                                                    \
        break;                                                                                                         \
    }                                                                                                                  \
    case EclMultiplexerApproach::Stone2: {                                                                             \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::Stone2;                   \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = Stone2Material;                                                                              \
        codeToCall;                                                                                                    \
        break;                                                                                                         \
    }                                                                                                                  \
    case EclMultiplexerApproach::Default: {                                                                            \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::Default;                  \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = DefaultMaterial;                                                                             \
        codeToCall;                                                                                                    \
        break;                                                                                                         \
    }                                                                                                                  \
    case EclMultiplexerApproach::TwoPhase: {                                                                           \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::TwoPhase;                  \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = TwoPhaseMaterial;                                                                            \
        codeToCall;                                                                                                    \
        break;                                                                                                         \
    }                                                                                                                  \
    case EclMultiplexerApproach::OnePhase: {                                                                           \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::OnePhase;                 \
        onePhaseCode;                                                                                                  \
        break;                                                                                                         \
    }                                                                                                                  \
    }

// The static_assert does not compile with gcc 12 and earlier when placed in the multiplexer below.
#if defined(__GNUC__) && (__GNUC__ < 13)
    #define STATIC_ASSERT_ECL_MULTIPLEXER_UNLESS_GCC_LT_13 throw std::logic_error("Unhandled EclMultiplexerApproach")
#else
    #define STATIC_ASSERT_ECL_MULTIPLEXER_UNLESS_GCC_LT_13 static_assert(false, "Unhandled EclMultiplexerApproach")
#endif

#define OPM_ECL_MULTIPLEXER_MATERIAL_CALL_COMPILETIME(codeToCall, onePhaseCode)                                        \
    if constexpr (Head::approach == EclMultiplexerApproach::Stone1) {                                                  \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::Stone1;                   \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = Stone1Material;                                                                              \
        codeToCall;                                                                                                    \
    } else if constexpr (Head::approach == EclMultiplexerApproach::Stone2) {                                           \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::Stone2;                   \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = Stone2Material;                                                                              \
        codeToCall;                                                                                                    \
    } else if constexpr (Head::approach == EclMultiplexerApproach::Default) {                                          \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::Default;                  \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = DefaultMaterial;                                                                             \
        codeToCall;                                                                                                    \
    } else if constexpr (Head::approach == EclMultiplexerApproach::TwoPhase) {                                         \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::TwoPhase;                 \
        auto& realParams = params.template getRealParams<approach>();                                                  \
        using ActualLaw = TwoPhaseMaterial;                                                                            \
        codeToCall;                                                                                                    \
    } else if constexpr (Head::approach == EclMultiplexerApproach::OnePhase) {                                         \
        [[maybe_unused]] constexpr EclMultiplexerApproach approach = EclMultiplexerApproach::OnePhase;                 \
        onePhaseCode;                                                                                                  \
    } else {                                                                                                           \
        STATIC_ASSERT_ECL_MULTIPLEXER_UNLESS_GCC_LT_13;                                                                \
    }

// Pass this for the onePhaseCode argument if nothing is to be done.
inline void doNothing() { };

/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Implements a multiplexer class that provides all three phase capillary pressure
 *        laws used by the ECLipse simulator.
 */
template <class TraitsT,
          class GasOilMaterialLawT,
          class OilWaterMaterialLawT,
          class GasWaterMaterialLawT,
          class ParamsT = EclMultiplexerMaterialParams<TraitsT,
                                                       GasOilMaterialLawT,
                                                       OilWaterMaterialLawT,
                                                       GasWaterMaterialLawT> >
class EclMultiplexerMaterial : public TraitsT
{
public:
    using GasOilMaterialLaw = GasOilMaterialLawT;
    using OilWaterMaterialLaw = OilWaterMaterialLawT;
    using GasWaterMaterialLaw = GasWaterMaterialLawT;

    using Stone1Material = EclStone1Material<TraitsT, GasOilMaterialLaw, OilWaterMaterialLaw>;
    using Stone2Material = EclStone2Material<TraitsT, GasOilMaterialLaw, OilWaterMaterialLaw>;
    using DefaultMaterial = EclDefaultMaterial<TraitsT, GasOilMaterialLaw, OilWaterMaterialLaw>;
    using TwoPhaseMaterial = EclTwoPhaseMaterial<TraitsT, GasOilMaterialLaw, OilWaterMaterialLaw, GasWaterMaterialLaw>;

    // some safety checks
    static_assert(TraitsT::numPhases == 3,
                  "The number of phases considered by this capillary pressure "
                  "law is always three!");
    static_assert(GasOilMaterialLaw::numPhases == 2,
                  "The number of phases considered by the gas-oil capillary "
                  "pressure law must be two!");
    static_assert(OilWaterMaterialLaw::numPhases == 2,
                  "The number of phases considered by the oil-water capillary "
                  "pressure law must be two!");
    static_assert(GasWaterMaterialLaw::numPhases == 2,
                  "The number of phases considered by the gas-water capillary "
                  "pressure law must be two!");
    static_assert(std::is_same<typename GasOilMaterialLaw::Scalar,
                               typename OilWaterMaterialLaw::Scalar>::value,
                  "The two two-phase capillary pressure laws must use the same "
                  "type of floating point values.");

    using Traits = TraitsT;
    using Params = ParamsT;
    using Scalar = typename Traits::Scalar;

    static constexpr int numPhases = 3;
    static constexpr int waterPhaseIdx = Traits::wettingPhaseIdx;
    static constexpr int oilPhaseIdx = Traits::nonWettingPhaseIdx;
    static constexpr int gasPhaseIdx = Traits::gasPhaseIdx;

    //! Specify whether this material law implements the two-phase
    //! convenience API
    static constexpr bool implementsTwoPhaseApi = false;

    //! Specify whether this material law implements the two-phase
    //! convenience API which only depends on the phase saturations
    static constexpr bool implementsTwoPhaseSatApi = false;

    //! Specify whether the quantities defined by this material law
    //! are saturation dependent
    static constexpr bool isSaturationDependent = true;

    //! Specify whether the quantities defined by this material law
    //! are dependent on the absolute pressure
    static constexpr bool isPressureDependent = false;

    //! Specify whether the quantities defined by this material law
    //! are temperature dependent
    static constexpr bool isTemperatureDependent = false;

    //! Specify whether the quantities defined by this material law
    //! are dependent on the phase composition
    static constexpr bool isCompositionDependent = false;

    /*!
     * \brief Implements the multiplexer three phase capillary pressure law
     *        used by the ECLipse simulator.
     *
     * This material law is valid for three fluid phases and only
     * depends on the saturations.
     *
     * The required two-phase relations are supplied by means of template
     * arguments and can be an arbitrary other material laws.
     *
     * \param values Container for the return values
     * \param params Parameters
     * \param fluidState The fluid state
     */
    template <class ContainerT, class FluidState, class ...Args>
    static void capillaryPressures(ContainerT& values,
                                   const Params& params,
                                   const FluidState& fluidState)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        if constexpr (FrontIsEclMultiplexerDispatchV<Args...>) {
            capillaryPressuresT<ContainerT, FluidState, Args...>(values, params, fluidState);
            return;
        }
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(ActualLaw::capillaryPressures(values, realParams, fluidState),
                                          values[0] = 0.0);
    }


    template <class ContainerT, class FluidState, class Head, class ...Args>
    static void capillaryPressuresT(ContainerT& values,
                                    const Params& params,
                                    const FluidState& fluidState)
    {
#define OPM_LOCAL_TEMPLATE_ARGS ContainerT, FluidState, Args...
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL_COMPILETIME(
            ActualLaw::template capillaryPressures<OPM_LOCAL_TEMPLATE_ARGS>(values, realParams, fluidState),
            values[0] = 0.0
        );
#undef OPM_LOCAL_TEMPLATE_ARGS
    }

    /*
     * Hysteresis parameters for oil-water
     * @see EclHysteresisTwoPhaseLawParams::soMax(...)
     * @see EclHysteresisTwoPhaseLawParams::swMax(...)
     * @see EclHysteresisTwoPhaseLawParams::swMin(...)
     * \param params Parameters
     */
    static void oilWaterHysteresisParams(Scalar& soMax,
                                         Scalar& swMax,
                                         Scalar& swMin,
                                         const Params& params)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(ActualLaw::oilWaterHysteresisParams(soMax, swMax, swMin, realParams),
                                          doNothing());
    }

    /*
     * Hysteresis parameters for oil-water
     * @see EclHysteresisTwoPhaseLawParams::soMax(...)
     * @see EclHysteresisTwoPhaseLawParams::swMax(...)
     * @see EclHysteresisTwoPhaseLawParams::swMin(...)
     * \param params Parameters
     */
    static void setOilWaterHysteresisParams(const Scalar& soMax,
                                            const Scalar& swMax,
                                            const Scalar& swMin,
                                            Params& params)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(ActualLaw::setOilWaterHysteresisParams(soMax, swMax, swMin, realParams),
                                          doNothing());
    }

    /*
     * Hysteresis parameters for gas-oil
     * @see EclHysteresisTwoPhaseLawParams::sgmax(...)
     * @see EclHysteresisTwoPhaseLawParams::shmax(...)
     * @see EclHysteresisTwoPhaseLawParams::somin(...)
     * \param params Parameters
     */
    static void gasOilHysteresisParams(Scalar& sgmax,
                                       Scalar& shmax,
                                       Scalar& somin,
                                       const Params& params)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(ActualLaw::gasOilHysteresisParams(sgmax, shmax, somin, realParams),
                                          doNothing());
    }

    static Scalar trappedGasSaturation(const Params& params, bool maximumTrapping)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(return ActualLaw::trappedGasSaturation(realParams, maximumTrapping),
                                          return 0.0);
        return 0.0;
    }

    static Scalar strandedGasSaturation(const Params& params, Scalar Sg, Scalar Kg)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(return ActualLaw::strandedGasSaturation(realParams, Sg, Kg),
                                          return 0.0);
        return 0.0;
    }

    static Scalar trappedOilSaturation(const Params& params, bool maximumTrapping)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(return ActualLaw::trappedOilSaturation(realParams, maximumTrapping),
                                          return 0.0);
        return 0.0;
    }

    static Scalar trappedWaterSaturation(const Params& params)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(return ActualLaw::trappedWaterSaturation(realParams),
                                          return 0.0);
        return 0.0;
    }
    /*
     * Hysteresis parameters for gas-oil
     * @see EclHysteresisTwoPhaseLawParams::sgmax(...)
     * @see EclHysteresisTwoPhaseLawParams::shmax(...)
     * @see EclHysteresisTwoPhaseLawParams::somin(...)
     * \param params Parameters
     */
    static void setGasOilHysteresisParams(const Scalar& sgmax,
                                          const Scalar& shmax,
                                          const Scalar& somin,
                                          Params& params)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(ActualLaw::setGasOilHysteresisParams(sgmax, shmax, somin, realParams),
                                          doNothing());
    }

    /*!
     * \brief Capillary pressure between the gas and the non-wetting
     *        liquid (i.e., oil) phase.
     *
     * This is defined as
     * \f[
     * p_{c,gn} = p_g - p_n
     * \f]
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation pcgn(const Params& /* params */,
                           const FluidState& /* fs */)
    {
        throw std::logic_error("Not implemented: pcgn()");
    }

    /*!
     * \brief Capillary pressure between the non-wetting liquid (i.e.,
     *        oil) and the wetting liquid (i.e., water) phase.
     *
     * This is defined as
     * \f[
     * p_{c,nw} = p_n - p_w
     * \f]
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation pcnw(const Params& /* params */,
                           const FluidState& /* fs */)
    {
        throw std::logic_error("Not implemented: pcnw()");
    }

    /*!
     * \brief The inverse of the capillary pressure
     */
    template <class ContainerT, class FluidState>
    static void saturations(ContainerT& /* values */,
                            const Params& /* params */,
                            const FluidState& /* fs */)
    {
        throw std::logic_error("Not implemented: saturations()");
    }

    /*!
     * \brief The saturation of the gas phase.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sg(const Params& /* params */,
                         const FluidState& /* fluidState */)
    {
        throw std::logic_error("Not implemented: Sg()");
    }

    /*!
     * \brief The saturation of the non-wetting (i.e., oil) phase.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sn(const Params& /* params */,
                         const FluidState& /* fluidState */)
    {
        throw std::logic_error("Not implemented: Sn()");
    }

    /*!
     * \brief The saturation of the wetting (i.e., water) phase.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sw(const Params& /* params */,
                         const FluidState& /* fluidState */)
    {
        throw std::logic_error("Not implemented: Sw()");
    }

    /*!
     * \brief The relative permeability of all phases.
     *
     * The relative permeability of the water phase it uses the same
     * value as the relative permeability for water in the water-oil
     * law with \f$S_o = 1 - S_w\f$. The gas relative permebility is
     * taken from the gas-oil material law, but with \f$S_o = 1 -
     * S_g\f$.  The relative permeability of the oil phase is
     * calculated using the relative permeabilities of the oil phase
     * in the two two-phase systems.
     *
     * A more detailed description can be found in the "Three phase
     * oil relative permeability models" section of the ECLipse
     * technical description.
     */
    template <class ContainerT, class FluidState, class ...Args>
    static void relativePermeabilities(ContainerT& values,
                                       const Params& params,
                                       const FluidState& fluidState)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        if constexpr (FrontIsEclMultiplexerDispatchV<Args...>) {
            relativePermeabilitiesT<ContainerT, FluidState, Args...>(values, params, fluidState);
            return;
        }
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(ActualLaw::relativePermeabilities(values, realParams, fluidState),
                                          values[0] = 1.0);
    }

    template <class ContainerT, class FluidState, class Head, class ...Args>
    static void relativePermeabilitiesT(ContainerT& values,
                                        const Params& params,
                                        const FluidState& fluidState)
    {
#define OPM_LOCAL_TEMPLATE_ARGS ContainerT, FluidState, Args...
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL_COMPILETIME(
            ActualLaw::template relativePermeabilities<OPM_LOCAL_TEMPLATE_ARGS>(values, realParams, fluidState),
            values[0] = 1.0
        );
#undef OPM_LOCAL_TEMPLATE_ARGS
    }




    /*!
     * \brief The relative permeability of oil in oil/gas system.
     */
    template <class Evaluation, class FluidState>
    static Evaluation relpermOilInOilGasSystem(const Params& params,
                                               const FluidState& fluidState)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        switch (params.approach()) {
        case EclMultiplexerApproach::Stone1:
            return Stone1Material::template relpermOilInOilGasSystem<Evaluation>
                (params.template getRealParams<EclMultiplexerApproach::Stone1>(),
                 fluidState);

        case EclMultiplexerApproach::Stone2:
            return Stone2Material::template relpermOilInOilGasSystem<Evaluation>
                (params.template getRealParams<EclMultiplexerApproach::Stone2>(),
                 fluidState);

        case EclMultiplexerApproach::Default:
            return DefaultMaterial::template relpermOilInOilGasSystem<Evaluation>
                (params.template getRealParams<EclMultiplexerApproach::Default>(),
                 fluidState);

        default:
            throw std::logic_error {
                "relpermOilInOilGasSystem() is specific to three phases"
            };
        }
    }

    /*!
     * \brief The relative permeability of oil in oil/water system.
     */
    template <class Evaluation, class FluidState>
    static Evaluation relpermOilInOilWaterSystem(const Params& params,
                                                 const FluidState& fluidState)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        switch (params.approach()) {
        case EclMultiplexerApproach::Stone1:
            return Stone1Material::template relpermOilInOilWaterSystem<Evaluation>
                (params.template getRealParams<EclMultiplexerApproach::Stone1>(),
                 fluidState);

        case EclMultiplexerApproach::Stone2:
            return Stone2Material::template relpermOilInOilWaterSystem<Evaluation>
                (params.template getRealParams<EclMultiplexerApproach::Stone2>(),
                 fluidState);

        case EclMultiplexerApproach::Default:
            return DefaultMaterial::template relpermOilInOilWaterSystem<Evaluation>
                (params.template getRealParams<EclMultiplexerApproach::Default>(),
                 fluidState);

        default:
            throw std::logic_error {
                "relpermOilInOilWaterSystem() is specific to three phases"
            };
        }
    }

    /*!
     * \brief The relative permeability of the gas phase.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krg(const Params& /* params */,
                          const FluidState& /* fluidState */)
    {
        throw std::logic_error("Not implemented: krg()");
    }

    /*!
     * \brief The relative permeability of the wetting phase.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krw(const Params& /* params */,
                          const FluidState& /* fluidState */)
    {
        throw std::logic_error("Not implemented: krw()");
    }

    /*!
     * \brief The relative permeability of the non-wetting (i.e., oil) phase.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krn(const Params& /* params */,
                          const FluidState& /* fluidState */)
    {
        throw std::logic_error("Not implemented: krn()");
    }


    /*!
     * \brief Update the hysteresis parameters after a time step.
     *
     * This assumes that the nested two-phase material laws are parameters for
     * EclHysteresisLaw. If they are not, calling this methid will cause a compiler
     * error. (But not calling it will still work.)
     */
    template <class FluidState>
    static bool updateHysteresis(Params& params, const FluidState& fluidState)
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::SatProps);
        OPM_ECL_MULTIPLEXER_MATERIAL_CALL(return ActualLaw::updateHysteresis(realParams, fluidState),
                                          return false);
        return false;
    }
};

} // namespace Opm

#endif
