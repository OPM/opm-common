/*
  Copyright 2020 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#define BOOST_TEST_MODULE Inplace
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/Inplace.hpp>

#include <stdexcept>
#include <vector>

using namespace Opm;

namespace {

bool contains(const std::vector<Inplace::Phase>& phases, Inplace::Phase phase)
{
    auto find_iter = std::find(phases.begin(), phases.end(), phase);
    return find_iter != phases.end();
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TESTInplace)
{
    Inplace oip;

    oip.add("FIPNUM", Inplace::Phase::OIL, 3, 100);
    oip.add("FIPNUM", Inplace::Phase::OIL, 6, 50);

    BOOST_CHECK_EQUAL( oip.get("FIPNUM", Inplace::Phase::OIL, 3) , 100);
    BOOST_CHECK_EQUAL( oip.get("FIPNUM", Inplace::Phase::OIL, 6) , 50);

    BOOST_CHECK_THROW( oip.get("FIPNUM", Inplace::Phase::OIL, 4), std::exception);
    BOOST_CHECK_THROW( oip.get("FIPNUM", Inplace::Phase::GAS, 3), std::exception);
    BOOST_CHECK_THROW( oip.get("FIPX", Inplace::Phase::OIL, 3)  , std::exception);

    BOOST_CHECK_EQUAL( oip.max_region(), 6);
    BOOST_CHECK_EQUAL( oip.max_region("FIPNUM"), 6);
    BOOST_CHECK_THROW( oip.max_region("FIPX"), std::exception);

    oip.add(Inplace::Phase::GAS, 100);
    BOOST_CHECK_EQUAL( oip.get(Inplace::Phase::GAS) , 100);
    BOOST_CHECK_THROW( oip.get(Inplace::Phase::OIL), std::exception);

    {
        const auto v1 = oip.get_vector("FIPNUM", Inplace::Phase::OIL);
        const std::vector<double> e1 = {0,0,100,0,0,50};
        BOOST_CHECK_MESSAGE(v1 == e1, "In-place oil content must match expected");
    }
}

BOOST_AUTO_TEST_CASE(InPlace_Phases)
{
    const auto& phases = Inplace::phases();

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WATER),
                        R"(phases() must have "WATER")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OIL),
                        R"(phases() must have "OIL")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GAS),
                        R"(phases() must have "GAS")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OilInLiquidPhase),
                        R"(phases() must have "OilInLiquidPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OilInGasPhase),
                        R"(phases() must have "OilInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GasInLiquidPhase),
                        R"(phases() must have "GasInLiquidPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GasInGasPhase),
                        R"(phases() must have "GasInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::PoreVolume),
                        R"(phases() must have "PoreVolume")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::PressurePV),
                        R"(phases() must NOT have "PressurePV")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::HydroCarbonPV),
                        R"(phases() must NOT have "HydroCarbonPV")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::PressureHydroCarbonPV),
                        R"(phases() must NOT have "PressureHydroCarbonPV")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::DynamicPoreVolume),
                        R"(phases() must NOT have "DynamicPoreVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WaterResVolume),
                        R"(phases() must have "WaterResVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OilResVolume),
                        R"(phases() must have "OilResVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GasResVolume),
                        R"(phases() must have "GasResVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::SALT),
                        R"(phases() must have "SALT")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InWaterPhase),
                        R"(phases() must have "CO2InWaterPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseInMob),
                        R"(phases() must have "CO2InGasPhaseInMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseMob),
                        R"(phases() must have "CO2InGasPhaseMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseInMobKrg),
                        R"(phases() must have "CO2InGasPhaseInMobKrg")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseMobKrg),
                        R"(phases() must have "CO2InGasPhaseMobKrg")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WaterInGasPhase),
                        R"(phases() must have "WaterInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WaterInWaterPhase),
                        R"(phases() must have "WaterInWaterPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2Mass),
                        R"(phases() must have "CO2Mass")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInWaterPhase),
                        R"(phases() must have "CO2MassInWaterPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhase),
                        R"(phases() must have "CO2MassInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseInMob),
                        R"(phases() must have "CO2MassInGasPhaseInMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseMob),
                        R"(phases() must have "CO2MassInGasPhaseMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseInMobKrg),
                        R"(phases() must have "CO2MassInGasPhaseInMobKrg")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseMobKrg),
                        R"(phases() must have "CO2MassInGasPhaseMobKrg")");
}

BOOST_AUTO_TEST_CASE(InPlace_Mixing_Phases)
{
    const auto& phases = Inplace::mixingPhases();

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::WATER),
                        R"(mixingPhases() must NOT have "WATER")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::OIL),
                        R"(mixingPhases() must NOT have "OIL")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::GAS),
                        R"(mixingPhases() must NOT have "GAS")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OilInLiquidPhase),
                        R"(mixingPhases() must have "OilInLiquidPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OilInGasPhase),
                        R"(mixingPhases() must have "OilInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GasInLiquidPhase),
                        R"(mixingPhases() must have "GasInLiquidPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GasInGasPhase),
                        R"(mixingPhases() must have "GasInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::PoreVolume),
                        R"(mixingPhases() must have "PoreVolume")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::PressurePV),
                        R"(mixingPhases() must NOT have "PressurePV")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::HydroCarbonPV),
                        R"(mixingPhases() must NOT have "HydroCarbonPV")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::PressureHydroCarbonPV),
                        R"(mixingPhases() must NOT have "PressureHydroCarbonPV")");

    BOOST_CHECK_MESSAGE(! contains(phases, Inplace::Phase::DynamicPoreVolume),
                        R"(mixingPhases() must NOT have "DynamicPoreVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WaterResVolume),
                        R"(mixingPhases() must have "WaterResVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::OilResVolume),
                        R"(mixingPhases() must have "OilResVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::GasResVolume),
                        R"(mixingPhases() must have "GasResVolume")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::SALT),
                        R"(mixingPhases() must have "SALT")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InWaterPhase),
                        R"(mixingPhases() must have "CO2InWaterPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseInMob),
                        R"(mixingPhases() must have "CO2InGasPhaseInMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseMob),
                        R"(mixingPhases() must have "CO2InGasPhaseMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseInMobKrg),
                        R"(mixingPhases() must have "CO2InGasPhaseInMobKrg")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2InGasPhaseMobKrg),
                        R"(mixingPhases() must have "CO2InGasPhaseMobKrg")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WaterInGasPhase),
                        R"(mixingPhases() must have "WaterInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::WaterInWaterPhase),
                        R"(mixingPhases() must have "WaterInWaterPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2Mass),
                        R"(mixingPhases() must have "CO2Mass")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInWaterPhase),
                        R"(mixingPhases() must have "CO2MassInWaterPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhase),
                        R"(mixingPhases() must have "CO2MassInGasPhase")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseInMob),
                        R"(mixingPhases() must have "CO2MassInGasPhaseInMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseMob),
                        R"(mixingPhases() must have "CO2MassInGasPhaseMob")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseInMobKrg),
                        R"(mixingPhases() must have "CO2MassInGasPhaseInMobKrg")");

    BOOST_CHECK_MESSAGE(contains(phases, Inplace::Phase::CO2MassInGasPhaseMobKrg),
                        R"(mixingPhases() must have "CO2MassInGasPhaseMobKrg")");
}
