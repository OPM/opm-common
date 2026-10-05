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

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <filesystem>
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
TOPS
100*1 /

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

    // Values inside CARFIN ... ENDFIN are not supported: the input stops
    // instead of switching off a cell of the main grid.
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    BOOST_CHECK_THROW(Opm::EclipseState{deck}, Opm::OpmInputError);
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

// Values inside a local grid block (CARFIN or REFINE ... ENDFIN) are not
// supported: the input stops instead of applying them to the main grid.
BOOST_AUTO_TEST_CASE(TestLGRvaluesInsideBlockStop)
{
    BOOST_CHECK_THROW(lgrBlockState(R"(PORO
 27*0.30 /)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState(R"(PERMX
 27*500 /)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState(R"(EQUALS
 PORO 0.30 1 1 1 1 1 1 /
/)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState(R"(MINPV
 1000 /)"), Opm::OpmInputError);

    BOOST_CHECK_THROW(lgrBlockState(R"(HXFIN
 0.2 0.3 0.5 /)"), Opm::OpmInputError);

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
TOPS
  9*8325 /
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
