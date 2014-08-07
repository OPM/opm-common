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
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>

static Opm::TimeMapPtr createXDaysTimeMap(size_t numDays) {
    boost::gregorian::date startDate( 2010 , boost::gregorian::Jan , 1);
    Opm::TimeMapPtr timeMap(new Opm::TimeMap(boost::posix_time::ptime(startDate)));
    for (size_t i = 0; i < numDays; i++)
        timeMap->addTStep( boost::posix_time::hours( (i+1) * 24 ));
    return timeMap;
}

BOOST_AUTO_TEST_CASE(CreateWell_CorrectNameAndDefaultValues) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);
    BOOST_CHECK_EQUAL( "WELL1" , well.name() );
    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).OilRate);
}

BOOST_AUTO_TEST_CASE(CreateWell_GetProductionPropertiesShouldReturnSameObject) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);

    BOOST_CHECK_EQUAL(&(well.getProductionProperties(5)), &(well.getProductionProperties(5)));
    BOOST_CHECK_EQUAL(&(well.getProductionProperties(8)), &(well.getProductionProperties(8)));
    BOOST_CHECK_EQUAL(&(well.getProductionProperties(5)), &(well.getProductionProperties(8)));
}

BOOST_AUTO_TEST_CASE(CreateWell_GetInjectionPropertiesShouldReturnSameObject) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::WATER, timeMap , 0);

    BOOST_CHECK_EQUAL(&(well.getInjectionProperties(5)), &(well.getInjectionProperties(5)));
    BOOST_CHECK_EQUAL(&(well.getInjectionProperties(8)), &(well.getInjectionProperties(8)));
    BOOST_CHECK_EQUAL(&(well.getInjectionProperties(5)), &(well.getInjectionProperties(8)));
}

BOOST_AUTO_TEST_CASE(CreateWellCreateTimeStepOK) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 5);
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(0) );
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(4) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(5) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(7) );
    
}


BOOST_AUTO_TEST_CASE(setWellProductionProperties_PropertiesSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy( 5 ).OilRate);
    Opm::WellProductionProperties props;
    props.OilRate = 99;
    props.GasRate  = 98;
    props.WaterRate = 97;
    props.LiquidRate = 96;
    props.ResVRate = 95;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy( 5 ).OilRate);
    BOOST_CHECK_EQUAL(98 , well.getProductionPropertiesCopy( 5 ).GasRate);
    BOOST_CHECK_EQUAL(97 , well.getProductionPropertiesCopy( 5 ).WaterRate);
    BOOST_CHECK_EQUAL(96 , well.getProductionPropertiesCopy( 5 ).LiquidRate);
    BOOST_CHECK_EQUAL(95 , well.getProductionPropertiesCopy( 5 ).ResVRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy( 8 ).OilRate);
    BOOST_CHECK_EQUAL(98 , well.getProductionPropertiesCopy( 8 ).GasRate);
    BOOST_CHECK_EQUAL(97 , well.getProductionPropertiesCopy( 8 ).WaterRate);
    BOOST_CHECK_EQUAL(96 , well.getProductionPropertiesCopy( 8 ).LiquidRate);
    BOOST_CHECK_EQUAL(95 , well.getProductionPropertiesCopy( 8 ).ResVRate);
}

BOOST_AUTO_TEST_CASE(setOilRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).OilRate);
    Opm::WellProductionProperties props;
    props.OilRate = 99;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(5).OilRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(8).OilRate);
}

BOOST_AUTO_TEST_CASE(seLiquidRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).LiquidRate);
    Opm::WellProductionProperties props;
    props.LiquidRate = 99;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(5).LiquidRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(8).LiquidRate);
}


BOOST_AUTO_TEST_CASE(setPredictionModeProduction_ModeSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);
    
    BOOST_CHECK_EQUAL( true, well.getProductionPropertiesCopy(5).predictionMode);
    Opm::WellProductionProperties props;
    props.predictionMode = false;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(false , well.getProductionPropertiesCopy(5).predictionMode);
    BOOST_CHECK_EQUAL(false , well.getProductionPropertiesCopy(8).predictionMode);
}



BOOST_AUTO_TEST_CASE(setpredictionModeInjection_ModeSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::WATER, timeMap , 0);

    BOOST_CHECK_EQUAL( true, well.getInjectionPropertiesCopy(5).predictionMode);
    Opm::WellInjectionProperties props;
    props.predictionMode = false;
    well.setInjectionProperties( 5 , props);
    BOOST_CHECK_EQUAL(false , well.getInjectionPropertiesCopy(5).predictionMode);
    BOOST_CHECK_EQUAL(false , well.getInjectionPropertiesCopy(8).predictionMode);
}



BOOST_AUTO_TEST_CASE(NewWellZeroCompletions) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);
    Opm::CompletionSetConstPtr completions = well.getCompletions( 0 );
    BOOST_CHECK_EQUAL( 0U , completions->size());
}


BOOST_AUTO_TEST_CASE(UpdateCompletions) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap , 0);
    Opm::CompletionSetConstPtr completions = well.getCompletions( 0 );
    BOOST_CHECK_EQUAL( 0U , completions->size());
    
    std::vector<Opm::CompletionPtr> newCompletions;
    std::vector<Opm::CompletionPtr> newCompletions2;
    Opm::CompletionPtr comp1(new Opm::Completion( 10 , 10 , 10 , Opm::AUTO , 99.0, 22.3, 33.2));
    Opm::CompletionPtr comp2(new Opm::Completion( 10 , 11 , 10 , Opm::SHUT , 99.0, 22.3, 33.2));
    Opm::CompletionPtr comp3(new Opm::Completion( 10 , 10 , 12 , Opm::OPEN , 99.0, 22.3, 33.2));
    Opm::CompletionPtr comp4(new Opm::Completion( 10 , 10 , 12 , Opm::SHUT , 99.0, 22.3, 33.2));
    Opm::CompletionPtr comp5(new Opm::Completion( 10 , 10 , 13 , Opm::OPEN , 99.0, 22.3, 33.2));

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
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::GAS, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).GasRate);
    Opm::WellProductionProperties properties;
    properties.GasRate = 108;
    well.setProductionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(5).GasRate);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(8).GasRate);
}



BOOST_AUTO_TEST_CASE(setWaterRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::WATER, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).WaterRate);
    Opm::WellProductionProperties properties;
    properties.WaterRate = 108;
    well.setProductionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(5).WaterRate);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(8).WaterRate);
}


BOOST_AUTO_TEST_CASE(setSurfaceInjectionRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::WATER, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getInjectionPropertiesCopy(5).surfaceInjectionRate);
    Opm::WellInjectionProperties props(well.getInjectionPropertiesCopy(5));
    props.surfaceInjectionRate = 108;
    well.setInjectionProperties(5, props);
    BOOST_CHECK_EQUAL(108 , well.getInjectionPropertiesCopy(5).surfaceInjectionRate);
    BOOST_CHECK_EQUAL(108 , well.getInjectionPropertiesCopy(8).surfaceInjectionRate);
}


BOOST_AUTO_TEST_CASE(setReservoirInjectionRate_RateSetCorrect) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::WATER, timeMap , 0);
    
    BOOST_CHECK_EQUAL(0.0 , well.getInjectionPropertiesCopy(5).reservoirInjectionRate);
    Opm::WellInjectionProperties properties(well.getInjectionPropertiesCopy(5));
    properties.reservoirInjectionRate = 108;
    well.setInjectionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getInjectionPropertiesCopy(5).reservoirInjectionRate);
    BOOST_CHECK_EQUAL(108 , well.getInjectionPropertiesCopy(8).reservoirInjectionRate);
}


BOOST_AUTO_TEST_CASE(isProducerCorrectlySet) {
    // HACK: This test checks correctly setting of isProducer/isInjector. This property depends on which of
    //       WellProductionProperties/WellInjectionProperties is set last, independent of actual values.
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::OIL, timeMap ,0);

    /* 1: Well is created as producer */
    BOOST_CHECK_EQUAL( false , well.isInjector(0));
    BOOST_CHECK_EQUAL( true , well.isProducer(0));

    /* Set a surface injection rate => Well becomes an Injector */
    Opm::WellInjectionProperties injectionProps1(well.getInjectionPropertiesCopy(3));
    injectionProps1.surfaceInjectionRate = 100;
    well.setInjectionProperties(3, injectionProps1);
    BOOST_CHECK_EQUAL( true  , well.isInjector(3));
    BOOST_CHECK_EQUAL( false , well.isProducer(3));
    BOOST_CHECK_EQUAL( 100 , well.getInjectionPropertiesCopy(3).surfaceInjectionRate);
    
    /* Set a reservoir injection rate => Well becomes an Injector */
    Opm::WellInjectionProperties injectionProps2(well.getInjectionPropertiesCopy(4));
    injectionProps2.reservoirInjectionRate = 200;
    well.setInjectionProperties(4, injectionProps2);
    BOOST_CHECK_EQUAL( true  , well.isInjector(4));
    BOOST_CHECK_EQUAL( false , well.isProducer(4));
    BOOST_CHECK_EQUAL( 200 , well.getInjectionPropertiesCopy(4).reservoirInjectionRate);
    

    /* Set rates => Well becomes a producer; injection rate should be set to 0. */
    Opm::WellInjectionProperties injectionProps3;
    well.setInjectionProperties(4, injectionProps3);
    Opm::WellProductionProperties properties(well.getProductionPropertiesCopy(4));
    properties.OilRate = 100;
    properties.GasRate = 200;
    properties.WaterRate = 300;
    well.setProductionProperties(4, properties);
    BOOST_CHECK_EQUAL( false , well.isInjector(4));
    BOOST_CHECK_EQUAL( true , well.isProducer(4));
    BOOST_CHECK_EQUAL( 0 , well.getInjectionPropertiesCopy(4).surfaceInjectionRate);
    BOOST_CHECK_EQUAL( 0 , well.getInjectionPropertiesCopy(4).reservoirInjectionRate);
    BOOST_CHECK_EQUAL( 100 , well.getProductionPropertiesCopy(4).OilRate);
    BOOST_CHECK_EQUAL( 200 , well.getProductionPropertiesCopy(4).GasRate);
    BOOST_CHECK_EQUAL( 300 , well.getProductionPropertiesCopy(4).WaterRate);
    
    /* Set injection rate => Well becomes injector - all produced rates -> 0 */
    Opm::WellProductionProperties prodProps2;
    well.setProductionProperties(6, prodProps2);
    Opm::WellInjectionProperties injectionProps4(well.getInjectionPropertiesCopy(6));
    injectionProps4.reservoirInjectionRate = 50;
    well.setInjectionProperties(6, injectionProps4);
    BOOST_CHECK_EQUAL( true  , well.isInjector(6));
    BOOST_CHECK_EQUAL( false , well.isProducer(6));
    BOOST_CHECK_EQUAL( 50 , well.getInjectionPropertiesCopy(6).reservoirInjectionRate);
    BOOST_CHECK_EQUAL( 0 , well.getProductionPropertiesCopy(6).OilRate);
    BOOST_CHECK_EQUAL( 0 , well.getProductionPropertiesCopy(6).GasRate);
    BOOST_CHECK_EQUAL( 0 , well.getProductionPropertiesCopy(6).WaterRate);
}


BOOST_AUTO_TEST_CASE(GroupnameCorretlySet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, false, 0.0, Opm::Phase::WATER, timeMap ,0);

    BOOST_CHECK_EQUAL("" , well.getGroupName(2));

    well.setGroupName(3 , "GROUP2");
    BOOST_CHECK_EQUAL("GROUP2" , well.getGroupName(3));
    BOOST_CHECK_EQUAL("GROUP2" , well.getGroupName(6));
    well.setGroupName(7 , "NEWGROUP");
    BOOST_CHECK_EQUAL("NEWGROUP" , well.getGroupName(7));
}


BOOST_AUTO_TEST_CASE(addWELSPECS_setData_dataSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 23, 42, false, 2334.32, Opm::Phase::WATER, timeMap, 3);

    BOOST_CHECK(!well.hasBeenDefined(2));
    BOOST_CHECK(well.hasBeenDefined(3));
    BOOST_CHECK_EQUAL(23, well.getHeadI());
    BOOST_CHECK_EQUAL(42, well.getHeadJ());
    BOOST_CHECK_EQUAL(2334.32, well.getRefDepth());
    BOOST_CHECK_EQUAL(Opm::Phase::WATER, well.getPreferredPhase());
}


BOOST_AUTO_TEST_CASE(XHPLimitDefault) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);
    

    Opm::WellProductionProperties productionProps(well.getProductionPropertiesCopy(1));
    productionProps.BHPLimit = 100;
    productionProps.addProductionControl(Opm::WellProducer::BHP);
    well.setProductionProperties(1, productionProps);
    BOOST_CHECK_EQUAL( 100 , well.getProductionPropertiesCopy(5).BHPLimit);
    BOOST_CHECK( well.getProductionPropertiesCopy(5).hasProductionControl( Opm::WellProducer::BHP ));
    
    Opm::WellInjectionProperties injProps(well.getInjectionPropertiesCopy(1));
    injProps.THPLimit = 200;
    well.setInjectionProperties(1, injProps);
    BOOST_CHECK_EQUAL( 200 , well.getInjectionPropertiesCopy(5).THPLimit);
    BOOST_CHECK( !well.getInjectionPropertiesCopy(5).hasInjectionControl( Opm::WellInjector::THP ));
}



BOOST_AUTO_TEST_CASE(InjectorType) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);
    
    Opm::WellInjectionProperties injectionProps(well.getInjectionPropertiesCopy(1));
    injectionProps.injectorType = Opm::WellInjector::WATER;
    well.setInjectionProperties(1, injectionProps);
    // TODO: Should we test for something other than water here, as long as
    //       the default value for InjectorType is WellInjector::WATER?
    BOOST_CHECK_EQUAL( Opm::WellInjector::WATER , well.getInjectionPropertiesCopy(5).injectorType);
}




BOOST_AUTO_TEST_CASE(WellStatus) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);
    
    well.setStatus( 1 , Opm::WellCommon::OPEN );
    BOOST_CHECK_EQUAL( Opm::WellCommon::OPEN , well.getStatus( 5 ));
}



/*****************************************************************/


BOOST_AUTO_TEST_CASE(WellHaveProductionControlLimit) {

    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::OIL, timeMap, 0);

    
    BOOST_CHECK( !well.getProductionPropertiesCopy(1).hasProductionControl( Opm::WellProducer::ORAT ));
    BOOST_CHECK( !well.getProductionPropertiesCopy(1).hasProductionControl( Opm::WellProducer::RESV ));
    
    Opm::WellProductionProperties properties(well.getProductionPropertiesCopy(1));
    properties.OilRate = 100;
    properties.addProductionControl(Opm::WellProducer::ORAT);
    well.setProductionProperties(2, properties);
    BOOST_CHECK(  well.getProductionPropertiesCopy(2).hasProductionControl( Opm::WellProducer::ORAT ));
    BOOST_CHECK( !well.getProductionPropertiesCopy(2).hasProductionControl( Opm::WellProducer::RESV ));

    Opm::WellProductionProperties properties2(well.getProductionPropertiesCopy(2));
    properties2.ResVRate = 100;
    properties2.addProductionControl(Opm::WellProducer::RESV);
    well.setProductionProperties(2, properties2);
    BOOST_CHECK( well.getProductionPropertiesCopy(2).hasProductionControl( Opm::WellProducer::RESV ));
    
    Opm::WellProductionProperties properties3(well.getProductionPropertiesCopy(2));
    properties3.OilRate = 100;
    properties3.WaterRate = 100;
    properties3.GasRate = 100;
    properties3.LiquidRate = 100;
    properties3.ResVRate = 100;
    properties3.BHPLimit = 100;
    properties3.THPLimit = 100;
    properties3.addProductionControl(Opm::WellProducer::ORAT);
    properties3.addProductionControl(Opm::WellProducer::LRAT);
    properties3.addProductionControl(Opm::WellProducer::BHP);
    well.setProductionProperties(10, properties3);

    BOOST_CHECK( well.getProductionPropertiesCopy(10).hasProductionControl( Opm::WellProducer::ORAT ));
    BOOST_CHECK( well.getProductionPropertiesCopy(10).hasProductionControl( Opm::WellProducer::LRAT ));
    BOOST_CHECK( well.getProductionPropertiesCopy(10).hasProductionControl( Opm::WellProducer::BHP ));
    
    Opm::WellProductionProperties properties4(well.getProductionPropertiesCopy(10));
    properties4.dropProductionControl( Opm::WellProducer::LRAT );
    well.setProductionProperties(10, properties4);

    BOOST_CHECK( well.getProductionPropertiesCopy(11).hasProductionControl( Opm::WellProducer::ORAT ));
    BOOST_CHECK( !well.getProductionPropertiesCopy(11).hasProductionControl( Opm::WellProducer::LRAT ));
    BOOST_CHECK( well.getProductionPropertiesCopy(11).hasProductionControl( Opm::WellProducer::BHP ));
}



BOOST_AUTO_TEST_CASE(WellHaveInjectionControlLimit) {

    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);
    
    BOOST_CHECK( !well.getInjectionPropertiesCopy(1).hasInjectionControl( Opm::WellInjector::RATE ));
    BOOST_CHECK( !well.getInjectionPropertiesCopy(1).hasInjectionControl( Opm::WellInjector::RESV ));
    
    Opm::WellInjectionProperties injProps1(well.getInjectionPropertiesCopy(2));
    injProps1.surfaceInjectionRate = 100;
    injProps1.addInjectionControl(Opm::WellInjector::RATE);
    well.setInjectionProperties(2, injProps1);
    BOOST_CHECK(  well.getInjectionPropertiesCopy(2).hasInjectionControl( Opm::WellInjector::RATE ));
    BOOST_CHECK( !well.getInjectionPropertiesCopy(2).hasInjectionControl( Opm::WellInjector::RESV ));

    Opm::WellInjectionProperties injProps2(well.getInjectionPropertiesCopy(2));
    injProps2.reservoirInjectionRate = 100;
    injProps2.addInjectionControl(Opm::WellInjector::RESV);
    well.setInjectionProperties(2, injProps2);
    BOOST_CHECK( well.getInjectionPropertiesCopy(2).hasInjectionControl( Opm::WellInjector::RESV ));
    
    Opm::WellInjectionProperties injProps3(well.getInjectionPropertiesCopy(10));
    injProps3.BHPLimit = 100;
    injProps3.addInjectionControl(Opm::WellInjector::BHP);
    injProps3.THPLimit = 100;
    injProps3.addInjectionControl(Opm::WellInjector::THP);
    well.setInjectionProperties(10, injProps3);

    BOOST_CHECK( well.getInjectionPropertiesCopy(10).hasInjectionControl( Opm::WellInjector::RATE ));
    BOOST_CHECK( well.getInjectionPropertiesCopy(10).hasInjectionControl( Opm::WellInjector::RESV ));
    BOOST_CHECK( well.getInjectionPropertiesCopy(10).hasInjectionControl( Opm::WellInjector::THP ));
    BOOST_CHECK( well.getInjectionPropertiesCopy(10).hasInjectionControl( Opm::WellInjector::BHP ));

    Opm::WellInjectionProperties injProps4(well.getInjectionPropertiesCopy(11));
    injProps4.dropInjectionControl( Opm::WellInjector::RESV );
    well.setInjectionProperties(11, injProps4);
    
    BOOST_CHECK(  well.getInjectionPropertiesCopy(11).hasInjectionControl( Opm::WellInjector::RATE ));
    BOOST_CHECK( !well.getInjectionPropertiesCopy(11).hasInjectionControl( Opm::WellInjector::RESV ));
    BOOST_CHECK(  well.getInjectionPropertiesCopy(11).hasInjectionControl( Opm::WellInjector::THP ));
    BOOST_CHECK(  well.getInjectionPropertiesCopy(11).hasInjectionControl( Opm::WellInjector::BHP ));
}
/*********************************************************************/


BOOST_AUTO_TEST_CASE(WellSetAvailableForGroupControl_ControlSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);

    BOOST_CHECK(well.isAvailableForGroupControl(10));
    well.setAvailableForGroupControl(12, false);
    BOOST_CHECK(!well.isAvailableForGroupControl(13));
    well.setAvailableForGroupControl(15, true);
    BOOST_CHECK(well.isAvailableForGroupControl(15));
}

BOOST_AUTO_TEST_CASE(WellSetGuideRate_GuideRateSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);

    BOOST_CHECK_LT(well.getGuideRate(0), 0);
    well.setGuideRate(1, 32.2);
    BOOST_CHECK_LT(well.getGuideRate(0), 0);
    BOOST_CHECK_EQUAL(32.2, well.getGuideRate(1));
}

BOOST_AUTO_TEST_CASE(WellGuideRatePhase_GuideRatePhaseSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);
    BOOST_CHECK_EQUAL(Opm::GuideRate::UNDEFINED, well.getGuideRatePhase(0));
    well.setGuideRatePhase(3, Opm::GuideRate::RAT);
    BOOST_CHECK_EQUAL(Opm::GuideRate::UNDEFINED, well.getGuideRatePhase(2));
    BOOST_CHECK_EQUAL(Opm::GuideRate::RAT, well.getGuideRatePhase(3));
}


BOOST_AUTO_TEST_CASE(WellSetScalingFactor_ScalingFactorSetSet) {
    Opm::TimeMapPtr timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, false, 2334.32, Opm::Phase::WATER, timeMap, 0);
    BOOST_CHECK_EQUAL(1.0, well.getGuideRateScalingFactor(0));
    well.setGuideRateScalingFactor(4, 0.6);
    BOOST_CHECK_EQUAL(1.0, well.getGuideRateScalingFactor(3));
    BOOST_CHECK_EQUAL(0.6, well.getGuideRateScalingFactor(4));
}
