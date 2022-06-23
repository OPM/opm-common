#ifndef OPM_CO2BRINEFLUIDSYSTEM_HH
#define OPM_CO2BRINEFLUIDSYSTEM_HH

#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/components/H2O.hpp>
#include <opm/material/components/Brine.hpp>

#include <opm/material/fluidsystems/PTFlashParameterCache.hpp>
#include <opm/material/viscositymodels/LBC.hpp>
//#include <opm/material/viscositymodels/LBC.hpp>

namespace Opm {
/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase two component system, co2 brine
 */

    template<class Scalar>
    class Co2BrineFluidSystem
            : public Opm::BaseFluidSystem<Scalar, Co2BrineFluidSystem<Scalar> > {
    public:
        // TODO: I do not think these should be constant in fluidsystem, will try to make it non-constant later
        static constexpr int numPhases=2;
        static constexpr int numComponents = 2;
        static constexpr int numMisciblePhases=2;
        static constexpr int numMiscibleComponents = 2;
        static constexpr int oilPhaseIdx = 0;
        static constexpr int gasPhaseIdx = 1;

        static constexpr int Comp0Idx = 0;
        static constexpr int Comp1Idx = 1;

        using Comp0 = Opm::SimpleCO2<Scalar>;
        using Comp1 = Opm::Brine<Scalar, Opm::H2O<Scalar>>;

        template <class ValueType>
        using ParameterCache = Opm::PTFlashParameterCache<ValueType, Co2BrineFluidSystem<Scalar>>;
        using ViscosityModel = typename Opm::ViscosityModels<Scalar, Co2BrineFluidSystem<Scalar>>;
        //using ViscosityModel = typename Opm::ViscosityModels<Scalar, Co2BrineFluidSystem<Scalar>>;

        using PengRobinsonMixture = typename Opm::PengRobinsonMixture<Scalar, Co2BrineFluidSystem<Scalar>>;


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
                default: throw std::runtime_error("Illegal component index for criticalVolume");
            }
        }

        //! \copydoc BaseFluidSystem::molarMass
        static Scalar molarMass(unsigned compIdx)
        {
            switch (compIdx) {
                case Comp0Idx: return Comp0::molarMass();
                case Comp1Idx: return Comp1::molarMass();
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

        //! \copydoc BaseFluidSystem::phaseName
        static const char* phaseName(unsigned phaseIdx)
        {
                static const char* name[] = {"o",  // oleic phase
                                             "g"};  // gas phase

                assert(0 <= phaseIdx && phaseIdx < 2);
                return name[phaseIdx];
        }

        //! \copydoc BaseFluidSystem::componentName
        static const char* componentName(unsigned compIdx)
        {
                static const char* name[] = {
                        Comp0::name(),
                        Comp1::name(),
                };

                assert(0 <= compIdx && compIdx < 3);
                return name[compIdx];
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
                dens = fluidState.averageMolarMass(phaseIdx) / paramCache.molarVolume(phaseIdx);
            }
            return dens;

        }

        /*!
         * \copydoc BaseFluidSystem::viscosity
         */
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval viscosity(const FluidState& fluidState,
                                 const ParameterCache<ParamCacheEval>& paramCache,
                                 unsigned phaseIdx)
        {
            // Use LBC method to calculate viscosity
            LhsEval mu;
            mu = ViscosityModel::LBC(fluidState, paramCache, phaseIdx); 

         
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

            LhsEval phi = PengRobinsonMixture::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx); 

            return phi;
        }

    };
}
#endif //OPM_CO2BRINEFLUIDSYSTEM_HH