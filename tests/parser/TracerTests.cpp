/*
  Copyright 2019 SINTEF Digital, Mathematics and Cybernetics.
  Copyright 2013 Statoil ASA.

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

#define BOOST_TEST_MODULE TracerTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/TracerConfig.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

using namespace Opm;

namespace {

Deck createDeck()
{
    // Using a raw string literal with xxx as delimiter.
    return Parser{}.parseString(R"xxx(RUNSPEC
DIMENS
 10 10 10 /
TABDIMS
3 /
GRID
DX
1000*0.25 /
DY
1000*0.25 /
DZ
1000*0.25 /
TOPS
100*0.25 /
EQLDIMS
 3 1 1 /

PROPS

TRACERS
--  oil  water  gas  env
    1*   1      1    1*   /
TRACER
SEA  WAT  /
OCE  GAS /
/
TVDPFSEA
1000   0.0
5000   0.0 /
TBLKFOCE
1.0 2.0 3.0 /
)xxx");

}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TracerConfigTest)
{
    const auto deck = createDeck();
    const auto state = EclipseState{deck};
    const auto& tc = state.tracer();
    BOOST_CHECK_EQUAL(tc.size(), 2U);

    BOOST_CHECK_MESSAGE(! tc.supportsSolutionGasTracer(), "Solution gas tracer should not be supported");
    BOOST_CHECK_MESSAGE(! tc.supportsVaporisedOilTracer(), "Vaporised oil tracer should not be supported");

    auto it = tc.begin();
    BOOST_CHECK_EQUAL(it->name, "SEA");
    BOOST_CHECK_EQUAL(it->phase, Phase::WATER);
    BOOST_CHECK(!it->free_concentration.has_value());
    BOOST_CHECK_EQUAL(it->free_tvdp.value().numColumns(), 2U);

    ++it;
    BOOST_CHECK_EQUAL(it->name, "OCE");
    BOOST_CHECK_EQUAL(it->phase, Phase::GAS);
    BOOST_CHECK_EQUAL(it->free_concentration.value().size(), 3U);
    BOOST_CHECK(!it->free_tvdp.has_value());
}

BOOST_AUTO_TEST_CASE(SolutionGasSupportTest)
{
    const auto deck = Parser{}.parseString(R"xxx(RUNSPEC
DIMENS
  5 1 2 /
OIL
GAS
WATER
DISGAS
TABDIMS
/
EQLDIMS
  3 1 1 /
TRACERS
--  oil  water  gas  env
  1*  1      1    1*   /
GRID
DXV
  5*100 /
DYV
  1*100 /
DZV
  5 10 /
DEPTHZ
  12*2000.0 /
EQUALS
  PORO 0.25 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
TRACER
SEA  WAT  /
OCE  GAS /
/
TVDPFSEA
1000   0.0
5000   0.0 /
TBLKFOCE
1.0 2.0 3.0 /
)xxx");

    const auto tc = TracerConfig{deck.getDefaultUnitSystem(), deck};

    BOOST_CHECK_MESSAGE(tc.supportsSolutionGasTracer(), "Solution gas tracer should be supported");
    BOOST_CHECK_MESSAGE(! tc.supportsVaporisedOilTracer(), "Vaporised oil tracer should not be supported");
}

BOOST_AUTO_TEST_CASE(VaporisedOilSupportTest)
{
    const auto deck = Parser{}.parseString(R"xxx(RUNSPEC
DIMENS
  5 1 2 /
OIL
GAS
WATER
VAPOIL
TABDIMS
/
EQLDIMS
  3 1 1 /
TRACERS
--  oil  water  gas  env
  1*  1      1    1*   /
GRID
DXV
  5*100 /
DYV
  1*100 /
DZV
  5 10 /
DEPTHZ
  12*2000.0 /
EQUALS
  PORO 0.25 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
TRACER
SEA  WAT  /
OCE  GAS  /
/
TVDPFSEA
1000   0.0
5000   0.0 /
TBLKFOCE
1.0 2.0 3.0 /
)xxx");

    const auto tc = TracerConfig{deck.getDefaultUnitSystem(), deck};

    BOOST_CHECK_MESSAGE(! tc.supportsSolutionGasTracer(), "Solution gas tracer should not be supported");
    BOOST_CHECK_MESSAGE(tc.supportsVaporisedOilTracer(), "Vaporised oil tracer should be supported");
}
