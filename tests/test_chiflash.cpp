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
 * \brief This is test for the ChiFlash flash solver.
 */
#include "config.h"

#include <opm/material/constraintsolvers/ChiFlash.hpp>
#include <opm/material/fluidsystems/ThreeComponentFluidSystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

#include <dune/common/parallel/mpihelper.hh>

void testChiFlash()
{
    using Scalar = double;
    using FluidSystem = Opm::ThreeComponentFluidSystem<Scalar>;

    constexpr auto numComponents = FluidSystem::numComponents;
    using Evaluation = Opm::DenseAd::Evaluation<double, numComponents>;
    typedef Dune::FieldVector<Evaluation, numComponents> ComponentVector;
    typedef Opm::CompositionalFluidState<Evaluation, FluidSystem> FluidState;

// It is a three component system
    // Initial: the primary variables are, pressure, molar fractions of the first and second component
    Evaluation p_init = Evaluation::createVariable(10e5, 0); // 10 bar
    ComponentVector comp;
    comp[0] = Evaluation::createVariable(0.5, 1);
    comp[1] = Evaluation::createVariable(0.3, 2);
    comp[2] = 1. - comp[0] - comp[1];
    
    // TODO: not sure whether the saturation matter here.
    ComponentVector sat;
    // We assume that currently everything is in the oil phase
    sat[0] = 1.0; sat[1] = 1.0-sat[0];
    Scalar temp = 300.0;

    
    // FluidState will be the input for the flash calculation
    FluidState fluid_state;
    fluid_state.setPressure(FluidSystem::oilPhaseIdx, p_init);
    fluid_state.setPressure(FluidSystem::gasPhaseIdx, p_init);

    fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp1Idx, comp[1]);
    fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp2Idx, comp[2]);

    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp1Idx, comp[1]);
    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp2Idx, comp[2]);

    // It is used here only for calculate the z
    fluid_state.setSaturation(FluidSystem::oilPhaseIdx, sat[0]);
    fluid_state.setSaturation(FluidSystem::gasPhaseIdx, sat[1]);

    fluid_state.setTemperature(temp);

    // ParameterCache paramCache;
    {
        typename FluidSystem::template ParameterCache<Evaluation> paramCache;
        paramCache.updatePhase(fluid_state, FluidSystem::oilPhaseIdx);
        paramCache.updatePhase(fluid_state, FluidSystem::gasPhaseIdx);
        fluid_state.setDensity(FluidSystem::oilPhaseIdx, FluidSystem::density(fluid_state, paramCache, FluidSystem::oilPhaseIdx));
        fluid_state.setDensity(FluidSystem::gasPhaseIdx, FluidSystem::density(fluid_state, paramCache, FluidSystem::gasPhaseIdx));
    }

    ComponentVector z(0.); // TODO; z needs to be normalized.
    {
        Scalar sumMoles = 0.0;
        for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
            for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                Scalar tmp = Opm::getValue(fluid_state.molarity(phaseIdx, compIdx) * fluid_state.saturation(phaseIdx));
                z[compIdx] += Opm::max(tmp, 1e-8);
                sumMoles += tmp;
            }
        }
        z /= sumMoles;
        // p And z is the primary variables
        Evaluation z_last = 1.;
        for (unsigned compIdx = 0; compIdx < numComponents - 1; ++compIdx) {
            z[compIdx] = Evaluation::createVariable(Opm::getValue(z[compIdx]), compIdx + 1);
            z_last -= z[compIdx];
        }
        z[numComponents - 1] = z_last;
    }

    const double flash_tolerance = 1.e-12; // just to test the setup in co2-compositional
    const int flash_verbosity = 1;
    const std::string flash_twophase_method = "newton";

    // TODO: should we set these?
    // Set initial K and L
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        const Evaluation Ktmp = fluid_state.wilsonK_(compIdx);
        fluid_state.setKvalue(compIdx, Ktmp);
    }
    const Evaluation Ltmp = 1.;
    fluid_state.setLvalue(Ltmp);

    const int spatialIdx = 0;
    using Flash = Opm::ChiFlash<double, FluidSystem>;
    Flash::solve(fluid_state, z, spatialIdx, flash_verbosity, flash_twophase_method, flash_tolerance);

}

int main(int argc, char **argv)
{
    Dune::MPIHelper::instance(argc, argv);

    testChiFlash();

    return 0;
}