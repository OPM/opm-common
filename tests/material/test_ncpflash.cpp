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
 *
 * \brief This is a program to test the flash calculation which uses
 *        non-linear complementarity problems (NCP)
 *
 * A flash calculation determines the pressures, saturations and
 * composition of all phases given the total mass (or, as in this case
 * the total number of moles) in a given amount of pore space.
 */
#include "config.h"

#define BOOST_TEST_MODULE NcpFlash
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/NcpFlash.hpp>
#include <opm/material/constraintsolvers/MiscibleMultiPhaseComposition.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <opm/material/fluidsystems/H2ON2FluidSystem.hpp>

#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/RegularizedBrooksCorey.hpp>
#include <opm/material/fluidmatrixinteractions/EffToAbsLaw.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>


template <class Scalar, class FluidState>
void checkSame(const FluidState& fsRef, const FluidState& fsFlash)
{
    enum { numPhases = FluidState::numPhases };
    enum { numComponents = FluidState::numComponents };

    Scalar tol = std::max(std::numeric_limits<Scalar>::epsilon()*1e4, 1e-6);

    for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
        Scalar error;

        // check the pressures
        error = 1 - fsRef.pressure(phaseIdx)/fsFlash.pressure(phaseIdx);
        BOOST_CHECK_MESSAGE(std::abs(error) <= tol,
                            "pressure error phase " << phaseIdx <<
                            " is incorrect: " << fsFlash.pressure(phaseIdx) <<
                            " flash vs " << fsRef.pressure(phaseIdx) <<
                            " reference error=" << error);

        // check the saturations
        error = fsRef.saturation(phaseIdx) - fsFlash.saturation(phaseIdx);
        BOOST_CHECK_MESSAGE(std::abs(error) <= tol,
                            "saturation error phase " << phaseIdx <<
                            " is incorrect: " << fsFlash.saturation(phaseIdx) <<
                            " flash vs " << fsRef.saturation(phaseIdx) <<
                            " reference error=" << error);

        // check the compositions
        for (unsigned compIdx = 0; compIdx < numComponents; ++ compIdx) {
            error = fsRef.moleFraction(phaseIdx, compIdx) - fsFlash.moleFraction(phaseIdx, compIdx);
            BOOST_CHECK_MESSAGE(std::abs(error) <= tol,
                                "composition error phase " << phaseIdx <<
                                ", component " <<  compIdx <<
                                " is incorrect: " <<
                                fsFlash.moleFraction(phaseIdx, compIdx) <<
                                " flash vs " << fsRef.moleFraction(phaseIdx, compIdx) <<
                                " reference error=" << error);
        }
    }
}

template <class Scalar, class FluidSystem, class MaterialLaw, class FluidState>
void checkNcpFlash(const FluidState& fsRef,
                   typename MaterialLaw::Params& matParams)
{
    enum { numPhases = FluidSystem::numPhases };
    enum { numComponents = FluidSystem::numComponents };
    using ComponentVector = Dune::FieldVector<Scalar, numComponents>;
    using ParameterCache = typename FluidSystem::template ParameterCache<typename FluidState::Scalar>;

    // calculate the total amount of stuff in the reference fluid
    // phase
    ComponentVector globalMolarities(0.0);
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            globalMolarities[compIdx] +=
                fsRef.saturation(phaseIdx)*fsRef.molarity(phaseIdx, compIdx);
        }
    }

    // initialize the fluid state for the flash calculation
    using NcpFlash = Opm::NcpFlash<Scalar, FluidSystem>;
    FluidState fsFlash;

    fsFlash.setTemperature(fsRef.temperature(/*phaseIdx=*/0));

    // run the flash calculation
    ParameterCache paramCache;
    paramCache.updateAll(fsFlash);
    NcpFlash::guessInitial(fsFlash, globalMolarities);
    NcpFlash::template solve<MaterialLaw>(fsFlash, matParams, paramCache, globalMolarities);

    // compare the "flashed" fluid state with the reference one
    checkSame<Scalar>(fsRef, fsFlash);
}


template <class Scalar, class FluidSystem, class MaterialLaw, class FluidState>
void completeReferenceFluidState(FluidState& fs,
                                 typename MaterialLaw::Params& matParams,
                                 unsigned refPhaseIdx)
{
    enum { numPhases = FluidSystem::numPhases };

    using ComputeFromReferencePhase = Opm::ComputeFromReferencePhase<Scalar, FluidSystem>;
    using PhaseVector = Dune::FieldVector<Scalar, numPhases>;

    unsigned otherPhaseIdx = 1 - refPhaseIdx;

    // calculate the other saturation
    fs.setSaturation(otherPhaseIdx, 1.0 - fs.saturation(refPhaseIdx));

    // calulate the capillary pressure
    PhaseVector pC;
    MaterialLaw::capillaryPressures(pC, matParams, fs);
    fs.setPressure(otherPhaseIdx,
                   fs.pressure(refPhaseIdx)
                   + (pC[otherPhaseIdx] - pC[refPhaseIdx]));

    // make the fluid state consistent with local thermodynamic
    // equilibrium
    typename FluidSystem::template ParameterCache<typename FluidState::Scalar> paramCache;
    ComputeFromReferencePhase::solve(fs,
                                     paramCache,
                                     refPhaseIdx,
                                     /*setViscosity=*/false,
                                     /*setEnthalpy=*/false);
}

template<class Scalar>
struct Fixture {
    using FluidSystem = Opm::H2ON2FluidSystem<Scalar>;
    using CompositionalFluidState = Opm::CompositionalFluidState<Scalar, FluidSystem>;

    enum { numPhases = FluidSystem::numPhases };
    enum { numComponents = FluidSystem::numComponents };
    enum { liquidPhaseIdx = FluidSystem::liquidPhaseIdx };
    enum { gasPhaseIdx = FluidSystem::gasPhaseIdx };

    enum { H2OIdx = FluidSystem::H2OIdx };
    enum { N2Idx = FluidSystem::N2Idx };

    using MaterialLawTraits = Opm::TwoPhaseMaterialTraits<Scalar, liquidPhaseIdx, gasPhaseIdx>;
    using EffMaterialLaw = Opm::RegularizedBrooksCorey<MaterialLawTraits>;
    using MaterialLaw = Opm::EffToAbsLaw<EffMaterialLaw>;
    using MaterialLawParams = typename MaterialLaw::Params;

    Fixture()
    {
        Scalar T = 273.15 + 25;

        // initialize the tables of the fluid system
        Scalar Tmin = T - 1.0;
        Scalar Tmax = T + 1.0;
        unsigned nT = 3;

        Scalar pmin = 0.0;
        Scalar pmax = 1.25 * 2e6;
        unsigned np = 100;

        FluidSystem::init(Tmin, Tmax, nT, pmin, pmax, np);

        // set the parameters for the capillary pressure law
        matParams.setResidualSaturation(MaterialLaw::wettingPhaseIdx, 0.0);
        matParams.setResidualSaturation(MaterialLaw::nonWettingPhaseIdx, 0.0);
        matParams.setEntryPressure(0);
        matParams.setLambda(2.0);
        matParams.finalize();

        // create an fluid state which is consistent

        // set the fluid temperatures
        fsRef.setTemperature(T);
    }

    static Fixture& getInstance()
    {
        // we use a pointer to make sure data is in the heap and not the bss
        static std::unique_ptr<Fixture> instance;
        if (!instance) {
            instance = std::make_unique<Fixture>();
        }

        return *instance;
    }

    CompositionalFluidState fsRef;
    MaterialLawParams matParams;
};

using Types = std::tuple<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(SinglePhaseGas, Scalar, Types)
{
    auto& fixture = Fixture<Scalar>::getInstance();
    using FluidSystem = typename Fixture<Scalar>::FluidSystem;
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;

    // only gas
    ////////////////
    std::cout << "testing single-phase gas\n";

    // set gas saturation
    fixture.fsRef.setSaturation(fixture.gasPhaseIdx, 1.0);

    // set pressure of the gas phase
    fixture.fsRef.setPressure(fixture.gasPhaseIdx, 1e6);

    // set the gas composition to 99.9% nitrogen and 0.1% water
    fixture.fsRef.setMoleFraction(fixture.gasPhaseIdx,
                                  fixture.N2Idx, 0.999);
    fixture.fsRef.setMoleFraction(fixture.gasPhaseIdx,
                                  fixture.H2OIdx, 0.001);

    // "complete" the fluid state
    completeReferenceFluidState<Scalar, FluidSystem, MaterialLaw>(fixture.fsRef,
                                                                  fixture.matParams,
                                                                  fixture.gasPhaseIdx);

    // check the flash calculation
    checkNcpFlash<Scalar, FluidSystem, MaterialLaw>(fixture.fsRef, fixture.matParams);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SinglePhaseLiquid, Scalar, Types)
{
    auto& fixture = Fixture<Scalar>::getInstance();
    using FluidSystem = typename Fixture<Scalar>::FluidSystem;
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;

    ////////////////
    // only liquid
    ////////////////
    std::cout << "testing single-phase liquid\n";

    // set liquid saturation
    fixture.fsRef.setSaturation(fixture.liquidPhaseIdx, 1.0);

    // set pressure of the liquid phase
    fixture.fsRef.setPressure(fixture.liquidPhaseIdx, 2e5);

    // set the liquid composition to pure water
    fixture.fsRef.setMoleFraction(fixture.liquidPhaseIdx,
                                  fixture.N2Idx, 0.0);
    fixture.fsRef.setMoleFraction(fixture.liquidPhaseIdx,
                                  fixture.H2OIdx,
                                  1.0 - fixture.fsRef.moleFraction(fixture.liquidPhaseIdx, fixture.N2Idx));

    // "complete" the fluid state
    completeReferenceFluidState<Scalar, FluidSystem, MaterialLaw>(fixture.fsRef,
                                                                  fixture.matParams,
                                                                  fixture.liquidPhaseIdx);

    // check the flash calculation
    checkNcpFlash<Scalar, FluidSystem, MaterialLaw>(fixture.fsRef, fixture.matParams);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(TwoPhase, Scalar, Types)
{
    auto& fixture = Fixture<Scalar>::getInstance();
    using FluidSystem = typename Fixture<Scalar>::FluidSystem;
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;

    ////////////////
    // both phases
    ////////////////
    std::cout << "testing two-phase\n";

    // set saturations
    fixture.fsRef.setSaturation(fixture.liquidPhaseIdx, 0.5);
    fixture.fsRef.setSaturation(fixture.gasPhaseIdx, 0.5);

    // set pressures
    fixture.fsRef.setPressure(fixture.liquidPhaseIdx, 1e6);
    fixture.fsRef.setPressure(fixture.gasPhaseIdx, 1e6);

    typename FluidSystem::template ParameterCache<Scalar> paramCache;
    using MiscibleMultiPhaseComposition = Opm::MiscibleMultiPhaseComposition<Scalar, FluidSystem>;
    MiscibleMultiPhaseComposition::solve(fixture.fsRef, paramCache,
                                         /*setViscosity=*/false,
                                         /*setEnthalpy=*/false);

    // check the flash calculation
    checkNcpFlash<Scalar, FluidSystem, MaterialLaw>(fixture.fsRef, fixture.matParams);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(TwoPhaseCapillaryPressure, Scalar, Types)
{
    auto& fixture = Fixture<Scalar>::getInstance();
    using FluidSystem = typename Fixture<Scalar>::FluidSystem;
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawParams = typename Fixture<Scalar>::MaterialLawParams;

    ////////////////
    // with capillary pressure
    ////////////////
    MaterialLawParams matParams2;
    matParams2.setResidualSaturation(MaterialLaw::wettingPhaseIdx, 0.0);
    matParams2.setResidualSaturation(MaterialLaw::nonWettingPhaseIdx, 0.0);
    matParams2.setEntryPressure(1e3);
    matParams2.setLambda(2.0);
    matParams2.finalize();

    // set gas saturation
    fixture.fsRef.setSaturation(fixture.gasPhaseIdx, 0.5);
    fixture.fsRef.setSaturation(fixture.liquidPhaseIdx, 0.5);

    // set pressure of the liquid phase
    fixture.fsRef.setPressure(fixture.liquidPhaseIdx, 1e6);

    // calulate the capillary pressure
    using PhaseVector = Dune::FieldVector<Scalar, Fixture<Scalar>::numPhases>;
    PhaseVector pC;
    MaterialLaw::capillaryPressures(pC, matParams2, fixture.fsRef);
    fixture.fsRef.setPressure(fixture.gasPhaseIdx,
                              fixture.fsRef.pressure(fixture.liquidPhaseIdx) +
                              (pC[fixture.gasPhaseIdx] - pC[fixture.liquidPhaseIdx]));

    using MiscibleMultiPhaseComposition = Opm::MiscibleMultiPhaseComposition<Scalar, FluidSystem>;
    typename FluidSystem::template ParameterCache<Scalar> paramCache;
    MiscibleMultiPhaseComposition::solve(fixture.fsRef, paramCache,
                                         /*setViscosity=*/false,
                                         /*setEnthalpy=*/false);

    // check the flash calculation
    checkNcpFlash<Scalar, FluidSystem, MaterialLaw>(fixture.fsRef, matParams2);
}
