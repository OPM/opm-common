#define BOOST_TEST_MODULE PhaseUsageInfoTest
#include <boost/test/included/unit_test.hpp>

// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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
 * \brief This is a simple test that illustrates how to use the Opm:PhaseUsageInfo class.
 */
#include "config.h"

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#endif

#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/material/fluidsystems/BlackOilDefaultFluidSystemIndices.hpp>
#include <opm/material/fluidsystems/PhaseUsageInfo.hpp>

namespace Opm
{

BOOST_AUTO_TEST_CASE(default_constructor)
{
    using PhaseUsage = PhaseUsageInfo<BlackOilDefaultFluidSystemIndices>;
    PhaseUsage pu;
    BOOST_CHECK(pu.numActivePhases() == 0);
    for (unsigned i = 0; i < PhaseUsage::numPhases; ++i) {
        BOOST_CHECK(!pu.phaseIsActive(i));
        BOOST_CHECK(pu.canonicalToActivePhaseIdx(i) == -1);
    }
    BOOST_CHECK(!pu.hasSolvent());
    BOOST_CHECK(!pu.hasPolymer());
    BOOST_CHECK(!pu.hasEnergy());
    BOOST_CHECK(!pu.hasPolymerMW());
    BOOST_CHECK(!pu.hasFoam());
    BOOST_CHECK(!pu.hasBrine());
    BOOST_CHECK(!pu.hasZFraction());
    BOOST_CHECK(!pu.hasMICP());
    BOOST_CHECK(!pu.hasCO2orH2Store());

    BOOST_TEST_MESSAGE("Default constructor test passed.");
}

BOOST_AUTO_TEST_CASE(constructor_with_phases)
{
    // water and oil active, gas inactive
    const Phases phases {/*oil*/ true,
        /*gas*/ false,
        /*water*/ true,
        /*solvent*/ false,
        /*polymer*/ true,
        /*energy*/ false,
        /*polymw*/ false,
        /*foam*/ false,
        /*brine*/ true,
        /*zfraction*/ false};

    using PhaseUsage = PhaseUsageInfo<BlackOilDefaultFluidSystemIndices>;
    PhaseUsage pu;
    pu.initFromPhases(phases);
    BOOST_CHECK(pu.numActivePhases() == 2);
    BOOST_CHECK(pu.phaseIsActive(PhaseUsage::waterPhaseIdx));
    BOOST_CHECK(pu.phaseIsActive(PhaseUsage::oilPhaseIdx));
    BOOST_CHECK(!pu.phaseIsActive(PhaseUsage::gasPhaseIdx));

    BOOST_CHECK(!pu.hasSolvent());
    BOOST_CHECK(pu.hasPolymer());
    BOOST_CHECK(!pu.hasEnergy());
    BOOST_CHECK(!pu.hasPolymerMW());
    BOOST_CHECK(!pu.hasFoam());
    BOOST_CHECK(pu.hasBrine());
    BOOST_CHECK(!pu.hasZFraction());
    BOOST_CHECK(!pu.hasMICP());
    BOOST_CHECK(!pu.hasCO2orH2Store());

    BOOST_CHECK(pu.canonicalToActivePhaseIdx(PhaseUsage::waterPhaseIdx) == 0);
    BOOST_CHECK(pu.canonicalToActivePhaseIdx(PhaseUsage::oilPhaseIdx) == 1);
    BOOST_CHECK(pu.canonicalToActivePhaseIdx(PhaseUsage::gasPhaseIdx) == -1);

    BOOST_CHECK(pu.activeToCanonicalPhaseIdx(0) == PhaseUsage::waterPhaseIdx);
    BOOST_CHECK(pu.activeToCanonicalPhaseIdx(1) == PhaseUsage::oilPhaseIdx);

    BOOST_TEST_MESSAGE("Constructor with phases test passed.");
}


#if HAVE_ECL_INPUT
BOOST_AUTO_TEST_CASE(constructor_with_datafile)
{
    const std::string data_file {"equil_co2store_go.DATA"};
    const EclipseState ecl_state {Parser{}.parseFile(data_file)};
    using PhaseUsage = PhaseUsageInfo<BlackOilDefaultFluidSystemIndices>;

    PhaseUsage pu;
    pu.initFromState(ecl_state);

    BOOST_CHECK(pu.numActivePhases() == 2);
    BOOST_CHECK(!pu.phaseIsActive(PhaseUsage::waterPhaseIdx));
    BOOST_CHECK(pu.phaseIsActive(PhaseUsage::oilPhaseIdx));
    BOOST_CHECK(pu.phaseIsActive(PhaseUsage::gasPhaseIdx));

    BOOST_CHECK(!pu.hasSolvent());
    BOOST_CHECK(!pu.hasPolymer());
    BOOST_CHECK(!pu.hasEnergy());
    BOOST_CHECK(!pu.hasPolymerMW());
    BOOST_CHECK(!pu.hasFoam());
    BOOST_CHECK(!pu.hasBrine());
    BOOST_CHECK(!pu.hasZFraction());
    BOOST_CHECK(!pu.hasMICP());

    BOOST_CHECK(pu.hasCO2orH2Store());

    BOOST_CHECK(pu.canonicalToActivePhaseIdx(PhaseUsage::waterPhaseIdx) == -1);
    BOOST_CHECK(pu.canonicalToActivePhaseIdx(PhaseUsage::oilPhaseIdx) == 0);
    BOOST_CHECK(pu.canonicalToActivePhaseIdx(PhaseUsage::gasPhaseIdx) == 1);

    BOOST_CHECK(pu.activeToCanonicalPhaseIdx(0) == PhaseUsage::oilPhaseIdx);
    BOOST_CHECK(pu.activeToCanonicalPhaseIdx(1) == PhaseUsage::gasPhaseIdx);
}
#endif
}