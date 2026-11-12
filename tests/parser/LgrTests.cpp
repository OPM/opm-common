/*
 Copyright (C) 2023 Equinor
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

#define BOOST_TEST_MODULE LgrTests

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrConnectionCheck.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include <fmt/format.h>

using namespace Opm;

BOOST_AUTO_TEST_CASE(CreateLgrCollection) {
    Opm::LgrCollection lgrs;
    BOOST_CHECK_EQUAL( lgrs.size() , 0U );
    BOOST_CHECK(! lgrs.hasLgr("NO-NotThisOne"));
    BOOST_CHECK_THROW( lgrs.getLgr("NO") , std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(ReadLgrCollection) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
 10 10 10 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  5  6  5  6  1  3  6  6  9 /
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  7  8  7  8  1  3  8  8  9 /
ENDFIN


DX
1000*1 /
DY
1000*1 /
DZ
1000*1 /
BOX
1 10 1 10 1 1 /
TOPS
100*1 /
ENDBOX

PORO
  1000*0.15 /

PERMX
  1000*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::LgrCollection lgrs = state.getLgrs();

    BOOST_CHECK_MESSAGE(state.hasInputLGR(), "EclipseState should have LGRs");
    BOOST_CHECK_EQUAL(
      lgrs.size() , 2U );
    BOOST_CHECK(lgrs.hasLgr("LGR1"));
    BOOST_CHECK(lgrs.hasLgr("LGR2"));

    const auto& lgr1 = state.getLgrs().getLgr("LGR1");
    BOOST_CHECK_EQUAL(lgr1.NAME(), "LGR1");
    const auto& lgr2 = lgrs.getLgr("LGR2");
    BOOST_CHECK_EQUAL( lgr2.NAME() , "LGR2");

    const auto& lgr3 = state.getLgrs().getLgr(0);
    BOOST_CHECK_EQUAL( lgr1.NAME() , lgr3.NAME());
}


BOOST_AUTO_TEST_CASE(TestLgrNeighbor) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  3  3   /
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  2  2  1  1  1  1  3  3   /
ENDFIN


DX
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();

    BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 25U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(0).getTotalActiveLGR() , 9U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(1).getTotalActiveLGR() , 9U );

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0,0,0), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 24U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 12U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",2,2,0), 20U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",0,0,0), 1U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",2,2,0), 9U);
  }

BOOST_AUTO_TEST_CASE(TestLgrColumnCells) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  1  1  1  2  1  1  2  4   /
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  3  3  1  2  1  1  2  4   /
ENDFIN


DX
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();

    BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 21U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(0).getTotalActiveLGR() , 8U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(1).getTotalActiveLGR() , 8U );

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",1,3,0), 7U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",1,0,0), 8U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",1,1,0), 17U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 20U);


    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",0,0,0), 9U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",1,3,0), 16U);
  }





BOOST_AUTO_TEST_CASE(TestLgrNested) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
LGR1  2  2  2  2  1  1  3  3   1 1*  GLOBAL/
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
LGR2  2  2  2  2  1  1  3  3   1 1*  LGR1/
ENDFIN


DX
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();

    BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 25U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(0).getTotalActiveLGR() , 17U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(0).getLGRCell(0).getTotalActiveLGR() , 9U );

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0,0,0), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 24U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 4U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",2,2,0), 20U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",0,0,0), 8U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",2,2,0), 16U);

    BOOST_CHECK_THROW(eclipse_grid.getActiveIndexLGR("GLOBAL",1,1,0), std::invalid_argument);
    BOOST_CHECK_THROW(eclipse_grid.getActiveIndexLGR("LGR1",1,1,0), std::invalid_argument);
    BOOST_CHECK_THROW(eclipse_grid.getActiveIndexLGR("LGR3",1,1,0), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(TestGLOBALinactivecells) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

ACTNUM
1 0 1
1 1 1
1 1 1
/

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  3  3   1/
ENDFIN


DX
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();

    BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 16U );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell(0).getTotalActiveLGR() , 9U );
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0,0,0), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 15U);


    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0U), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",8U), 15U);


    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 3U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",2,2,0), 11U);


    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0), 3U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",8), 11U);

}

BOOST_AUTO_TEST_CASE(TestLGRinactivecells) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  3  3   1/
ACTNUM
1 0 1
1 1 1
1 1 1
/
ENDFIN

DX
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);

    // The block's ACTNUM removes the second refined cell.
    const auto& lgrGrid = state.getInputGrid().getLGRCell("LGR1");
    BOOST_CHECK_EQUAL(lgrGrid.getNumActive(), 8U);
    BOOST_CHECK(!lgrGrid.cellActive(1));
    const auto& removed = state.getLgrs().getLgr("LGR1").minpvRemoved();
    BOOST_CHECK_EQUAL(std::count(removed.begin(), removed.end(), 1), 1);
}

namespace {

// 3x3x1 grid with LGR1 over cell (2,2), refined 3x3x1, and the given text in
// each section.
std::string lgrBlockDeck(const std::string& grid, const std::string& edit,
                         const std::string& props, const std::string& regions,
                         const std::string& solution)
{
    return fmt::format(R"(RUNSPEC
DIMENS
 3 3 1 /
OIL
WATER
GRID
CARFIN
'LGR1' 2 2 2 2 1 1 3 3 1 /
{}
ENDFIN
DXV
 3*100.0 /
DYV
 3*100.0 /
DZ
 9*10.0 /
TOPS
 9*2000 /
PORO
 9*0.25 /
PERMX
 9*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
EDIT
{}
PROPS
{}
REGIONS
{}
SOLUTION
{}
)", grid, edit, props, regions, solution);
}

Opm::EclipseState lgrBlockState(const std::string& grid,
                                const std::string& edit = "",
                                const std::string& props = "",
                                const std::string& regions = "",
                                const std::string& solution = "")
{
    return Opm::EclipseState {
        Opm::Parser{}.parseString(lgrBlockDeck(grid, edit, props, regions, solution))
    };
}

} // Anonymous namespace

// Values inside a REFINE ... ENDFIN block are not supported: the input stops
// instead of applying them to the main grid.
BOOST_AUTO_TEST_CASE(TestLGRvaluesInsideBlockStop)
{
    BOOST_CHECK_THROW(lgrBlockState("", R"(REFINE
 'LGR1' /
MULTPV
 27*2.0 /
ENDFIN)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState("", "", R"(REFINE
 'LGR1' /
SWATINIT
 27*0.5 /
ENDFIN)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState("", "", "", R"(REFINE
 'LGR1' /
SATNUM
 27*1 /
ENDFIN)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState("", "", "", "", R"(REFINE
 'LGR1' /
PRESSURE
 27*250.0 /
ENDFIN)"), Opm::OpmInputError);

    // A block with nothing inside it; the keywords after it belong to the
    // main grid.
    const auto state = lgrBlockState("");
    BOOST_CHECK_CLOSE(state.fieldProps().get_double("PORO")[0], 0.25, 1.0e-8);
}

// A CARFIN block's values belong to its refined cells, never to the main grid.
BOOST_AUTO_TEST_CASE(TestCARFINvaluesStayOffMainGrid)
{
    for (const std::string block : { "PORO\n 9*0.30 /",
                                     "EQUALS\n PORO 0.30 1 1 1 1 1 1 /\n/",
                                     "MINPV\n 1000 /" })
    {
        const auto state = lgrBlockState(block);
        BOOST_CHECK_CLOSE(state.fieldProps().get_double("PORO")[4], 0.25, 1.0e-8);
    }
}

// A local grid block ends with ENDFIN, before the next block and before the
// end of its section.
BOOST_AUTO_TEST_CASE(TestLGRblockWithoutENDFINStops)
{
    const auto state = [](const std::string& blocks)
    {
        return Opm::EclipseState { Opm::Parser{}.parseString(fmt::format(R"(RUNSPEC
DIMENS
 3 3 1 /
GRID
DXV
 3*100.0 /
DYV
 3*100.0 /
DZ
 9*10.0 /
TOPS
 9*2000 /
PORO
 9*0.25 /
{}
)", blocks)) };
    };

    BOOST_CHECK_THROW(state(R"(CARFIN
'LGR1' 2 2 2 2 1 1 3 3 1 /)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(state(R"(CARFIN
'LGR1' 1 1 1 1 1 1 3 3 1 /
CARFIN
'LGR2' 3 3 3 3 1 1 3 3 1 /
ENDFIN)"), Opm::OpmInputError);

    BOOST_CHECK_NO_THROW(state(R"(CARFIN
'LGR1' 1 1 1 1 1 1 3 3 1 /
ENDFIN
CARFIN
'LGR2' 3 3 3 3 1 1 3 3 1 /
ENDFIN)"));

    // RADFIN and RADFIN4 open a block as well.
    BOOST_CHECK_THROW(state(R"(RADFIN
'LGR3' 2 2 1 1 2 4 1 1* 0.1524 50.0 /)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(state(R"(RADFIN4
'LGR4' 1 2 1 2 1 1 2 4 1 /)"), Opm::OpmInputError);
}

namespace {

// A 3 x 3 x NZ grid of 1000 ft x 1000 ft x 20 ft cells whose GRID section
// starts with the given keywords (CARFIN, ACTNUM).
Opm::EclipseGrid gridWithLgrs(const int nz, const std::string& grid_keywords)
{
    const auto deck = Opm::Parser{}.parseString(fmt::format(R"(
RUNSPEC
DIMENS
  3 3 {0} /
OIL
GAS
START
16 JUN 1988 /
GRID
{1}
DX
  {2}*1000 /
DY
  {2}*1000 /
DZ
  {2}*20 /
BOX
  1 3 1 3 1 1 /
TOPS
  9*8325 /
ENDBOX
PORO
  {2}*0.15 /
PERMX
  {2}*1 /
COPY
  PERMX PERMZ /
  PERMX PERMY /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)", nz, grid_keywords, 9 * nz));

    return Opm::EclipseState(deck).getInputGrid();
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TestLGRresetACTNUM) {
    auto eclipse_grid = gridWithLgrs(1, R"(
CARFIN
'LGR1'  1  2  1  1  1  1  6  3  1 /
ENDFIN
CARFIN
'LGR2'  3  3  3  3  1  1  3  3  1 /
ENDFIN
)");
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell("LGR1").getNumActive() , 18U );

    // Host cell (2,1) of LGR1 turned off after input, as the simulator does
    // for MINPV: its LGR cells become inactive, the others stay active.
    std::vector<int> actnum(9, 1);
    actnum[1] = 0;
    eclipse_grid.resetACTNUM(actnum);

    const auto& lgr1 = eclipse_grid.getLGRCell("LGR1");
    BOOST_CHECK_EQUAL( lgr1.getNumActive() , 9U );
    BOOST_CHECK( lgr1.cellActive(2,2,0) );
    BOOST_CHECK( !lgr1.cellActive(3,0,0) );
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell("LGR2").getNumActive() , 9U );

    // The host cell back on: its LGR cells keep their activity, as a host
    // never makes an LGR cell active.
    eclipse_grid.resetACTNUM();
    BOOST_CHECK_EQUAL( eclipse_grid.getLGRCell("LGR1").getNumActive() , 9U );
}

// Every host cell of an LGR turned off after input, as MINPV can do: the LGR
// keeps no active cell, the other LGR keeps its own.
BOOST_AUTO_TEST_CASE(TestLGRresetACTNUMAllHostCellsOff) {
    auto eclipse_grid = gridWithLgrs(1, R"(
CARFIN
'LGR1'  1  2  1  1  1  1  6  3  1 /
ENDFIN
CARFIN
'LGR2'  3  3  3  3  1  1  3  3  1 /
ENDFIN
)");

    std::vector<int> actnum(9, 1);
    actnum[0] = 0;
    actnum[1] = 0;
    auto output_grid = eclipse_grid;
    output_grid.resetACTNUM(actnum);

    BOOST_CHECK_EQUAL( output_grid.getLGRCell("LGR1").getNumActive() , 0U );
    BOOST_CHECK_EQUAL( output_grid.getLGRCell("LGR2").getNumActive() , 9U );
}

namespace {

std::string blockDeck(const std::string& block)
{
    return R"(
RUNSPEC
DIMENS
  3 3 1 /
OIL
GAS
START
16 JUN 1988 /
GRID
CARFIN
'LGR1'  2  2  2  2  1  1  3  3  1 /
)" + block + R"(
ENDFIN
DX
  9*1000 /
DY
  9*1000 /
DZ
  9*20 /
TOPS
  9*8325 /
PORO
  9*0.2 /
PERMX
  9*100 /
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)";
}

}

BOOST_AUTO_TEST_CASE(TestLgrBlockValues) {
    const auto deck = Opm::Parser{}.parseString(blockDeck(R"(
EQUALS
  PERMX 25 1 1 1 3 1 1 /
/
MULTIPLY
  PORO 0.5 /
/
)"));
    const Opm::EclipseState state(deck);
    const auto& fp = state.fieldProps();

    // Local indices: the block's column i = 1, all of j.
    const auto* permx = fp.lgr_double("LGR1", "PERMX");
    BOOST_REQUIRE(permx != nullptr);
    BOOST_REQUIRE_EQUAL(permx->size(), 9U);
    const auto host = fp.get_double("PERMX")[4];
    for (std::size_t j = 0; j < 3; ++j) {
        BOOST_CHECK_CLOSE((*permx)[3*j + 0], host / 4, 1e-10);
        BOOST_CHECK_CLOSE((*permx)[3*j + 1], host, 1e-10);
        BOOST_CHECK_CLOSE((*permx)[3*j + 2], host, 1e-10);
    }

    // MULTIPLY acts on the inherited value.
    const auto* poro = fp.lgr_double("LGR1", "PORO");
    BOOST_REQUIRE(poro != nullptr);
    for (const auto value : *poro) {
        BOOST_CHECK_CLOSE(value, 0.1, 1e-10);
    }

    // Untouched arrays are inherited, not stored.
    BOOST_CHECK(fp.lgr_double("LGR1", "PERMY") == nullptr);
    BOOST_CHECK(fp.lgr_double("GLOBAL", "PERMX") == nullptr);

    // The global grid is not affected.
    BOOST_CHECK_CLOSE(fp.get_double("PORO")[4], 0.2, 1e-10);
}

BOOST_AUTO_TEST_CASE(TestLgrBlockActnumEmptiesHost) {
    const auto deck = Opm::Parser{}.parseString(blockDeck(R"(
ACTNUM
  9*0 /
)"));
    BOOST_CHECK_THROW(Opm::EclipseState{deck}, std::exception);
}

// A keyword of a CARFIN block whose values the refined cells do not take
// stops the input, as in every other local grid block: an array the
// simulator does not read per refined cell, MINPVV, part of an array the
// father has no value of, and a keyword that is no array.
BOOST_AUTO_TEST_CASE(TestLgrBlockUnappliedKeywordStops)
{
    for (const std::string block : { "MULTX\n 9*0.0 /",
                                     "MINPVV\n 9*1000 /",
                                     "FLUXNUM\n 9*2 /",
                                     "EQUALS\n MINPVV 1000 1 3 1 1 1 1 /\n/",
                                     "PINCH\n /" })
    {
        BOOST_CHECK_THROW(lgrBlockState(block), Opm::OpmInputError);
    }

    // The values the refined cells take.
    BOOST_CHECK_NO_THROW(lgrBlockState("PORO\n 9*0.30 /\nNTG\n 9*0.5 /\nMULTPV\n 9*2.0 /"));
}

// Only a block refining the global grid gives its refined cells values of
// their own: a nested block with a property or MINPV stops the input.
BOOST_AUTO_TEST_CASE(TestLgrNestedBlockValuesStop)
{
    const auto nested = [](const std::string& block)
    {
        return gridWithLgrs(1, fmt::format(R"(
CARFIN
'LGR1'  2  2  2  2  1  1  3  3  1  1*  GLOBAL /
ENDFIN
CARFIN
'LGR2'  2  2  2  2  1  1  3  3  1  1*  LGR1 /
{}
ENDFIN
)", block));
    };

    BOOST_CHECK_NO_THROW(nested(""));
    BOOST_CHECK_THROW(nested("PORO\n 9*0.30 /"), Opm::OpmInputError);
    BOOST_CHECK_THROW(nested("MINPV\n 10 /"), Opm::OpmInputError);
}

namespace {

// One column of two layers on sheared pillars; the lower layer thickens towards
// i = 1 more than the upper one. Fitting pillars to either layer would give
// different lines, whereas line interpolation must be layer independent.
std::vector<double> lgrPillarCoord(const int k)
{
    const auto deck = fmt::format(R"(RUNSPEC
DIMENS
 1 1 2 /
OIL
WATER
LGR
 1 /
GRID
COORD
 0 0 0      5 0 100
 100 0 0    100 8 100
 0 100 0    -6 100 100
 100 100 0  103 104 100 /
ZCORN
 4*10  20 30 20 30
 20 30 20 30  40 70 40 70 /
CARFIN
'LGR1' 1 1 1 1 {} {} 2 2 1 /
ENDFIN
PORO
 2*0.25 /
PERMX
 2*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)", k, k);

    const Opm::EclipseState state(Opm::Parser{}.parseString(deck));
    return state.getInputGrid().getLGRCell("LGR1").getCOORD();
}

} // Anonymous namespace

// Pillar lines depend on the host lines, not the layer being refined.
BOOST_AUTO_TEST_CASE(TestLgrPillarsIndependentOfLayer)
{
    const auto column = lgrPillarCoord(2);
    BOOST_CHECK(column == lgrPillarCoord(1));
    // Centre pillar: interpolate the four lines at common depths 0 and 100.
    const std::vector<double> centre {50, 50, 0, 50.5, 53, 100};
    BOOST_CHECK_EQUAL_COLLECTIONS(column.begin() + 24, column.begin() + 30,
                                  centre.begin(), centre.end());
}

BOOST_AUTO_TEST_CASE(TestLgrPillarEndpointIndependence)
{
    auto refined = [](const std::array<double, 4>& top,
                      const std::array<double, 4>& bottom) {
        std::string coord;
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < 2; ++i) {
                const int n = i + 2*j;
                // Different nonparallel line directions at all four corners.
                const double sx = 0.1*(n - 1);
                const double sy = 0.1*(2 - n);
                coord += fmt::format("{} {} {} {} {} {}\n",
                                     100*i + sx*top[n], 100*j + sy*top[n], top[n],
                                     100*i + sx*bottom[n], 100*j + sy*bottom[n], bottom[n]);
            }
        }
        const auto deck = Opm::Parser{}.parseString(fmt::format(R"(RUNSPEC
DIMENS
 1 1 1 /
OIL
WATER
LGR
 1 /
GRID
COORD
{} /
ZCORN
 4*20 4*80 /
CARFIN
 'LGR1' 1 1 1 1 1 1 3 2 1 /
ENDFIN
PORO
 0.25 /
PERMX
 100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)", coord));
        const Opm::EclipseState state(deck);
        const auto& lgr = state.getInputGrid().getLGRCell("LGR1");
        return std::pair {lgr.getCOORD(), lgr.getZCORN()};
    };
    const auto [first, zcorn] = refined({-20, 0, 10, -10}, {100, 120, 90, 110});
    const auto [moved, movedZcorn] = refined({-100, -50, 0, 10}, {200, 150, 120, 140});
    BOOST_CHECK(zcorn == movedZcorn);
    BOOST_REQUIRE_EQUAL(first.size(), 6*4*3);
    BOOST_REQUIRE_EQUAL(moved.size(), first.size());
    const std::array<double, 12> expectedTop {
        -20, -20, -20, 0, -20, -20, -20, -10, 10, -10, -10, -10
    };
    const std::array<double, 12> expectedBottom {
        100, 120, 120, 120, 100, 120, 120, 120, 90, 110, 110, 110
    };
    const std::array<double, 12> expectedMovedTop {
        -100, -100, -100, -50, -100, -100, -100, -50, 0, 0, 0, 10
    };
    const std::array<double, 12> expectedMovedBottom {
        200, 200, 200, 150, 200, 200, 200, 150, 120, 140, 140, 140
    };
    for (int j = 0; j <= 2; ++j) {
        for (int i = 0; i <= 3; ++i) {
            const auto pillar = j*4 + i;
            const auto p = 6*pillar;
            BOOST_CHECK_EQUAL(first[p + 2], expectedTop[pillar]);
            BOOST_CHECK_EQUAL(first[p + 5], expectedBottom[pillar]);
            BOOST_CHECK_EQUAL(moved[p + 2], expectedMovedTop[pillar]);
            BOOST_CHECK_EQUAL(moved[p + 5], expectedMovedBottom[pillar]);
            const double u = i/3.0;
            const double v = j/2.0;
            const double n = u + 2*v;
            for (double z : {-150.0, 50.0, 250.0}) {
                const double t = (z - first[p + 2])/(first[p + 5] - first[p + 2]);
                const double q = (z - moved[p + 2])/(moved[p + 5] - moved[p + 2]);
                const double x = first[p] + t*(first[p + 3] - first[p]);
                const double y = first[p + 1] + t*(first[p + 4] - first[p + 1]);
                BOOST_CHECK_SMALL(x - (100*u + 0.1*(n - 1)*z), 1e-10);
                BOOST_CHECK_SMALL(y - (100*v + 0.1*(2 - n)*z), 1e-10);
                BOOST_CHECK_SMALL(x - (moved[p] + q*(moved[p + 3] - moved[p])), 1e-10);
                BOOST_CHECK_SMALL(y - (moved[p + 1] + q*(moved[p + 4] - moved[p + 1])), 1e-10);
            }
        }
    }
}

// Each box computes this face from a different host column. The fault and
// off-face endpoint depths must not change the shared COORD representation.
BOOST_AUTO_TEST_CASE(TestLgrPillarsAgreeAcrossFault)
{
    std::string coord;
    for (int j = 0; j <= 1; ++j) {
        for (int i = 0; i <= 2; ++i) {
            const double top = -50.0*i;
            const double bottom = 600.0 + 70.0*i;
            coord += fmt::format("{} {} {} {} {} {}\n",
                                 100*i, 100*j - 0.1*j*top, top,
                                 100*i, 100*j - 0.1*j*bottom, bottom);
        }
    }
    const Opm::EclipseState state(Opm::Parser{}.parseString(fmt::format(R"(
RUNSPEC
DIMENS
 2 1 1 /
OIL
WATER
LGR
 2 /
GRID
COORD
{} /
ZCORN
 0 0 200 200  0 0 200 200  90 90 290 290  10 10 210 210 /
CARFIN
 'LGR1' 1 1 1 1 1 1 2 2 1 /
ENDFIN
CARFIN
 'LGR2' 2 2 1 1 1 1 2 2 1 /
ENDFIN
PORO
 2*0.25 /
PERMX
 2*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)", coord)));
    const auto& left = state.getInputGrid().getLGRCell("LGR1").getCOORD();
    const auto& right = state.getInputGrid().getLGRCell("LGR2").getCOORD();
    for (std::size_t j = 0; j <= 2; ++j) {
        BOOST_CHECK_EQUAL_COLLECTIONS(left.begin() + 6*(3*j + 2), left.begin() + 6*(3*j + 3),
                                      right.begin() + 6*(3*j), right.begin() + 6*(3*j + 1));
    }
}

// A spatial counterpart to the planar converging-pillar case. Fitting the
// refined pillars to either host layer folds children of the other layer.
// Sample the trilinear hexahedron Jacobian; this is a regression check, not
// a proof of positivity throughout arbitrary corner-point cells.
BOOST_AUTO_TEST_CASE(TestLgrSpatialRefinementKeepsPositiveSampledJacobians)
{
    const Opm::EclipseState state(Opm::Parser{}.parseString(R"(
RUNSPEC
DIMENS
 1 1 2 /
OIL
WATER
LGR
 1 /
GRID
COORD
 0   0   0  -128   3   600
 100 0   0   154   62  600
 0   100 0  -76    56  600
 100 100 0   185   212 600 /
ZCORN
 4*0
 444 270 144 558
 444 270 144 558
 4*600 /
CARFIN
 'LGR1' 1 1 1 1 1 2 2 2 8 /
ENDFIN
PORO
 2*0.25 /
PERMX
 2*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)"));
    const auto minimumSampledJacobian = [](const Opm::EclipseGrid& grid,
                                           std::size_t i, std::size_t j, std::size_t k) {
        std::array<std::array<double, 3>, 8> corners;
        for (std::size_t n = 0; n < corners.size(); ++n) {
            corners[n] = grid.getCornerPos(i, j, k, n);
        }
        double minimum = std::numeric_limits<double>::max();
        for (int a = 0; a <= 10; ++a) {
            for (int b = 0; b <= 10; ++b) {
                for (int c = 0; c <= 10; ++c) {
                    const std::array<double, 3> local {a/10.0, b/10.0, c/10.0};
                    std::array<std::array<double, 3>, 3> jacobian{};
                    for (std::size_t n = 0; n < corners.size(); ++n) {
                        for (int derivative = 0; derivative < 3; ++derivative) {
                            double weight = 1.0;
                            for (int dim = 0; dim < 3; ++dim) {
                                const bool high = (n >> dim) & 1;
                                weight *= dim == derivative ? (high ? 1.0 : -1.0)
                                    : (high ? local[dim] : 1.0 - local[dim]);
                            }
                            for (int dim = 0; dim < 3; ++dim) {
                                jacobian[derivative][dim] += weight*corners[n][dim];
                            }
                        }
                    }
                    const auto& x = jacobian[0];
                    const auto& y = jacobian[1];
                    const auto& z = jacobian[2];
                    const double determinant = x[0]*(y[1]*z[2] - y[2]*z[1])
                        - x[1]*(y[0]*z[2] - y[2]*z[0]) + x[2]*(y[0]*z[1] - y[1]*z[0]);
                    minimum = std::min(minimum, determinant);
                }
            }
        }
        return minimum;
    };
    const auto& parent = state.getInputGrid();
    for (std::size_t k = 0; k < 2; ++k) {
        BOOST_CHECK_GT(minimumSampledJacobian(parent, 0, 0, k), 0.0);
    }
    const auto& refined = parent.getLGRCell("LGR1");
    BOOST_REQUIRE_EQUAL(refined.getCartesianSize(), 32);
    for (std::size_t k = 0; k < refined.getNZ(); ++k) {
        for (std::size_t j = 0; j < refined.getNY(); ++j) {
            for (std::size_t i = 0; i < refined.getNX(); ++i) {
                BOOST_TEST_CONTEXT("refined cell " << i << ',' << j << ',' << k) {
                    BOOST_CHECK_GT(minimumSampledJacobian(refined, i, j, k), 0.0);
                }
            }
        }
    }
}

// Norne's host cell (10,33,22) alone: its four pillars lean in different
// directions, and the line rule folds one of its 2 x 2 x 2 refined cells.
// Refining it warns, naming that cell; an upright cell gives no warning.
BOOST_AUTO_TEST_CASE(TestLgrWarnsAboutFoldedRefinedCells)
{
    const auto warningsFor = [](const std::string& grid) {
        std::ostringstream warnings;
        Opm::OpmLog::addBackend("LGRFOLDS", std::make_shared<Opm::StreamLog>(warnings, Opm::Log::MessageType::Warning));
        const Opm::EclipseState state(Opm::Parser{}.parseString(R"(
RUNSPEC
DIMENS
 1 1 1 /
OIL
WATER
LGR
 1 /
GRID
)" + grid + R"(
CARFIN
 'LGR1' 1 1 1 1 1 1 2 2 2 /
ENDFIN
PORO
 0.25 /
PERMX
 100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)"));
        Opm::OpmLog::removeBackend("LGRFOLDS");
        return warnings.str();
    };

    const auto folded = warningsFor(R"(
COORD
 456901.656 7322248.5 2513.483 456996.812 7322205.5 2802.334
 457009.781 7322128.5 2527.199 456982.312 7322165.5 2801.748
 456976.5 7322302.0 2526.897 457022.0 7322293.0 2796.347
 457059.531 7322200.0 2526.393 457034.0 7322234.5 2803.671 /
ZCORN
 2771.63 2765.843 2766.262 2765.645
 2802.334 2795.837 2796.347 2795.715 /
)");
    BOOST_CHECK_MESSAGE(folded.find("LGR LGR1: 1 refined cell(s) folded") != std::string::npos, folded);
    BOOST_CHECK_MESSAGE(folded.find("(1,1,2) in (1,1,1)") != std::string::npos, folded);

    const auto upright = warningsFor(R"(
COORD
 0   0   0   0   0   10
 100 0   0   100 0   10
 0   100 0   0   100 10
 100 100 0   100 100 10 /
ZCORN
 4*0
 4*10 /
)");
    BOOST_CHECK_MESSAGE(upright.find("fold") == std::string::npos, upright);
}

BOOST_AUTO_TEST_CASE(TestLgrZeroThicknessFatherGivesFlatRefinedCells)
{
    // The middle layer has zero thickness. Its refined cells must be flat, and
    // no refined cell may be upside down; the fractions of 3 refined layers per
    // father do not round exactly.
    const Opm::EclipseState state(Opm::Parser{}.parseString(R"(
RUNSPEC
DIMENS
 3 3 3 /
OIL
WATER
LGR
 1 1 /
GRID
DX
 27*100 /
DY
 27*100 /
DZ
 9*10.37 9*0 9*7.91 /
TOPS
 2075.438530 2094.930120 2011.741428 2089.191318 2043.775731
 2061.402219 2018.327457 2071.668803 2033.119022 /
PORO
 27*0.25 /
PERMX
 27*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
CARFIN
 'LGR1' 1 2 1 2 1 3 6 6 9 /
ENDFIN
PROPS
REGIONS
SOLUTION
SCHEDULE
)"));
    const auto& refined = state.getInputGrid().getLGRCell("LGR1");
    BOOST_REQUIRE_EQUAL(refined.getNZ(), 9);
    for (std::size_t k = 0; k < refined.getNZ(); ++k) {
        for (std::size_t j = 0; j < refined.getNY(); ++j) {
            for (std::size_t i = 0; i < refined.getNX(); ++i) {
                for (std::size_t corner = 0; corner < 4; ++corner) {
                    const double top = refined.getCornerPos(i, j, k, corner)[2];
                    const double bottom = refined.getCornerPos(i, j, k, corner + 4)[2];
                    BOOST_TEST_CONTEXT("refined cell " << i << ',' << j << ',' << k << " corner " << corner) {
                        BOOST_CHECK_GE(bottom, top);
                        if (k >= 3 && k < 6) {
                            BOOST_CHECK_EQUAL(bottom, top);
                        }
                    }
                }
            }
        }
    }
}

namespace {

Opm::EclipseState nncNextToBox(const std::string& nnc)
{
    return Opm::EclipseState { Opm::Parser{}.parseString(fmt::format(R"(RUNSPEC
DIMENS
 3 3 1 /
OIL
WATER
LGR
 1 /
GRID
DX
 9*100 /
DY
 9*100 /
DZ
 9*10 /
TOPS
 9*2000 /
PORO
 9*0.25 /
PERMX
 9*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
CARFIN
'LGR1' 2 2 2 2 1 1 3 3 1 /
ENDFIN
NNC
 {} 1.0 /
/
PROPS
REGIONS
SOLUTION
SCHEDULE
)", nnc)) };
}

} // Anonymous namespace

// An explicit NNC into a refinement box is one the refined grid cannot carry.
BOOST_AUTO_TEST_CASE(TestNncIntoLgrRefused)
{
    const auto refuse = [](const Opm::EclipseState& state) {
        Opm::refuseDeckConnectionsInsideBoxes(state, state.getInputGrid().getNXYZ(),
                                              Opm::lgrCellBoxes(state.getLgrs()));
    };
    BOOST_CHECK_THROW(refuse(nncNextToBox("2 2 1 3 3 1")), std::invalid_argument);
    BOOST_CHECK_NO_THROW(refuse(nncNextToBox("1 1 1 3 3 1")));
}

// A refined cell's level-zero host follows N*FIN, not the average ratio.
BOOST_AUTO_TEST_CASE(TestLgrLevelZeroIJKGraded)
{
    const Opm::EclipseState state { Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 1 /
OIL
WATER
LGR
 1 /
GRID
DX
 9*100 /
DY
 9*100 /
DZ
 9*10 /
TOPS
 9*2000 /
PORO
 9*0.25 /
PERMX
 9*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
CARFIN
'LGR1' 1 2 1 1 1 1 5 1 1 /
NXFIN
 2 3 /
ENDFIN
PROPS
REGIONS
SOLUTION
SCHEDULE
)") };
    const auto& lgrs = state.getLgrs();
    BOOST_CHECK((lgrs.levelZeroIJK(1, {1, 0, 0}) == std::array<int,3>{0, 0, 0}));
    BOOST_CHECK((lgrs.levelZeroIJK(1, {2, 0, 0}) == std::array<int,3>{1, 0, 0}));
    BOOST_CHECK((lgrs.levelZeroIJK(1, {4, 0, 0}) == std::array<int,3>{1, 0, 0}));
}

BOOST_AUTO_TEST_CASE(TestLgrGradedIndexList)
{
    // Five refined columns over two parent cells: an even split does not
    // cover them, NXFIN 1 4 does. Every refined cell must map to the parent
    // cell NXFIN gives it.
    const Opm::EclipseState state { Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 1 /
OIL
WATER
LGR
 1 /
GRID
DX
 9*100 /
DY
 9*100 /
DZ
 9*10 /
TOPS
 9*2000 /
PORO
 9*0.25 /
PERMX
 9*100 /
COPY
 PERMX PERMY /
 PERMX PERMZ /
/
CARFIN
'LGR1' 1 2 1 1 1 1 5 1 1 /
NXFIN
 1 4 /
ENDFIN
PROPS
REGIONS
SOLUTION
SCHEDULE
)") };
    const auto& lgr = state.getLgrs().getLgr("LGR1");
    const auto& cells = lgr.global_index_list();
    BOOST_REQUIRE_EQUAL(cells.size(), 5U);

    const std::array<std::size_t, 5> expectedParent { 0, 1, 1, 1, 1 };
    for (const auto& cell : cells) {
        BOOST_CHECK_EQUAL(cell.global_index, expectedParent[cell.data_index]);
    }
    BOOST_CHECK_EQUAL(lgr.index_list().size(), 5U);
}
