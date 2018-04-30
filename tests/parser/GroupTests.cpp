/*
  Copyright 2013 Statoil ASA.

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

#define BOOST_TEST_MODULE GroupTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

using namespace Opm;

static TimeMap createXDaysTimeMap(size_t numDays) {
    const std::time_t startDate = Opm::TimeMap::mkdate(2010, 1, 1);
    Opm::TimeMap timeMap{ startDate };
    for (size_t i = 0; i < numDays; i++)
        timeMap.addTStep((i+1) * 24 * 60 * 60);
    return timeMap;
}



BOOST_AUTO_TEST_CASE(CreateGroup_CorrectNameAndDefaultValues) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 0);
    BOOST_CHECK_EQUAL( "G1" , group.name() );
}


BOOST_AUTO_TEST_CASE(CreateGroupCreateTimeOK) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 5);
    BOOST_CHECK_EQUAL( false, group.hasBeenDefined( 4 ));
    BOOST_CHECK_EQUAL( true, group.hasBeenDefined( 5 ));
    BOOST_CHECK_EQUAL( true, group.hasBeenDefined( 6 ));
}



BOOST_AUTO_TEST_CASE(CreateGroup_SetInjectorProducer_CorrectStatusSet) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group1("IGROUP" , 1, timeMap , 0);
    Opm::Group group2("PGROUP" , 2, timeMap , 0);

    group1.setProductionGroup(0, true);
    BOOST_CHECK(group1.isProductionGroup(1));
    BOOST_CHECK(!group1.isInjectionGroup(1));
    group1.setProductionGroup(3, false);
    BOOST_CHECK(!group1.isProductionGroup(3));
    BOOST_CHECK(!group1.isInjectionGroup(3));

    group2.setProductionGroup(0, false);
    BOOST_CHECK(!group2.isProductionGroup(1));
    BOOST_CHECK(!group2.isInjectionGroup(1));
    group2.setProductionGroup(3, true);
    BOOST_CHECK(group2.isProductionGroup(4));
    BOOST_CHECK(!group2.isInjectionGroup(4));
    group2.setInjectionGroup(4, true);
    BOOST_CHECK(group2.isProductionGroup(5));
    BOOST_CHECK(group2.isInjectionGroup(5));

}


BOOST_AUTO_TEST_CASE(InjectRateOK) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 0);
    BOOST_CHECK_EQUAL( 0 , group.getInjectionRate( 0 ));
    group.setInjectionRate( 2 , 100 );
    BOOST_CHECK_EQUAL( 100 , group.getInjectionRate( 2 ));
    BOOST_CHECK_EQUAL( 100 , group.getInjectionRate( 8 ));
}


BOOST_AUTO_TEST_CASE(ControlModeOK) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 0);
    BOOST_CHECK_EQUAL( Opm::GroupInjection::NONE , group.getInjectionControlMode( 0 ));
    group.setInjectionControlMode( 2 , Opm::GroupInjection::RESV );
    BOOST_CHECK_EQUAL( Opm::GroupInjection::RESV , group.getInjectionControlMode( 2 ));
    BOOST_CHECK_EQUAL( Opm::GroupInjection::RESV , group.getInjectionControlMode( 8 ));
}



BOOST_AUTO_TEST_CASE(GroupChangePhaseSameTimeThrows) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 0);
    BOOST_CHECK_EQUAL( Opm::Phase::WATER , group.getInjectionPhase( 0 )); // Default phase - assumed WATER
    group.setInjectionPhase( 5, Opm::Phase::WATER );
    group.setInjectionPhase( 5, Opm::Phase::WATER );
    group.setInjectionPhase( 6, Opm::Phase::GAS );
    BOOST_CHECK_EQUAL( Opm::Phase::GAS , group.getInjectionPhase( 6 ));
    BOOST_CHECK_EQUAL( Opm::Phase::GAS , group.getInjectionPhase( 8 ));
}




BOOST_AUTO_TEST_CASE(GroupMiscInjection) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 0);

    group.setSurfaceMaxRate( 3 , 100 );
    BOOST_CHECK_EQUAL( 100 , group.getSurfaceMaxRate( 5 ));

    group.setReservoirMaxRate( 3 , 200 );
    BOOST_CHECK_EQUAL( 200 , group.getReservoirMaxRate( 5 ));

    group.setTargetReinjectFraction( 3 , 300 );
    BOOST_CHECK_EQUAL( 300 , group.getTargetReinjectFraction( 5 ));

    group.setTargetVoidReplacementFraction( 3 , 400 );
    BOOST_CHECK_EQUAL( 400 , group.getTargetVoidReplacementFraction( 5 ));
}



BOOST_AUTO_TEST_CASE(GroupDoesNotHaveWell) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , 1, timeMap , 0);

    BOOST_CHECK_EQUAL(false , group.hasWell("NO", 2));
    BOOST_CHECK_EQUAL(0U , group.numWells(2));
}


BOOST_AUTO_TEST_CASE(GroupAddWell) {

    auto timeMap = createXDaysTimeMap( 10 );
    Opm::Group group("G1" , 1, timeMap , 0);
    auto well1 = std::make_shared< Well >("WELL1", 1, 0, 0, 0.0, Opm::Phase::OIL, timeMap, 0);
    auto well2 = std::make_shared< Well >("WELL2", 2, 0, 0, 0.0, Opm::Phase::OIL, timeMap, 0);

    BOOST_CHECK_EQUAL(0U , group.numWells(2));
    group.addWell( 3 , well1.get() );
    BOOST_CHECK_EQUAL( 1U , group.numWells(3));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));

    group.addWell( 4 , well1.get() );
    BOOST_CHECK_EQUAL( 1U , group.numWells(4));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));
    BOOST_CHECK_EQUAL( 1U , group.numWells(5));

    group.addWell( 6 , well2.get() );
    BOOST_CHECK_EQUAL( 1U , group.numWells(4));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));
    BOOST_CHECK_EQUAL( 1U , group.numWells(5));
    BOOST_CHECK_EQUAL( 2U , group.numWells(6));
    BOOST_CHECK_EQUAL( 2U , group.numWells(8));

    BOOST_CHECK(group.hasWell("WELL1" , 8 ));
    BOOST_CHECK(group.hasWell("WELL2" , 8 ));

    BOOST_CHECK_EQUAL(false , group.hasWell("WELL1" , 0 ));
    BOOST_CHECK_EQUAL(false , group.hasWell("WELL2" , 0 ));

    BOOST_CHECK_EQUAL(true  , group.hasWell("WELL1" , 5 ));
    BOOST_CHECK_EQUAL(false , group.hasWell("WELL2" , 5 ));

}



BOOST_AUTO_TEST_CASE(GroupAddAndDelWell) {

    auto timeMap = createXDaysTimeMap( 10 );
    Opm::Group group("G1" , 1, timeMap , 0);
    auto well1 = std::make_shared< Well >("WELL1", 1, 0, 0, 0.0, Opm::Phase::OIL, timeMap, 0);
    auto well2 = std::make_shared< Well >("WELL2", 2, 0, 0, 0.0, Opm::Phase::OIL, timeMap, 0);

    BOOST_CHECK_EQUAL(0U , group.numWells(2));
    group.addWell( 3 , well1.get() );
    BOOST_CHECK_EQUAL( 1U , group.numWells(3));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));

    group.addWell( 6 , well2.get() );
    BOOST_CHECK_EQUAL( 1U , group.numWells(4));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));
    BOOST_CHECK_EQUAL( 1U , group.numWells(5));
    BOOST_CHECK_EQUAL( 2U , group.numWells(6));
    BOOST_CHECK_EQUAL( 2U , group.numWells(8));

    group.delWell( 7 , "WELL1");
    BOOST_CHECK_EQUAL(false , group.hasWell("WELL1" , 7));
    BOOST_CHECK_EQUAL(true , group.hasWell("WELL2" , 7));
    BOOST_CHECK_EQUAL( 1U , group.numWells(7));
    BOOST_CHECK_EQUAL( 2U , group.numWells(6));


    group.delWell( 8 , "WELL2");
    BOOST_CHECK_EQUAL(false , group.hasWell("WELL1" , 8));
    BOOST_CHECK_EQUAL(false , group.hasWell("WELL2" , 8));
    BOOST_CHECK_EQUAL( 0U , group.numWells(8));
    BOOST_CHECK_EQUAL( 1U , group.numWells(7));
    BOOST_CHECK_EQUAL( 2U , group.numWells(6));
}

BOOST_AUTO_TEST_CASE(getWells) {
    auto timeMap = createXDaysTimeMap( 10 );
    Opm::Group group("G1" , 1, timeMap , 0);
    auto well1 = std::make_shared< Well >("WELL1", 1, 0, 0, 0.0, Opm::Phase::OIL, timeMap, 0);
    auto well2 = std::make_shared< Well >("WELL2", 2, 0, 0, 0.0, Opm::Phase::OIL, timeMap, 0);

    group.addWell( 2 , well1.get() );
    group.addWell( 3 , well1.get() );
    group.addWell( 3 , well2.get() );
    group.addWell( 4 , well1.get() );

    std::vector< std::string > names = { "WELL1", "WELL2" };
    std::vector< std::string > wnames;
    for( const auto& wname : group.getWells( 3 ) )
        wnames.push_back( wname );

    BOOST_CHECK_EQUAL_COLLECTIONS( names.begin(), names.end(),
                                   wnames.begin(), wnames.end() );

    const auto& wells = group.getWells( 3 );
    BOOST_CHECK_EQUAL( wells.size(), 2U );
    BOOST_CHECK( wells.count( "WELL1" ) > 0 );
    BOOST_CHECK( wells.count( "WELL2" ) > 0 );

    BOOST_CHECK_EQUAL( group.getWells( 0 ).size(), 0U );
    BOOST_CHECK_EQUAL( group.getWells( 2 ).size(), 1U );
    BOOST_CHECK( group.getWells( 2 ).count( "WELL1" ) > 0 );
    BOOST_CHECK( group.getWells( 2 ).count( "WELL2" ) == 0 );
    BOOST_CHECK_EQUAL( group.getWells( 4 ).size(), 2U );
    BOOST_CHECK( group.getWells( 4 ).count( "WELL1" ) > 0 );
    BOOST_CHECK( group.getWells( 4 ).count( "WELL2" ) > 0 );

}

BOOST_AUTO_TEST_CASE(createDeckWithGEFAC) {
    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"

	    "WELSPECS\n"
     	     " 'B-37T2' 'PRODUC'  9  9   1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
	     " 'B-43A'  'PRODUC'  8  8   1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
	     "/\n"

	     "COMPDAT\n"
	     " 'B-37T2'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
             " 'B-43A'   8  8   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
	     "/\n"

            "GEFAC\n"
            " 'PRODUC' 0.85   / \n"
            "/\n";

    Opm::ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Opm::Schedule schedule(deck,  grid, eclipseProperties, Opm::Phases(true, true, true) , parseContext);

    const auto& group1 = schedule.getGroup("PRODUC");
    BOOST_CHECK_EQUAL(group1.getGroupEfficiencyFactor(0), 0.85);
    BOOST_CHECK(group1.getTransferGroupEfficiencyFactor(0));
}



BOOST_AUTO_TEST_CASE(createDeckWithWGRUPCONandWCONPROD) {

    /* Test deck with well guide rates for group control:
       GRUPCON (well guide rates for group control)
       WCONPROD (conrol data for production wells) with GRUP control mode */

    Opm::Parser parser;
    std::string input =
            "START             -- 0 \n"
            "19 JUN 2007 / \n"
            "SCHEDULE\n"

	    "WELSPECS\n"
     	     " 'B-37T2' 'PRODUC'  9  9   1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
	     " 'B-43A'  'PRODUC'  8  8   1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
	     "/\n"

	     "COMPDAT\n"
	     " 'B-37T2'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
             " 'B-43A'   8  8   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
	     "/\n"


             "WGRUPCON\n"
             " 'B-37T2'  YES 30 OIL / \n"
             " 'B-43A'   YES 30 OIL / \n"
             "/\n"

             "WCONPROD\n"
             " 'B-37T2'    'OPEN'     'GRUP'  1000  2*   2000.000  2* 1*   10 200000.000  5* /  / \n"
             " 'B-43A'     'OPEN'     'GRUP'  1200  2*   3000.000  2* 1*   11  0.000      5* /  / \n"
             "/\n";



    Opm::ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Opm::Schedule schedule(deck,  grid, eclipseProperties, Opm::Phases(true, true, true) , parseContext);
    const auto* currentWell = schedule.getWell("B-37T2");
    const Opm::WellProductionProperties& wellProductionProperties = currentWell->getProductionProperties(0);
    BOOST_CHECK_EQUAL(wellProductionProperties.controlMode, Opm::WellProducer::ControlModeEnum::GRUP);

    BOOST_CHECK_EQUAL(currentWell->isAvailableForGroupControl(0), true);
    BOOST_CHECK_EQUAL(currentWell->getGuideRate(0), 30);
    BOOST_CHECK_EQUAL(currentWell->getGuideRatePhase(0), Opm::GuideRate::OIL);
    BOOST_CHECK_EQUAL(currentWell->getGuideRateScalingFactor(0), 1.0);


}

BOOST_AUTO_TEST_CASE(CreateGroupTree_DefaultConstructor_HasFieldNode) {
    GroupTree tree;
    BOOST_CHECK(tree.exists("FIELD"));
}

BOOST_AUTO_TEST_CASE(GetNode_NonExistingNode_ReturnsNull) {
    GroupTree tree;
    BOOST_CHECK(!tree.exists("Non-existing"));
}

BOOST_AUTO_TEST_CASE(GetNodeAndParent_AllOK) {
    GroupTree tree;
    tree.update("GRANDPARENT", "FIELD");
    tree.update("PARENT", "GRANDPARENT");
    tree.update("GRANDCHILD", "PARENT");

    const auto grandchild = tree.exists("GRANDCHILD");
    BOOST_CHECK(grandchild);
    auto parent = tree.parent("GRANDCHILD");
    BOOST_CHECK_EQUAL("PARENT", parent);
    BOOST_CHECK( tree.children( "PARENT" ).front() == "GRANDCHILD" );
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentNotSpecified_AddedUnderField) {
    GroupTree tree;
    tree.update("CHILD_OF_FIELD");
    BOOST_CHECK(tree.exists("CHILD_OF_FIELD"));
    BOOST_CHECK_EQUAL( "FIELD", tree.parent( "CHILD_OF_FIELD" ) );
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentIsField_AddedUnderField) {
    GroupTree tree;
    tree.update("CHILD_OF_FIELD", "FIELD");
    BOOST_CHECK( tree.exists( "CHILD_OF_FIELD" ) );
    BOOST_CHECK_EQUAL( "FIELD", tree.parent( "CHILD_OF_FIELD" ) );
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentNotAdded_ChildAndParentAdded) {
    GroupTree tree;
    tree.update("CHILD", "NEWPARENT");
    BOOST_CHECK( tree.exists("CHILD") );
    BOOST_CHECK_EQUAL( "NEWPARENT", tree.parent( "CHILD" ) );
    BOOST_CHECK_EQUAL( "CHILD", tree.children( "NEWPARENT" ).front() );
}

BOOST_AUTO_TEST_CASE(UpdateTree_AddFieldNode_Throws) {
    GroupTree tree;
    BOOST_CHECK_THROW(tree.update("FIELD", "NEWPARENT"), std::invalid_argument );
    BOOST_CHECK_THROW(tree.update("FIELD"), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(createDeckWithGRUPNET) {
        Opm::Parser parser;
        std::string input =
        "START             -- 0 \n"
        "31 AUG 1993 / \n"
        "SCHEDULE\n"

        "GRUPNET \n"
        " 'FIELD'     20.000  5* / \n"
        " 'PROD'     20.000  5* / \n"
        " 'MANI-B2'  1*    8  1*        'NO'  2* / \n"
        " 'MANI-B1'  1*    8  1*        'NO'  2* / \n"
        " 'MANI-K1'  1* 9999  4* / \n"
        " 'B1-DUMMY'  1* 9999  4* / \n"
        " 'MANI-D1'  1*    8  1*        'NO'  2* / \n"
        " 'MANI-D2'  1*    8  1*        'NO'  2* / \n"
        " 'MANI-K2'  1* 9999  4* / \n"
        " 'D2-DUMMY'  1* 9999  4* / \n"
        " 'MANI-E1'  1*    9  1*        'NO'  2* / \n"
        " 'MANI-E2'  1*    9  4* / \n"
        "/\n";

        Opm::ParseContext parseContext;
        auto deck = parser.parseString(input, parseContext);
        EclipseGrid grid(10,10,10);
        TableManager table ( deck );
        Eclipse3DProperties eclipseProperties ( deck , table, grid);
        Opm::Schedule schedule(deck,  grid, eclipseProperties, Opm::Phases(true, true, true) , parseContext);

        const auto& group1 = schedule.getGroup("PROD");
        const auto& group2 = schedule.getGroup("MANI-E2");
        const auto& group3 = schedule.getGroup("MANI-K1");
        BOOST_CHECK_EQUAL(group1.getGroupNetVFPTable(0), 0);
        BOOST_CHECK_EQUAL(group2.getGroupNetVFPTable(0), 9);
        BOOST_CHECK_EQUAL(group3.getGroupNetVFPTable(0), 9999);
}
