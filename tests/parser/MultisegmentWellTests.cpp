/*
  Copyright 2017 SINTEF Digital, Mathematics and Cybernetics.

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
#include <memory>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE WellConnectionsTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>


#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/MSW/updatingConnectionsWithSegments.hpp>

static Opm::TimeMap createXDaysTimeMap(size_t numDays) {
const std::time_t startDate = Opm::TimeMap::mkdate(2010, 1, 1);
Opm::TimeMap timeMap{ startDate };
for (size_t i = 0; i < numDays; i++)
  timeMap.addTStep((i+1) * 24 * 60 * 60);
return timeMap;
}


BOOST_AUTO_TEST_CASE(MultisegmentWellTest) {
    const std::string compsegs_string =
        "COMPDAT\n"
        " 'PROD01'  20  1   1   3 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
        " 'PROD01'  19  1   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
        " 'PROD01'  18  1   2   2 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Y'  21.925 / \n"
        " 'PROD01'  17  1   2   2 'OPEN' 1*   15.071   0.311  1391.859 1*  1*  'Y'  21.920 / \n"
        " 'PROD01'  16  1   2   2 'OPEN' 1*    6.242   0.311   576.458 1*  1*  'Y'  21.915 / \n"
        "/\n"
        "WELSEGS \n"
        "'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'H--' 'HO' /\n"
        "2         2      1      1    2537.5 2537.5  0.3   0.00010 /\n"
        "3         3      1      2    2562.5 2562.5  0.2  0.00010 /\n"
        "4         4      2      2    2737.5 2537.5  0.2  0.00010 /\n"
        "6         6      2      4    3037.5 2539.5  0.2  0.00010 /\n"
        "7         7      2      6    3337.5 2534.5  0.2  0.00010 /\n"
        "/\n"
        "\n"
        "COMPSEGS\n"
        "PROD01 / \n"
        "20    1     1     1   2512.5   2525.0 /\n"
        "20    1     2     1   2525.0   2550.0 /\n"
        "20    1     3     1   2550.0   2575.0 /\n"
        "19    1     2     2   2637.5   2837.5 /\n"
        "18    1     2     2   2837.5   3037.5 /\n"
        "17    1     2     2   3037.5   3237.5 /\n"
        "16    1     2     2   3237.5   3437.5 /\n"
        "/\n";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string, Opm::ParseContext());
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("PROD01", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);
    Opm::EclipseGrid grid(100,100,100);
    Opm::UnitSystem unitSystem = Opm::UnitSystem::newMETRIC();
    Opm::TableManager tables;
    Opm::Eclipse3DProperties props(unitSystem, tables, grid);
    const Opm::DeckKeyword compsegs = deck.getKeyword("COMPSEGS");
    const Opm::DeckKeyword welsegs = deck.getKeyword("WELSEGS");
    const Opm::DeckKeyword compdat = deck.getKeyword("COMPDAT");

    for (const auto& record : compdat)
        well.handleCOMPDAT(record, 0, grid, props);
    well.handleWELSEGS(welsegs, 0);
    well.handleCOMPSEGS(compsegs, 0);
    BOOST_CHECK( true );

    const auto& new_connection_set = well.getConnections(0);
    BOOST_CHECK_EQUAL(7U, new_connection_set.size());

    const Opm::Connection& connection1 = new_connection_set.get(0);
    BOOST_CHECK_EQUAL(connection1.getSegmentNumber(), 1);
    BOOST_CHECK_EQUAL(connection1.center_depth, 2512.5);

    //const Opm::Connection& connection3 = new_connection_set.get(2);
    //const int segment_number_connection3 = connection3.getSegmentNumber();
    //const double center_depth_connection3 = connection3.center_depth;
    //BOOST_CHECK_EQUAL(segment_number_connection3, 3);
    //BOOST_CHECK_EQUAL(center_depth_connection3, 2562.5);

    //const Opm::Connection& connection5 = new_connection_set.get(4);
    //const int segment_number_connection5 = connection5.getSegmentNumber();
    //const double center_depth_connection5 = connection5.center_depth;
    //BOOST_CHECK_EQUAL(segment_number_connection5, 6);
    //BOOST_CHECK_CLOSE(center_depth_connection5, 2538.83, 0.001);

    //const Opm::Connection& connection6 = new_connection_set.get(5);
    //const int segment_number_connection6 = connection6.getSegmentNumber();
    //const double center_depth_connection6 = connection6.center_depth;
    //BOOST_CHECK_EQUAL(segment_number_connection6, 6);
    //BOOST_CHECK_CLOSE(center_depth_connection6,  2537.83, 0.001);

    //const Opm::Connection& connection7 = new_connection_set.get(6);
    //const int segment_number_connection7 = connection7.getSegmentNumber();
    //const double center_depth_connection7 = connection7.center_depth;
    //BOOST_CHECK_EQUAL(segment_number_connection7, 7);
    //BOOST_CHECK_EQUAL(center_depth_connection7, 2534.5);
}

