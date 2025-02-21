/*
  Copyright 2025 Equinor ASA

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

#define BOOST_TEST_MODULE TestBlackOilFluidSystemNonStatic
#include <config.h>

#include <cstddef>

#include <boost/test/unit_test.hpp>

#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>
#include <opm/material/fluidsystems/BlackOilFluidSystemNonStatic.hpp>

BOOST_AUTO_TEST_CASE(TestThrowOnUnInitialized)
{
    BOOST_CHECK(!Opm::BlackOilFluidSystem<double>::isInitialized());
    BOOST_CHECK_THROW(Opm::BlackOilFluidSystem<double>::getNonStaticInstance(), std::logic_error);
}

BOOST_AUTO_TEST_CASE(TestNonStaticCreation)
{
    Opm::BlackOilFluidSystem<double>::initBegin(5);
    Opm::BlackOilFluidSystem<double>::initEnd();
    BOOST_CHECK(Opm::BlackOilFluidSystem<double>::isInitialized());
    auto staticDummyInstance = Opm::BlackOilFluidSystem<double> {};
    BOOST_CHECK(staticDummyInstance.isInitialized());

    auto& fluidSystem = Opm::BlackOilFluidSystem<double>::getNonStaticInstance();
    BOOST_CHECK(fluidSystem.isInitialized());
}
BOOST_AUTO_TEST_CASE(TestNonStaticGetters)
{
    auto& fluidSystem = Opm::BlackOilFluidSystem<double>::getNonStaticInstance();
    BOOST_CHECK_EQUAL(fluidSystem.numActivePhases(), Opm::BlackOilFluidSystem<double>::numActivePhases());
    for (std::size_t phase = 0; phase < fluidSystem.numActivePhases(); ++phase) {
        BOOST_CHECK_EQUAL(fluidSystem.phaseIsActive(phase), Opm::BlackOilFluidSystem<double>::phaseIsActive(phase));
    }
    BOOST_CHECK_EQUAL(fluidSystem.enableDissolvedGas(), Opm::BlackOilFluidSystem<double>::enableDissolvedGas());
    BOOST_CHECK_EQUAL(fluidSystem.enableDissolvedGasInWater(),
                      Opm::BlackOilFluidSystem<double>::enableDissolvedGasInWater());
    BOOST_CHECK_EQUAL(fluidSystem.enableVaporizedOil(), Opm::BlackOilFluidSystem<double>::enableVaporizedOil());
    BOOST_CHECK_EQUAL(fluidSystem.enableVaporizedWater(), Opm::BlackOilFluidSystem<double>::enableVaporizedWater());
    BOOST_CHECK_EQUAL(fluidSystem.enableDiffusion(), Opm::BlackOilFluidSystem<double>::enableDiffusion());
    BOOST_CHECK_EQUAL(fluidSystem.isInitialized(), Opm::BlackOilFluidSystem<double>::isInitialized());
    BOOST_CHECK_EQUAL(fluidSystem.useSaturatedTables(), Opm::BlackOilFluidSystem<double>::useSaturatedTables());
}

BOOST_AUTO_TEST_CASE(TestCopyAndSet)
{
    auto& fluidSystem = Opm::BlackOilFluidSystem<double>::getNonStaticInstance();
    auto fluidSystemCopy = fluidSystem;
    BOOST_CHECK_EQUAL(fluidSystem.numActivePhases(), fluidSystemCopy.numActivePhases());
    for (std::size_t phase = 0; phase < fluidSystem.numActivePhases(); ++phase) {
        BOOST_CHECK_EQUAL(fluidSystem.phaseIsActive(phase), fluidSystemCopy.phaseIsActive(phase));
    }
    BOOST_CHECK_EQUAL(fluidSystem.enableDissolvedGas(), fluidSystemCopy.enableDissolvedGas());
    BOOST_CHECK_EQUAL(fluidSystem.enableDissolvedGasInWater(), fluidSystemCopy.enableDissolvedGasInWater());
    BOOST_CHECK_EQUAL(fluidSystem.enableVaporizedOil(), fluidSystemCopy.enableVaporizedOil());
    BOOST_CHECK_EQUAL(fluidSystem.enableVaporizedWater(), fluidSystemCopy.enableVaporizedWater());
    BOOST_CHECK_EQUAL(fluidSystem.enableDiffusion(), fluidSystemCopy.enableDiffusion());
    BOOST_CHECK_EQUAL(fluidSystem.isInitialized(), fluidSystemCopy.isInitialized());
    BOOST_CHECK_EQUAL(fluidSystem.useSaturatedTables(), fluidSystemCopy.useSaturatedTables());

    fluidSystemCopy.setEnableDissolvedGas(false);
    BOOST_CHECK_EQUAL(fluidSystem.enableDissolvedGas(), true);
    BOOST_CHECK_EQUAL(fluidSystemCopy.enableDissolvedGas(), false);
}
