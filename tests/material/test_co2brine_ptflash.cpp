// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 NORCE.
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
/*!
 * \file
 *
 * \brief This is test for the PTFlash flash solver.
 */
#include "config.h"

#define BOOST_TEST_MODULE Co2BrinePtFlash
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidsystems/Co2BrineFluidSystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

#include <stdexcept>

// It is a two component system
using Scalar = double;
using FluidSystem = Opm::Co2BrineFluidSystem<Scalar>;

constexpr auto numComponents = FluidSystem::numComponents;
using Evaluation = Opm::DenseAd::Evaluation<double, numComponents>;
using ComponentVector = Dune::FieldVector<Evaluation, numComponents>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;

std::vector<std::string> test_methods {"newton", "ssi", "ssi+newton"};

BOOST_DATA_TEST_CASE(PtFlash, test_methods)
{
    // Initial: the primary variables are, pressure, molar fractions of the first and second component
    Evaluation p_init = Evaluation::createVariable(10e5, 0); // 10 bar
    ComponentVector comp;
    comp[0] = Evaluation::createVariable(0.5, 1);
    comp[1] = 1. - comp[0];

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

    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp1Idx, comp[1]);

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
            z[compIdx] = Evaluation::createVariable(Opm::getValue(z[compIdx]), int(compIdx) + 1);
            z_last -= z[compIdx];
        }
        z[numComponents - 1] = z_last;
    }

    const double flash_tolerance = 1.e-12; // just to test the setup in co2-compositional
    const int flash_verbosity = 0;

    // TODO: should we set these?
    // Set initial K and L
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        const Evaluation Ktmp = fluid_state.wilsonK_(compIdx);
        fluid_state.setKvalue(compIdx, Ktmp);
    }
    const Evaluation Ltmp = 1.;
    fluid_state.setLvalue(Ltmp);

    const int spatialIdx = 0;
    using Flash = Opm::PTFlash<double, FluidSystem>;
    Flash::solve(fluid_state, z, spatialIdx, sample, flash_tolerance, flash_verbosity);

    ComponentVector x, y;
    const Evaluation L = fluid_state.L();
    for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
        x[comp_idx] = fluid_state.moleFraction(FluidSystem::oilPhaseIdx, comp_idx);
        y[comp_idx] = fluid_state.moleFraction(FluidSystem::gasPhaseIdx, comp_idx);
    }

    Evaluation ref_L = 1 - 0.5013878578252918;
    ref_L.setDerivative(0, -0.00010420367632860657);
    ref_L.setDerivative(1, -1.0043436395393446);

    ComponentVector ref_x;
    ref_x[0].setValue(0.0007805714232572864);
    ref_x[0].setDerivative(0, 4.316797623360392e-6);
    ref_x[0].setDerivative(1, 1.0842021724855044e-19);

    ref_x[1].setValue(0.9992194285767426);
    ref_x[1].setDerivative(0, -4.316797623360802e-6);
    ref_x[1].setDerivative(1, -2.220446049250313e-16);

    ComponentVector ref_y;
    ref_y[0].setValue(0.9964557174909056);
    ref_y[0].setDerivative(0, -0.00021122453746465807);
    ref_y[0].setDerivative(1, -2.220446049250313e-16);

    ref_y[1].setValue(0.003544282509094506);
    ref_y[1].setDerivative(0, -3.0239852847431828e-9);
    ref_y[1].setDerivative(1, -8.673617379884035e-19);

    for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(x[comp_idx], ref_x[comp_idx], 2e-3),
                            "component " << comp_idx << " of x does not match");
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(y[comp_idx], ref_y[comp_idx], 2e-3),
                            "component " << comp_idx << " of y does not match");
    }

    BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(L, ref_L, 2e-3),
                        "L does not match");
}

BOOST_DATA_TEST_CASE(PtFlashSingle, test_methods)
{
    // setting up a system that we know activates the calculations for a single-phase system
    // Initial: the primary variables are, pressure, molar fractions of the first and second component
    ComponentVector comp;
    Evaluation p_init = Evaluation::createVariable(9999307.201, 0);
    comp[0] = Evaluation::createVariable(0.99772060, 1);
    comp[1] = 1. - comp[0];
    Scalar temp = 300.0;
    ComponentVector sat;
    sat[0] = 1.0; sat[1] = 1.0-sat[0];

    // FluidState will be the input for the flash calculation
    FluidState fluid_state;
    fluid_state.setPressure(FluidSystem::oilPhaseIdx, p_init);
    fluid_state.setPressure(FluidSystem::gasPhaseIdx, p_init);

    fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fluid_state.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp1Idx, comp[1]);

    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fluid_state.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp1Idx, comp[1]);

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
            z[compIdx] = Evaluation::createVariable(Opm::getValue(z[compIdx]), int(compIdx) + 1);
            z_last -= z[compIdx];
        }
        z[numComponents - 1] = z_last;
    }

    const double flash_tolerance = 1.e-12;
    const int flash_verbosity = 0;

    // TODO: should we set these?
    // Set initial K and L
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        const Evaluation Ktmp = fluid_state.wilsonK_(compIdx);
        fluid_state.setKvalue(compIdx, Ktmp);
    }
    const Evaluation Ltmp = 1.;
    fluid_state.setLvalue(Ltmp);

    const int spatialIdx = 0;
    using Flash = Opm::PTFlash<double, FluidSystem>;
    Flash::solve(fluid_state, z, spatialIdx, sample, flash_tolerance, flash_verbosity);

    ComponentVector x, y;
    const Evaluation L = fluid_state.L();
    for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
        x[comp_idx] = fluid_state.moleFraction(FluidSystem::oilPhaseIdx, comp_idx);
        y[comp_idx] = fluid_state.moleFraction(FluidSystem::gasPhaseIdx, comp_idx);
    }

    Evaluation ref_L = 1.;

    ComponentVector ref_x = z;
    ComponentVector ref_y = z;

    for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(x[comp_idx], ref_x[comp_idx], 2e-3),
                            "component " + std::to_string(comp_idx) + " of x does not match");
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(y[comp_idx], ref_y[comp_idx], 2e-3),
                            "component " + std::to_string(comp_idx) + " of y does not match");
    }

    BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(L, ref_L, 2e-3),
                        "L does not match");

    // TODO: we should also check densities, viscosities, saturations and so on
}
