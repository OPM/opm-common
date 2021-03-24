/*
  Copyright 2015 Statoil ASA.

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

#define BOOST_TEST_MODULE IOCONFIG_INTEGRATION_TEST
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <tuple>
#include <map>
#include <vector>
#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

inline std::string path_prefix() {
    return boost::unit_test::framework::master_test_suite().argv[1];
}

inline void verifyRestartConfig( const Schedule& sched, std::map<int, boost::gregorian::date>& rptConfig) {
    auto last = *rptConfig.rbegin();
    for (int step = 0; step <= last.first; step++) {
        if (rptConfig.count(step) == 1) {
            BOOST_CHECK( sched.write_rst_file(step) );

            auto report_date = rptConfig.at(step);
            std::time_t t = sched.simTime(step);
            boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
            boost::posix_time::ptime report_date_ptime(report_date);
            boost::posix_time::time_duration::sec_type duration = (report_date_ptime - epoch).total_seconds();

            BOOST_CHECK_EQUAL( duration , t );
        } else
            BOOST_CHECK(! sched.write_rst_file(step) );
    }
}

BOOST_AUTO_TEST_CASE( NorneRestartConfig ) {
    std::map<int, boost::gregorian::date> rptConfig;
    rptConfig.emplace(0 , boost::gregorian::date( 1997,11,6));
    rptConfig.emplace(1 , boost::gregorian::date( 1997,11,14));
    rptConfig.emplace(2 , boost::gregorian::date( 1997,12,1));
    rptConfig.emplace(3 , boost::gregorian::date( 1997,12,17));
    rptConfig.emplace(4 , boost::gregorian::date( 1998,1,1));
    rptConfig.emplace(5 , boost::gregorian::date( 1998,2,1));
    rptConfig.emplace(10 , boost::gregorian::date( 1998,4,23));
    rptConfig.emplace(19 , boost::gregorian::date( 1998,7,16));
    rptConfig.emplace(27 , boost::gregorian::date( 1998,10,13));
    rptConfig.emplace(33 , boost::gregorian::date( 1999,1,4));
    rptConfig.emplace(44 , boost::gregorian::date( 1999,5,1));
    rptConfig.emplace(53 , boost::gregorian::date( 1999,7,15));
    rptConfig.emplace(62 , boost::gregorian::date( 1999,10,3));
    rptConfig.emplace(72 , boost::gregorian::date( 2000,2,1));
    rptConfig.emplace(77 , boost::gregorian::date( 2000,5,1));
    rptConfig.emplace(83 , boost::gregorian::date( 2000,8,1));
    rptConfig.emplace(95 , boost::gregorian::date( 2000,11,1));
    rptConfig.emplace(98 , boost::gregorian::date( 2001,2,1));
    rptConfig.emplace(101 , boost::gregorian::date( 2001,5,1));
    rptConfig.emplace(109 , boost::gregorian::date( 2001,7,2));
    rptConfig.emplace(112 , boost::gregorian::date( 2001,7,16));
    rptConfig.emplace(113 , boost::gregorian::date( 2001,7,30));
    rptConfig.emplace(114 , boost::gregorian::date( 2001,8,1));
    rptConfig.emplace(115 , boost::gregorian::date( 2001,8,10));
    rptConfig.emplace(116 , boost::gregorian::date( 2001,8,16));
    rptConfig.emplace(117 , boost::gregorian::date( 2001,9,1));
    rptConfig.emplace(118 , boost::gregorian::date( 2001,9,10));
    rptConfig.emplace(119 , boost::gregorian::date( 2001,10,1));
    rptConfig.emplace(120 , boost::gregorian::date( 2001,11,1));
    rptConfig.emplace(124 , boost::gregorian::date( 2002,2,1));
    rptConfig.emplace(129 , boost::gregorian::date( 2002,5,1));
    rptConfig.emplace(132 , boost::gregorian::date( 2002,7,8));
    rptConfig.emplace(141 , boost::gregorian::date( 2002,10,7));
    rptConfig.emplace(148 , boost::gregorian::date( 2003,1,2));
    rptConfig.emplace(157 , boost::gregorian::date( 2003,5,1));
    rptConfig.emplace(161 , boost::gregorian::date( 2003,7,10));
    rptConfig.emplace(164 , boost::gregorian::date( 2003,8,12));
    rptConfig.emplace(165 , boost::gregorian::date( 2003,9,1));
    rptConfig.emplace(166 , boost::gregorian::date( 2003,9,2));
    rptConfig.emplace(167 , boost::gregorian::date( 2003,9,10));
    rptConfig.emplace(168 , boost::gregorian::date( 2003,9,12));
    rptConfig.emplace(169 , boost::gregorian::date( 2003,9,13));
    rptConfig.emplace(170 , boost::gregorian::date( 2003,9,16));
    rptConfig.emplace(171 , boost::gregorian::date( 2003,10,1));
    rptConfig.emplace(172 , boost::gregorian::date( 2003,10,23));
    rptConfig.emplace(180 , boost::gregorian::date( 2004,1,19));
    rptConfig.emplace(185 , boost::gregorian::date( 2004,5,1));
    rptConfig.emplace(188 , boost::gregorian::date( 2004,7,3));
    rptConfig.emplace(192 , boost::gregorian::date( 2004,8,16));
    rptConfig.emplace(193 , boost::gregorian::date( 2004,9,1));
    rptConfig.emplace(194 , boost::gregorian::date( 2004,9,20));
    rptConfig.emplace(195 , boost::gregorian::date( 2004,10,1));
    rptConfig.emplace(196 , boost::gregorian::date( 2004,11,1));
    rptConfig.emplace(199 , boost::gregorian::date( 2005,1,12));
    rptConfig.emplace(206 , boost::gregorian::date( 2005,4,24));
    rptConfig.emplace(212 , boost::gregorian::date( 2005,7,10));
    rptConfig.emplace(221 , boost::gregorian::date( 2005,11,1));
    rptConfig.emplace(226 , boost::gregorian::date( 2006,1,18));
    rptConfig.emplace(231 , boost::gregorian::date( 2006,4,25));
    rptConfig.emplace(235 , boost::gregorian::date( 2006,8,1));
    rptConfig.emplace(237 , boost::gregorian::date( 2006,8,16));
    rptConfig.emplace(238 , boost::gregorian::date( 2006,9,1));
    rptConfig.emplace(239 , boost::gregorian::date( 2006,9,14));
    rptConfig.emplace(240 , boost::gregorian::date( 2006,10,1));
    rptConfig.emplace(241 , boost::gregorian::date( 2006,10,10));

    auto python = std::make_shared<Python>();
    Parser parser;
    auto deck = parser.parseFile( path_prefix() + "IOConfig/RPTRST_DECK.DATA");
    EclipseState state(deck);
    Schedule schedule(deck, state, python);

    verifyRestartConfig(schedule, rptConfig);
}




BOOST_AUTO_TEST_CASE( RestartConfig2 ) {
    std::map<int, boost::gregorian::date> rptConfig;

    rptConfig.emplace(0  , boost::gregorian::date(2000,1,1));
    rptConfig.emplace(8  , boost::gregorian::date(2000,7,1));
    rptConfig.emplace(27 , boost::gregorian::date(2001,1,1));
    rptConfig.emplace(45 , boost::gregorian::date(2001,7,1));
    rptConfig.emplace(50 , boost::gregorian::date(2001,8,24));
    rptConfig.emplace(61 , boost::gregorian::date(2002,1,1));
    rptConfig.emplace(79 , boost::gregorian::date(2002,7,1));
    rptConfig.emplace(89 , boost::gregorian::date(2003,1,1));
    rptConfig.emplace(99 , boost::gregorian::date(2003,7,1));
    rptConfig.emplace(109, boost::gregorian::date(2004,1,1));
    rptConfig.emplace(128, boost::gregorian::date(2004,7,1));
    rptConfig.emplace(136, boost::gregorian::date(2005,1,1));
    rptConfig.emplace(146, boost::gregorian::date(2005,7,1));
    rptConfig.emplace(158, boost::gregorian::date(2006,1,1));
    rptConfig.emplace(164, boost::gregorian::date(2006,7,1));
    rptConfig.emplace(170, boost::gregorian::date(2007,1,1));
    rptConfig.emplace(178, boost::gregorian::date(2007,7,1));
    rptConfig.emplace(184, boost::gregorian::date(2008,1,1));
    rptConfig.emplace(192, boost::gregorian::date(2008,7,1));
    rptConfig.emplace(198, boost::gregorian::date(2009,1,1));
    rptConfig.emplace(204, boost::gregorian::date(2009,7,1));
    rptConfig.emplace(210, boost::gregorian::date(2010,1,1));
    rptConfig.emplace(216, boost::gregorian::date(2010,7,1));
    rptConfig.emplace(222, boost::gregorian::date(2011,1,1));
    rptConfig.emplace(228, boost::gregorian::date(2011,7,1));
    rptConfig.emplace(234, boost::gregorian::date(2012,1,1));
    rptConfig.emplace(240, boost::gregorian::date(2012,7,1));
    rptConfig.emplace(246, boost::gregorian::date(2013,1,1));
    rptConfig.emplace(251, boost::gregorian::date(2013,5,2));


    auto python = std::make_shared<Python>();
    Parser parser;
    auto deck = parser.parseFile(path_prefix() + "IOConfig/RPT_TEST2.DATA");
    EclipseState state( deck);
    Schedule schedule(deck, state, python);
    verifyRestartConfig(schedule, rptConfig);

    auto keywords0 = schedule.rst_keywords(0);
    std::map<std::string, int> expected0 = {{"BG", 1},
                                           {"BO", 1},
                                           {"BW", 1},
                                           {"KRG", 1},
                                           {"KRO", 1},
                                           {"KRW", 1},
                                           {"VOIL", 1},
                                           {"VGAS", 1},
                                           {"VWAT", 1},
                                           {"DEN", 1},
                                           {"RVSAT", 1},
                                           {"RSSAT", 1},
                                           {"PBPD", 1},
                                           {"NORST", 1}};
    for (const auto& [kw, num] : expected0)
        BOOST_CHECK_EQUAL( keywords0.at(kw), num );

    auto keywords1 = schedule.rst_keywords(1);
    std::map<std::string, int> expected1 = {{"BG", 1},
                                            {"BO", 1},
                                            {"BW", 1},
                                            {"KRG", 1},
                                            {"KRO", 1},
                                            {"KRW", 1},
                                            {"VOIL", 1},
                                            {"VGAS", 1},
                                            {"VWAT", 1},
                                            {"DEN", 1},
                                            {"RVSAT", 1},
                                            {"RSSAT", 1},
                                            {"PBPD", 1},
                                            {"NORST", 1},
                                            {"FIP", 3},
                                            {"WELSPECS", 1},
                                            {"WELLS", 0},
                                            {"NEWTON", 1},
                                            {"SUMMARY", 1},
                                            {"CPU", 1},
                                            {"CONV", 10}};

    for (const auto& [kw, num] : expected1)
        BOOST_CHECK_EQUAL( keywords1.at(kw), num );

    BOOST_CHECK_EQUAL(expected1.size(), keywords1.size());

    auto keywords10 = schedule.rst_keywords(10);
    BOOST_CHECK( keywords10 == keywords1 );
}



BOOST_AUTO_TEST_CASE( SPE9END ) {
    Parser parser;
    auto deck = parser.parseFile(path_prefix() + "IOConfig/SPE9_END.DATA");
    BOOST_CHECK_NO_THROW( EclipseState state( deck) );
}
