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

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidsystems/Co2BrineFluidSystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

#include <dune/common/parallel/mpihelper.hh>

#include <stdexcept>

// It is a two component system
using Scalar = double;
using FluidSystem = Opm::Co2BrineFluidSystem<Scalar>;

constexpr auto numComponents = FluidSystem::numComponents;
using Evaluation = Opm::DenseAd::Evaluation<double, numComponents>;
typedef Dune::FieldVector<Evaluation, numComponents> ComponentVector;
typedef Opm::CompositionalFluidState<Evaluation, FluidSystem> FluidState;

bool result_okay(const FluidState& fluid_state);

bool testPTFlash(const std::string& flash_twophase_method)
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
    const int flash_verbosity = 1;

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
    Flash::solve(fluid_state, z, spatialIdx, flash_verbosity, flash_twophase_method, flash_tolerance);

    return result_okay(fluid_state);
}

bool result_okay(const FluidState& fluid_state)
{
    bool res_okay = true;
    auto almost_equal = [](const double x, const double y, const double rel_tol = 2.e-3, const double abs_tol = 1.e-3)->bool {
        return std::fabs(x - y) <= rel_tol * std::fabs(x + y) * 2 || std::fabs(x - y) < abs_tol;
    };

    auto eval_almost_equal = [almost_equal](const Evaluation& val, const Evaluation& ref) -> bool {
        bool equal_okay = true;
        if (!almost_equal(val.value(), ref.value())) {
            equal_okay = false;
            std::cout << " the value are different with " << val.value() << " against the reference " << ref.value() << std::endl;
        }

        for (int i = 0; i < val.size(); ++i) {
            if (!almost_equal(val.derivative(i), ref.derivative(i))) {
                equal_okay = false;
                std::cout << " the " << i << "th derivative is different with value " << val.derivative(i) << " against the reference " << ref.derivative(i) << std::endl;
            }
        }

        return equal_okay;
    };

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
        if (!eval_almost_equal(x[comp_idx], ref_x[comp_idx])) {
            res_okay = false;
            std::cout << " the " << comp_idx << "th x does not match" << std::endl;
        }
        if (!eval_almost_equal(y[comp_idx], ref_y[comp_idx])) {
            res_okay = false;
            std::cout << " the " << comp_idx << "th x does not match" << std::endl;
        }
    }

    if (!eval_almost_equal(L, ref_L)) {
        res_okay = false;
        std::cout << " the L does not match" << std::endl;
    }

    // TODO: we should also check densities, viscosities, saturations and so on

    return res_okay;
}

int main(int argc, char **argv)
{
    Dune::MPIHelper::instance(argc, argv);
    bool test_passed = true;

    std::vector<std::string> test_methods {"newton", "ssi", "ssi+newton"};

    for (const auto& method : test_methods) {
        if (!testPTFlash(method) ) {
            std::cout << method << " solution for PTFlash failed " << std::endl;
            test_passed = false;
        } else {
            std::cout << method << " solution for PTFlash passed " << std::endl;
        }
    }

    if (!test_passed) {
        throw std::runtime_error(" PTFlash tests failed");
    }

    return 0;
}