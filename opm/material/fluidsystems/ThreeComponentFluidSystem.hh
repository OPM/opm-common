// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_THREECOMPONENTFLUIDSYSTEM_HH
#define OPM_THREECOMPONENTFLUIDSYSTEM_HH

#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/C1.hpp>


// TODO: this is something else need to check
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp>
#include <opm/material/viscositymodels/LBC.hpp>

namespace Opm {
/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase three component fluid system with components
 * CO2, Methane and NDekan
 */

    template<class Scalar>
    class ThreeComponentFluidSystem
            : public Opm::BaseFluidSystem<Scalar, ThreeComponentFluidSystem<Scalar> > {
    public:
        // TODO: I do not think these should be constant in fluidsystem, will try to make it non-constant later
        static constexpr int numPhases=2;
        static constexpr int numComponents = 3;
        static constexpr int numMisciblePhases=2;
        static constexpr int numMiscibleComponents = 3;
        // TODO: phase location should be more general
        static constexpr int oilPhaseIdx = 0;
        static constexpr int gasPhaseIdx = 1;

        static constexpr int Comp0Idx = 0;
        static constexpr int Comp1Idx = 1;
        static constexpr int Comp2Idx = 2;

        // TODO: needs to be more general
        using Comp0 = Opm::SimpleCO2<Scalar>;
        using Comp1 = Opm::C1<Scalar>;
        using Comp2 = Opm::C10<Scalar>;

        template <class ValueType>
        using ParameterCache = Opm::PTFlashParameterCache<ValueType, ThreeComponentFluidSystem<Scalar>>;
        using ViscosityModel = typename Opm::ViscosityModels<Scalar, ThreeComponentFluidSystem<Scalar>>;
        using PengRobinsonMixture = typename Opm::PengRobinsonMixture<Scalar, ThreeComponentFluidSystem<Scalar>>;

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
                        Comp2::name(),
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

        //! \copydoc BaseFluidSystem::viscosity
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
            assert(phaseIdx < numPhases);
            assert(compIdx < numComponents);

            LhsEval phi = PengRobinsonMixture::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx);
            return phi;
        }

    };
}
#endif //OPM_THREECOMPONENTFLUIDSYSTEM_HH
