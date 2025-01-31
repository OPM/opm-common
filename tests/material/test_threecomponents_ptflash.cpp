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

#include <boost/version.hpp>
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 > 66
#include <boost/test/data/test_case.hpp>
#endif

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidsystems/ThreeComponentFluidSystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <opm/json/JsonObject.hpp>

// It is a three component system
using Scalar = double;
using EOSType = Opm::CompositionalConfig::EOSType;
using FluidSystem = Opm::ThreeComponentFluidSystem<Scalar>;

constexpr auto numComponents = FluidSystem::numComponents;
using Evaluation = Opm::DenseAd::Evaluation<double, numComponents>;
using ComponentVector = Dune::FieldVector<Evaluation, numComponents>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;

std::vector<std::string> test_methods {"newton", "ssi", "ssi+newton"};
// std::vector<std::string> test_methods {"ssi"};
std::vector<EOSType> test_eos_types {EOSType::PR, EOSType::PRCORR, EOSType::SRK, EOSType::RK};

#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 > 66
BOOST_DATA_TEST_CASE(PtFlash, test_methods)
#else
BOOST_AUTO_TEST_CASE(PtFlash)
#endif
{
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
for (const auto& sample : test_methods) {
#endif

    // Initialize JSON file with reference values
    std::filesystem::path jsonFile("material/ref_values_threecomponents_ptflash.json");
    Json::JsonObject parser(jsonFile);
    
    // Initial: the primary variables are, pressure, molar fractions of the first and second component
    Evaluation p_init = Evaluation::createVariable(10e5, 0); // 10 bar
    ComponentVector comp;
    comp[0] = Evaluation::createVariable(0.5, 1);
    comp[1] = Evaluation::createVariable(0.3, 2);
    comp[2] = 1. - comp[0] - comp[1];

    const Scalar temp = 300.0;

    // Loop over EOS types
    for (const auto& eos_type : test_eos_types) {
        // FluidState will be the input for the flash calculation
        FluidState fluid_state;
        fluid_state.setPressure(FluidSystem::oilPhaseIdx, p_init);
        fluid_state.setPressure(FluidSystem::gasPhaseIdx, p_init);

        fluid_state.setMoleFraction(FluidSystem::Comp0Idx, comp[0]);
        fluid_state.setMoleFraction(FluidSystem::Comp1Idx, comp[1]);
        fluid_state.setMoleFraction(FluidSystem::Comp2Idx, comp[2]);

        fluid_state.setTemperature(temp);

        const double flash_tolerance = 1.e-8; // just to test the setup in co2-compositional
        const int flash_verbosity = 0;

        // TODO: should we set these?
        // Set initial K and L
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            const Evaluation Ktmp = fluid_state.wilsonK_(compIdx);
            fluid_state.setKvalue(compIdx, Ktmp);
        }
        const Evaluation Ltmp = 1.;
        fluid_state.setLvalue(Ltmp);

        using Flash = Opm::PTFlash<double, FluidSystem>;
        Flash::solve(fluid_state, sample, flash_tolerance, eos_type, flash_verbosity);

        ComponentVector x, y;
        const Evaluation L = fluid_state.L();
        for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            x[comp_idx] = fluid_state.moleFraction(FluidSystem::oilPhaseIdx, comp_idx);
            y[comp_idx] = fluid_state.moleFraction(FluidSystem::gasPhaseIdx, comp_idx);
        }

        if (flash_verbosity >= 1) {
            std::cout << "Results for " << Opm::CompositionalConfig::eosTypeToString(eos_type) << " :" << std::endl;
            for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
                std::cout << " x for component " << comp_idx << " :" << std::endl;
                std::cout << " \tvalue = " << x[comp_idx].value() << std::endl;
                for (int i = 0; i < 3; ++i) {
                    std::cout << " \tderiv " << i << " = " << x[comp_idx].derivative(i) << std::endl;
                }

                std::cout << " y for component " << comp_idx << ":" << std::endl;
                std::cout << " \tvalue = " << y[comp_idx].value() << std::endl;
                for (int i = 0; i < 3; ++i) {
                    std::cout << " \tderiv " << i << " = " << y[comp_idx].derivative(i) << std::endl;
                }
            }
            std::cout << " L: " << std::endl; 
            std::cout << " \tvalue = " << L.value() << std::endl;
            for (int i = 0; i < L.size(); ++i) {
                    std::cout << " \tderiv " << i << " = " << L.derivative(i) << std::endl;
            }
        }

        // Reference values from JSON file
        const auto eos_string = Opm::CompositionalConfig::eosTypeToString(eos_type);
        Json::JsonObject eos_ref = parser.get_item(eos_string);

        Json::JsonObject L_ref_array = eos_ref.get_item("L");
        Evaluation ref_L = L_ref_array.get_array_item(0).as_double();
        for (unsigned i = 0; i < 3; ++i) {
            ref_L.setDerivative(i, L_ref_array.get_array_item(i + 1).as_double());
        }

        Json::JsonObject x_ref_array = eos_ref.get_item("x");
        ComponentVector ref_x;
        for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            Json::JsonObject x_ref_row = x_ref_array.get_array_item(comp_idx);
            ref_x[comp_idx].setValue(x_ref_row.get_array_item(0).as_double());
            for (unsigned i = 0; i < 3; ++i) {
                ref_x[comp_idx].setDerivative(i, x_ref_row.get_array_item(i + 1).as_double());
            }
        }

        Json::JsonObject y_ref_array = eos_ref.get_item("y");
        ComponentVector ref_y;
        for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            Json::JsonObject y_ref_row = y_ref_array.get_array_item(comp_idx);
            ref_y[comp_idx].setValue(y_ref_row.get_array_item(0).as_double());
            for (unsigned i = 0; i < 3; ++i) {
                ref_y[comp_idx].setDerivative(i, y_ref_row.get_array_item(i + 1).as_double());
            }
        }

        for (unsigned comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(x[comp_idx],
                                                                     ref_x[comp_idx], 2e-3),
                                "EOS type " << eos_string << ": component " << comp_idx << " of x does not match");
            BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(y[comp_idx],
                                                                     ref_y[comp_idx], 2e-3),
                                "EOS type " << eos_string << ": component " << comp_idx << " of y does not match");
        }

        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(L, ref_L, 2e-3),
                            "EOS type " << eos_string << ": L does not match");
    }
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
}
#endif
}
