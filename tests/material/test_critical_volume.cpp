// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 SINTEF Digital

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
 * \brief ZCRIT sets the critical volumes, Vc = Zc R Tc / Pc, ahead of VCRIT.
 *
 * Component properties come from EQUIL_1D_COMPVD_WATER_GASCAP, which gives
 * both keywords. The expected critical volumes, and the PRESSURE, ZMF, DENG
 * and VGAS of cells 1 and 34 at report step 6, come from a run of its
 * contact-mismatch variant.
 */
#include "config.h"

#define BOOST_TEST_MODULE CriticalVolume
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <array>
#include <stdexcept>
#include <string>

namespace
{

using Scalar = double;
constexpr int numComponents = 7;
using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
using CompVec = std::array<Scalar, numComponents>;

constexpr auto eosType = Opm::CompositionalConfig::EOSType::PR;

// RTEMP = 100 degC.
constexpr Scalar temperature = 373.15;

const std::string vcritKeyword = R"(
VCRIT
 0.099 0.094 0.148 0.203 0.32321204 0.63255728 0.97988801 /
)";

const std::string zcritKeyword = R"(
ZCRIT
 0.2876 0.2736 0.2789 0.2763 0.250711098445488 0.23338268298063 0.250420786731195 /
)";

// VCRIT of the deck, in m^3/kmol.
const CompVec vcrit {0.099, 0.094, 0.148, 0.203, 0.32321204, 0.63255728, 0.97988801};

void
initFromDeck(const std::string& criticalVolumeKeywords)
{
    const auto deck = Opm::Parser {}.parseString(R"(
RUNSPEC
METRIC
DIMENS
 1 1 1 /
COMPS
7 /
TABDIMS
 1 /
OIL
GAS
WATER
GRID
DXV
 1 /
DYV
 1 /
DZV
 1 /
DEPTHZ
 4*2000 /
PROPS
CNAMES
 CH4 CO2 C2H6 C3H8 C4-C9 C10-C20 C21+ /
EOS
 PR /
BIC
 1.0500000E-01
 2.6890022E-03 1.3000000E-01
 8.5370405E-03 1.2500000E-01 1.6620489E-03
 2.2915937E-02 0.0000000E+00 1.0088696E-02 3.5952572E-03
 5.4875390E-02 0.0000000E+00 3.4227722E-02 2.1174720E-02 7.4706799E-03
 8.1972182E-02 0.0000000E+00 5.6906187E-02 4.0015457E-02 2.0180685E-02 3.1846386E-03 /
ACF
 0.008 0.225 0.098 0.152 0.2549 0.551 0.909 /
PCRIT
 45.4 72.8 48.2 41.9 32.4 20.42 14.5 /
TCRIT
 190.6 304.2 305.4 369.8 474.7 646.0 812.0 /
MW
 16.043 44.01 30.07 44.097 76.9 163.0 310.85 /
OMEGAA
 0.45724 0.45724 0.45724 0.45724 0.4572356 0.4572355 0.4572356 /
OMEGAB
 0.0778 0.0778 0.0778 0.0778 0.0777961 0.0777961 0.0777961 /
SSHIFT
 -0.1595 -0.0817 -0.1134 -0.0863 -0.0243568 0.10784125400986 0.206892761354429 /
)" + criticalVolumeKeywords + R"(
SOLUTION
SCHEDULE
END
)");
    const auto eclState = Opm::EclipseState {deck};
    const auto schedule = Opm::Schedule {deck, eclState};
    FluidSystem::initFromState(eclState, schedule);
}

struct PhaseProperties
{
    Scalar density {};
    Scalar viscosity {};
};

// Density and viscosity of a single-phase fluid on the EOS root of phaseIdx.
PhaseProperties
phaseProperties(const unsigned phaseIdx, const Scalar pressure, const CompVec& z)
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(phaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(phaseIdx, c, z[c]);
    }

    typename FluidSystem::template ParameterCache<Scalar> paramCache(eosType);
    paramCache.updatePhase(fs, phaseIdx);

    return {FluidSystem::density(fs, paramCache, phaseIdx),
            FluidSystem::viscosity(fs, paramCache, phaseIdx)};
}

// Vapour at the top of the gas cap (cell 1) and liquid just above the
// water-oil contact (cell 34). The compositions are the normalized COMPVD rows.
constexpr Scalar vapourPressure = 274.18167e5;
const CompVec vapourComposition {
    0.7378047, 0.042610522, 0.1224405, 0.04501055, 0.043510534, 0.007502092, 0.0011211138};
constexpr Scalar liquidPressure = 280.3792e5;
const CompVec liquidComposition {0.41, 0.045, 0.105, 0.0527, 0.0951, 0.0844, 0.2078};

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(ZcritSetsTheCriticalVolumes)
{
    // In m^3/kmol. They carry seven significant digits and a gas constant that
    // differs from Opm::Constants by 2e-5.
    const CompVec expected {
        0.1003880, 0.09505383, 0.1469252, 0.2027493, 0.3054031, 0.6138625, 1.165961};

    // ZCRIT needs no VCRIT, and VCRIT does not change what ZCRIT gives.
    for (const auto& keywords : {zcritKeyword, vcritKeyword + zcritKeyword}) {
        initFromDeck(keywords);
        for (int c = 0; c < numComponents; ++c) {
            BOOST_TEST_CONTEXT(keywords << "component " << c)
            {
                BOOST_CHECK_CLOSE(FluidSystem::criticalVolume(c), expected[c], 5.0e-3);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(VcritAppliesWithoutZcrit)
{
    initFromDeck(vcritKeyword);

    for (int c = 0; c < numComponents; ++c) {
        BOOST_TEST_CONTEXT("component " << c)
        {
            BOOST_CHECK_CLOSE(FluidSystem::criticalVolume(c), vcrit[c], 1.0e-12);
        }
    }
}

BOOST_AUTO_TEST_CASE(CriticalVolumesAreRequired)
{
    BOOST_CHECK_THROW(initFromDeck(""), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ZcritSetsTheViscosity)
{
    initFromDeck(vcritKeyword + zcritKeyword);

    // The density does not depend on the critical volumes; it confirms the
    // state and differs by the same 2e-5 through the gas constant. The gas
    // constant cancels in the reduced density of the viscosity correlation,
    // so the viscosity is checked more tightly. The viscosity from VCRIT is
    // 0.18% lower for the vapour and half the expected value for the liquid.
    const auto vapour
        = phaseProperties(FluidSystem::gasPhaseIdx, vapourPressure, vapourComposition);
    BOOST_CHECK_CLOSE(vapour.density, 236.53436, 5.0e-3);
    BOOST_CHECK_CLOSE(vapour.viscosity, 0.028169552e-3, 1.0e-3);

    const auto liquid
        = phaseProperties(FluidSystem::oilPhaseIdx, liquidPressure, liquidComposition);
    BOOST_CHECK_CLOSE(liquid.density, 743.7006, 5.0e-3);
    BOOST_CHECK_CLOSE(liquid.viscosity, 0.53925025e-3, 1.0e-3);
}
