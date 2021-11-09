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
 * \brief This is test for the SPE5 fluid system (which uses the
 *        Peng-Robinson EOS) and the NCP flash solver.
 */
#include "config.h"

#include <opm/material/constraintsolvers/ChiFlash.hpp>
#include <opm/material/fluidsystems/chifluid/twophasefluidsystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/constraintsolvers/NcpFlash.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/Spe5FluidSystem.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>

#include <dune/common/parallel/mpihelper.hh>

void testChiFlash()
{
    using Scalar = double;
    using FluidSystem = Opm::TwoPhaseThreeComponentFluidSystem<Scalar>;

    constexpr auto numComponents = FluidSystem::numComponents;
    typedef Dune::FieldVector<Scalar, numComponents> ComponentVector;
    typedef Opm::CompositionalFluidState<Scalar, FluidSystem> FluidState;

    typedef Opm::LinearMaterial<MaterialTraits> MaterialLaw;
    typedef typename MaterialLaw::Params MaterialLawParams;

    typedef typename FluidSystem::template ParameterCache<Scalar> ParameterCache;

    ////////////
    // Initialize the fluid system and create the capillary pressure
    // parameters
    ////////////
    Scalar T = 273.15 + 20; // 20 deg Celsius
    FluidSystem::init(/*minTemperature=*/T - 1,
                      /*maxTemperature=*/T + 1,
                      /*minPressure=*/1.0e4,
                      /*maxTemperature=*/40.0e6);

    // set the parameters for the capillary pressure law
    MaterialLawParams matParams;
    for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
        matParams.setPcMinSat(phaseIdx, 0.0);
        matParams.setPcMaxSat(phaseIdx, 0.0);
    }
    matParams.finalize();

    ////////////
    // Create a fluid state
    ////////////
    FluidState gasFluidState;
    createSurfaceGasFluidSystem<FluidSystem>(gasFluidState);

    FluidState fluidState;
    ParameterCache paramCache;

    // temperature
    fluidState.setTemperature(T);

    // oil pressure
    fluidState.setPressure(oilPhaseIdx, 4000 * 6894.7573); // 4000 PSI

    // oil saturation
    fluidState.setSaturation(oilPhaseIdx, 1.0);
    fluidState.setSaturation(gasPhaseIdx, 1.0 - fluidState.saturation(oilPhaseIdx));

    // oil composition: SPE-5 reservoir oil
    fluidState.setMoleFraction(oilPhaseIdx, H2OIdx, 0.0);
    fluidState.setMoleFraction(oilPhaseIdx, C1Idx, 0.50);
    fluidState.setMoleFraction(oilPhaseIdx, C3Idx, 0.03);
    fluidState.setMoleFraction(oilPhaseIdx, C6Idx, 0.07);
    fluidState.setMoleFraction(oilPhaseIdx, C10Idx, 0.20);
    fluidState.setMoleFraction(oilPhaseIdx, C15Idx, 0.15);
    fluidState.setMoleFraction(oilPhaseIdx, C20Idx, 0.05);

    //makeOilSaturated<Scalar, FluidSystem>(fluidState, gasFluidState);

    // set the saturations and pressures of the other phases
    for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
        if (phaseIdx != oilPhaseIdx) {
            fluidState.setSaturation(phaseIdx, 0.0);
            fluidState.setPressure(phaseIdx, fluidState.pressure(oilPhaseIdx));
        }

        // initial guess for the composition (needed by the ComputeFromReferencePhase
        // constraint solver. TODO: bug in ComputeFromReferencePhase?)
        guessInitial<FluidSystem>(fluidState, phaseIdx);
    }

    typedef Opm::ComputeFromReferencePhase<Scalar, FluidSystem> CFRP;
    CFRP::solve(fluidState,
                paramCache,
                /*refPhaseIdx=*/oilPhaseIdx,
                /*setViscosity=*/false,
                /*setEnthalpy=*/false);

    ////////////
    // Calculate the total molarities of the components
    ////////////
    ComponentVector totalMolarities;
    for (unsigned compIdx = 0; compIdx < numComponents; ++ compIdx)
        totalMolarities[compIdx] = fluidState.saturation(oilPhaseIdx)*fluidState.molarity(oilPhaseIdx, compIdx);

    ////////////
    // Gradually increase the volume for and calculate the gas
    // formation factor, oil formation volume factor and gas formation
    // volume factor.
    ////////////

    FluidState flashFluidState, surfaceFluidState;
    flashFluidState.assign(fluidState);
    //Flash::guessInitial(flashFluidState, totalMolarities);

    using E = Opm::DenseAd::Evaluation<double, 3>;
    using Flash = Opm::ChiFlash<double, E, FluidSystem>;
    Flash::solve(flashFluidState, {0.9, 0.1}, 123456, 5, "SSI", 1e-8);

}

int main(int argc, char **argv)
{
    Dune::MPIHelper::instance(argc, argv);

    testAll<double>();

    // the Peng-Robinson test currently does not work with single-precision floating
    // point scalars because of precision issues. (these are caused by the fact that the
    // test uses finite differences to calculate derivatives.)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunreachable-code"
    while (0) testAll<float>();
#pragma GCC diagnostic pop


    testChiFlash();

    return 0;
}
