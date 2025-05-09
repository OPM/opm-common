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

#define BOOST_TEST_MODULE WellConnectionsTests
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Schedule/MSW/SICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/Valve.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include "opm/input/eclipse/Schedule/MSW/Compsegs.hpp"
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/CompletedCells.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <memory>
#include <set>
#include <stdexcept>
#include <string>

BOOST_AUTO_TEST_CASE(AICDWellTest)
{
    const auto dir_z = Opm::Connection::Direction::Z;
    const auto dir_x = Opm::Connection::Direction::X;
    const auto kind = Opm::Connection::CTFKind::DeckValue;

    Opm::EclipseGrid grid { 20,20,20, 1.0, 1.0, 25.0, 2500.0 };

    const auto depth = 0.0;
    const auto state = Opm::Connection::State::OPEN;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 200.0;
    ctf_props.Kh = 17.29;
    ctf_props.rw = 0.25;

    Opm::WellConnections connection_set(Opm::Connection::Order::TRACK, 10,10);
    connection_set.add({ 19, 0, 0, grid.getGlobalIndex(19,0,0), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 1, grid.getGlobalIndex(19,0,1), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 2, grid.getGlobalIndex(19,0,2), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });

    connection_set.add({ 18, 0, 1, grid.getGlobalIndex(18,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 17, 0, 1, grid.getGlobalIndex(17,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 16, 0, 1, grid.getGlobalIndex(16,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 15, 0, 1, grid.getGlobalIndex(15,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string = R"(
GRID

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSEGS
'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'HF-' 'HO' /
2         2      1      1    2537.5 2537.5  0.3   0.00010 /
3         3      1      2    2562.5 2562.5  0.2  0.00010 /
4         4      2      2    2737.5 2537.5  0.2  0.00010 /
6         6      2      4    3037.5 2539.5  0.2  0.00010 /
7         7      2      6    3337.5 2534.5  0.2  0.00010 /
8         8      3      7    3337.6 2534.5  0.2  0.00015 /
/

COMPSEGS
PROD01 /
20    1     1     1   2512.5   2525.0 /
20    1     2     1   2525.0   2550.0 /
20    1     3     1   2550.0   2575.0 /
19    1     2     2   2637.5   2837.5 /
18    1     2     2   2837.5   3037.5 /
17    1     2     2   3037.5   3237.5 /
16    1     2     3   3237.5   3437.5 /
/

WSEGAICD
'PROD01'  8   8   0.002   -0.7  1* 1* 0.6 1* 1* 2* 1.0 1.0 'SHUT' /
/
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string);

    const Opm::DeckKeyword compsegs = deck["COMPSEGS"].back();
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    const Opm::DeckKeyword welsegs = deck["WELSEGS"].back();
    Opm::WellSegments segment_set{};
    const Opm::UnitSystem unit_system {}; // Metric by default
    segment_set.loadWELSEGS(welsegs, unit_system);

    BOOST_CHECK_EQUAL(7U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputErrorAction::THROW_EXCEPTION);
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputErrorAction::THROW_EXCEPTION);
    Opm::CompletedCells cells(grid);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, grid, Opm::TableManager());
    const auto& [new_connection_set, new_segment_set] = Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard);

    // checking the ICD segment
    const Opm::DeckKeyword wsegaicd = deck["WSEGAICD"].back();
    const auto aicd_map = Opm::AutoICD::fromWSEGAICD(wsegaicd);
    BOOST_CHECK_EQUAL(1U, aicd_map.size());

    const auto it = aicd_map.begin();
    const std::string& well_name = it->first;
    BOOST_CHECK_EQUAL(well_name, "PROD01");

    const auto& aicd_vector = it->second;
    BOOST_CHECK_EQUAL(1U, aicd_vector.size());
    const int segment_number = aicd_vector[0].first;
    const Opm::AutoICD& aicd0 = aicd_vector[0].second;
    BOOST_CHECK_EQUAL(8, segment_number);

    Opm::Segment segment = segment_set.getFromSegmentNumber(segment_number);
    segment.updateAutoICD(aicd0);

    BOOST_CHECK(Opm::Segment::SegmentType::AICD == segment.segmentType());

    auto aicd = segment.autoICD();
    BOOST_CHECK(!aicd.maxAbsoluteRate().has_value());
    BOOST_CHECK(aicd.status()==Opm::ICDStatus::SHUT);
    // 0.002 bars*day*day/Volume^2
    BOOST_CHECK_EQUAL(aicd.strength(), 0.002*1.e5*86400.*86400.);
    BOOST_CHECK_EQUAL(aicd.length(), -0.7);
    BOOST_CHECK_EQUAL(aicd.densityCalibration(), 1000.25);
    // 1.45 cp
    BOOST_CHECK_EQUAL(aicd.viscosityCalibration(), 1.45 * 0.001);
    BOOST_CHECK_EQUAL(aicd.criticalValue(), 0.6);
    BOOST_CHECK_EQUAL(aicd.widthTransitionRegion(), 0.05);
    BOOST_CHECK_EQUAL(aicd.maxViscosityRatio(), 5.0);
    BOOST_CHECK_EQUAL(aicd.methodFlowScaling(), -1);

    const int outlet_segment_number = segment.outletSegment();
    const double outlet_segment_length = segment_set.segmentLength(outlet_segment_number);
    // only one connection attached to the outlet segment in this case
    const Opm::Connection& connection = new_connection_set.getFromIJK(15, 0, 1);
    const auto& perf_range = connection.perf_range();
    const auto connection_length = perf_range->second - perf_range->first;
    aicd.updateScalingFactor(outlet_segment_length, connection_length);

    BOOST_CHECK_EQUAL(7U, new_segment_set.size());

    // updated, so it should not throw
    BOOST_CHECK_NO_THROW(aicd.scalingFactor());
    BOOST_CHECK_EQUAL(0.7, aicd.scalingFactor());

    BOOST_CHECK_EQUAL(7U, new_connection_set.size());

    const Opm::Connection& connection1 = new_connection_set.get(0);
    const int segment_number_connection1 = connection1.segment();
    const double center_depth_connection1 = connection1.depth();
    BOOST_CHECK_EQUAL(segment_number_connection1, 1);
    BOOST_CHECK_EQUAL(center_depth_connection1, 2512.5);

    const Opm::Connection& connection3 = new_connection_set.get(2);
    const int segment_number_connection3 = connection3.segment();
    const double center_depth_connection3 = connection3.depth();
    BOOST_CHECK_EQUAL(segment_number_connection3, 3);
    BOOST_CHECK_EQUAL(center_depth_connection3, 2562.5);

    const Opm::Connection& connection5 = new_connection_set.get(4);
    const int segment_number_connection5 = connection5.segment();
    const double center_depth_connection5 = connection5.depth();
    BOOST_CHECK_EQUAL(segment_number_connection5, 6);
    BOOST_CHECK_CLOSE(center_depth_connection5, 2538.83, 0.001);

    const Opm::Connection& connection6 = new_connection_set.get(5);
    const int segment_number_connection6 = connection6.segment();
    const double center_depth_connection6 = connection6.depth();
    BOOST_CHECK_EQUAL(segment_number_connection6, 6);
    BOOST_CHECK_CLOSE(center_depth_connection6,  2537.83, 0.001);

    const Opm::Connection& connection7 = new_connection_set.get(6);
    const int segment_number_connection7 = connection7.segment();
    const double center_depth_connection7 = connection7.depth();
    BOOST_CHECK_EQUAL(segment_number_connection7, 8);
    BOOST_CHECK_EQUAL(center_depth_connection7, 2534.5);
}

BOOST_AUTO_TEST_CASE(MultisegmentWellTest)
{
    const auto dir_z = Opm::Connection::Direction::Z;
    const auto dir_x = Opm::Connection::Direction::X;
    const auto kind = Opm::Connection::CTFKind::DeckValue;
    Opm::EclipseGrid grid { 20,20,20, 1.0, 1.0, 25.0, 2500.0 };

    const auto depth = 0.0;
    const auto state = Opm::Connection::State::OPEN;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 200.0;
    ctf_props.Kh = 17.29;
    ctf_props.rw = 0.25;

    Opm::WellConnections connection_set(Opm::Connection::Order::TRACK, 10,10);
    connection_set.add({ 19, 0, 0, grid.getGlobalIndex(19,0,0), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 1, grid.getGlobalIndex(19,0,1), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 2, grid.getGlobalIndex(19,0,2), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });

    connection_set.add({ 18, 0, 1, grid.getGlobalIndex(18,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 17, 0, 1, grid.getGlobalIndex(17,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 16, 0, 1, grid.getGlobalIndex(16,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 15, 0, 1, grid.getGlobalIndex(15,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string = R"(
GRID

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSEGS
'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'HF-' 'HO' /
2         2      1      1    2537.5 2537.5  0.3   0.00010 /
3         3      1      2    2562.5 2562.5  0.2  0.00010 /
4         4      2      2    2737.5 2537.5  0.2  0.00010 /
6         6      2      4    3037.5 2539.5  0.2  0.00010 /
7         7      2      6    3337.5 2534.5  0.2  0.00010 /
8         8      3      7    3337.6 2534.5  0.2  0.00015 /
/

COMPSEGS
PROD01 /
20    1     1     1   2512.5   2525.0 /
20    1     2     1   2525.0   2550.0 /
20    1     3     1   2550.0   2575.0 /
19    1     2     2   2637.5   2837.5 /
18    1     2     2   2837.5   3037.5 /
17    1     2     2   3037.5   3237.5 /
16    1     2     3   3237.5   3437.5 /
/
WSEGSICD
'PROD01'  8   8   0.002   -0.7  1* 1* 0.6 1* 1* 2* 'SHUT' /
/
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string);

    const Opm::DeckKeyword compsegs = deck["COMPSEGS"].back();
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    const Opm::DeckKeyword welsegs = deck["WELSEGS"].back();
    Opm::WellSegments segment_set{};
    const Opm::UnitSystem unit_system {}; // Metric by default
    segment_set.loadWELSEGS(welsegs, unit_system);

    BOOST_CHECK_EQUAL(7U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    Opm::CompletedCells cells(grid);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, grid, Opm::TableManager());
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputErrorAction::THROW_EXCEPTION);
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputErrorAction::THROW_EXCEPTION);
    const auto& [new_connection_set, new_segment_set] = Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard);

    // checking the ICD segment
    const Opm::DeckKeyword wsegsicd = deck["WSEGSICD"].back();
    BOOST_CHECK_EQUAL(1U, wsegsicd.size());
    const Opm::DeckRecord& record = wsegsicd.getRecord(0);
    const int start_segment = record.getItem("SEGMENT1").get< int >(0);
    const int end_segment = record.getItem("SEGMENT2").get< int >(0);
    BOOST_CHECK_EQUAL(8, start_segment);
    BOOST_CHECK_EQUAL(8, end_segment);

    const auto sicd_map = Opm::SICD::fromWSEGSICD(wsegsicd);

    BOOST_CHECK_EQUAL(1U, sicd_map.size());

    const auto it = sicd_map.begin();
    const std::string& well_name = it->first;
    BOOST_CHECK_EQUAL(well_name, "PROD01");

    const auto& sicd_vector = it->second;
    BOOST_CHECK_EQUAL(1U, sicd_vector.size());
    const int segment_number = sicd_vector[0].first;
    const Opm::SICD& sicd0 = sicd_vector[0].second;

    BOOST_CHECK_EQUAL(8, segment_number);

    Opm::Segment segment = segment_set.getFromSegmentNumber(segment_number);
    segment.updateSpiralICD(sicd0);

    BOOST_CHECK(Opm::Segment::SegmentType::SICD==segment.segmentType());

    auto sicd = segment.spiralICD();
    BOOST_CHECK(!sicd.maxAbsoluteRate().has_value());
    BOOST_CHECK(sicd.status()==Opm::ICDStatus::SHUT);
    // 0.002 bars*day*day/Volume^2
    BOOST_CHECK_EQUAL(sicd.strength(), 0.002*1.e5*86400.*86400.);
    BOOST_CHECK_EQUAL(sicd.length(), -0.7);
    BOOST_CHECK_EQUAL(sicd.densityCalibration(), 1000.25);
    // 1.45 cp
    BOOST_CHECK_EQUAL(sicd.viscosityCalibration(), 1.45 * 0.001);
    BOOST_CHECK_EQUAL(sicd.criticalValue(), 0.6);
    BOOST_CHECK_EQUAL(sicd.widthTransitionRegion(), 0.05);
    BOOST_CHECK_EQUAL(sicd.maxViscosityRatio(), 5.0);
    BOOST_CHECK_EQUAL(sicd.methodFlowScaling(), -1);

    const int outlet_segment_number = segment.outletSegment();
    const double outlet_segment_length = segment_set.segmentLength(outlet_segment_number);
    // only one connection attached to the outlet segment in this case
    const Opm::Connection& connection = new_connection_set.getFromIJK(15, 0, 1);
    const auto& perf_range = connection.perf_range();
    const auto connection_length = perf_range->second - perf_range->first;
    sicd.updateScalingFactor(outlet_segment_length, connection_length);

    // updated, so it should not throw
    BOOST_CHECK_NO_THROW(sicd.scalingFactor());
    BOOST_CHECK_EQUAL(0.7, sicd.scalingFactor());

    BOOST_CHECK_EQUAL(7U, new_segment_set.size());

    BOOST_CHECK_EQUAL(7U, new_connection_set.size());

    const Opm::Connection& connection1 = new_connection_set.get(0);
    const int segment_number_connection1 = connection1.segment();
    const double center_depth_connection1 = connection1.depth();
    BOOST_CHECK_EQUAL(segment_number_connection1, 1);
    BOOST_CHECK_EQUAL(center_depth_connection1, 2512.5);

    const Opm::Connection& connection3 = new_connection_set.get(2);
    const int segment_number_connection3 = connection3.segment();
    const double center_depth_connection3 = connection3.depth();
    BOOST_CHECK_EQUAL(segment_number_connection3, 3);
    BOOST_CHECK_EQUAL(center_depth_connection3, 2562.5);

    const Opm::Connection& connection5 = new_connection_set.get(4);
    const int segment_number_connection5 = connection5.segment();
    const double center_depth_connection5 = connection5.depth();
    BOOST_CHECK_EQUAL(segment_number_connection5, 6);
    BOOST_CHECK_CLOSE(center_depth_connection5, 2538.83, 0.001);

    const Opm::Connection& connection6 = new_connection_set.get(5);
    const int segment_number_connection6 = connection6.segment();
    const double center_depth_connection6 = connection6.depth();
    BOOST_CHECK_EQUAL(segment_number_connection6, 6);
    BOOST_CHECK_CLOSE(center_depth_connection6,  2537.83, 0.001);

    const Opm::Connection& connection7 = new_connection_set.get(6);
    const int segment_number_connection7 = connection7.segment();
    const double center_depth_connection7 = connection7.depth();
    BOOST_CHECK_EQUAL(segment_number_connection7, 8);
    BOOST_CHECK_EQUAL(center_depth_connection7, 2534.5);

}

BOOST_AUTO_TEST_CASE(WrongDistanceCOMPSEGS)
{
    const auto dir_z = Opm::Connection::Direction::Z;
    const auto dir_x = Opm::Connection::Direction::X;
    const auto kind = Opm::Connection::CTFKind::DeckValue;

    Opm::EclipseGrid grid { 20,20,20, 1.0, 1.0, 25., 2500.0 };

    const auto depth = 0.0;
    const auto state = Opm::Connection::State::OPEN;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 200.0;
    ctf_props.Kh = 17.29;
    ctf_props.rw = 0.25;

    Opm::WellConnections connection_set(Opm::Connection::Order::TRACK, 10,10);
    connection_set.add({ 19, 0, 0, grid.getGlobalIndex(19,0,0), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 1, grid.getGlobalIndex(19,0,1), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 2, grid.getGlobalIndex(19,0,2), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });

    connection_set.add({ 18, 0, 1, grid.getGlobalIndex(18,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 17, 0, 1, grid.getGlobalIndex(17,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 16, 0, 1, grid.getGlobalIndex(16,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 15, 0, 1, grid.getGlobalIndex(15,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string =
        "GRID \n"
        "PORO \n"
        "8000*0.1 /\n"
        "PERMX \n"
        "8000*1 /\n"
        "PERMY \n"
        "8000*0.1 /\n"
        "PERMZ \n"
        "8000*0.01 /\n"
        "SCHEDULE \n"
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

    const Opm::DeckKeyword compsegs = deck["COMPSEGS"].back();
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    const Opm::DeckKeyword welsegs = deck["WELSEGS"].back();
    Opm::WellSegments segment_set{};
    const Opm::UnitSystem unit_system {}; // Metric by default
    segment_set.loadWELSEGS(welsegs, unit_system);

    BOOST_CHECK_EQUAL(6U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    Opm::CompletedCells cells(grid);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, grid, Opm::TableManager());
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputErrorAction::THROW_EXCEPTION);
    BOOST_CHECK_THROW(Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard), Opm::OpmInputError);

    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputErrorAction::IGNORE);
    BOOST_CHECK_NO_THROW(Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard));
}

BOOST_AUTO_TEST_CASE(NegativeDepthCOMPSEGS)
{
    const auto dir_z = Opm::Connection::Direction::Z;
    const auto dir_x = Opm::Connection::Direction::X;
    const auto kind = Opm::Connection::CTFKind::DeckValue;

    Opm::EclipseGrid grid { 20,20,20, 1.0, 1.0, 25.0, 2500.0 };

    const auto depth = 0.0;
    const auto state = Opm::Connection::State::OPEN;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 200.0;
    ctf_props.Kh = 17.29;
    ctf_props.rw = 0.25;

    Opm::WellConnections connection_set(Opm::Connection::Order::TRACK, 10,10);
    connection_set.add({ 19, 0, 0, grid.getGlobalIndex(19,0,0), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 1, grid.getGlobalIndex(19,0,1), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 2, grid.getGlobalIndex(19,0,2), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });

    connection_set.add({ 18, 0, 1, grid.getGlobalIndex(18,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 17, 0, 1, grid.getGlobalIndex(17,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 16, 0, 1, grid.getGlobalIndex(16,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 15, 0, 1, grid.getGlobalIndex(15,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string =
        "GRID \n"
        "PORO \n"
        "8000*0.1 /\n"
        "PERMX \n"
        "8000*1 /\n"
        "PERMY \n"
        "8000*0.1 /\n"
        "PERMZ \n"
        "8000*0.01 /\n"
        "SCHEDULE \n"
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

    const Opm::DeckKeyword compsegs = deck["COMPSEGS"].back();
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    const Opm::DeckKeyword welsegs = deck["WELSEGS"].back();
    Opm::WellSegments segment_set{};
    const Opm::UnitSystem unit_system {}; // Metric by default
    segment_set.loadWELSEGS(welsegs, unit_system);

    BOOST_CHECK_EQUAL(6U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    Opm::CompletedCells cells(grid);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, grid, Opm::TableManager());
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputErrorAction::THROW_EXCEPTION);
    BOOST_CHECK_THROW(Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard), Opm::OpmInputError);

    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputErrorAction::IGNORE);
    BOOST_CHECK_NO_THROW( Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard) );
}

BOOST_AUTO_TEST_CASE(testwsegvalv)
{
    const auto dir_z = Opm::Connection::Direction::Z;
    const auto dir_x = Opm::Connection::Direction::X;
    const auto kind = Opm::Connection::CTFKind::DeckValue;
    Opm::EclipseGrid grid { 20,20,20, 1.0, 1.0, 25.0, 2500.0 };

    const auto depth = 0.0;
    const auto state = Opm::Connection::State::OPEN;

    auto ctf_props = Opm::Connection::CTFProperties{};

    ctf_props.CF = 200.0;
    ctf_props.Kh = 17.29;
    ctf_props.rw = 0.25;

    Opm::WellConnections connection_set(Opm::Connection::Order::TRACK, 10,10);
    connection_set.add({ 19, 0, 0, grid.getGlobalIndex(19,0,0), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 1, grid.getGlobalIndex(19,0,1), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 19, 0, 2, grid.getGlobalIndex(19,0,2), 1, state, dir_z, kind, 0, depth, ctf_props, 0, true });

    connection_set.add({ 18, 0, 1, grid.getGlobalIndex(18,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 17, 0, 1, grid.getGlobalIndex(17,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 16, 0, 1, grid.getGlobalIndex(16,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });
    connection_set.add({ 15, 0, 1, grid.getGlobalIndex(15,0,1), 1, state, dir_x, kind, 0, depth, ctf_props, 0, true });

    BOOST_CHECK_EQUAL( 7U , connection_set.size() );

    const std::string compsegs_string =
        "GRID \n"
        "PORO \n"
        "8000*0.1 /\n"
        "PERMX \n"
        "8000*1 /\n"
        "PERMY \n"
        "8000*0.1 /\n"
        "PERMZ \n"
        "8000*0.01 /\n"
        "SCHEDULE \n"
        "WELSEGS \n"
        "'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'HF-' 'HO' /\n"
        "2         2      1      1    2537.5 2537.5  0.3   0.00010 /\n"
        "3         3      1      2    2562.5 2562.5  0.2  0.00010 /\n"
        "4         4      2      2    2737.5 2537.5  0.2  0.00010 /\n"
        "6         6      2      4    3037.5 2539.5  0.2  0.00010 /\n"
        "7         7      2      6    3337.5 2534.5  0.2  0.00010 /\n"
        "8         8      3      6    3037.6 2539.5  0.2  0.00015 /\n"
        "9         9      4      7    3337.6 2534.5  0.2  0.00015 /\n"
        "/\n"
        "\n"
        "COMPSEGS\n"
        "PROD01 / \n"
        "20    1     1     1   2512.5   2525.0 /\n"
        "20    1     2     1   2525.0   2550.0 /\n"
        "20    1     3     1   2550.0   2575.0 /\n"
        "19    1     2     2   2637.5   2837.5 /\n"
        "18    1     2     2   2837.5   3037.5 /\n"
        "17    1     2     3   2937.5   3137.5 /\n"
        "16    1     2     4   3237.5   3437.5 /\n"
        "/\n"
        "WSEGVALV\n"
        "'PROD01'  8   0.002  5. /\n"
        "'PROD01'  9   0.001  6. 0. 1.2 0.1 8. SHUT 9./\n"
        "/\n";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string);

    const Opm::DeckKeyword compsegs = deck["COMPSEGS"].back();
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    const Opm::DeckKeyword welsegs = deck["WELSEGS"].back();
    Opm::WellSegments segment_set{};
    const Opm::UnitSystem unit_system {}; // Metric by default
    segment_set.loadWELSEGS(welsegs, unit_system);

    BOOST_CHECK_EQUAL(8U, segment_set.size());

    Opm::ErrorGuard   errorGuard;
    Opm::ParseContext parseContext;
    Opm::CompletedCells cells(grid);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, grid, Opm::TableManager());
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID, Opm::InputErrorAction::THROW_EXCEPTION);
    parseContext.update(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED, Opm::InputErrorAction::THROW_EXCEPTION);
    BOOST_CHECK_NO_THROW( Opm::Compsegs::processCOMPSEGS(compsegs, connection_set, segment_set, Opm::ScheduleGrid(grid, fp, cells), parseContext, errorGuard));

    // checking the WSEGVALV segment
    const Opm::DeckKeyword wsegvalv = deck["WSEGVALV"].back();
    BOOST_CHECK_EQUAL(2U, wsegvalv.size());

    const Opm::DeckRecord& record1 = wsegvalv.getRecord(0);
    const int seg1 = record1.getItem("SEGMENT_NUMBER").get< int >(0);
    BOOST_CHECK_EQUAL(8, seg1);

    const Opm::DeckRecord& record2 = wsegvalv.getRecord(1);
    const int seg2 = record2.getItem("SEGMENT_NUMBER").get< int >(0);
    BOOST_CHECK_EQUAL(9, seg2);

    const auto segvalv_map = Opm::Valve::fromWSEGVALV(wsegvalv);
    BOOST_CHECK_EQUAL(1U, segvalv_map.size());

    const auto it = segvalv_map.begin();
    const std::string& well_name = it->first;
    BOOST_CHECK_EQUAL(well_name, "PROD01");

    const auto& segvalv_vector = it->second;
    BOOST_CHECK_EQUAL(2U, segvalv_vector.size());

    const int segment_number1 = segvalv_vector[0].first;
    BOOST_CHECK_EQUAL(8, segment_number1);
    const Opm::Valve& valve1 = segvalv_vector[0].second;

    Opm::Segment segment1 = segment_set.getFromSegmentNumber(segment_number1);
    const double segment_length1 = segment_set.segmentLength(segment_number1);
    segment1.updateValve(valve1, segment_length1);
    BOOST_CHECK(Opm::Segment::SegmentType::VALVE==segment1.segmentType());

    const Opm::Valve& valv1 = segment1.valve();
    BOOST_CHECK_EQUAL(valv1.conFlowCoefficient(), 0.002);
    BOOST_CHECK_EQUAL(valv1.conCrossArea(), 5.);
    BOOST_CHECK_EQUAL(valv1.conMaxCrossArea(), 0.031415926535897934);
    BOOST_CHECK_CLOSE(valv1.pipeAdditionalLength(), 0.1, 1.e-10);
    BOOST_CHECK_EQUAL(valv1.pipeDiameter(), 0.2);
    BOOST_CHECK_EQUAL(valv1.pipeRoughness(), 0.00015);
    BOOST_CHECK_EQUAL(valv1.pipeCrossArea(), 0.031415926535897934);
    BOOST_CHECK(valv1.status()==Opm::ICDStatus::OPEN);

    const int segment_number2 = segvalv_vector[1].first;
    BOOST_CHECK_EQUAL(9, segment_number2);
    const Opm::Valve& valve2 = segvalv_vector[1].second;
    Opm::Segment segment2 = segment_set.getFromSegmentNumber(segment_number1);
    const double segment_length2 = segment_set.segmentLength(segment_number2);
    // checking the original segment input
    BOOST_CHECK_EQUAL(segment2.internalDiameter(), 0.2);
    BOOST_CHECK_EQUAL(segment2.roughness(), 0.00015);
    BOOST_CHECK_EQUAL(segment2.crossArea(), 0.031415926535897934);

    segment2.updateValve(valve2, segment_length2);
    BOOST_CHECK(Opm::Segment::SegmentType::VALVE ==segment2.segmentType());

    const Opm::Valve& valv2 = segment2.valve();
    BOOST_CHECK_EQUAL(valv2.conFlowCoefficient(), 0.001);
    BOOST_CHECK_EQUAL(valv2.conCrossArea(), 6.);
    BOOST_CHECK_EQUAL(valv2.conMaxCrossArea(), 9.);
    BOOST_CHECK_EQUAL(valv2.pipeAdditionalLength(), 0.0);
    BOOST_CHECK_EQUAL(valv2.pipeDiameter(), 1.2);
    BOOST_CHECK_EQUAL(valv2.pipeRoughness(), 0.1);
    BOOST_CHECK_EQUAL(valv2.pipeCrossArea(), 8.);
    BOOST_CHECK(valv2.status()==Opm::ICDStatus::SHUT);

    // valve changes the segment data
    BOOST_CHECK_EQUAL(segment2.internalDiameter(), valv2.pipeDiameter());
    BOOST_CHECK_EQUAL(segment2.roughness(), valv2.pipeRoughness());
    BOOST_CHECK_EQUAL(segment2.crossArea(), valv2.pipeCrossArea());
}

namespace {
    Opm::Schedule make_schedule(const std::string& fname)
    {
        Opm::Parser parser;
        Opm::Deck deck = parser.parseFile(fname);
        Opm::EclipseState st(deck);
        return Opm::Schedule(deck, st);
    }
} // Anonymous namespace


BOOST_AUTO_TEST_CASE(MSW_SEGMENT_LENGTH) {
    const auto& sched = make_schedule("MSW.DATA");
    const auto& well = sched.getWell("PROD01", 0);
    const auto& segments = well.getSegments();
    BOOST_CHECK_CLOSE( segments.segmentLength(1), 2512.50, 1e-5);
    BOOST_CHECK_CLOSE( segments.segmentLength(2), 25, 1e-5);
    BOOST_CHECK_CLOSE( segments.segmentLength(6), 25, 1e-5);
    BOOST_CHECK_CLOSE( segments.segmentLength(7), 200, 1e-5);

    BOOST_CHECK_CLOSE( segments.segmentDepthChange(1), 2512.50, 1e-5);
    BOOST_CHECK_CLOSE( segments.segmentDepthChange(2), 22, 1e-5);
    BOOST_CHECK_CLOSE( segments.segmentDepthChange(6), 21, 1e-5);
    BOOST_CHECK_CLOSE( segments.segmentDepthChange(7),  4, 1e-5);
}

BOOST_AUTO_TEST_CASE(MSW_BRANCH_SEGMENTS) {
    const auto& sched = make_schedule("MSW.DATA");
    const auto& well = sched.getWell("PROD01", 0);
    const auto& segments = well.getSegments();
    {
        auto seg100 = segments.branchSegments(100);
        BOOST_CHECK(seg100.empty());
    }
    {
        auto seg1 = segments.branchSegments(1);
        BOOST_CHECK_EQUAL( seg1.size(), 6U );
        const std::vector<int> expected = {1,2,3,4,5,6};
        for (std::size_t index = 0; index < seg1.size(); index++)
            BOOST_CHECK_EQUAL( expected[index], seg1[index].segmentNumber());
    }
    {
        auto seg2 = segments.branchSegments(2);
        const std::vector<int> expected = {7,8,9,10,11};
        BOOST_CHECK_EQUAL( seg2.size(), 5U );
        for (std::size_t index = 0; index < seg2.size(); index++)
            BOOST_CHECK_EQUAL( expected[index], seg2[index].segmentNumber());
    }
    {
        auto seg5 = segments.branchSegments(5);
        const std::vector<int> expected = {22,23,24,25,26};
        BOOST_CHECK_EQUAL( seg5.size(), 5U );
        for (std::size_t index = 0; index < seg5.size(); index++)
            BOOST_CHECK_EQUAL( expected[index], seg5[index].segmentNumber());
    }
}


BOOST_AUTO_TEST_CASE(Branches) {
    const auto& sched = make_schedule("MSW.DATA");
    const auto& well = sched.getWell("PROD01", 0);
    const auto& segments = well.getSegments();
    std::set<int> expected = {1,2,3,4,5};
    BOOST_CHECK( expected == segments.branches() );
}

BOOST_AUTO_TEST_CASE(MULTIPLE_WELSEGS) {
    const auto& sched1 = make_schedule("MSW.DATA");
    const auto& sched2 = make_schedule("MSW_2WELSEGS.DATA");

    const auto& well1 = sched1.getWell("PROD01", 0);
    const auto& segments1 = well1.getSegments();
    const auto& well2 = sched2.getWell("PROD01", 0);
    const auto& segments2 = well2.getSegments();

    BOOST_CHECK(segments1 == segments2);
}

BOOST_AUTO_TEST_CASE(Node_XY_ABS_Individual)
{
    const auto deck = ::Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
  20 20 20 /

GRID

DXV
  20*100 /

DYV
  20*100 /

DZV
  20*10 /

DEPTHZ
  441*2000.0 /

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSPECS
 'PROD01' 'P' 20 20 1* OIL /
/

COMPDAT
 'PROD01' 20 20 1 5 'OPEN' /
/

WELSEGS
'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'HF-' 'HO' 123.456 789.012 /
2         2      1      1    2537.5 2525.5  0.3  0.00010 2* 123.456 789.012 /
3         3      1      2    2562.5 2562.5  0.2  0.00010 2* 123.456 789.012 /
4         7      2      2    2737.5 2537.5  0.2  0.00010 2* 123.456 789.012 /
8         8      3      7    3337.6 2534.5  0.2  0.00015 2* 123.456 789.012 /
/

COMPSEGS
-- Name
  'PROD01' /
-- I    J     K   Branch
  20    20     1     1   2512.5   2525.0 /
  20    20     2     1   2525.0   2550.0 /
  20    20     3     1   2550.0   2575.0 /
  20    20     4     1   2637.5   2837.5 /
  20    20     5     1   2837.5   3037.5 /
/
)");

    const auto es    = ::Opm::EclipseState { deck };
    const auto sched = ::Opm::Schedule { deck, es, std::make_shared<const ::Opm::Python>() };

    const auto& segments = sched[0].wells("PROD01").getSegments();
    for (const auto& segment : segments) {
        BOOST_CHECK_CLOSE(segment.node_X(), 123.456, 1.0e-8);
        BOOST_CHECK_CLOSE(segment.node_Y(), 789.012, 1.0e-8);
    }
    // checking the segments that input with multiple segments per record in WELSEGS
    constexpr double inc_length = (2737.5 - 2537.5) / 4.;
    constexpr double inc_depth = (2537.5 - 2525.5) / 4.;
    for (int segment_number = 4; segment_number <= 7; ++segment_number) {
        const auto& segment = segments.getFromSegmentNumber(segment_number);
        BOOST_CHECK_CLOSE(segment.depth(), 2525.5 + (segment_number - 3) * inc_depth, 1.e-8);
        BOOST_CHECK_CLOSE(segment.totalLength(), 2537.5 + (segment_number - 3) * inc_length, 1.e-8);
    }
}

BOOST_AUTO_TEST_CASE(Node_XY_ABS_Range)
{
    const auto deck = ::Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
  20 20 20 /

GRID

DXV
  20*100 /

DYV
  20*100 /

DZV
  20*10 /

DEPTHZ
  441*2000.0 /

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSPECS
 'PROD01' 'P' 20 20 1* OIL /
/

COMPDAT
 'PROD01' 20 20 1 5 'OPEN' /
/

WELSEGS
'PROD01' 2512.5 2512.5 1.0e-5 'ABS' 'HF-' 'HO' 123.456 789.012 /
2         2      1      1    2537.5 2537.5  0.3  0.00010 2* 123.456 789.012 /
3         3      1      2    2562.5 2562.5  0.2  0.00010 2* 123.456 789.012 /
4         4      2      2    2587.5 2537.5  0.2  0.00010 2* 123.456 789.012 /
5         5      2      4    2637.5 2537.5  0.2  0.00010 2* 123.456 789.012 /
6         6      2      5    2687.5 2537.5  0.2  0.00010 2* 123.456 789.012 /
7         7      2      6    2737.5 2537.5  0.2  0.00010 2* 123.456 789.012 /
8         8      3      7    3337.6 2534.5  0.2  0.00015 2* 123.456 789.012 /
/

COMPSEGS
-- Name
  'PROD01' /
-- I    J     K   Branch
  20    20     1     1   2512.5   2525.0 /
  20    20     2     1   2525.0   2550.0 /
  20    20     3     1   2550.0   2575.0 /
  20    20     4     1   2637.5   2837.5 /
  20    20     5     1   2837.5   3037.5 /
/
)");

    const auto es    = ::Opm::EclipseState { deck };
    const auto sched = ::Opm::Schedule { deck, es, std::make_shared<const ::Opm::Python>() };

    const auto& segments = sched[0].wells("PROD01").getSegments();
    for (const auto& segment : segments) {
        BOOST_CHECK_CLOSE(segment.node_X(), 123.456, 1.0e-8);
        BOOST_CHECK_CLOSE(segment.node_Y(), 789.012, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Node_XY_INC_Individual)
{
    const auto deck = ::Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
  20 20 20 /

GRID

DXV
  20*100 /

DYV
  20*100 /

DZV
  20*10 /

DEPTHZ
  441*2000.0 /

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSPECS
 'PROD01' 'P' 20 20 1* OIL /
/

COMPDAT
 'PROD01' 20 20 1 5 'OPEN' /
/

WELSEGS
-- Name      Dep 1          Tlen 1      Vol 1     Len&Dep     PresDrop
   PROD01     2557.18408     0.00000     1*        INC         'HF-'    'HO' 12.3 45.6 /
-- First Seg     Last Seg     Branch Num     Outlet Seg     Length       Depth Change     Diam        Rough
-- Main Stem Segments
   2             2            1              1              5.09434      4.95609          0.15200     0.0000100 2* 10.1 20.2 /
   3             3            1              2              10.21718     9.93992          0.15200     0.0000100 2* 10.1 20.2 /
   4             4            1              3              10.24573     9.96769          0.15200     0.0000100 2* 10.1 20.2 /
   5             5            1              4              10.24574     9.96770          0.15200     0.0000100 2* 10.1 20.2 /
   6             6            1              5              6.40355      6.22978          0.15200     0.0000100 2* 10.1 20.2 /
   7             7            1              6              6.40355      6.22978          0.15200     0.0000100 2* 10.1 20.2 /
   8             8            1              7              10.24567     9.96764          0.15200     0.0000100 2* 10.1 20.2 /
   9             9            1              8              10.24571     9.96767          0.15200     0.0000100 2* 10.1 20.2 /
   10            10           1              9              10.24570     9.96767          0.15200     0.0000100 2* 10.1 20.2 /
   11            11           1              10             10.24571     9.96767          0.15200     0.0000100 2* 10.1 20.2 /
   12            12           1              11             5.97902      5.81677          0.15200     0.0000100 2* 10.1 20.2 /
/

COMPSEGS
-- Name
  'PROD01' /
-- I    J     K   Branch
  20    20     1     1   2512.5   2525.0 /
  20    20     2     1   2525.0   2550.0 /
  20    20     3     1   2550.0   2575.0 /
  20    20     4     1   2637.5   2837.5 /
  20    20     5     1   2837.5   3037.5 /
/
)");

    const auto es    = ::Opm::EclipseState { deck };
    const auto sched = ::Opm::Schedule { deck, es, std::make_shared<const ::Opm::Python>() };

    auto i = 0;
    const auto& segments = sched[0].wells("PROD01").getSegments();
    for (const auto& segment : segments) {
        BOOST_CHECK_CLOSE(segment.node_X(), 12.3 + i*10.1, 1.0e-8);
        BOOST_CHECK_CLOSE(segment.node_Y(), 45.6 + i*20.2, 1.0e-8);

        ++i;
    }
}

BOOST_AUTO_TEST_CASE(Node_XY_INC_Range)
{
    const auto deck = ::Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
  20 20 20 /

GRID

DXV
  20*100 /

DYV
  20*100 /

DZV
  20*10 /

DEPTHZ
  441*2000.0 /

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSPECS
 'PROD01' 'P' 20 20 1* OIL /
/

COMPDAT
 'PROD01' 20 20 1 5 'OPEN' /
/

WELSEGS
-- Name      Dep 1          Tlen 1      Vol 1     Len&Dep     PresDrop
   PROD01     2557.18408     0.00000     1*        INC         'HF-'    'HO' 12.3 45.6 /
-- First Seg     Last Seg     Branch Num     Outlet Seg     Length       Depth Change     Diam        Rough
-- Main Stem Segments
   2             12           1              1              5.09434      4.95609          0.15200     0.0000100 2* 10.1 20.2 /
/

COMPSEGS
-- Name
  'PROD01' /
-- I    J     K   Branch
  20    20     1     1   2512.5   2525.0 /
  20    20     2     1   2525.0   2550.0 /
  20    20     3     1   2550.0   2575.0 /
  20    20     4     1   2637.5   2837.5 /
  20    20     5     1   2837.5   3037.5 /
/
)");

    const auto es    = ::Opm::EclipseState { deck };
    const auto sched = ::Opm::Schedule { deck, es, std::make_shared<const ::Opm::Python>() };

    auto i = 0;
    const auto& segments = sched[0].wells("PROD01").getSegments();
    for (const auto& segment : segments) {
        BOOST_CHECK_CLOSE(segment.node_X(), 12.3 + i*10.1, 1.0e-8);
        BOOST_CHECK_CLOSE(segment.node_Y(), 45.6 + i*20.2, 1.0e-8);

        ++i;
    }
}

BOOST_AUTO_TEST_CASE(MissingCOMPSEGS)
{
    const auto deck = ::Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
  20 20 20 /

GRID

DXV
  20*100 /

DYV
  20*100 /

DZV
  20*10 /

DEPTHZ
  441*2000.0 /

PORO
    8000*0.1 /
PERMX
    8000*1 /
PERMY
    8000*0.1 /
PERMZ
    8000*0.01 /

SCHEDULE

WELSPECS
 'PROD01' 'P' 20 20 1* OIL /
/

COMPDAT
 'PROD01' 20 20 1 5 'OPEN' /
/

WELSEGS
-- Name      Dep 1          Tlen 1      Vol 1     Len&Dep     PresDrop
   PROD01     2557.18408     0.00000     1*        INC         'HF-'    'HO' 12.3 45.6 /
-- First Seg     Last Seg     Branch Num     Outlet Seg     Length       Depth Change     Diam        Rough
-- Main Stem Segments
   2             12           1              1              5.09434      4.95609          0.15200     0.0000100 2* 10.1 20.2 /
/
)");

    const auto es    = ::Opm::EclipseState { deck };
    BOOST_CHECK_THROW(::Opm::Schedule(deck, es, std::make_shared<const ::Opm::Python>()), ::Opm::OpmInputError);
}
