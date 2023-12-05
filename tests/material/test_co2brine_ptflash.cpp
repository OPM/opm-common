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

#include <boost/version.hpp>
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 > 66
#include <boost/test/data/test_case.hpp>
#endif

#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/fluidsystems/Co2BrineFluidSystem.hh>
#include <opm/material/fluidsystems/ThreeComponentFluidSystem.hh>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

#include <stdexcept>

// It is a two component system
using Scalar = double;
using FluidSystem = Opm::Co2BrineFluidSystem<Scalar>;
using ThreeComponentSystem = Opm::ThreeComponentFluidSystem<Scalar>;

constexpr auto numComponents = FluidSystem::numComponents;
using Evaluation = Opm::DenseAd::Evaluation<double, numComponents>;
using ComponentVector = Dune::FieldVector<Evaluation, numComponents>;
using FluidState = Opm::CompositionalFluidState<Evaluation, FluidSystem>;

std::vector<std::string> test_methods {"newton", "ssi", "ssi+newton"};

#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 > 66
BOOST_DATA_TEST_CASE(PtFlash, test_methods)
#else
BOOST_AUTO_TEST_CASE(PtFlash)
#endif
{
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
for (const auto& sample : test_methods) {
#endif
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

    using Flash = Opm::PTFlash<double, FluidSystem>;
    Flash::solve(fluid_state, z, sample, flash_tolerance, flash_verbosity);

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
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
}
#endif
}

#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 > 66
BOOST_DATA_TEST_CASE(PtFlashSingle, test_methods)
#else
BOOST_AUTO_TEST_CASE(PtFlashSingle)
#endif
{
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
for (const auto& sample : test_methods) {
#endif
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

    using Flash = Opm::PTFlash<double, FluidSystem>;
    Flash::solve(fluid_state, z, sample, flash_tolerance, flash_verbosity);

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
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 67
}
#endif
}

BOOST_AUTO_TEST_CASE(RachfordRice) {
    using NumVector = Dune::FieldVector<double, 3>;
    // TODO: Should not really need flash for this part.
    using Flash = Opm::PTFlash<double, ThreeComponentSystem>;

    // Scalar temp = 273.15;
    // Scalar p = 150000.0;

    // ComponentVector z;
    // z[0] = 0.2; z[1] = 0.5; z[2] = 0.3;
    auto K_values = std::vector<std::vector<double>>();
    auto z_values = std::vector<std::vector<double>>();
    auto vapor_reference = std::vector<double>();

    // sol for p = 1.5 bar, T = 273.15 K
    K_values.push_back({19742.008209810265, 104061.44705736745, 0.29692744936348753});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.9956234231343956);

    // sol for p = 112.44444444444444 bar, T = 273.15 K
    K_values.push_back({0.6247560532583887, 1.754176409580374, 0.00264809842113736});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.004634932674127616);

    // sol for p = 1.5 bar, T = 298.35555555555555 K
    K_values.push_back({5011.808655921476, 20394.761667099738, 0.2981316471374891});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.9973104175796784);

    // sol for p = 112.44444444444444 bar, T = 298.35555555555555 K
    K_values.push_back({0.699756966810626, 1.7243997063092453, 0.0032527792216226767});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.00540010197110916);

    // sol for p = 1.5 bar, T = 323.56111111111113 K
    K_values.push_back({1602.907259275084, 5278.701579138767, 0.29886066422781404});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.998280924170745);

    // sol for p = 112.44444444444444 bar, T = 323.56111111111113 K
    K_values.push_back({0.7857993293648643, 1.6860914516428702, 0.0052022817582215155});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.003259588429511987);

    // sol for p = 1.5 bar, T = 348.76666666666665 K
    K_values.push_back({623.5248499334857, 1730.8998902544013, 0.28483826298498793});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.9785380752533385);

    // sol for p = 112.44444444444444 bar, T = 348.76666666666665 K
    K_values.push_back({0.8738459013983119, 1.6483861730818257, 0.009126227905247714});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.003343693200458504);

    // sol for p = 1.5 bar, T = 373.97222222222223 K
    K_values.push_back({273.5620907524389, 654.8043484805389, 0.28778776891147734});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.9822111573271644);

    // sol for p = 112.44444444444444 bar, T = 373.97222222222223 K
    K_values.push_back({0.9594432989887618, 1.6087045715588943, 0.01668691940009185});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.002619159406464278);

    // sol for p = 1.5 bar, T = 399.1777777777778 K
    K_values.push_back({134.27575624313343, 283.4218134512125, 0.2904166028146171});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.9850934240313229);

    // sol for p = 112.44444444444444 bar, T = 399.1777777777778 K
    K_values.push_back({1.1056927210301593, 1.7637285066428006, 0.025528660493607223});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.1829805554210638);

    // sol for p = 112.44444444444444 bar, T = 424.3833333333333 K
    K_values.push_back({1.2988026430219273, 1.9765552313014425, 0.04040594253059525});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.33270889928548736);

    // sol for p = 112.44444444444444 bar, T = 449.5888888888889 K
    K_values.push_back({1.523322259507949, 2.2013619219246285, 0.06723018079131272});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.44310335029510134);

    // sol for p = 112.44444444444444 bar, T = 474.7944444444444 K
    K_values.push_back({1.7176562249206835, 2.3413966156487644, 0.11537246148979083});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.5269214180997791);

    // sol for p = 112.44444444444444 bar, T = 500.0 K
    K_values.push_back({1.787578933245329, 2.282308523254643, 0.19884032011593508});
    z_values.push_back({0.2, 0.5, 0.3});
    vapor_reference.push_back(0.6062547183490403);

    for(unsigned int i = 0; i < K_values.size(); i++){
        auto z_i = z_values[i];
        auto K_i = K_values[i];
        std::cout << "Perform RR test " << i << " of " << K_values.size() << std::endl
                  << "z = " << z_i[0] << ", "  << z_i[1]  << ", " << z_i[2] << std::endl
                  << "K = " << K_i[0] << ", "  << K_i[1]  << ", " << K_i[2] << std::endl;


        NumVector K = {K_i[0], K_i[1], K_i[2]};
        NumVector z = {z_i[0], z_i[1], z_i[2]};

        auto L = Flash::solveRachfordRice_g_(K, z, 3);
        auto L_ref = 1.0 - vapor_reference[i];

        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(L, L_ref, 1e-5),
                            "Test sol #" + std::to_string(i+1) +
                            " Computed vapor fraction " + std::to_string(L) +
                            " does not match reference " + std::to_string(L_ref)
        );
    }

    /*
    NumVector K(1.7176562249206835, 2.3413966156487644, 0.11537246148979083);

    NumVector z = {0.2, 0.5, 0.3};
    using Flash = Opm::PTFlash<double, FluidSystem>;
    auto V_ref = 0.5269214180997791;

   for(unsigned int i = 0; i < K_values.size(); i++){
        auto L = Flash::solveRachfordRice_g_(K, z, 1);
        auto V = 1.0 - L;
        BOOST_CHECK_MESSAGE(Opm::MathToolbox<Evaluation>::isSame(V, V_ref, 2e-3),
                            "Computed vapor fraction " + std::to_string(V) + "  does not match reference " + std::to_string(V_ref));
    }
    */
}
