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
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE CompletionSetTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>


#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/MSW/updatingCompletionsWithSegments.hpp>

BOOST_AUTO_TEST_CASE(MultisegmentWellTest) {
    Opm::CompletionSet completion_set;
    completion_set.add(Opm::Completion( 19, 0, 0, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.5), Opm::Value<double>("SKIN", 0.), 0) );
    completion_set.add(Opm::Completion( 19, 0, 1, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.5), Opm::Value<double>("SKIN", 0.), 0) );
    completion_set.add(Opm::Completion( 19, 0, 2, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.4), Opm::Value<double>("SKIN", 0.), 0) );
    completion_set.add(Opm::Completion( 18, 0, 1, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.4), Opm::Value<double>("SKIN", 0.), 0,  Opm::WellCompletion::DirectionEnum::X) );
    completion_set.add(Opm::Completion( 17, 0, 1, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.4), Opm::Value<double>("SKIN", 0.), 0,  Opm::WellCompletion::DirectionEnum::X) );
    completion_set.add(Opm::Completion( 16, 0, 1, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.4), Opm::Value<double>("SKIN", 0.), 0,  Opm::WellCompletion::DirectionEnum::X) );
    completion_set.add(Opm::Completion( 15, 0, 1, 1, 0.0, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor", 200.), Opm::Value<double>("D", 0.4), Opm::Value<double>("SKIN", 0.), 0,  Opm::WellCompletion::DirectionEnum::X) );

    BOOST_CHECK_EQUAL( 7U , completion_set.size() );

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
        "19    1     2     2   2637.5   2837.5 /\n"
        "18    1     2     2   2837.5   3037.5 /\n"
        "17    1     2     2   3037.5   3237.5 /\n"
        "16    1     2     2   3237.5   3437.5 /\n"
        "/\n";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(compsegs_string, Opm::ParseContext());

    const Opm::DeckKeyword compsegs = deck.getKeyword("COMPSEGS");
    BOOST_CHECK_EQUAL( 8U, compsegs.size() );

    Opm::SegmentSet segment_set;
    const Opm::DeckKeyword welsegs = deck.getKeyword("WELSEGS");
    segment_set.segmentsFromWELSEGSKeyword(welsegs);

    BOOST_CHECK_EQUAL(6U, segment_set.numberSegment());

    const Opm::CompletionSet new_completion_set = Opm::updatingCompletionsWithSegments(compsegs, completion_set, segment_set);

    BOOST_CHECK_EQUAL(7U, new_completion_set.size());

    const Opm::Completion& completion1 = new_completion_set.get(0);
    const int segment_number_completion1 = completion1.getSegmentNumber();
    const double center_depth_completion1 = completion1.getCenterDepth();
    BOOST_CHECK_EQUAL(segment_number_completion1, 1);
    BOOST_CHECK_EQUAL(center_depth_completion1, 2512.5);

    const Opm::Completion& completion3 = new_completion_set.get(2);
    const int segment_number_completion3 = completion3.getSegmentNumber();
    const double center_depth_completion3 = completion3.getCenterDepth();
    BOOST_CHECK_EQUAL(segment_number_completion3, 3);
    BOOST_CHECK_EQUAL(center_depth_completion3, 2562.5);

    const Opm::Completion& completion5 = new_completion_set.get(4);
    const int segment_number_completion5 = completion5.getSegmentNumber();
    const double center_depth_completion5 = completion5.getCenterDepth();
    BOOST_CHECK_EQUAL(segment_number_completion5, 6);
    BOOST_CHECK_CLOSE(center_depth_completion5, 2538.83, 0.001);

    const Opm::Completion& completion6 = new_completion_set.get(5);
    const int segment_number_completion6 = completion6.getSegmentNumber();
    const double center_depth_completion6 = completion6.getCenterDepth();
    BOOST_CHECK_EQUAL(segment_number_completion6, 6);
    BOOST_CHECK_CLOSE(center_depth_completion6,  2537.83, 0.001);

    const Opm::Completion& completion7 = new_completion_set.get(6);
    const int segment_number_completion7 = completion7.getSegmentNumber();
    const double center_depth_completion7 = completion7.getCenterDepth();
    BOOST_CHECK_EQUAL(segment_number_completion7, 7);
    BOOST_CHECK_EQUAL(center_depth_completion7, 2534.5);
}

