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

#define BOOST_TEST_MODULE WellTest
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Units/ConversionFactors.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

Opm::TimeMapPtr createXDaysTimeMap(size_t numDays) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(startDate));
    for (size_t i = 0; i < numDays; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    return timeMap;
}

BOOST_AUTO_TEST_CASE(CreateWell_CorrectNameAndDefaultValues) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    BOOST_CHECK_EQUAL( "WELL1" , well.name() );
    BOOST_CHECK_EQUAL(0.0 , well.getOilRate( 5 ));
}

BOOST_AUTO_TEST_CASE(CreateWellCreateTimeStepOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 5);
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(0) );
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(4) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(5) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(7) );
    
}


BOOST_AUTO_TEST_CASE(setOilRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getOilRate( 5 ));
    well.setOilRate( 5 , 99 );
    BOOST_CHECK_EQUAL(99 , well.getOilRate( 5 ));
    BOOST_CHECK_EQUAL(99 , well.getOilRate( 8 ));
}

BOOST_AUTO_TEST_CASE(setPredictionMode_ModeSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL( true, well.isInPredictionMode( 5 ));
    well.setInPredictionMode( 5 , false ); // Go to history mode
    BOOST_CHECK_EQUAL(false , well.isInPredictionMode( 5 ));
    BOOST_CHECK_EQUAL(false , well.isInPredictionMode( 8 ));
}



BOOST_AUTO_TEST_CASE(NewWellZeroCompletions) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    Opm::CompletionSetConstPtr completions = well.getCompletions( 0 );
    BOOST_CHECK_EQUAL( 0U , completions->size());
}


BOOST_AUTO_TEST_CASE(UpdateCompletions) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    Opm::CompletionSetConstPtr completions = well.getCompletions( 0 );
    BOOST_CHECK_EQUAL( 0U , completions->size());
    
    std::vector<Opm::CompletionConstPtr> newCompletions;
    std::vector<Opm::CompletionConstPtr> newCompletions2;
    Opm::CompletionConstPtr comp1(new Opm::Completion( 10 , 10 , 10 , Opm::AUTO , 99.0, 22.3, 33.2));
    Opm::CompletionConstPtr comp2(new Opm::Completion( 10 , 11 , 10 , Opm::SHUT , 99.0, 22.3, 33.2));
    Opm::CompletionConstPtr comp3(new Opm::Completion( 10 , 10 , 12 , Opm::OPEN , 99.0, 22.3, 33.2));
    Opm::CompletionConstPtr comp4(new Opm::Completion( 10 , 10 , 12 , Opm::SHUT , 99.0, 22.3, 33.2));
    Opm::CompletionConstPtr comp5(new Opm::Completion( 10 , 10 , 13 , Opm::OPEN , 99.0, 22.3, 33.2));

    //std::vector<Opm::CompletionConstPtr> newCompletions2{ comp4 , comp5}; Newer c++

    newCompletions.push_back( comp1 );
    newCompletions.push_back( comp2 );
    newCompletions.push_back( comp3 );

    newCompletions2.push_back( comp4 );
    newCompletions2.push_back( comp5 );

    BOOST_CHECK_EQUAL( 3U , newCompletions.size());
    well.addCompletions( 5 , newCompletions );
    completions = well.getCompletions( 5 );
    BOOST_CHECK_EQUAL( 3U , completions->size());
    BOOST_CHECK_EQUAL( comp3 , completions->get(2));

    well.addCompletions( 6 , newCompletions2 );

    completions = well.getCompletions( 6 );
    BOOST_CHECK_EQUAL( 4U , completions->size());
    BOOST_CHECK_EQUAL( comp4 , completions->get(2));
    
}


BOOST_AUTO_TEST_CASE(setGasRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getGasRate( 5 ));
    well.setGasRate( 5 , 108 );
    BOOST_CHECK_EQUAL(108 , well.getGasRate( 5 ));
    BOOST_CHECK_EQUAL(108 , well.getGasRate( 8 ));
}



BOOST_AUTO_TEST_CASE(setWaterRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getWaterRate( 5 ));
    well.setWaterRate( 5 , 108 );
    BOOST_CHECK_EQUAL(108 , well.getWaterRate( 5 ));
    BOOST_CHECK_EQUAL(108 , well.getWaterRate( 8 ));
}


BOOST_AUTO_TEST_CASE(setSurfaceInjectionRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getSurfaceInjectionRate( 5 ));
    well.setSurfaceInjectionRate( 5 , 108 );
    BOOST_CHECK_EQUAL(108 , well.getSurfaceInjectionRate( 5 ));
    BOOST_CHECK_EQUAL(108 , well.getSurfaceInjectionRate( 8 ));
}


BOOST_AUTO_TEST_CASE(setReservoirInjectionRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getReservoirInjectionRate( 5 ));
    well.setReservoirInjectionRate( 5 , 108 );
    BOOST_CHECK_EQUAL(108 , well.getReservoirInjectionRate( 5 ));
    BOOST_CHECK_EQUAL(108 , well.getReservoirInjectionRate( 8 ));
}


BOOST_AUTO_TEST_CASE(isProducerCorrectlySet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap ,0);

    /* 1: Well is created as producer */
    BOOST_CHECK_EQUAL( false , well.isInjector(0));
    BOOST_CHECK_EQUAL( true , well.isProducer(0));

    /* Set a surface injection rate => Well becomes an Injector */
    well.setSurfaceInjectionRate(3 , 100);
    BOOST_CHECK_EQUAL( true  , well.isInjector(3));
    BOOST_CHECK_EQUAL( false , well.isProducer(3));
    BOOST_CHECK_EQUAL( 100 , well.getSurfaceInjectionRate(3));
    
    /* Set a reservoir injection rate => Well becomes an Injector */
    well.setReservoirInjectionRate(4 , 200);
    BOOST_CHECK_EQUAL( true  , well.isInjector(4));
    BOOST_CHECK_EQUAL( false , well.isProducer(4));
    BOOST_CHECK_EQUAL( 200 , well.getReservoirInjectionRate(4));
    

    /* Set rates => Well becomes a producer; injection rate should be set to 0. */
    well.setOilRate(4 , 100 );
    well.setGasRate(4 , 200 );  
    well.setWaterRate(4 , 300 );

    BOOST_CHECK_EQUAL( false , well.isInjector(4));
    BOOST_CHECK_EQUAL( true , well.isProducer(4));
    BOOST_CHECK_EQUAL( 0 , well.getSurfaceInjectionRate(4));
    BOOST_CHECK_EQUAL( 0 , well.getReservoirInjectionRate(4));
    BOOST_CHECK_EQUAL( 100 , well.getOilRate(4));
    BOOST_CHECK_EQUAL( 200 , well.getGasRate(4));
    BOOST_CHECK_EQUAL( 300 , well.getWaterRate(4));
    
    /* Set injection rate => Well becomes injector - all produced rates -> 0 */
    well.setReservoirInjectionRate( 6 , 50 );
    BOOST_CHECK_EQUAL( true  , well.isInjector(6));
    BOOST_CHECK_EQUAL( false , well.isProducer(6));
    BOOST_CHECK_EQUAL( 50 , well.getReservoirInjectionRate(6));
    BOOST_CHECK_EQUAL( 0 , well.getOilRate(6));
    BOOST_CHECK_EQUAL( 0 , well.getGasRate(6));
    BOOST_CHECK_EQUAL( 0 , well.getWaterRate(6));
}


BOOST_AUTO_TEST_CASE(GroupnameCorretlySet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap ,0);

    BOOST_CHECK_EQUAL("" , well.getGroupName(2));

    well.setGroupName(3 , "GROUP2");
    BOOST_CHECK_EQUAL("GROUP2" , well.getGroupName(3));
    BOOST_CHECK_EQUAL("GROUP2" , well.getGroupName(6));
    well.setGroupName(7 , "NEWGROUP");
    BOOST_CHECK_EQUAL("NEWGROUP" , well.getGroupName(7));
}


BOOST_AUTO_TEST_CASE(addWELSPECS_setData_dataSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 23, 42, 2334.32, timeMap, 3);

    BOOST_CHECK(!well.hasBeenDefined(2));
    BOOST_CHECK(well.hasBeenDefined(3));
    BOOST_CHECK_EQUAL(23, well.getHeadI());
    BOOST_CHECK_EQUAL(42, well.getHeadJ());
    BOOST_CHECK_EQUAL(2334.32, well.getRefDepth());
}


BOOST_AUTO_TEST_CASE(XHPLimitDefault) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    
    well.setBHPLimit( 1 , 100 );
    BOOST_CHECK_EQUAL( 100 , well.getBHPLimit( 5 ));

    well.setTHPLimit( 1 , 200 );
    BOOST_CHECK_EQUAL( 200 , well.getTHPLimit( 5 ));
}



BOOST_AUTO_TEST_CASE(InjectorType) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    
    well.setInjectorType( 1 , Opm::InjectorType::WATER );
    BOOST_CHECK_EQUAL( Opm::InjectorType::WATER , well.getInjectorType( 5 ));
}

