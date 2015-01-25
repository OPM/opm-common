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



#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

static Opm::TimeMapPtr createXDaysTimeMap(size_t numDays) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    for (size_t i = 0; i < numDays; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    return timeMap;
}



BOOST_AUTO_TEST_CASE(CreateGroup_CorrectNameAndDefaultValues) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);
    BOOST_CHECK_EQUAL( "G1" , group.name() );
}


BOOST_AUTO_TEST_CASE(CreateGroupCreateTimeOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 5);
    BOOST_CHECK_EQUAL( false, group.hasBeenDefined( 4 ));
    BOOST_CHECK_EQUAL( true, group.hasBeenDefined( 5 ));
    BOOST_CHECK_EQUAL( true, group.hasBeenDefined( 6 ));
}



BOOST_AUTO_TEST_CASE(CreateGroup_SetInjectorProducer_CorrectStatusSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group1("IGROUP" , timeMap , 0);
    Opm::Group group2("PGROUP" , timeMap , 0);

    group1.setProductionGroup(0, true);
    BOOST_CHECK(group1.isProductionGroup(1));
    BOOST_CHECK(!group1.isInjectionGroup(1));
    group1.setProductionGroup(3, false);
    BOOST_CHECK(!group1.isProductionGroup(3));
    BOOST_CHECK(group1.isInjectionGroup(3));

    group2.setProductionGroup(0, false);
    BOOST_CHECK(!group2.isProductionGroup(1));
    BOOST_CHECK(group2.isInjectionGroup(1));
    group2.setProductionGroup(3, true);
    BOOST_CHECK(group2.isProductionGroup(4));
    BOOST_CHECK(!group2.isInjectionGroup(4));
}


BOOST_AUTO_TEST_CASE(InjectRateOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);
    BOOST_CHECK_EQUAL( 0 , group.getInjectionRate( 0 ));
    group.setInjectionRate( 2 , 100 );
    BOOST_CHECK_EQUAL( 100 , group.getInjectionRate( 2 ));
    BOOST_CHECK_EQUAL( 100 , group.getInjectionRate( 8 ));
}


BOOST_AUTO_TEST_CASE(ControlModeOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);
    BOOST_CHECK_EQUAL( Opm::GroupInjection::NONE , group.getInjectionControlMode( 0 ));
    group.setInjectionControlMode( 2 , Opm::GroupInjection::RESV );
    BOOST_CHECK_EQUAL( Opm::GroupInjection::RESV , group.getInjectionControlMode( 2 ));
    BOOST_CHECK_EQUAL( Opm::GroupInjection::RESV , group.getInjectionControlMode( 8 ));
}



BOOST_AUTO_TEST_CASE(GroupChangePhaseSameTimeThrows) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);
    BOOST_CHECK_EQUAL( Opm::Phase::WATER , group.getInjectionPhase( 0 )); // Default phase - assumed WATER
    group.setInjectionPhase(5 , Opm::Phase::WATER );
    BOOST_CHECK_THROW( group.setInjectionPhase( 5 , Opm::Phase::GAS ) , std::invalid_argument );
    BOOST_CHECK_NO_THROW( group.setInjectionPhase( 5 , Opm::Phase::WATER ));
    BOOST_CHECK_NO_THROW( group.setInjectionPhase( 6 , Opm::Phase::GAS ));
    BOOST_CHECK_EQUAL( Opm::Phase::GAS , group.getInjectionPhase( 6 ));
    BOOST_CHECK_EQUAL( Opm::Phase::GAS , group.getInjectionPhase( 8 ));
}




BOOST_AUTO_TEST_CASE(GroupMiscInjection) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);

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
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);

    BOOST_CHECK_EQUAL(false , group.hasWell("NO", 2));
    BOOST_CHECK_EQUAL(0U , group.numWells(2));
    BOOST_CHECK_THROW(group.getWell("NO" , 2) , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(GroupAddWell) {

    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);
    Opm::WellPtr well1(new Opm::Well("WELL1" , 0, 0, Opm::Value<double>("REF_DEPTH"), Opm::Phase::OIL, timeMap, 0));
    Opm::WellPtr well2(new Opm::Well("WELL2" , 0, 0, Opm::Value<double>("REF_DEPTH"), Opm::Phase::OIL, timeMap, 0));

    BOOST_CHECK_EQUAL(0U , group.numWells(2));
    group.addWell( 3 , well1 );
    BOOST_CHECK_EQUAL( 1U , group.numWells(3));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));

    group.addWell( 4 , well1 );
    BOOST_CHECK_EQUAL( 1U , group.numWells(4));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));
    BOOST_CHECK_EQUAL( 1U , group.numWells(5));

    group.addWell( 6 , well2 );
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

    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Group group("G1" , timeMap , 0);
    Opm::WellPtr well1(new Opm::Well("WELL1" , 0, 0, Opm::Value<double>("REF_DEPTH"), Opm::Phase::OIL, timeMap, 0));
    Opm::WellPtr well2(new Opm::Well("WELL2" , 0, 0, Opm::Value<double>("REF_DEPTH"), Opm::Phase::OIL, timeMap, 0));

    BOOST_CHECK_EQUAL(0U , group.numWells(2));
    group.addWell( 3 , well1 );
    BOOST_CHECK_EQUAL( 1U , group.numWells(3));
    BOOST_CHECK_EQUAL( 0U , group.numWells(1));

    group.addWell( 6 , well2 );
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

    BOOST_CHECK_THROW( group.delWell( 8 , "WeLLDOESNOT" ) , std::invalid_argument);
    BOOST_CHECK_THROW( group.delWell( 8 , "WELL1" ) , std::invalid_argument);
}
