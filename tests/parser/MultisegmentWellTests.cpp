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

#include <iostream>
#include <memory>
#include <stdexcept>

#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE WellConnectionsTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>


#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/MSW/updatingConnectionsWithSegments.hpp>

BOOST_AUTO_TEST_CASE(MultisegmentWellTest) {

    auto dir = Opm::Connection::Direction::Z;
    Opm::WellConnections connection_set(10,10);
    Opm::EclipseGrid grid(20,20,20);
    connection_set.add(Opm::Connection( 19, 0, 0, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 19, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 19, 0, 2, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );

    connection_set.add(Opm::Connection( 18, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 17, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 16, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 15, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string =
        "WELSEGS \n"
        "'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'HF-' 'HO' /\n"
        "2         2      1      1    2537.5 2537.5  0.3   0.00010 /\n"
        "3         3      1      2    2562.5 2562.5  0.2  0.00010 /\n"
        "4         4      2      2    2737.5 2537.5  0.2  0.00010 /\n"
        "6         6      2      4    3037.5 2539.5  0.2  0.00010 /\n"
        "7         7      2      6    3337.5 2534.5  0.2  0.00010 /\n"
        "8         8      3      7    3337.6 2534.5  0.2  0.00015 /\n"
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
        "16    1     2     3   3237.5   3437.5 /\n"
        "/\n"
        "WSEGSICD\n"
        "'PROD01'  8   8   0.002   -0.7  1* 1* 0.6 1* 1* 2* 'SHUT' /\n"
        "/\n";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string);

    const Opm::DeckKeyword compsegs = deck.getKeyword("COMPSEGS");
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    Opm::WellSegments segment_set;
    const Opm::DeckKeyword welsegs = deck.getKeyword("WELSEGS");

    segment_set.loadWELSEGS(welsegs);

    BOOST_CHECK_EQUAL(6U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputError::THROW_EXCEPTION);
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputError::THROW_EXCEPTION);
    std::unique_ptr<Opm::WellConnections> new_connection_set{nullptr};
    BOOST_CHECK_NO_THROW(new_connection_set.reset(Opm::newConnectionsWithSegments(compsegs, connection_set, segment_set, grid, parseContext, errorGuard)));

    BOOST_CHECK_EQUAL(7U, new_connection_set->size());

    const Opm::Connection& connection1 = new_connection_set->get(0);
    const int segment_number_connection1 = connection1.segment();
    const double center_depth_connection1 = connection1.depth();
    BOOST_CHECK_EQUAL(segment_number_connection1, 1);
    BOOST_CHECK_EQUAL(center_depth_connection1, 2512.5);

    const Opm::Connection& connection3 = new_connection_set->get(2);
    const int segment_number_connection3 = connection3.segment();
    const double center_depth_connection3 = connection3.depth();
    BOOST_CHECK_EQUAL(segment_number_connection3, 3);
    BOOST_CHECK_EQUAL(center_depth_connection3, 2562.5);

    const Opm::Connection& connection5 = new_connection_set->get(4);
    const int segment_number_connection5 = connection5.segment();
    const double center_depth_connection5 = connection5.depth();
    BOOST_CHECK_EQUAL(segment_number_connection5, 6);
    BOOST_CHECK_CLOSE(center_depth_connection5, 2538.83, 0.001);

    const Opm::Connection& connection6 = new_connection_set->get(5);
    const int segment_number_connection6 = connection6.segment();
    const double center_depth_connection6 = connection6.depth();
    BOOST_CHECK_EQUAL(segment_number_connection6, 6);
    BOOST_CHECK_CLOSE(center_depth_connection6,  2537.83, 0.001);

    const Opm::Connection& connection7 = new_connection_set->get(6);
    const int segment_number_connection7 = connection7.segment();
    const double center_depth_connection7 = connection7.depth();
    BOOST_CHECK_EQUAL(segment_number_connection7, 7);
    BOOST_CHECK_EQUAL(center_depth_connection7, 2534.5);

}

BOOST_AUTO_TEST_CASE(WrongDistanceCOMPSEGS) {
    auto dir = Opm::Connection::Direction::Z;
    Opm::WellConnections connection_set(10,10);
    Opm::EclipseGrid grid(20,20,20);
    connection_set.add(Opm::Connection( 19, 0, 0, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 19, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 19, 0, 2, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );

    connection_set.add(Opm::Connection( 18, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 17, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 16, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 15, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string =
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
        "20    1     3     1   2550.0   2545.0 /\n"
        "19    1     2     2   2637.5   2837.5 /\n"
        "18    1     2     2   2837.5   3037.5 /\n"
        "17    1     2     2   3037.5   3237.5 /\n"
        "16    1     2     2   3237.5   3437.5 /\n"
        "/\n";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string);

    const Opm::DeckKeyword compsegs = deck.getKeyword("COMPSEGS");
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    Opm::WellSegments segment_set;
    const Opm::DeckKeyword welsegs = deck.getKeyword("WELSEGS");
    segment_set.loadWELSEGS(welsegs);

    BOOST_CHECK_EQUAL(6U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputError::THROW_EXCEPTION);
    BOOST_CHECK_THROW(std::unique_ptr<Opm::WellConnections>(Opm::newConnectionsWithSegments(compsegs, connection_set, segment_set, grid, parseContext, errorGuard)), std::invalid_argument);

    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputError::IGNORE);
    BOOST_CHECK_NO_THROW(std::unique_ptr<Opm::WellConnections>(Opm::newConnectionsWithSegments(compsegs, connection_set, segment_set, grid, parseContext, errorGuard)));
}

BOOST_AUTO_TEST_CASE(NegativeDepthCOMPSEGS) {
    auto dir = Opm::Connection::Direction::Z;
    Opm::WellConnections connection_set(10,10);
    Opm::EclipseGrid grid(20,20,20);
    connection_set.add(Opm::Connection( 19, 0, 0, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 19, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 19, 0, 2, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0, dir,0, 0., 0., true) );

    connection_set.add(Opm::Connection( 18, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 17, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 16, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );
    connection_set.add(Opm::Connection( 15, 0, 1, 1, 0.0, Opm::Connection::State::OPEN , 200, 17.29, 0.25, 0.0, 0.0, 0,  Opm::Connection::Direction::X,0, 0., 0., true) );

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string =
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
        "19    1     2     2   2637.5   2837.5 2* -8./\n"
        "18    1     2     2   2837.5   3037.5 /\n"
        "17    1     2     2   3037.5   3237.5 /\n"
        "16    1     2     2   3237.5   3437.5 /\n"
        "/\n";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string);

    const Opm::DeckKeyword compsegs = deck.getKeyword("COMPSEGS");
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    Opm::WellSegments segment_set;
    const Opm::DeckKeyword welsegs = deck.getKeyword("WELSEGS");
    segment_set.loadWELSEGS(welsegs);

    BOOST_CHECK_EQUAL(6U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    std::unique_ptr<Opm::WellConnections> wconns{nullptr};
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputError::THROW_EXCEPTION);
    BOOST_CHECK_THROW(wconns.reset(Opm::newConnectionsWithSegments(compsegs, connection_set, segment_set, grid, parseContext, errorGuard)), std::invalid_argument);

    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputError::IGNORE);
    BOOST_CHECK_NO_THROW(wconns.reset(Opm::newConnectionsWithSegments(compsegs, connection_set, segment_set, grid, parseContext, errorGuard)));
}
