#ifndef TWOPHASEFLUIDSYSTEM_HH
#define TWOPHASEFLUIDSYSTEM_HH

#include "components.hh"
#include "chiwoms.h"
#include "LBCviscosity.hpp"

#include <iostream>
#include <cassert>
#include <stdexcept>  // invalid_argument
#include <sstream>
#include <iostream>
#include <string>
#include <random>    // mt19937, normal_distribution
#include <limits>    // epsilon
#include <boost/format.hpp>  // boost::format

#include <opm/common/Exceptions.hpp>
#include <opm/material/IdealGas.hpp>

#include <opm/material/components/Component.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/components/Brine.hpp>
#include <opm/material/components/SimpleH2O.hpp>
#include <opm/material/eos/PengRobinsonMixture.hpp>
#include <opm/material/eos/PengRobinsonParamsMixture.hpp>
#include "ChiParameterCache.hpp"

#include <opm/material/common/Valgrind.hpp>
#include <opm/material/common/Exceptions.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/common/Unused.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/NullParameterCache.hpp>
#include <opm/material/fluidsystems/H2ON2FluidSystem.hpp>
#include <opm/material/fluidsystems/BrineCO2FluidSystem.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidstates/ImmiscibleFluidState.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/RegularizedBrooksCorey.hpp>
#include <opm/material/fluidmatrixinteractions/EffToAbsLaw.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>

#include <opm/material/thermal/SomertonThermalConductionLaw.hpp>
#include <opm/material/thermal/ConstantSolidHeatCapLaw.hpp>

#include <opm/material/binarycoefficients/H2O_CO2.hpp>
#include <opm/material/binarycoefficients/Brine_CO2.hpp>


namespace Opm {
/*!
 * \ingroup Fluidsystems
 *
 * \brief A two-phase fluid system with three components
 */
template <class Scalar>
class TwoPhaseThreeComponentFluidSystem
        : public Opm::BaseFluidSystem<Scalar, TwoPhaseThreeComponentFluidSystem<Scalar> >
{
    using ThisType = TwoPhaseThreeComponentFluidSystem<Scalar>;
    using Base = Opm::BaseFluidSystem<Scalar, ThisType>;
    using PengRobinson = typename Opm::PengRobinson<Scalar>;
    using PengRobinsonMixture = typename Opm::PengRobinsonMixture<Scalar, ThisType>;
    using LBCviscosity = typename Opm::LBCviscosity<Scalar, ThisType>;
    using H2O = typename Opm::H2O<Scalar>;
    using Brine = typename Opm::Brine<Scalar, H2O>;

public:
        //! \copydoc BaseFluidSystem::ParameterCache
    //template <class Evaluation>
    //using ParameterCache = Opm::NullParameterCache<Evaluation>;

    //! \copydoc BaseFluidSystem::ParameterCache
    template <class Evaluation>
    using ParameterCache = Opm::ChiParameterCache<Evaluation, ThisType>;

    /****************************************
         * Fluid phase related static parameters
         ****************************************/

        //! \copydoc BaseFluidSystem::numPhases
        static const int numPhases = 2;

        //! Index of the liquid phase
    static const int oilPhaseIdx = 0;
    static const int gasPhaseIdx = 1;

        //! \copydoc BaseFluidSystem::phaseName
        static const char* phaseName(unsigned phaseIdx)
        {
                static const char* name[] = {"o",  // oleic phase
                                             "g"};  // gas phase

                assert(0 <= phaseIdx && phaseIdx < numPhases);
                return name[phaseIdx];
        }

        //! \copydoc BaseFluidSystem::isIdealMixture
    static bool isIdealMixture(unsigned phaseIdx)
        {
        if (phaseIdx == oilPhaseIdx)
            return false;

        // CO2 have associative effects
        return true;
        }


        /****************************************
         * Component related static parameters
         ****************************************/

        //! \copydoc BaseFluidSystem::numComponents
        static const int numComponents = 2;  // Comp0, Comp1 and Comp2

        //! first comp idx
        static const int Comp0Idx = 0;

        //! second comp idx 
        static const int Comp1Idx = 1;

       // TODO: make this a loop over choises in chiwoms.hh
        // using Comp0 = Opm::Methane<Scalar>;
        using Comp0 = Opm::ChiwomsBrine<Scalar>;
        using Comp1 = Opm::ChiwomsCO2<Scalar>;

    static void init(Scalar minT = 273.15,
                     Scalar maxT = 373.15,
                     Scalar minP = 1e4,
                     Scalar maxP = 100e6)
    {
        Opm::PengRobinsonParamsMixture<Scalar, ThisType, oilPhaseIdx, /*useSpe5=*/false> prParams;

        // find envelopes of the 'a' and 'b' parameters for the range
        // minT <= T <= maxT and minP <= p <= maxP. For
        // this we take advantage of the fact that 'a' and 'b' for
        // mixtures is just a convex combination of the attractive and
        // repulsive parameters of the pure components

        Scalar minA = 1e30, maxA = -1e30;
        Scalar minB = 1e30, maxB = -1e30;

        prParams.updatePure(minT, minP);
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            minA = std::min(prParams.pureParams(compIdx).a(), minA);
            maxA = std::max(prParams.pureParams(compIdx).a(), maxA);
            minB = std::min(prParams.pureParams(compIdx).b(), minB);
            maxB = std::max(prParams.pureParams(compIdx).b(), maxB);
        };

        prParams.updatePure(maxT, minP);
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            minA = std::min(prParams.pureParams(compIdx).a(), minA);
            maxA = std::max(prParams.pureParams(compIdx).a(), maxA);
            minB = std::min(prParams.pureParams(compIdx).b(), minB);
            maxB = std::max(prParams.pureParams(compIdx).b(), maxB);
        };

        prParams.updatePure(minT, maxP);
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            minA = std::min(prParams.pureParams(compIdx).a(), minA);
            maxA = std::max(prParams.pureParams(compIdx).a(), maxA);
            minB = std::min(prParams.pureParams(compIdx).b(), minB);
            maxB = std::max(prParams.pureParams(compIdx).b(), maxB);
        };

        prParams.updatePure(maxT, maxP);
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            minA = std::min(prParams.pureParams(compIdx).a(), minA);
            maxA = std::max(prParams.pureParams(compIdx).a(), maxA);
            minB = std::min(prParams.pureParams(compIdx).b(), minB);
            maxB = std::max(prParams.pureParams(compIdx).b(), maxB);
        };
      //  PengRobinson::init(/*aMin=*/minA, /*aMax=*/maxA, /*na=*/100,
      //                     /*bMin=*/minB, /*bMax=*/maxB, /*nb=*/200);
    }

        //! \copydoc BaseFluidSystem::componentName
        static const char* componentName(unsigned compIdx)
        {
                static const char* name[] = {
                        Comp0::name(),
                        Comp1::name(),
                };

                assert(0 <= compIdx && compIdx < numComponents);
                return name[compIdx];
        }

        //! \copydoc BaseFluidSystem::molarMass
        static Scalar molarMass(unsigned compIdx)
        {
                return (compIdx == Comp0Idx)
                        ? Comp0::molarMass()
                        : (compIdx == Comp1Idx)
                        ? Comp1::molarMass()
                        : throw std::invalid_argument("Molar mass component index");
        }

        /*!
         * \brief Critical temperature of a component [K].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalTemperature(unsigned compIdx)
        {
                return (compIdx == Comp0Idx)
                        ? Comp0::criticalTemperature()
                        : (compIdx == Comp1Idx)
                        ? Comp1::criticalTemperature()
                        : throw std::invalid_argument("Critical temperature component index");
        }

        /*!
         * \brief Critical pressure of a component [Pa].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalPressure(unsigned compIdx)
        {
                return (compIdx == Comp0Idx)
                        ? Comp0::criticalPressure()
                        : (compIdx == Comp1Idx)
                        ? Comp1::criticalPressure()
                        : throw std::invalid_argument("Critical pressure component index");
        }

        /*!
         * \brief Critical volume of a component [m3].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalVolume(unsigned compIdx)
        {
                return (compIdx == Comp0Idx)
                        ? Comp0::criticalVolume()
                        : (compIdx == Comp1Idx)
                        ? Comp1::criticalVolume()
                        : throw std::invalid_argument("Critical volume component index");
        }

        /*!
         * \brief The acentric factor of a component [].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar acentricFactor(unsigned compIdx)
        {
                return (compIdx == Comp0Idx)
                        ? Comp0::acentricFactor()
                        : (compIdx == Comp1Idx)
                        ? Comp1::acentricFactor()
                        : throw std::invalid_argument("Molar mass component index");
        }

        /****************************************
         * thermodynamic relations
         ****************************************/

        /*!
         * \copydoc BaseFluidSystem::density
         */
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval density(const FluidState& fluidState,
                           const ParameterCache<ParamCacheEval>& paramCache,
                               unsigned phaseIdx)
        {
        
            LhsEval dens;
            if (phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx) {
                // paramCache.updatePhase(fluidState, phaseIdx);
                dens = fluidState.averageMolarMass(phaseIdx) / paramCache.molarVolume(phaseIdx);
            }
            return dens;

        }

        //! \copydoc BaseFluidSystem::viscosity
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval viscosity(const FluidState& fluidState,
                                 const ParameterCache<ParamCacheEval>& paramCache,
                                 unsigned phaseIdx)
        {
            // Use LBC method to calculate viscosity
            LhsEval mu;
            // if (phaseIdx == gasPhaseIdx) {
                mu = LBCviscosity::LBCmod(fluidState, paramCache, phaseIdx);
            // }
            // else {
            //     const auto& T = Opm::decay<LhsEval>(fluidState.temperature(phaseIdx));
            //     const auto& p = Opm::decay<LhsEval>(fluidState.pressure(0));
            //     mu = Brine::liquidViscosity(T, p);
            // }
            return mu;

        }

        //! \copydoc BaseFluidSystem::enthalpy
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval enthalpy(const FluidState& fluidState,
                                const ParameterCache<ParamCacheEval>& /*paramCache*/,
                                unsigned phaseIdx)
        {
                const auto& T = Opm::decay<LhsEval>(fluidState.temperature(phaseIdx));
                const auto& p = Opm::decay<LhsEval>(fluidState.pressure(phaseIdx));
                const auto& x = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, Comp1Idx));

        if(phaseIdx == oilPhaseIdx) {
                        return EOS::oleic_enthalpy(T, p, x); //TODO
                }
                else {
                        return EOS::aqueous_enthalpy(T, p, x); //TODO
                }
        }

        //! \copydoc BaseFluidSystem::fugacityCoefficient
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval fugacityCoefficient(const FluidState& fluidState,
                                       const ParameterCache<ParamCacheEval>& paramCache,
                                           unsigned phaseIdx,
                                           unsigned compIdx)
        {
                assert(0 <= phaseIdx && phaseIdx < numPhases);
                assert(0 <= compIdx && compIdx < numComponents);

            Scalar phi = Opm::getValue(
                PengRobinsonMixture::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx));
            return phi;


        throw std::invalid_argument("crap!");
        }

        //! \copydoc BaseFluidSystem::diffusionCoefficient
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval diffusionCoefficient(const FluidState& /*fluidState*/,
                                            const ParameterCache<ParamCacheEval>& /*paramCache*/,
                                            unsigned /*phaseIdx*/,
                                            unsigned /*compIdx*/)
        {
                return DIFFUSIVITY;
        }

    /*!
     * \brief Returns the interaction coefficient for two components.
     *.
     */
    static Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx)
    {
        return 0.0; //-0.101;//0.1089;
    }

};

};//namespace opm

#endif // TWOPHASEFLUIDSYSTEM_HH
