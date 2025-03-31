/*
  Copyright 2015 IRIS

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

#define BOOST_TEST_MODULE WellSolventTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

using namespace Opm;

namespace {

Deck createDeckWithOutSolvent()
{
    return Parser{}.parseString(R"(
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE
WELSPECS
  'W_1'  'OP' 2 2  1*  'OIL'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
 'W_1' 'WATER' 'OPEN' 'BHP' 1 2 3/
/
END
)");
}

Deck createDeckWithGasInjector()
{
    return Parser{}.parseString(R"(
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE
WELSPECS
     'W_1'        'OP'   1   1  1*       'GAS'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
     'W_1' 'GAS' 'OPEN' 'BHP' 1 2 3/
/
WSOLVENT
     'W_1'        1 /
/
END
)");
}

Deck createDeckWithDynamicWSOLVENT()
{
    return Parser{}.parseString(R"(
START             -- 0
1 JAN 2000 /
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE
WELSPECS
     'W_1'        'OP'   1   1  1*       'GAS'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
     'W_1' 'GAS' 'OPEN' 'BHP' 1 2 3/
/
DATES             -- 2
 1  MAY 2000 /
/
WSOLVENT
     'W_1'        1 /
/
DATES             -- 3,4
 1  JUL 2000 /
 1  AUG 2000 /
/
WSOLVENT
     'W_1'        0 /
/
END
)");
}

Deck createDeckWithOilInjector()
{
    return Parser{}.parseString(R"(
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
SCHEDULE
WELSPECS
     'W_1'        'OP'   2   2  1*       'OIL'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
     'W_1' 'OIL' 'OPEN' 'BHP' 1 2 3/
/
WSOLVENT
     'W_1'        1 /
/
END
)");
}

Deck createDeckWithWaterInjector()
{
    return Parser{}.parseString(R"(
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
SCHEDULE
WELSPECS
     'W_1'        'OP'   2   2  1*       'OIL'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
     'W_1' 'WATER' 'OPEN' 'BHP' 1 2 3/
/
WSOLVENT
     'W_1'        1 /
/
END
)");
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TestNoSolvent)
{
    const auto deck = createDeckWithOutSolvent();
    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec(deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK(!deck.hasKeyword("WSOLVENT"));
}

BOOST_AUTO_TEST_CASE(TestGasInjector) {
    const auto deck = createDeckWithGasInjector();
    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec(deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK(deck.hasKeyword("WSOLVENT"));
}

BOOST_AUTO_TEST_CASE(TestDynamicWSOLVENT)
{
    const auto deck = createDeckWithDynamicWSOLVENT();
    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec(deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK(deck.hasKeyword("WSOLVENT"));

    const auto& keyword = deck["WSOLVENT"].back();
    BOOST_CHECK_EQUAL(keyword.size(),1U);

    const auto& record = keyword.getRecord(0);
    const std::string& well_name = record.getItem("WELL").getTrimmedString(0);
    BOOST_CHECK_EQUAL(well_name, "W_1");
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 0).getSolventFraction(),0); //default 0
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 1).getSolventFraction(),1);
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 2).getSolventFraction(),1);
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 3).getSolventFraction(),0);
}

BOOST_AUTO_TEST_CASE(TestOilInjector)
{
    const auto deck = createDeckWithOilInjector();
    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec(deck);

    BOOST_CHECK_THROW(Schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()), std::exception);
}

BOOST_AUTO_TEST_CASE(TestWaterInjector)
{
    const auto deck = createDeckWithWaterInjector();
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec(deck);

    BOOST_CHECK_THROW(Schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()), std::exception);
}
