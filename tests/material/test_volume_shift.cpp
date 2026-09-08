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
 * \brief Tests SSHIFT density, viscosity, equilibrium ratios and derivatives.
 *
 * Component properties come from COMP_EQUIL_1D_VERTICAL_EQLNUM7. Its
 * reference gas density is 165.87 kg/m3 at 272.599 bar and 393.15 K;
 * the unshifted calculation gives 172.84 kg/m3. The viscosity reference
 * comes from VGAS at report step 1 of the same run.
 */
#include "config.h"

#define BOOST_TEST_MODULE VolumeShift
#include <boost/test/unit_test.hpp>

#include <opm/material/Constants.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>
#include <opm/material/viscositymodels/ViscosityModels.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/common/Exceptions.hpp>

#include <array>
#include <cmath>
#include <utility>

namespace {

using Scalar = double;
constexpr int numComponents = 7;

using FluidSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, false>;
// Changing enableWater gives the reference its own static component data.
// It uses the same component properties with zero shifts; only oil and gas
// properties are evaluated in these comparisons.
using UnshiftedSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, numComponents, true>;
using CompVec = std::array<Scalar, numComponents>;

constexpr auto eosType = Opm::CompositionalConfig::EOSType::PR;

// Reference conditions at the top of the column: RTEMP = 120 degC.
constexpr Scalar temperature = 393.15;
constexpr Scalar pressure = 272.599e5;

// A seven-component reservoir fluid: methane, CO2, ethane, propane and three
// lumped fractions.  The shifts are the SSHIFT values of the same deck.
const CompVec z{0.88583, 0.0437, 0.034, 0.0189, 0.0152, 0.0023, 0.00007};
const CompVec shift{-0.1595, -0.0817, -0.1134, -0.0863, -0.0243568,
                    0.10784125400986, 0.206892761354429};

struct Component
{
    const char* name;
    Scalar molarMass;   // kg/mol
    Scalar criticalT;   // K
    Scalar criticalP;   // Pa
    Scalar criticalV;   // m^3/kmol
    Scalar acentric;
};

// Deck component properties converted to the units listed above.
const std::array<Component, numComponents> components{{
    {"CH4",     0.016043, 190.60, 45.40e5, 0.09900000, 0.00800},
    {"CO2",     0.044010, 304.20, 72.80e5, 0.09400000, 0.22500},
    {"C2H6",    0.030070, 305.40, 48.20e5, 0.14800000, 0.09800},
    {"C3H8",    0.044097, 369.80, 41.90e5, 0.20300000, 0.15200},
    {"C4-C9",   0.076900, 474.70, 32.40e5, 0.32321204, 0.25490},
    {"C10-C20", 0.163000, 646.00, 20.42e5, 0.63255728, 0.55100},
    {"C21+",    0.310850, 812.00, 14.50e5, 0.97988801, 0.90900},
}};

// Initialize both systems once for the tests that share their static data.
struct Fixture
{
    Fixture()
    {
        using CompParam = typename FluidSystem::ComponentParam;
        FluidSystem::init();
        for (int c = 0; c < numComponents; ++c) {
            const auto& p = components[c];
            FluidSystem::addComponent(CompParam{p.name, p.molarMass, p.criticalT,
                                                p.criticalP, p.criticalV, p.acentric,
                                                shift[c]});
        }

        using UnshiftedParam = typename UnshiftedSystem::ComponentParam;
        UnshiftedSystem::init();
        for (int c = 0; c < numComponents; ++c) {
            const auto& p = components[c];
            UnshiftedSystem::addComponent(UnshiftedParam{p.name, p.molarMass, p.criticalT,
                                                         p.criticalP, p.criticalV,
                                                         p.acentric, 0.0});
        }
    }
};

/// Gas density, viscosity and fugacity coefficients at the reference conditions.
struct PhaseState
{
    Scalar density{};
    Scalar viscosity{};
    CompVec fugacityCoefficient{};
};

PhaseState gasState()
{
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
    }

    typename FluidSystem::template ParameterCache<Scalar> paramCache(eosType);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);

    // Store the unshifted EOS compressibility factor, as the simulator does.
    const Scalar Z = paramCache.molarVolume(FluidSystem::gasPhaseIdx) * pressure
                   / (Opm::Constants<Scalar>::R * temperature);
    fs.setCompressFactor(FluidSystem::gasPhaseIdx, Z);

    PhaseState state;
    state.density = FluidSystem::density(fs, paramCache, FluidSystem::gasPhaseIdx);
    state.viscosity = FluidSystem::viscosity(fs, paramCache, FluidSystem::gasPhaseIdx);
    for (int c = 0; c < numComponents; ++c) {
        state.fugacityCoefficient[c] =
            FluidSystem::fugacityCoefficient(fs, paramCache, FluidSystem::gasPhaseIdx, c);
    }
    return state;
}

// Evaluate the same property calculation with scalar or AD inputs.
template <class Eval>
std::pair<Eval, Eval> shiftedGasProperties(const Eval& p, const Eval& xCH4)
{
    Opm::CompositionalFluidState<Eval, FluidSystem> fs;
    fs.setTemperature(Eval{temperature});
    fs.setPressure(FluidSystem::oilPhaseIdx, p);
    fs.setPressure(FluidSystem::gasPhaseIdx, p);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, 0, xCH4);
    for (int c = 1; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, Eval{z[c]});
    }

    typename FluidSystem::template ParameterCache<Eval> paramCache(eosType);
    paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);

    return {FluidSystem::density(fs, paramCache, FluidSystem::gasPhaseIdx),
            FluidSystem::viscosity(fs, paramCache, FluidSystem::gasPhaseIdx)};
}

} // Anonymous namespace

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(ShiftMovesTheDensityOntoTheReference)
{
    const auto gas = gasState();

    // COMPVD supplies the test composition; the reference uses the equilibrated
    // composition at the top of the column. The 1% tolerance allows for this
    // difference while excluding the unshifted result of 172.84 kg/m3.
    BOOST_CHECK_CLOSE(gas.density, 165.87, 1.0);
}

BOOST_AUTO_TEST_CASE(ShiftLeavesTheEquilibriumRatiosAlone)
{
    // At equal phase pressure and temperature, the Peneloux factor
    // exp(-s_c b_c p / (R T)) cancels from phi_c^liquid / phi_c^vapour.
    // Compare this ratio with a separate system whose shifts are zero.
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    Opm::CompositionalFluidState<Scalar, UnshiftedSystem> fsRef;
    fs.setTemperature(temperature);
    fsRef.setTemperature(temperature);
    for (int ph : {FluidSystem::oilPhaseIdx, FluidSystem::gasPhaseIdx}) {
        fs.setPressure(ph, pressure);
        fsRef.setPressure(ph, pressure);
    }
    // Use distinct liquid and vapour compositions so the ratio does not
    // trivially compare identical phase properties. These are prescribed
    // compositions; this test does not perform a flash calculation.
    const CompVec x{0.55, 0.09, 0.12, 0.10, 0.11, 0.028, 0.002};
    const CompVec y{0.95, 0.03, 0.012, 0.005, 0.0025, 0.0004, 0.0001};
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, x[c]);
        fsRef.setMoleFraction(UnshiftedSystem::oilPhaseIdx, c, x[c]);
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, y[c]);
        fsRef.setMoleFraction(UnshiftedSystem::gasPhaseIdx, c, y[c]);
    }

    typename FluidSystem::template ParameterCache<Scalar> pc(eosType);
    typename UnshiftedSystem::template ParameterCache<Scalar> pcRef(eosType);
    for (int ph : {FluidSystem::oilPhaseIdx, FluidSystem::gasPhaseIdx}) {
        pc.updatePhase(fs, ph);
        pcRef.updatePhase(fsRef, ph);
    }

    // Confirm that the two systems have different volume translations.
    BOOST_CHECK_GT(std::abs(pc.phaseVolumeShift(FluidSystem::gasPhaseIdx)), 0.0);
    BOOST_CHECK_SMALL(pcRef.phaseVolumeShift(UnshiftedSystem::gasPhaseIdx), 1.0e-30);

    for (int c = 0; c < numComponents; ++c) {
        const Scalar K =
            FluidSystem::fugacityCoefficient(fs, pc, FluidSystem::oilPhaseIdx, c) /
            FluidSystem::fugacityCoefficient(fs, pc, FluidSystem::gasPhaseIdx, c);
        const Scalar KRef =
            UnshiftedSystem::fugacityCoefficient(fsRef, pcRef, UnshiftedSystem::oilPhaseIdx, c) /
            UnshiftedSystem::fugacityCoefficient(fsRef, pcRef, UnshiftedSystem::gasPhaseIdx, c);
        BOOST_CHECK_CLOSE(K, KRef, 1.0e-10);
    }
}

BOOST_AUTO_TEST_CASE(ShiftDoesNotReachTheCachedVolume)
{
    // Check that the corrected volume subtracts the cached translation and
    // that density uses the resulting volume.
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
    }
    typename FluidSystem::template ParameterCache<Scalar> pc(eosType);
    pc.updatePhase(fs, FluidSystem::gasPhaseIdx);

    const Scalar Vm = pc.molarVolume(FluidSystem::gasPhaseIdx);
    const Scalar shift = pc.phaseVolumeShift(FluidSystem::gasPhaseIdx);
    BOOST_CHECK_CLOSE(pc.correctedMolarVolume(FluidSystem::gasPhaseIdx),
                      Vm - shift, 1.0e-10);
    // Density uses the translated molar volume.
    BOOST_CHECK_CLOSE(FluidSystem::density(fs, pc, FluidSystem::gasPhaseIdx),
                      fs.averageMolarMass(FluidSystem::gasPhaseIdx) / (Vm - shift),
                      1.0e-10);
}

BOOST_AUTO_TEST_CASE(ShiftMovesTheLiquidDensityToo)
{
    // Check the oil-phase path against an independently calculated translation
    // using the component SSHIFT values and b_c = Omega_b R Tc/pc.
    Opm::CompositionalFluidState<Scalar, FluidSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(FluidSystem::oilPhaseIdx, pressure);
    fs.setPressure(FluidSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(FluidSystem::oilPhaseIdx, c, z[c]);
        fs.setMoleFraction(FluidSystem::gasPhaseIdx, c, z[c]);
    }
    typename FluidSystem::template ParameterCache<Scalar> pc(eosType);
    pc.updatePhase(fs, FluidSystem::oilPhaseIdx);

    constexpr Scalar OmegaB = 0.0777960739;   // Peng-Robinson
    constexpr Scalar R = 8.31446261815324;
    Scalar expectedShift = 0.0;
    for (int c = 0; c < numComponents; ++c) {
        const auto& p = components[c];
        const Scalar b = OmegaB * R * p.criticalT / p.criticalP;
        expectedShift += z[c] * shift[c] * b;
    }
    // Allow for rounding in the independently specified EOS constants.
    BOOST_CHECK_CLOSE(pc.phaseVolumeShift(FluidSystem::oilPhaseIdx), expectedShift, 1.0e-3);

    const Scalar Vm = pc.molarVolume(FluidSystem::oilPhaseIdx);
    const Scalar rho = FluidSystem::density(fs, pc, FluidSystem::oilPhaseIdx);
    BOOST_CHECK_CLOSE(rho,
                      fs.averageMolarMass(FluidSystem::oilPhaseIdx) / (Vm - expectedShift),
                      1.0e-4);
    // The negative mixture translation increases the volume and lowers density.
    BOOST_CHECK_LT(rho, fs.averageMolarMass(FluidSystem::oilPhaseIdx) / Vm);
}

BOOST_AUTO_TEST_CASE(ShiftMovesTheViscosityOntoTheReference)
{
    // Reference VGAS for cell 1 at report step 1 is 0.0223926 cP. The 1%
    // tolerance allows for the different pressure (272.64 bar) and equilibrated
    // composition at that step. The unshifted result is about 2.3% higher.
    BOOST_CHECK_CLOSE(gasState().viscosity, 2.23926e-5, 1.0);
}

BOOST_AUTO_TEST_CASE(WithoutAShiftTheTwoViscosityPathsAgree)
{
    // With zero shifts, the explicit molar-density and compressibility-factor
    // entry points must produce the same LBC viscosity.
    Opm::CompositionalFluidState<Scalar, UnshiftedSystem> fs;
    fs.setTemperature(temperature);
    fs.setPressure(UnshiftedSystem::gasPhaseIdx, pressure);
    for (int c = 0; c < numComponents; ++c) {
        fs.setMoleFraction(UnshiftedSystem::gasPhaseIdx, c, z[c]);
    }

    typename UnshiftedSystem::template ParameterCache<Scalar> paramCache(eosType);
    paramCache.updatePhase(fs, UnshiftedSystem::gasPhaseIdx);
    const Scalar Z = paramCache.molarVolume(UnshiftedSystem::gasPhaseIdx) * pressure
                   / (Opm::Constants<Scalar>::R * temperature);
    fs.setCompressFactor(UnshiftedSystem::gasPhaseIdx, Z);

    const Scalar viaMolarDensity =
        UnshiftedSystem::viscosity(fs, paramCache, UnshiftedSystem::gasPhaseIdx);
    const Scalar viaCompressFactor =
        Opm::ViscosityModels<Scalar, UnshiftedSystem>::LBC(fs, paramCache,
                                                           UnshiftedSystem::gasPhaseIdx);

    BOOST_CHECK_CLOSE(viaMolarDensity, viaCompressFactor, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(TheShiftedPathCarriesItsDerivatives)
{
    // Compare pressure and composition derivatives with central differences.
    // The composition perturbation also exercises the derivative of the
    // mixture volume translation.
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 2>;
    constexpr unsigned pIdx = 0;
    constexpr unsigned xIdx = 1;

    const auto ad = shiftedGasProperties<Evaluation>(
        Evaluation::createVariable(pressure, pIdx),
        Evaluation::createVariable(z[0], xIdx));

    const auto plain = shiftedGasProperties<Scalar>(pressure, z[0]);
    BOOST_CHECK_CLOSE(ad.first.value(), plain.first, 1.0e-8);
    BOOST_CHECK_CLOSE(ad.second.value(), plain.second, 1.0e-8);

    const Scalar dp = 1.0e-6 * pressure;
    const auto pUp = shiftedGasProperties<Scalar>(pressure + dp, z[0]);
    const auto pDown = shiftedGasProperties<Scalar>(pressure - dp, z[0]);
    BOOST_CHECK_CLOSE(ad.first.derivative(pIdx),
                      (pUp.first - pDown.first) / (2 * dp), 1.0e-4);
    BOOST_CHECK_CLOSE(ad.second.derivative(pIdx),
                      (pUp.second - pDown.second) / (2 * dp), 1.0e-4);

    const Scalar dx = 1.0e-6;
    const auto xUp = shiftedGasProperties<Scalar>(pressure, z[0] + dx);
    const auto xDown = shiftedGasProperties<Scalar>(pressure, z[0] - dx);
    BOOST_CHECK_CLOSE(ad.first.derivative(xIdx),
                      (xUp.first - xDown.first) / (2 * dx), 1.0e-4);
    BOOST_CHECK_CLOSE(ad.second.derivative(xIdx),
                      (xUp.second - xDown.second) / (2 * dx), 1.0e-4);
}

// Separate static component data for the deck-initialization test.
using DeckSystem = Opm::GenericOilGasWaterFluidSystem<Scalar, 3, false>;

namespace {

Opm::Deck deckWithVolumeShift()
{
    return Opm::Parser{}.parseString(R"(
RUNSPEC
METRIC
DIMENS
 1 1 1 /
COMPS
3 /
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
 C1 C10 CO2 /
TCRIT
 190.6 617.7 304.2 /
PCRIT
 46.0 21.1 73.8 /
VCRIT
 0.0990 0.6240 0.0940 /
MW
 16.043 142.285 44.010 /
ACF
 0.008 0.4885 0.225 /
BIC
 0.12 0.23 0.34 /
LBCCOEF
 0.2 0.03 0.06 -0.04 0.009 /
SSHIFT
 -0.1595 0.10784 -0.0817 /
SOLUTION
SCHEDULE
END
)");
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(ParsedShiftReachesTheFluidSystem)
{
    // Verify the handoff from parsed SSHIFT values to the component parameters
    // through initFromState(). The other tests register components directly.
    const auto deck = deckWithVolumeShift();
    const auto eclState = Opm::EclipseState{ deck };
    const auto schedule = Opm::Schedule{ deck, eclState };

    BOOST_REQUIRE_NO_THROW(DeckSystem::initFromState(eclState, schedule));

    const std::array<Scalar, 3> expected{-0.1595, 0.10784, -0.0817};
    for (unsigned c = 0; c < 3; ++c) {
        BOOST_CHECK_CLOSE(DeckSystem::volumeShift(c), expected[c], 1.0e-10);
    }
}

BOOST_AUTO_TEST_CASE(ReinitializationRestoresDefaultCoefficients)
{
    const auto deck = deckWithVolumeShift();
    const auto eclState = Opm::EclipseState{ deck };
    const auto schedule = Opm::Schedule{ deck, eclState };
    DeckSystem::initFromState(eclState, schedule);

    const std::array<Scalar, 5> deckLbc{0.2, 0.03, 0.06, -0.04, 0.009};
    const auto& loadedLbc = DeckSystem::lbcCoefficients();
    BOOST_REQUIRE_EQUAL_COLLECTIONS(loadedLbc.begin(), loadedLbc.end(),
                                    deckLbc.begin(), deckLbc.end());
    BOOST_REQUIRE_EQUAL(DeckSystem::interactionCoefficient(0, 1), 0.12);
    BOOST_REQUIRE_EQUAL(DeckSystem::interactionCoefficient(0, 2), 0.23);
    BOOST_REQUIRE_EQUAL(DeckSystem::interactionCoefficient(1, 2), 0.34);

    // Manual registration after init() must use defaults for properties that
    // are not supplied by ComponentParam, regardless of the previous deck.
    DeckSystem::init();
    const auto& config = eclState.compositionalConfig();
    const auto& props = config.eosProps(0);
    for (unsigned c = 0; c < DeckSystem::numComponents; ++c) {
        DeckSystem::addComponent(DeckSystem::ComponentParam{
            config.compName()[c], props.molecular_weights[c],
            props.critical_temperature[c], props.critical_pressure[c],
            props.critical_volume[c] * 1.e3, props.acentric_factors[c]});
    }

    for (unsigned c = 0; c < DeckSystem::numComponents; ++c) {
        BOOST_CHECK_EQUAL(DeckSystem::volumeShift(c), 0.0);
        for (unsigned d = 0; d < DeckSystem::numComponents; ++d) {
            BOOST_CHECK_EQUAL(DeckSystem::interactionCoefficient(c, d), 0.0);
        }
    }
    const auto defaultLbc = DeckSystem::ViscosityModel::defaultLBCCoefficients();
    const auto& resetLbc = DeckSystem::lbcCoefficients();
    BOOST_CHECK_EQUAL_COLLECTIONS(resetLbc.begin(), resetLbc.end(),
                                  defaultLbc.begin(), defaultLbc.end());
}

BOOST_AUTO_TEST_CASE(AnOverlargeShiftIsRejectedRatherThanReturned)
{
    // A translation larger than the EOS molar volume must raise an error
    // before density is evaluated from a non-positive volume.
    using BigShift = Opm::GenericOilGasWaterFluidSystem<Scalar, 1, false>;
    using CompParam = typename BigShift::ComponentParam;
    BigShift::init();
    BigShift::addComponent(CompParam{"C1", 0.016043, 190.60, 45.40e5, 0.099, 0.008,
                                     /*volume_shift=*/1.0e3});

    Opm::CompositionalFluidState<Scalar, BigShift> fs;
    fs.setTemperature(Scalar{400});
    fs.setPressure(BigShift::oilPhaseIdx, Scalar{100e5});
    fs.setPressure(BigShift::gasPhaseIdx, Scalar{100e5});
    fs.setMoleFraction(BigShift::gasPhaseIdx, 0, Scalar{1});
    fs.setMoleFraction(BigShift::oilPhaseIdx, 0, Scalar{1});

    typename BigShift::template ParameterCache<Scalar> pc(eosType);
    pc.updatePhase(fs, BigShift::gasPhaseIdx);

    BOOST_CHECK_THROW(BigShift::density(fs, pc, BigShift::gasPhaseIdx),
                      Opm::NumericalProblem);
}
