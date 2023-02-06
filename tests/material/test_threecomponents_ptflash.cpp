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
/*!
 * \file
 *
 * \brief This is test for the PTFlash flash solver.
 */
#include "config.h"

#define BOOST_TEST_MODULE PtFlash
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidsystems/ThreeComponentFluidSystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

// It is a three component system
using Scalar = double;
using FluidSystem = Opm::ThreeComponentFluidSystem<Scalar>;

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

    for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
        std::cout << " x for component: " << comp_idx << "is " << x[comp_idx] << std::endl;
         for (int i = 0; i < 3; ++i) {
             std::cout << " x deriv " << i << " is: " << x[comp_idx].derivative(i) << std::endl;
         }

        std::cout << " y for component: " << comp_idx << "is " << y[comp_idx] << std::endl;
         for (int i = 0; i < 3; ++i) {
             std::cout << " y deriv " << i << " is: " << y[comp_idx].derivative(i) << std::endl;
         }
    }
    std::cout << " L is " << L << std::endl;
     for (int i = 0; i < L.size(); ++i) {
             std::cout << " L deriv " << i << " is: " << L.derivative(i) << std::endl;
     }


    Evaluation ref_L = 1 - 0.763309246;
    ref_L.setDerivative(0, 4.072857907696467e-8);
    ref_L.setDerivative(1, -1.1606117844565438);
    ref_L.setDerivative(2, -1.2182584016253868);

    ComponentVector ref_x;
    ref_x[0].setValue(0.134348016);
    ref_x[0].setDerivative(0, 1.225204984e-7);
    ref_x[0].setDerivative(1, 0.1193427625186);
    ref_x[0].setDerivative(2, -0.15685356397);

    ref_x[1].setValue(0.021791990);
    ref_x[1].setDerivative(0, 2.1923329015033e-8);
    ref_x[1].setDerivative(1, -0.030587169734517);
    ref_x[1].setDerivative(2, 0.0402010686143);

    ref_x[2].setValue(0.84385999349);
    ref_x[2].setDerivative(0, -1.44443827440285e-7);
    ref_x[2].setDerivative(1, -0.088755592784150);
    ref_x[2].setDerivative(2, 0.11665249535641);

    ComponentVector ref_y;
    ref_y[0].setValue(0.61338319);
    ref_y[0].setDerivative(0, -1.2431457946797125e-8);
    ref_y[0].setDerivative(1, 0.5447055650444589);
    ref_y[0].setDerivative(2, -0.7159127825498286);

    ref_y[1].setValue(0.38626813278337335);
    ref_y[1].setDerivative(0, 1.2649586224979342e-8);
    ref_y[1].setDerivative(1, -0.5447013877995585);
    ref_y[1].setDerivative(2, 0.7159072923488614);

    ref_y[2].setValue(0.00034866911404565206);
    ref_y[2].setDerivative(0,-2.1812827818225162e-10);
    ref_y[2].setDerivative(1, -4.177244900520176e-6);
    ref_y[2].setDerivative(2, 5.490200967341757e-6);

    for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(x[comp_idx],
                                                                 ref_x[comp_idx], 2e-3),
                            "component " << comp_idx << " of x does not match");
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(y[comp_idx],
                                                                 ref_y[comp_idx], 2e-3),
                            "component " << comp_idx << " of y does not match");
    }

    BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(L, ref_L, 2e-3),
                        "L does not match");
}
