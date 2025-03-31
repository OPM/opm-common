/*
  Copyright 2018 NORCE

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

#define BOOST_TEST_MODULE WellTracerTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTracerProperties.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

using namespace Opm;

namespace {

Deck createDeckWithOutTracer()
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
     'W_1'        'OP'   2   2  1*       'OIL'  7* /
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

Deck createDeckWithDynamicWTRACER()
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
DATES             -- 1
 1  MAY 2000 /
/
WTRACER
     'W_1' 'I1'       1 /
     'W_1' 'I2'       1 /
/
DATES             -- 2, 3
 1  JUL 2000 /
 1  AUG 2000 /
/
WTRACER
     'W_1' 'I1'       0 /
/
DATES             -- 4
 1  SEP 2000 /
/

END
)");
}

Deck createDeckWithTracerInProducer()
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
WCONPROD
   'W_1' 'OPEN' 'ORAT' 20000  4* 1000 /
WTRACER
     'W_1' 'I1'       1 /
     'W_1' 'I2'       1 /
/
END
)");
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TestNoTracer)
{
    const auto deck = createDeckWithOutTracer();

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK(!deck.hasKeyword("WTRACER"));
}


BOOST_AUTO_TEST_CASE(TestDynamicWTRACER)
{
    const auto deck = createDeckWithDynamicWTRACER();

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK(deck.hasKeyword("WTRACER"));

    const auto& keyword = deck["WTRACER"].back();
    BOOST_CHECK_EQUAL(keyword.size(),1U);

    const auto& record = keyword.getRecord(0);
    const std::string& well_name = record.getItem("WELL").getTrimmedString(0);
    BOOST_CHECK_EQUAL(well_name, "W_1");
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 0).getTracerProperties().getConcentration("I1"),0); //default 0
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 0).getTracerProperties().getConcentration("I2"),0); //default 0
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 1).getTracerProperties().getConcentration("I1"),1);
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 2).getTracerProperties().getConcentration("I1"),1);
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 4).getTracerProperties().getConcentration("I1"),0);
    BOOST_CHECK_EQUAL(schedule.getWell("W_1", 4).getTracerProperties().getConcentration("I2"),1);
}


BOOST_AUTO_TEST_CASE(TestTracerInProducerTHROW)
{
    const auto deck = createDeckWithTracerInProducer();
    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);

    BOOST_CHECK_THROW(Schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()), OpmInputError);
}
