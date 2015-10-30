/*
  Copyright (C) 2015 SINTEF ICT, Applied Mathematics

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

#define BOOST_TEST_MODULE ParseMULTSEGWELL
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE( PARSE_MULTISEGMENT_ABS ) {

    ParserPtr parser(new Parser());
    boost::filesystem::path deckFile("testdata/integration_tests/SCHEDULE/SCHEDULE_MULTISEGMENT_WELL");
    DeckPtr deck =  parser->parseFile(deckFile.string(), ParseMode());
    DeckKeywordConstPtr kw = deck->getKeyword("WELSEGS");

    // check the size of the keywords
    BOOST_CHECK_EQUAL( 6, kw->size() );

    DeckRecordConstPtr rec1 = kw->getRecord(0); // top segment

    const std::string well_name = rec1->getItem("WELL")->getTrimmedString(0);
    const double depth_top = rec1->getItem("DEPTH")->getRawDouble(0);
    const double length_top = rec1->getItem("LENGTH")->getRawDouble(0);
    const double volume_top = rec1->getItem("WELLBORE_VOLUME")->getRawDouble(0);
    const WellSegment::LengthDepthEnum length_depth_type = WellSegment::LengthDepthEnumFromString(rec1->getItem("INFO_TYPE")->getTrimmedString(0));
    const WellSegment::CompPresureDropEnum comp_pressure_drop = WellSegment::CompPressureDropEnumFromString(rec1->getItem("PRESSURE_COMPONENTS")->getTrimmedString(0));
    const WellSegment::MultiPhaseModelEnum multiphase_model = WellSegment::MultiPhaseModelEnumFromString(rec1->getItem("FLOW_MODEL")->getTrimmedString(0));

    BOOST_CHECK_EQUAL( "PROD01", well_name );
    BOOST_CHECK_EQUAL( 2512.5, depth_top );
    BOOST_CHECK_EQUAL( 2512.5, length_top );
    BOOST_CHECK_EQUAL( 1.0e-5, volume_top );
    const std::string length_depth_type_string = WellSegment::LengthDepthEnumToString(length_depth_type);
    BOOST_CHECK_EQUAL( length_depth_type_string, "ABS" );
    const std::string comp_pressure_drop_string = WellSegment::CompPresureDropEnumToString(comp_pressure_drop);
    BOOST_CHECK_EQUAL( comp_pressure_drop_string, "H--" );
    const std::string multiphase_model_string = WellSegment::MultiPhaseModelEnumToString(multiphase_model);
    BOOST_CHECK_EQUAL( multiphase_model_string, "HO" );

    // check the information for the other segments
    // Here, we check the information for the segment 2 and 6 as samples.
    {
        DeckRecordConstPtr rec2 = kw->getRecord(1);
        const int segment1 = rec2->getItem("SEGMENT2")->getInt(0);
        const int segment2 = rec2->getItem("SEGMENT2")->getInt(0);
        BOOST_CHECK_EQUAL( 2, segment1 );
        BOOST_CHECK_EQUAL( 2, segment2 );
        const int branch = rec2->getItem("BRANCH")->getInt(0);
        const int outlet_segment = rec2->getItem("JOIN_SEGMENT")->getInt(0);
        const double segment_length = rec2->getItem("SEGMENT_LENGTH")->getSIDouble(0);
        const double depth_change = rec2->getItem("DEPTH_CHANGE")->getSIDouble(0);
        const double diameter = rec2->getItem("DIAMETER")->getSIDouble(0);
        const double roughness = rec2->getItem("ROUGHNESS")->getSIDouble(0);
        BOOST_CHECK_EQUAL( 1, branch );
        BOOST_CHECK_EQUAL( 1, outlet_segment );
        BOOST_CHECK_EQUAL( 2537.5, segment_length );
        BOOST_CHECK_EQUAL( 2537.5, depth_change );
        BOOST_CHECK_EQUAL( 0.3, diameter );
        BOOST_CHECK_EQUAL( 0.0001, roughness );
    }

    {
        DeckRecordConstPtr rec6 = kw->getRecord(5);
        const int segment1 = rec6->getItem("SEGMENT2")->getInt(0);
        const int segment2 = rec6->getItem("SEGMENT2")->getInt(0);
        BOOST_CHECK_EQUAL( 6, segment1 );
        BOOST_CHECK_EQUAL( 6, segment2 );
        const int branch = rec6->getItem("BRANCH")->getInt(0);
        const int outlet_segment = rec6->getItem("JOIN_SEGMENT")->getInt(0);
        const double segment_length = rec6->getItem("SEGMENT_LENGTH")->getSIDouble(0);
        const double depth_change = rec6->getItem("DEPTH_CHANGE")->getSIDouble(0);
        const double diameter = rec6->getItem("DIAMETER")->getSIDouble(0);
        const double roughness = rec6->getItem("ROUGHNESS")->getSIDouble(0);
        BOOST_CHECK_EQUAL( 2, branch );
        BOOST_CHECK_EQUAL( 5, outlet_segment );
        BOOST_CHECK_EQUAL( 3137.5, segment_length );
        BOOST_CHECK_EQUAL( 2537.5, depth_change );
        BOOST_CHECK_EQUAL( 0.2, diameter );
        BOOST_CHECK_EQUAL( 0.0001, roughness );
    }

}
