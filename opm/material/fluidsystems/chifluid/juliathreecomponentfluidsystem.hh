#ifndef OPM_JULIATHREECOMPONENTFLUIDSYSTEM_HH
#define OPM_JULIATHREECOMPONENTFLUIDSYSTEM_HH

#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/chifluid/components.hh>

// TODO: this is something else need to check
#include "ChiParameterCache.hpp"
#include "LBCviscosity.hpp"

namespace Opm {
/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase three component fluid system from the Julia test
 */

    template<class Scalar>
    class JuliaThreeComponentFluidSystem
            : public Opm::BaseFluidSystem<Scalar, JuliaThreeComponentFluidSystem<Scalar> > {
    public:
        // TODO: I do not think these should be constant in fluidsystem, will try to make it non-constant later
        static constexpr int numPhases=2;
        static constexpr int numComponents = 3;
        // TODO: phase location should be more general
        static constexpr int oilPhaseIdx = 0;
        static constexpr int gasPhaseIdx = 1;

        static constexpr int Comp0Idx = 0;
        static constexpr int Comp1Idx = 1;
        static constexpr int Comp2Idx = 2;

        // TODO: needs to be more general
        using Comp0 = Opm::JuliaCO2<Scalar>;
        using Comp1 = Opm::JuliaC1<Scalar>;
        using Comp2 = Opm::JuliaC10<Scalar>;

        template <class ValueType>
        using ParameterCache = Opm::ChiParameterCache<ValueType, JuliaThreeComponentFluidSystem<Scalar>>;
        using LBCviscosity = typename Opm::LBCviscosity<Scalar, JuliaThreeComponentFluidSystem<Scalar>>;
        using PengRobinsonMixture = typename Opm::PengRobinsonMixture<Scalar, JuliaThreeComponentFluidSystem<Scalar>>;

        /*!
         * \brief The acentric factor of a component [].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar acentricFactor(unsigned compIdx)
        {
            switch (compIdx) {
            case Comp0Idx: return Comp0::acentricFactor();
            case Comp1Idx: return Comp1::acentricFactor();
            case Comp2Idx: return Comp2::acentricFactor();
            default: throw std::runtime_error("Illegal component index for acentricFactor");
            }
        }
        /*!
         * \brief Critical temperature of a component [K].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalTemperature(unsigned compIdx)
        {
            switch (compIdx) {
                case Comp0Idx: return Comp0::criticalTemperature();
                case Comp1Idx: return Comp1::criticalTemperature();
                case Comp2Idx: return Comp2::criticalTemperature();
                default: throw std::runtime_error("Illegal component index for criticalTemperature");
            }
        }
        /*!
         * \brief Critical pressure of a component [Pa].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalPressure(unsigned compIdx) {
            switch (compIdx) {
                case Comp0Idx: return Comp0::criticalPressure();
                case Comp1Idx: return Comp1::criticalPressure();
                case Comp2Idx: return Comp2::criticalPressure();
                default: throw std::runtime_error("Illegal component index for criticalPressure");
            }
        }
        /*!
        * \brief Critical volume of a component [m3].
        *
        * \copydetails Doxygen::compIdxParam
        */
        static Scalar criticalVolume(unsigned compIdx)
        {
            switch (compIdx) {
                case Comp0Idx: return Comp0::criticalVolume();
                case Comp1Idx: return Comp1::criticalVolume();
                case Comp2Idx: return Comp2::criticalVolume();
                default: throw std::runtime_error("Illegal component index for criticalVolume");
            }
        }

        //! \copydoc BaseFluidSystem::molarMass
        static Scalar molarMass(unsigned compIdx)
        {
            switch (compIdx) {
                case Comp0Idx: return Comp0::molarMass();
                case Comp1Idx: return Comp1::molarMass();
                case Comp2Idx: return Comp2::molarMass();
                default: throw std::runtime_error("Illegal component index for molarMass");
            }
        }

        /*!
         * \brief Returns the interaction coefficient for two components.
         *.
         */
        static Scalar interactionCoefficient(unsigned /*comp1Idx*/, unsigned /*comp2Idx*/)
        {
            return 0.0;
        }

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
            // LhsEval mu = LBCviscosity::LBCmod(fluidState, paramCache, phaseIdx);
            // LhsEval mu = LBCviscosity::LBC(fluidState, paramCache, phaseIdx);
            LhsEval mu = LBCviscosity::LBCJulia(fluidState, paramCache, phaseIdx);
            return mu;

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

            // TODO: here the derivatives for the phi are dropped. Should we keep the derivatives against the pressure
            // and temperature?
            Scalar phi = Opm::getValue(
                    PengRobinsonMixture::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx));
            return phi;
        }

    };
}
#endif //OPM_JULIATHREECOMPONENTFLUIDSYSTEM_HH