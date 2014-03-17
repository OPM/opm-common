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
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    for (size_t i = 0; i < numDays; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    return timeMap;
}

BOOST_AUTO_TEST_CASE(CreateWell_CorrectNameAndDefaultValues) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    BOOST_CHECK_EQUAL( "WELL1" , well.name() );
    BOOST_CHECK_EQUAL(0.0 , well.getProductionProperties(5).OilRate);
}

BOOST_AUTO_TEST_CASE(CreateWellCreateTimeStepOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 5);
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(0) );
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(4) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(5) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(7) );
    
}


BOOST_AUTO_TEST_CASE(setWellProductionProperties_PropertiesSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getProductionProperties( 5 ).OilRate);
    Opm::WellProductionProperties props;
    props.OilRate = 99;
    props.GasRate  = 98;
    props.WaterRate = 97;
    props.LiquidRate = 96;
    props.ResVRate = 95;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionProperties( 5 ).OilRate);
    BOOST_CHECK_EQUAL(98 , well.getProductionProperties( 5 ).GasRate);
    BOOST_CHECK_EQUAL(97 , well.getProductionProperties( 5 ).WaterRate);
    BOOST_CHECK_EQUAL(96 , well.getProductionProperties( 5 ).LiquidRate);
    BOOST_CHECK_EQUAL(95 , well.getProductionProperties( 5 ).ResVRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionProperties( 8 ).OilRate);
    BOOST_CHECK_EQUAL(98 , well.getProductionProperties( 8 ).GasRate);
    BOOST_CHECK_EQUAL(97 , well.getProductionProperties( 8 ).WaterRate);
    BOOST_CHECK_EQUAL(96 , well.getProductionProperties( 8 ).LiquidRate);
    BOOST_CHECK_EQUAL(95 , well.getProductionProperties( 8 ).ResVRate);
}

BOOST_AUTO_TEST_CASE(setOilRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionProperties(5).OilRate);
    Opm::WellProductionProperties props;
    props.OilRate = 99;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionProperties(5).OilRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionProperties(8).OilRate);
}

BOOST_AUTO_TEST_CASE(seLiquidRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionProperties(5).LiquidRate);
    Opm::WellProductionProperties props;
    props.LiquidRate = 99;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionProperties(5).LiquidRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionProperties(8).LiquidRate);
}


BOOST_AUTO_TEST_CASE(setPredictionModeProduction_ModeSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL( true, well.getProductionProperties(5).PredictionMode);
    Opm::WellProductionProperties props;
    props.PredictionMode = false;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(false , well.getProductionProperties(5).PredictionMode);
    BOOST_CHECK_EQUAL(false , well.getProductionProperties(8).PredictionMode);
}



BOOST_AUTO_TEST_CASE(setPredictionModeInjection_ModeSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);

    BOOST_CHECK_EQUAL( true, well.getInjectionProperties(5).PredictionMode);
    Opm::WellInjectionProperties props;
    props.PredictionMode = false;
    well.setInjectionProperties( 5 , props);
    BOOST_CHECK_EQUAL(false , well.getInjectionProperties(5).PredictionMode);
    BOOST_CHECK_EQUAL(false , well.getInjectionProperties(8).PredictionMode);
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
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionProperties(5).GasRate);
    Opm::WellProductionProperties properties;
    properties.GasRate = 108;
    well.setProductionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getProductionProperties(5).GasRate);
    BOOST_CHECK_EQUAL(108 , well.getProductionProperties(8).GasRate);
}



BOOST_AUTO_TEST_CASE(setWaterRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionProperties(5).WaterRate);
    Opm::WellProductionProperties properties;
    properties.WaterRate = 108;
    well.setProductionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getProductionProperties(5).WaterRate);
    BOOST_CHECK_EQUAL(108 , well.getProductionProperties(8).WaterRate);
}


BOOST_AUTO_TEST_CASE(setSurfaceInjectionRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getInjectionProperties(5).SurfaceInjectionRate);
    Opm::WellInjectionProperties props(well.getInjectionProperties(5));
    props.SurfaceInjectionRate = 108;
    well.setInjectionProperties(5, props);
    BOOST_CHECK_EQUAL(108 , well.getInjectionProperties(5).SurfaceInjectionRate);
    BOOST_CHECK_EQUAL(108 , well.getInjectionProperties(8).SurfaceInjectionRate);
}


BOOST_AUTO_TEST_CASE(setReservoirInjectionRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getInjectionProperties(5).ReservoirInjectionRate);
    Opm::WellInjectionProperties properties(well.getInjectionProperties(5));
    properties.ReservoirInjectionRate = 108;
    well.setInjectionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getInjectionProperties(5).ReservoirInjectionRate);
    BOOST_CHECK_EQUAL(108 , well.getInjectionProperties(8).ReservoirInjectionRate);
}


BOOST_AUTO_TEST_CASE(isProducerCorrectlySet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, timeMap ,0);

    /* 1: Well is created as producer */
    BOOST_CHECK_EQUAL( false , well.isInjector(0));
    BOOST_CHECK_EQUAL( true , well.isProducer(0));

    /* Set a surface injection rate => Well becomes an Injector */
    Opm::WellInjectionProperties injectionProps1(well.getInjectionProperties(3));
    injectionProps1.SurfaceInjectionRate = 100;
    well.setInjectionProperties(3, injectionProps1);
    BOOST_CHECK_EQUAL( true  , well.isInjector(3));
    BOOST_CHECK_EQUAL( false , well.isProducer(3));
    BOOST_CHECK_EQUAL( 100 , well.getInjectionProperties(3).SurfaceInjectionRate);
    
    /* Set a reservoir injection rate => Well becomes an Injector */
    Opm::WellInjectionProperties injectionProps2(well.getInjectionProperties(4));
    injectionProps2.ReservoirInjectionRate = 200;
    well.setInjectionProperties(4, injectionProps2);
    BOOST_CHECK_EQUAL( true  , well.isInjector(4));
    BOOST_CHECK_EQUAL( false , well.isProducer(4));
    BOOST_CHECK_EQUAL( 200 , well.getInjectionProperties(4).ReservoirInjectionRate);
    

    /* Set rates => Well becomes a producer; injection rate should be set to 0. */
    Opm::WellProductionProperties properties(well.getProductionProperties(4));
    properties.OilRate = 100;
    properties.GasRate = 200;
    properties.WaterRate = 300;
    well.setProductionProperties(4, properties);

    BOOST_CHECK_EQUAL( false , well.isInjector(4));
    BOOST_CHECK_EQUAL( true , well.isProducer(4));
//    BOOST_CHECK_EQUAL( 0 , well.getInjectionProperties(4).SurfaceInjectionRate);
//    BOOST_CHECK_EQUAL( 0 , well.getInjectionProperties(4).ReservoirInjectionRate);
    BOOST_CHECK_EQUAL( 100 , well.getProductionProperties(4).OilRate);
    BOOST_CHECK_EQUAL( 200 , well.getProductionProperties(4).GasRate);
    BOOST_CHECK_EQUAL( 300 , well.getProductionProperties(4).WaterRate);
    
    /* Set injection rate => Well becomes injector - all produced rates -> 0 */
    Opm::WellInjectionProperties injectionProps3(well.getInjectionProperties(6));
    injectionProps3.ReservoirInjectionRate = 50;
    well.setInjectionProperties(6, injectionProps3);
    BOOST_CHECK_EQUAL( true  , well.isInjector(6));
    BOOST_CHECK_EQUAL( false , well.isProducer(6));
    BOOST_CHECK_EQUAL( 50 , well.getInjectionProperties(6).ReservoirInjectionRate);
//    BOOST_CHECK_EQUAL( 0 , well.getProductionProperties(6).OilRate);
//    BOOST_CHECK_EQUAL( 0 , well.getProductionProperties(6).GasRate);
//    BOOST_CHECK_EQUAL( 0 , well.getProductionProperties(6).WaterRate);
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
    

    Opm::WellProductionProperties productionProps(well.getProductionProperties(1));
    productionProps.BHPLimit = 100;
    well.setProductionProperties(1, productionProps);
    BOOST_CHECK_EQUAL( 100 , well.getProductionProperties(5).BHPLimit);
//    BOOST_CHECK( well.hasProductionControl( 5 , Opm::WellProducer::BHP ));
    
    Opm::WellInjectionProperties injectionProps(well.getInjectionProperties(1));
    injectionProps.THPLimit = 200;
    well.setInjectionProperties(1, injectionProps);
    BOOST_CHECK_EQUAL( 200 , well.getInjectionProperties(5).THPLimit);
//    BOOST_CHECK( !well.hasProductionControl( 5 , Opm::WellProducer::THP ));
}



BOOST_AUTO_TEST_CASE(InjectorType) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    
    Opm::WellInjectionProperties injectionProps(well.getInjectionProperties(1));
    injectionProps.InjectorType = Opm::WellInjector::WATER;
    well.setInjectionProperties(1, injectionProps);
    // TODO: Should we test for something other than water here, as long as
    //       the default value for InjectorType is WellInjector::WATER?
    BOOST_CHECK_EQUAL( Opm::WellInjector::WATER , well.getInjectionProperties(5).InjectorType);
}


BOOST_AUTO_TEST_CASE(ControlMode) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    
    well.setInjectorControlMode( 1 , Opm::WellInjector::RESV );
    BOOST_CHECK_EQUAL( Opm::WellInjector::RESV , well.getInjectorControlMode( 5 ));

    well.setProducerControlMode( 1 , Opm::WellProducer::GRUP );
    BOOST_CHECK_EQUAL( Opm::WellProducer::GRUP , well.getProducerControlMode( 5 ));
}


BOOST_AUTO_TEST_CASE(WellStatus) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    
    well.setStatus( 1 , Opm::WellCommon::OPEN );
    BOOST_CHECK_EQUAL( Opm::WellCommon::OPEN , well.getStatus( 5 ));
}



/*****************************************************************/


/*
BOOST_AUTO_TEST_CASE(WellHaveProductionControlLimit) {

    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);

    
    BOOST_CHECK( !well.getProductionProperties(1).hasProductionControl( Opm::WellProducer::ORAT ));
    BOOST_CHECK( !well.getProductionProperties(1).hasProductionControl( Opm::WellProducer::RESV ));

    Opm::WellProductionProperties properties(well.getProductionProperties(1));
    properties.OilRate = 100;
    well.setProductionProperties(2, properties);
    BOOST_CHECK(  well.getProductionProperties(2).hasProductionControl( Opm::WellProducer::ORAT ));
    BOOST_CHECK( !well.getProductionProperties(2).hasProductionControl( Opm::WellProducer::RESV ));

    Opm::WellProductionProperties properties2(well.getProductionProperties(2));
    properties2.ResVRate = 100;
    well.setProductionProperties(2, properties2);
    BOOST_CHECK( well.getProductionProperties(2).hasProductionControl( Opm::WellProducer::RESV ));
    
    Opm::WellProductionProperties properties3(well.getProductionProperties(2));
    properties3.OilRate = 100;
    properties3.WaterRate = 100;
    properties3.GasRate = 100;
    properties3.LiquidRate = 100;
    properties3.ResVRate = 100;
    well.setBHPLimit( 10 , 100 , true);
    well.setTHPLimit( 10 , 100 , true);
    
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::ORAT ));
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::WRAT ));
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::GRAT ));
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::LRAT ));
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::RESV ));
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::BHP ));
    BOOST_CHECK( well.hasProductionControl( 10 , Opm::WellProducer::THP ));
    
    well.dropProductionControl( 11 , Opm::WellProducer::RESV );

    BOOST_CHECK( well.hasProductionControl( 11 , Opm::WellProducer::ORAT ));
    BOOST_CHECK( well.hasProductionControl( 11 , Opm::WellProducer::WRAT ));
    BOOST_CHECK( well.hasProductionControl( 11 , Opm::WellProducer::GRAT ));
    BOOST_CHECK( well.hasProductionControl( 11 , Opm::WellProducer::LRAT ));
    BOOST_CHECK( !well.hasProductionControl( 11 , Opm::WellProducer::RESV ));
    BOOST_CHECK( well.hasProductionControl( 11 , Opm::WellProducer::BHP ));
    BOOST_CHECK( well.hasProductionControl( 11 , Opm::WellProducer::THP ));
}



BOOST_AUTO_TEST_CASE(WellHaveInjectionControlLimit) {

    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);

    
    BOOST_CHECK( !well.hasInjectionControl( 1 , Opm::WellInjector::RATE ));
    BOOST_CHECK( !well.hasInjectionControl( 1 , Opm::WellInjector::RESV ));
    
    Opm::WellInjectionProperties injectorProperties1(well.getInjectionProperties(2));
    injectorProperties1.SurfaceInjectionRate = 100;
    well.setInjectionProperties(2, injectorProperties1);
    BOOST_CHECK(  well.hasInjectionControl( 2, Opm::WellInjector::RATE ));
    BOOST_CHECK( !well.hasInjectionControl( 2 , Opm::WellInjector::RESV ));

    Opm::WellInjectionProperties injectorProperties2(well.getInjectionProperties(2));
    injectorProperties2.ReservoirInjectionRate = 100;
    well.setInjectionProperties(2, injectorProperties2);
    BOOST_CHECK( well.hasInjectionControl( 2 , Opm::WellInjector::RESV ));
    
    well.setBHPLimit( 10 , 100 , false);
    well.setTHPLimit( 10 , 100 , false);
    
    BOOST_CHECK( well.hasInjectionControl( 10 , Opm::WellInjector::RATE ));
    BOOST_CHECK( well.hasInjectionControl( 10 , Opm::WellInjector::RESV ));
    BOOST_CHECK( well.hasInjectionControl( 10 , Opm::WellInjector::THP ));
    BOOST_CHECK( well.hasInjectionControl( 10 , Opm::WellInjector::BHP ));

    well.dropInjectionControl( 11 , Opm::WellInjector::RESV );

    BOOST_CHECK(  well.hasInjectionControl( 11 , Opm::WellInjector::RATE ));
    BOOST_CHECK( !well.hasInjectionControl( 11 , Opm::WellInjector::RESV ));
    BOOST_CHECK(  well.hasInjectionControl( 11 , Opm::WellInjector::THP ));
    BOOST_CHECK(  well.hasInjectionControl( 11 , Opm::WellInjector::BHP ));
}
*/
/*********************************************************************/


BOOST_AUTO_TEST_CASE(WellSetAvailableForGroupControl_ControlSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);

    BOOST_CHECK(well.isAvailableForGroupControl(10));
    well.setAvailableForGroupControl(12, false);
    BOOST_CHECK(!well.isAvailableForGroupControl(13));
    well.setAvailableForGroupControl(15, true);
    BOOST_CHECK(well.isAvailableForGroupControl(15));
}

BOOST_AUTO_TEST_CASE(WellSetGuideRate_GuideRateSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);

    BOOST_CHECK_LT(well.getGuideRate(0), 0);
    well.setGuideRate(1, 32.2);
    BOOST_CHECK_LT(well.getGuideRate(0), 0);
    BOOST_CHECK_EQUAL(32.2, well.getGuideRate(1));
}

BOOST_AUTO_TEST_CASE(WellGuideRatePhase_GuideRatePhaseSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    BOOST_CHECK_EQUAL(Opm::GuideRate::UNDEFINED, well.getGuideRatePhase(0));
    well.setGuideRatePhase(3, Opm::GuideRate::RAT);
    BOOST_CHECK_EQUAL(Opm::GuideRate::UNDEFINED, well.getGuideRatePhase(2));
    BOOST_CHECK_EQUAL(Opm::GuideRate::RAT, well.getGuideRatePhase(3));
}


BOOST_AUTO_TEST_CASE(WellSetScalingFactor_ScalingFactorSetSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, timeMap, 0);
    BOOST_CHECK_EQUAL(1.0, well.getGuideRateScalingFactor(0));
    well.setGuideRateScalingFactor(4, 0.6);
    BOOST_CHECK_EQUAL(1.0, well.getGuideRateScalingFactor(3));
    BOOST_CHECK_EQUAL(0.6, well.getGuideRateScalingFactor(4));
}

