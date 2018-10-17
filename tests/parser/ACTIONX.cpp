/*
  Copyright 2018 Statoil ASA.

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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE ACTIONX

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <ert/util/util.h>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ActionContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Actions.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ActionX.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

using namespace Opm;




BOOST_AUTO_TEST_CASE(Create) {
    const auto action_kw = std::string{ R"(
ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/
)"};
    ActionX action1("NAME", 10, 100, 0);
    BOOST_CHECK_EQUAL(action1.name(), "NAME");

    const auto deck = Parser{}.parseString( action_kw );
    const auto& kw = deck.getKeyword("ACTIONX");

    ActionX action2(kw, 0);
    BOOST_CHECK_EQUAL(action2.name(), "ACTION");
}


BOOST_AUTO_TEST_CASE(SCAN) {
    const auto MISSING_END= std::string{ R"(
SCHEDULE

ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/

TSTEP
   10 /
)"};

    const auto WITH_WELSPECS = std::string{ R"(
SCHEDULE

WELSPECS
  'W2'  'OP'  1 1 3.33  'OIL' 7*/
/

ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/

WELSPECS
  'W1'  'OP'  1 1 3.33  'OIL' 7*/
/

ENDACTIO

TSTEP
   10 /
)"};

    const auto WITH_GRID = std::string{ R"(
SCHEDULE

WELSPECS
  'W2'  'OP'  1 1 3.33  'OIL' 7*/
/

ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/

PORO
  100*0.78 /

ENDACTIO

TSTEP
   10 /
)"};
    Opm::Parser parser;
    auto deck1 = parser.parseString(MISSING_END, Opm::ParseContext());
    auto deck2 = parser.parseString(WITH_WELSPECS, Opm::ParseContext());
    auto deck3 = parser.parseString(WITH_GRID, Opm::ParseContext());
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck1 );
    Eclipse3DProperties eclipseProperties ( deck1 , table, grid1);
    Runspec runspec (deck1);

    // The ACTIONX keyword has no matching 'ENDACTIO' -> exception
    BOOST_CHECK_THROW(Schedule(deck1, grid1, eclipseProperties, runspec, ParseContext()), std::invalid_argument);

    Schedule sched(deck2, grid1, eclipseProperties, runspec, ParseContext());
    BOOST_CHECK( !sched.hasWell("W1") );
    BOOST_CHECK( sched.hasWell("W2"));

    // The deck3 contains the 'GRID' keyword in the ACTIONX block - that is not a whitelisted keyword.
    ParseContext parseContext( {{ParseContext::ACTIONX_ILLEGAL_KEYWORD, InputError::THROW_EXCEPTION}} );
    BOOST_CHECK_THROW(Schedule(deck3, grid1, eclipseProperties, Phases(true,true,true), parseContext), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(TestActions) {
    Opm::ActionContext context;
    Opm::Actions config;
    BOOST_CHECK_EQUAL(config.size(), 0);

    Opm::ActionX action1("NAME", 10, 100, 0);
    config.add(action1);
    BOOST_CHECK_EQUAL(config.size(), 1);


    double min_wait = 86400;
    size_t max_eval = 3;
    {
        Opm::ActionX action("NAME", max_eval, min_wait, util_make_date_utc(1, 7, 2000));
        config.add(action);
        BOOST_CHECK_EQUAL(config.size(), 1);


        Opm::ActionX action3("NAME3", 1000000, 0, util_make_date_utc(1,7,2000));
        config.add(action3);
    }
    Opm::ActionX& action2 = config.at("NAME");
    BOOST_CHECK(action2.ready(util_make_date_utc(1,7,2000)));
    BOOST_CHECK(!action2.ready(util_make_date_utc(1,6,2000)));
    BOOST_CHECK(!action2.eval(util_make_date_utc(1,6,2000), context));

    BOOST_CHECK(action2.eval(util_make_date_utc(1,8,2000), context));
    BOOST_CHECK(!action2.ready(util_make_date_utc(1,8,2000)));
    BOOST_CHECK(!action2.eval(util_make_date_utc(1,8,2000), context));

    BOOST_CHECK(action2.ready(util_make_date_utc(3,8,2000)));
    BOOST_CHECK(config.ready(util_make_date_utc(3,8,2000)));
    BOOST_CHECK(action2.eval(util_make_date_utc(3,8,2000), context));

    BOOST_CHECK(action2.ready(util_make_date_utc(5,8,2000)));
    BOOST_CHECK(action2.eval(util_make_date_utc(5,8,2000), context));

    BOOST_CHECK(!action2.ready(util_make_date_utc(7,8,2000)));
    BOOST_CHECK(!action2.eval(util_make_date_utc(7,8,2000), context));
    BOOST_CHECK(config.ready(util_make_date_utc(7,8,2000)));

    auto pending = config.pending( util_make_date_utc(7,8,2000));
    BOOST_CHECK_EQUAL( pending.size(), 1);
    for (auto& ptr : pending) {
        BOOST_CHECK( ptr->ready(util_make_date_utc(7,8,2000)));
        BOOST_CHECK( ptr->eval(util_make_date_utc(7,8,2000), context));
    }

    BOOST_CHECK(!action2.eval(util_make_date_utc(7,8,2000), context ));
}



BOOST_AUTO_TEST_CASE(TestContext) {
    Opm::ActionContext context;

    BOOST_REQUIRE_THROW(context.get("func", "arg"), std::out_of_range);

    context.add("FUNC", "ARG", 100);
    BOOST_CHECK_EQUAL(context.get("FUNC", "ARG"), 100);
}
