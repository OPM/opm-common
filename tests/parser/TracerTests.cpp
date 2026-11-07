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
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/TracerConfig.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <vector>

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
BOX
1 10 1 10 1 1 /
TOPS
100*0.25 /
ENDBOX
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
    BOOST_REQUIRE_EQUAL(it->free_tvdp.size(), 1U);
    BOOST_CHECK_EQUAL(it->free_tvdp.front().numColumns(), 2U);

    ++it;
    BOOST_CHECK_EQUAL(it->name, "OCE");
    BOOST_CHECK_EQUAL(it->phase, Phase::GAS);
    BOOST_CHECK_EQUAL(it->free_concentration.value().size(), 3U);
    BOOST_CHECK(it->free_tvdp.empty());
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

    const auto tc = TracerConfig{deck.getDefaultUnitSystem(), deck, false};

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

    const auto tc = TracerConfig{deck.getDefaultUnitSystem(), deck, false};

    BOOST_CHECK_MESSAGE(! tc.supportsSolutionGasTracer(), "Solution gas tracer should not be supported");
    BOOST_CHECK_MESSAGE(tc.supportsVaporisedOilTracer(), "Vaporised oil tracer should be supported");
}

BOOST_AUTO_TEST_CASE(Tracer_region_arrays_are_integer_regions)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
  1 1 4 /
OIL
WATER
TABDIMS
/
EQLDIMS
  2 /
GRID
DXV
  100 /
DYV
  100 /
DZV
  4*10 /
DEPTHZ
  4*2000.0 /
PORO
  4*0.25 /
PERMX
  4*100 /
PERMY
  4*100 /
PERMZ
  4*10 /
REGIONS
EQLNUM
  1 1 2 2 /
TNUMFSEA
  2 2 1 1 /
COPY
  EQLNUM TNUMCT1 /
/
)");

    const auto state = EclipseState{deck};
    const auto& fp = state.fieldProps();
    BOOST_CHECK(fp.get_int("TNUMFSEA") == (std::vector<int>{2, 2, 1, 1}));
    BOOST_CHECK(fp.get_int("TNUMCT1") == (std::vector<int>{1, 1, 2, 2}));
}

BOOST_AUTO_TEST_CASE(Black_oil_tracer_has_a_depth_table_per_tracer_region)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
EQLDIMS
  2 1* 1* 2 /
PROPS
TRACER
SEA  WAT  /
OCE  GAS  /
/
TVDPFSEA
1000   0.0
5000   0.0 /
1000   1.0
5000   1.0 /
TVDPFOCE
1000   0.0
5000   0.0 /
1000   0.5
5000   0.5 /
TVDPSOCE
1000   0.0
5000   0.0 /
1000   0.25
5000   0.25 /
)");

    const auto tc = TracerConfig{deck.getDefaultUnitSystem(), deck, false};

    const auto& sea = tc["SEA"];
    BOOST_CHECK(!sea.component.has_value());
    BOOST_CHECK_EQUAL(sea.free_region(), "TNUMFSEA");
    BOOST_REQUIRE_EQUAL(sea.free_tvdp.size(), 2U);
    BOOST_CHECK_CLOSE(sea.free_tvdp[1].evaluate("TRACER_CONCENTRATION", 2000.0), 1.0, 1.0e-10);
    BOOST_CHECK(sea.solution_tvdp.empty());

    const auto& oce = tc["OCE"];
    BOOST_CHECK_EQUAL(oce.solution_region(), "TNUMSOCE");
    BOOST_REQUIRE_EQUAL(oce.free_tvdp.size(), 2U);
    BOOST_REQUIRE_EQUAL(oce.solution_tvdp.size(), 2U);
    BOOST_CHECK_CLOSE(oce.solution_tvdp[1].evaluate("TRACER_CONCENTRATION", 2000.0), 0.25, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Compositional_tracer_is_associated_with_a_component)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
EQLDIMS
  2 1* 1* 2 /
COMPS
  2 /
PROPS
CNAMES
  C1 CO2 /
TRACER
CT1  CO2   /
WT   WATER /
/
TVDPCT1
1000   0.0
5000   0.0 /
1000   1.0
5000   1.0 /
TBLKWT
1.0 2.0 /
)");

    const auto tc = TracerConfig{deck.getDefaultUnitSystem(), deck, true};

    const auto& ct1 = tc["CT1"];
    BOOST_CHECK_EQUAL(ct1.component.value(), "CO2");
    BOOST_CHECK_EQUAL(ct1.free_region(), "TNUMCT1");
    BOOST_REQUIRE_EQUAL(ct1.free_tvdp.size(), 2U);
    BOOST_CHECK_CLOSE(ct1.free_tvdp[1].evaluate("TRACER_CONCENTRATION", 2000.0), 1.0, 1.0e-10);

    const auto& wt = tc["WT"];
    BOOST_CHECK_EQUAL(wt.component.value(), "WATER");
    BOOST_CHECK_EQUAL(wt.phase, Phase::WATER);
    BOOST_CHECK_EQUAL(wt.free_concentration.value().size(), 2U);
}

BOOST_AUTO_TEST_CASE(Compositional_tracer_needs_a_known_component)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
COMPS
  2 /
PROPS
CNAMES
  C1 CO2 /
TRACER
CT1  GAS /
/
)");

    BOOST_CHECK_THROW(TracerConfig(deck.getDefaultUnitSystem(), deck, true), OpmInputError);
}
