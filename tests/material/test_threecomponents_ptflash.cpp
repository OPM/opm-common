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

#include <fmt/format.h>

// It is a three component system
using Scalar = double;
using EOSType = Opm::CompositionalConfig::EOSType;
using FluidSystem = Opm::ThreeComponentFluidSystem<Scalar>;

constexpr int numComponents = FluidSystem::numComponents;
constexpr int numPrimaryVariables = numComponents + 1; // pressure + temperature + molar fractions of the first n-1 component
using Evaluation = Opm::DenseAd::Evaluation<double, numPrimaryVariables>;
using ComponentVector = Dune::FieldVector<Evaluation, numComponents>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;

std::vector<std::string> test_methods {"newton", "ssi", "ssi+newton"};
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

    // Initial: the primary variables are pressure, temperature, and the overall molar fractions of the first and second components
    Evaluation p_init = Evaluation::createVariable(10e5, 0); // 10 bar
    Evaluation T_init = Evaluation::createVariable(300.0, 1); // 300 K

    ComponentVector comp;
    comp[0] = Evaluation::createVariable(0.5, 2);
    comp[1] = Evaluation::createVariable(0.3, 3);
    comp[2] = 1. - comp[0] - comp[1];

    constexpr bool output_results_json = false;
    // Build a JSON string with simulation results, which can be used for investigation
    // or generating reference values for future if needed.
    [[maybe_unused]] std::string json_output;
    if constexpr (output_results_json) {
        json_output = "{\n";
    }
    for (const auto& eos_type : test_eos_types) {
        // FluidState will be the input for the flash calculation
        FluidState fluid_state;
        fluid_state.setPressure(FluidSystem::oilPhaseIdx, p_init);
        fluid_state.setPressure(FluidSystem::gasPhaseIdx, p_init);

        fluid_state.setMoleFraction(FluidSystem::Comp0Idx, comp[0]);
        fluid_state.setMoleFraction(FluidSystem::Comp1Idx, comp[1]);
        fluid_state.setMoleFraction(FluidSystem::Comp2Idx, comp[2]);

        fluid_state.setTemperature(T_init);

        const double flash_tolerance = 1.e-8; // just to test the setup in co2-compositional
        constexpr int flash_verbosity = 0;

        // TODO: should we set these?
        // Set initial K and L
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            const Evaluation Ktmp = fluid_state.wilsonK_(compIdx);
            fluid_state.setKvalue(compIdx, Ktmp);
        }
        const Evaluation Ltmp = 1.;
        fluid_state.setLvalue(Ltmp);

        // thermal PTFlash
        using Flash = Opm::PTFlash<double, FluidSystem, true>;
        Flash::solve(fluid_state, sample, flash_tolerance, eos_type, flash_verbosity);

        ComponentVector x, y;
        const Evaluation L = fluid_state.L();
        for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            x[comp_idx] = fluid_state.moleFraction(FluidSystem::oilPhaseIdx, comp_idx);
            y[comp_idx] = fluid_state.moleFraction(FluidSystem::gasPhaseIdx, comp_idx);
        }


        // Reference values from JSON file
        const auto eos_string = Opm::CompositionalConfig::eosTypeToString(eos_type);
        Json::JsonObject eos_ref = parser.get_item(eos_string);

        Json::JsonObject L_ref_array = eos_ref.get_item("L");
        Evaluation ref_L = L_ref_array.get_array_item(0).as_double();
        for (int i = 0; i < numPrimaryVariables; ++i) {
            ref_L.setDerivative(i, L_ref_array.get_array_item(i + 1).as_double());
        }

        Json::JsonObject x_ref_array = eos_ref.get_item("x");
        ComponentVector ref_x;
        for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            Json::JsonObject x_ref_row = x_ref_array.get_array_item(comp_idx);
            ref_x[comp_idx].setValue(x_ref_row.get_array_item(0).as_double());
            for (int i = 0; i < numPrimaryVariables; ++i) {
                ref_x[comp_idx].setDerivative(i, x_ref_row.get_array_item(i + 1).as_double());
            }
        }

        Json::JsonObject y_ref_array = eos_ref.get_item("y");
        ComponentVector ref_y;
        for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            Json::JsonObject y_ref_row = y_ref_array.get_array_item(comp_idx);
            ref_y[comp_idx].setValue(y_ref_row.get_array_item(0).as_double());
            for (int i = 0; i < numPrimaryVariables; ++i) {
                ref_y[comp_idx].setDerivative(i, y_ref_row.get_array_item(i + 1).as_double());
            }
        }

        for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
            BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(x[comp_idx],
                                                                     ref_x[comp_idx], 2e-3),
                                "EOS type " << eos_string << ": component " << comp_idx << " of x does not match");
            BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(y[comp_idx],
                                                                     ref_y[comp_idx], 2e-3),
                                "EOS type " << eos_string << ": component " << comp_idx << " of y does not match");
        }

        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(L, ref_L, 2e-3),
                            "EOS type " << eos_string << ": L does not match");

        if (output_results_json) {
            json_output += fmt::format("    \"{}\": {{\n", eos_string);

            auto fmt_eval = [](const Evaluation& eval) {
                std::string s = fmt::format("[{}", eval.value());
                for (int i = 0; i < numPrimaryVariables; ++i) {
                    s += fmt::format(", {}", eval.derivative(i));
                }
                s += "]";
                return s;
            };

            // x
            json_output += "        \"x\" : [\n";
            for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
                json_output += fmt::format("            {}{}\n",
                    fmt_eval(x[comp_idx]),
                    comp_idx < numComponents - 1 ? "," : "");
            }
            json_output += "        ],\n";

            // y
            json_output += "        \"y\" : [\n";
            for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
                json_output += fmt::format("            {}{}\n",
                    fmt_eval(y[comp_idx]),
                    comp_idx < numComponents - 1 ? "," : "");
            }
            json_output += "        ],\n";

            // L
            json_output += fmt::format("        \"L\" : {}\n", fmt_eval(L));
            json_output += "    },\n\n";
        }
    }
    if (output_results_json) {
        json_output += fmt::format("    \"T\" : {:.1f},\n", T_init.value());
        json_output += "    \"P\" : 10e5,\n";
            json_output += "    \"z\" : [";
            for (int comp_idx = 0; comp_idx < numComponents; ++comp_idx) {
                json_output += fmt::format("{}{}", Opm::getValue(comp[comp_idx]), comp_idx < numComponents - 1 ? "," : "");
            }
            json_output += "]\n";
        json_output += "}\n";
        std::cout << json_output;
    }
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
}
#endif
}
