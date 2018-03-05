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

#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

static Opm::TimeMap createXDaysTimeMap(size_t numDays) {
    const std::time_t startDate = Opm::TimeMap::mkdate(2010, 1, 1);
    Opm::TimeMap timeMap{ startDate };
    for (size_t i = 0; i < numDays; i++)
        timeMap.addTStep((i+1) * 24 * 60 * 60);
    return timeMap;
}

namespace Opm {
inline std::ostream& operator<<( std::ostream& stream, const Completion& c ) {
    return stream << "(" << c.getI() << "," << c.getJ() << "," << c.getK() << ")";
}
inline std::ostream& operator<<( std::ostream& stream, const Well& well ) {
    return stream << "(" << well.name() << ")";
}
}

BOOST_AUTO_TEST_CASE(CreateWell_CorrectNameAndDefaultValues) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);
    BOOST_CHECK_EQUAL( "WELL1" , well.name() );
    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).OilRate);
}

BOOST_AUTO_TEST_CASE(CreateWell_Equals) {
    auto timeMap = createXDaysTimeMap(10);
    auto timeMap2 = createXDaysTimeMap(11);
    Opm::Well well1("WELL1" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);
    Opm::Well well2("WELL1" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);
    Opm::Well well3("WELL3" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);
    Opm::Well well4("WELL3" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap2 , 0);
    BOOST_CHECK_EQUAL( well1, well1 );
    BOOST_CHECK_EQUAL( well2, well1 );
    BOOST_CHECK( well1 == well2 );
    BOOST_CHECK( well1 != well3 );
    BOOST_CHECK( well3 != well2 );
    BOOST_CHECK( well3 == well3 );
    BOOST_CHECK( well4 != well3 );
}


BOOST_AUTO_TEST_CASE(CreateWell_GetProductionPropertiesShouldReturnSameObject) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);

    BOOST_CHECK_EQUAL(&(well.getProductionProperties(5)), &(well.getProductionProperties(5)));
    BOOST_CHECK_EQUAL(&(well.getProductionProperties(8)), &(well.getProductionProperties(8)));
    BOOST_CHECK_EQUAL(well.getProductionProperties(5), well.getProductionProperties(8));
}

BOOST_AUTO_TEST_CASE(CreateWell_GetInjectionPropertiesShouldReturnSameObject) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" ,  0, 0, 0.0, Opm::Phase::WATER, timeMap , 0);

    BOOST_CHECK_EQUAL(&(well.getInjectionProperties(5)), &(well.getInjectionProperties(5)));
    BOOST_CHECK_EQUAL(&(well.getInjectionProperties(8)), &(well.getInjectionProperties(8)));
    BOOST_CHECK_EQUAL(well.getInjectionProperties(5), well.getInjectionProperties(8));
}

BOOST_AUTO_TEST_CASE(CreateWellCreateTimeStepOK) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap , 5);
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(0) );
    BOOST_CHECK_EQUAL( false , well.hasBeenDefined(4) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(5) );
    BOOST_CHECK_EQUAL( true , well.hasBeenDefined(7) );

}


BOOST_AUTO_TEST_CASE(setWellProductionProperties_PropertiesSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);

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

    BOOST_CHECK_EQUAL(99, well.production_rate( Opm::Phase::OIL, 5 ) );
    BOOST_CHECK_EQUAL(99, well.production_rate( Opm::Phase::OIL, 8 ) );
    BOOST_CHECK_EQUAL(98, well.production_rate( Opm::Phase::GAS, 5 ) );
    BOOST_CHECK_EQUAL(98, well.production_rate( Opm::Phase::GAS, 8 ) );

    BOOST_CHECK_EQUAL(0.0, well.injection_rate( Opm::Phase::GAS, 8 ) );

    BOOST_CHECK_THROW( well.production_rate( Opm::Phase::SOLVENT, 5 ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(setOilRate_RateSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).OilRate);
    Opm::WellProductionProperties props;
    props.OilRate = 99;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(5).OilRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(8).OilRate);
}

BOOST_AUTO_TEST_CASE(seLiquidRate_RateSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).LiquidRate);
    Opm::WellProductionProperties props;
    props.LiquidRate = 99;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(5).LiquidRate);
    BOOST_CHECK_EQUAL(99 , well.getProductionPropertiesCopy(8).LiquidRate);
}


BOOST_AUTO_TEST_CASE(setPredictionModeProduction_ModeSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);

    BOOST_CHECK_EQUAL( true, well.getProductionPropertiesCopy(5).predictionMode);
    Opm::WellProductionProperties props;
    props.predictionMode = false;
    well.setProductionProperties( 5 , props);
    BOOST_CHECK( !well.getProductionPropertiesCopy(5).predictionMode );
    BOOST_CHECK( !well.getProductionPropertiesCopy(8).predictionMode );
}


BOOST_AUTO_TEST_CASE(setpredictionModeInjection_ModeSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::WATER, timeMap , 0);

    BOOST_CHECK_EQUAL( true, well.getInjectionPropertiesCopy(5).predictionMode);
    Opm::WellInjectionProperties props;
    props.predictionMode = false;
    well.setInjectionProperties( 5 , props);
    BOOST_CHECK_EQUAL(false , well.getInjectionPropertiesCopy(5).predictionMode);
    BOOST_CHECK_EQUAL(false , well.getInjectionPropertiesCopy(8).predictionMode);
}


BOOST_AUTO_TEST_CASE(WellCOMPDATtestTRACK) {
    Opm::Parser parser;
    std::string input =
                "START             -- 0 \n"
                "19 JUN 2007 / \n"
                "SCHEDULE\n"
                "DATES             -- 1\n"
                " 10  OKT 2008 / \n"
                "/\n"
                "WELSPECS\n"
                "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                "/\n"
                "COMPORD\n"
                " OP_1 TRACK / \n"
                "/\n"
                "COMPDAT\n"
                " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                " 'OP_1'  9  9   3   9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                "/\n"
                "DATES             -- 2\n"
                " 20  JAN 2010 / \n"
                "/\n";


    Opm::ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    Opm::EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Opm::Schedule schedule(deck, grid , eclipseProperties, Opm::Phases(true, true, true) , Opm::ParseContext());
    auto* op_1 = schedule.getWell("OP_1");

    size_t timestep = 2;
    const auto& completions = op_1->getCompletions( timestep );
    BOOST_CHECK_EQUAL(9U, completions.size());

    //Verify TRACK completion ordering
    for (size_t k = 0; k < completions.size(); ++k) {
        BOOST_CHECK_EQUAL(completions.get( k ).getK(), k);
    }
}


BOOST_AUTO_TEST_CASE(WellCOMPDATtestDefaultTRACK) {
    Opm::Parser parser;
    std::string input =
                "START             -- 0 \n"
                "19 JUN 2007 / \n"
                "SCHEDULE\n"
                "DATES             -- 1\n"
                " 10  OKT 2008 / \n"
                "/\n"
                "WELSPECS\n"
                "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                "/\n"
                "COMPDAT\n"
                " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                " 'OP_1'  9  9   3   9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                "/\n"
                "DATES             -- 2\n"
                " 20  JAN 2010 / \n"
                "/\n";


    Opm::ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    Opm::EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Opm::Schedule schedule(deck, grid , eclipseProperties, Opm::Phases(true, true, true) , Opm::ParseContext());
    auto* op_1 = schedule.getWell("OP_1");

    size_t timestep = 2;
    const auto& completions = op_1->getCompletions( timestep );
    BOOST_CHECK_EQUAL(9U, completions.size());

    //Verify TRACK completion ordering
    for (size_t k = 0; k < completions.size(); ++k) {
        BOOST_CHECK_EQUAL(completions.get( k ).getK(), k);
    }
}

BOOST_AUTO_TEST_CASE(WellCOMPDATtestINPUT) {
    Opm::Parser parser;
    std::string input =
                "START             -- 0 \n"
                "19 JUN 2007 / \n"
                "SCHEDULE\n"
                "DATES             -- 1\n"
                " 10  OKT 2008 / \n"
                "/\n"
                "WELSPECS\n"
                "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
                "/\n"
                "COMPORD\n"
                " OP_1 INPUT / \n"
                "/\n"
                "COMPDAT\n"
                " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                " 'OP_1'  9  9   3   9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
                " 'OP_1'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
                "/\n"
                "DATES             -- 2\n"
                " 20  JAN 2010 / \n"
                "/\n";


    Opm::ParseContext parseContext;
    auto deck = parser.parseString(input, parseContext);
    Opm::EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid);
    Opm::Schedule schedule(deck, grid , eclipseProperties, Opm::Phases(true, true, true) , Opm::ParseContext());
    auto* op_1 = schedule.getWell("OP_1");

    size_t timestep = 2;
    const auto& completions = op_1->getCompletions( timestep );
    BOOST_CHECK_EQUAL(9U, completions.size());

    //Verify INPUT completion ordering
    BOOST_CHECK_EQUAL(completions.get( 0 ).getK(), 0);
    BOOST_CHECK_EQUAL(completions.get( 1 ).getK(), 2);
    BOOST_CHECK_EQUAL(completions.get( 2 ).getK(), 3);
    BOOST_CHECK_EQUAL(completions.get( 3 ).getK(), 4);
    BOOST_CHECK_EQUAL(completions.get( 4 ).getK(), 5);
    BOOST_CHECK_EQUAL(completions.get( 5 ).getK(), 6);
    BOOST_CHECK_EQUAL(completions.get( 6 ).getK(), 7);
    BOOST_CHECK_EQUAL(completions.get( 7 ).getK(), 8);
    BOOST_CHECK_EQUAL(completions.get( 8 ).getK(), 1);
}

BOOST_AUTO_TEST_CASE(NewWellZeroCompletions) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);
    BOOST_CHECK_EQUAL( 0U , well.getCompletions( 0 ).size() );
}


BOOST_AUTO_TEST_CASE(UpdateCompletions) {
    auto timeMap = createXDaysTimeMap(10);

    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);
    const auto& completions = well.getCompletions( 0 );
    BOOST_CHECK_EQUAL( 0U , completions.size());

    Opm::Completion comp1( 10 , 10 , 10 , 1, 10, Opm::WellCompletion::AUTO , Opm::Value<double>("ConnectionTransmissibilityFactor",99.88), Opm::Value<double>("D",22.33), Opm::Value<double>("SKIN",33.22), 0);
    Opm::Completion comp2( 10 , 10 , 11 , 1, 11, Opm::WellCompletion::SHUT , Opm::Value<double>("ConnectionTransmissibilityFactor",99.88), Opm::Value<double>("D",22.33), Opm::Value<double>("SKIN",33.22), 0);
    Opm::Completion comp3( 10 , 10 , 12 , 1, 12, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor",99.88), Opm::Value<double>("D",22.33), Opm::Value<double>("SKIN",33.22), 0);
    Opm::Completion comp4( 10 , 10 , 12 , 1, 12, Opm::WellCompletion::SHUT , Opm::Value<double>("ConnectionTransmissibilityFactor",99.88), Opm::Value<double>("D",22.33), Opm::Value<double>("SKIN",33.22), 0);
    Opm::Completion comp5( 10 , 10 , 13 , 1, 13, Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor",99.88), Opm::Value<double>("D",22.33), Opm::Value<double>("SKIN",33.22), 0);

    //std::vector<Opm::CompletionConstPtr> newCompletions2{ comp4 , comp5}; Newer c++

    std::vector< Opm::Completion > newCompletions, newCompletions2;
    newCompletions.push_back( comp1 );
    newCompletions.push_back( comp2 );
    newCompletions.push_back( comp3 );

    newCompletions2.push_back( comp4 );
    newCompletions2.push_back( comp5 );

    BOOST_CHECK_EQUAL( 3U , newCompletions.size());
    well.addCompletions( 5 , newCompletions );
    BOOST_CHECK_EQUAL( 3U , well.getCompletions( 5 ).size());
    BOOST_CHECK_EQUAL( comp3 , well.getCompletions( 5 ).get(2));

    well.addCompletions( 6 , newCompletions2 );

    BOOST_CHECK_EQUAL( 4U , well.getCompletions( 6 ).size());
    BOOST_CHECK_EQUAL( comp4 , well.getCompletions( 6 ).get(2));
}

// Helper function for CompletionOrder test.
inline Opm::Completion completion( int i, int j, int k, int complnum = 1 ) {
    return Opm::Completion{ i, j, k,
                            complnum,
                            k*1.0,
                            Opm::WellCompletion::AUTO,
                            Opm::Value<double>("ConnectionTransmissibilityFactor",99.88),
                            Opm::Value<double>("D",22.33),
                            Opm::Value<double>("SKIN",33.22),
                            0,
                            Opm::WellCompletion::DirectionEnum::Z };
}


BOOST_AUTO_TEST_CASE(CompletionOrder) {
    auto timeMap = createXDaysTimeMap(10);
    {
        // Vertical well.
        Opm::Well well("WELL1" , 5, 5, 0.0, Opm::Phase::OIL, timeMap , 0);
        auto c1 = completion(5, 5, 8);
        auto c2 = completion(5, 5, 9);
        auto c3 = completion(5, 5, 1);
        auto c4 = completion(5, 5, 0);
        Opm::CompletionSet cv1 = { c1, c2 };
        well.addCompletionSet(1, cv1);
        BOOST_CHECK_EQUAL(well.getCompletions(1).get(0), c1);
        Opm::CompletionSet cv2 = { c3, c4 };
        well.addCompletionSet(2, cv2);
        BOOST_CHECK_EQUAL(well.getCompletions(1).get(0), c1);
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(0), c4);
    }

    {
        // Horizontal well.
        Opm::Well well("WELL1" ,  5, 5, 0.0, Opm::Phase::OIL, timeMap , 0);
        auto c1 = completion(6, 5, 8, 1);
        auto c2 = completion(5, 6, 7, 2);
        auto c3 = completion(7, 5, 8, 1);
        auto c4 = completion(9, 5, 8, 2);
        auto c5 = completion(8, 5, 9, 3);
        auto c6 = completion(5, 5, 4, 1);

        std::vector< Opm::Completion > cv1 = { c1, c2 };
        well.addCompletions(1, cv1);
        BOOST_CHECK_EQUAL(well.getCompletions(1).get(0), c2);

        /*
         * adding completions in batches like this will under the hood modify
         * completion numbers to match expectations, so we ensure that we're
         * comparing to the right value by forcing the right-hand-side of the
         * comparison to use the expected completion number
         */
        std::vector< Opm::Completion > cv2 = { c3, c4, c5 };
        well.addCompletions(2, cv2);
        BOOST_CHECK_EQUAL(well.getCompletions(1).get(0), Completion( c2, 2 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(0), Completion( c2, 2 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(1), Completion( c1, 1 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(2), Completion( c3, 3 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(3), Completion( c5, 5 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(4), Completion( c4, 4 ) );
        std::vector< Opm::Completion > cv3 = { c6 };

        well.addCompletions(3, cv3);
        BOOST_CHECK_EQUAL(well.getCompletions(1).get(0), Completion( c2, 2 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(0), Completion( c2, 2 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(1), Completion( c1, 1 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(2), Completion( c3, 3 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(3), Completion( c5, 5 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(2).get(4), Completion( c4, 4 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(3).get(0), Completion( c6, 6 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(3).get(1), Completion( c2, 2 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(3).get(2), Completion( c1, 1 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(3).get(3), Completion( c3, 3 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(3).get(4), Completion( c5, 5 ) );
        BOOST_CHECK_EQUAL(well.getCompletions(3).get(5), Completion( c4, 4 ) );
    }
}



BOOST_AUTO_TEST_CASE(setGasRate_RateSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::GAS, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).GasRate);
    Opm::WellProductionProperties properties;
    properties.GasRate = 108;
    well.setProductionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(5).GasRate);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(8).GasRate);
}



BOOST_AUTO_TEST_CASE(setWaterRate_RateSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::WATER, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getProductionPropertiesCopy(5).WaterRate);
    Opm::WellProductionProperties properties;
    properties.WaterRate = 108;
    well.setProductionProperties(5, properties);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(5).WaterRate);
    BOOST_CHECK_EQUAL(108 , well.getProductionPropertiesCopy(8).WaterRate);
}


BOOST_AUTO_TEST_CASE(setSurfaceInjectionRate_RateSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::WATER, timeMap , 0);

    BOOST_CHECK_EQUAL(0.0 , well.getInjectionPropertiesCopy(5).surfaceInjectionRate);
    Opm::WellInjectionProperties props(well.getInjectionPropertiesCopy(5));
    props.surfaceInjectionRate = 108;
    well.setInjectionProperties(5, props);
    BOOST_CHECK_EQUAL(108 , well.getInjectionPropertiesCopy(5).surfaceInjectionRate);
    BOOST_CHECK_EQUAL(108 , well.getInjectionPropertiesCopy(8).surfaceInjectionRate);

    BOOST_CHECK_EQUAL( 108, well.injection_rate(Opm::Phase::WATER, 5) );
    BOOST_CHECK_EQUAL( 108, well.injection_rate(Opm::Phase::WATER, 8) );

    BOOST_CHECK_EQUAL( 0.0, well.injection_rate(Opm::Phase::GAS, 5) );
    BOOST_CHECK_EQUAL( 0.0, well.injection_rate(Opm::Phase::GAS, 8) );
}


BOOST_AUTO_TEST_CASE(setReservoirInjectionRate_RateSetCorrect) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::WATER, timeMap , 0);

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
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" , 0, 0, 0.0, Opm::Phase::OIL, timeMap ,0);

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
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1" ,  0, 0, 0.0, Opm::Phase::WATER, timeMap ,0);

    BOOST_CHECK_EQUAL("" , well.getGroupName(2));

    well.setGroupName(3 , "GROUP2");
    BOOST_CHECK_EQUAL("GROUP2" , well.getGroupName(3));
    BOOST_CHECK_EQUAL("GROUP2" , well.getGroupName(6));
    well.setGroupName(7 , "NEWGROUP");
    BOOST_CHECK_EQUAL("NEWGROUP" , well.getGroupName(7));
}


BOOST_AUTO_TEST_CASE(addWELSPECS_setData_dataSet) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 23, 42, 2334.32, Opm::Phase::WATER, timeMap, 3);

    BOOST_CHECK(!well.hasBeenDefined(2));
    BOOST_CHECK(well.hasBeenDefined(3));
    BOOST_CHECK_EQUAL(23, well.getHeadI());
    BOOST_CHECK_EQUAL(42, well.getHeadJ());
    BOOST_CHECK_EQUAL(2334.32, well.getRefDepth());
    BOOST_CHECK_EQUAL(Opm::Phase::WATER, well.getPreferredPhase());
}


BOOST_AUTO_TEST_CASE(XHPLimitDefault) {
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);


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
    auto timeMap = createXDaysTimeMap(10);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);

    Opm::WellInjectionProperties injectionProps(well.getInjectionPropertiesCopy(1));
    injectionProps.injectorType = Opm::WellInjector::WATER;
    well.setInjectionProperties(1, injectionProps);
    // TODO: Should we test for something other than water here, as long as
    //       the default value for InjectorType is WellInjector::WATER?
    BOOST_CHECK_EQUAL( Opm::WellInjector::WATER , well.getInjectionPropertiesCopy(5).injectorType);
}




BOOST_AUTO_TEST_CASE(WellStatus) {
    auto timeMap = createXDaysTimeMap(10);

    Opm::Well well("WELL1" ,  0, 0, 0.0, Opm::Phase::OIL, timeMap , 0);

    std::vector<Opm::Completion> newCompletions;
    Opm::Completion comp1(10 , 10 , 10 , 1, 0.25 , Opm::WellCompletion::OPEN , Opm::Value<double>("ConnectionTransmissibilityFactor",99.88), Opm::Value<double>("D",22.33), Opm::Value<double>("SKIN",33.22), 0);

    newCompletions.push_back( comp1 );

    well.addCompletions( 2 , newCompletions );

    well.setStatus( 3 , Opm::WellCommon::OPEN );
    BOOST_CHECK_EQUAL( Opm::WellCommon::OPEN , well.getStatus( 5 ));
}



/*****************************************************************/


BOOST_AUTO_TEST_CASE(WellHaveProductionControlLimit) {

    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::OIL, timeMap, 0);


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

    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);

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
    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);

    BOOST_CHECK(well.isAvailableForGroupControl(10));
    well.setAvailableForGroupControl(12, false);
    BOOST_CHECK(!well.isAvailableForGroupControl(13));
    well.setAvailableForGroupControl(15, true);
    BOOST_CHECK(well.isAvailableForGroupControl(15));
}

BOOST_AUTO_TEST_CASE(WellSetGuideRate_GuideRateSet) {
    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);

    BOOST_CHECK_LT(well.getGuideRate(0), 0);
    well.setGuideRate(1, 32.2);
    BOOST_CHECK_LT(well.getGuideRate(0), 0);
    BOOST_CHECK_EQUAL(32.2, well.getGuideRate(1));
}

BOOST_AUTO_TEST_CASE(WellGuideRatePhase_GuideRatePhaseSet) {
    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);
    BOOST_CHECK_EQUAL(Opm::GuideRate::UNDEFINED, well.getGuideRatePhase(0));
    well.setGuideRatePhase(3, Opm::GuideRate::RAT);
    BOOST_CHECK_EQUAL(Opm::GuideRate::UNDEFINED, well.getGuideRatePhase(2));
    BOOST_CHECK_EQUAL(Opm::GuideRate::RAT, well.getGuideRatePhase(3));
}

BOOST_AUTO_TEST_CASE(WellEfficiencyFactorSet) {
    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);
    BOOST_CHECK_EQUAL(1.0, well.getEfficiencyFactor(0));
    well.setEfficiencyFactor(3, 0.9);
    BOOST_CHECK_EQUAL(1.0, well.getEfficiencyFactor(0));
    BOOST_CHECK_EQUAL(0.9, well.getEfficiencyFactor(3));
}

BOOST_AUTO_TEST_CASE(WellSetScalingFactor_ScalingFactorSetSet) {
    auto timeMap = createXDaysTimeMap(20);
    Opm::Well well("WELL1", 1, 2, 2334.32, Opm::Phase::WATER, timeMap, 0);
    BOOST_CHECK_EQUAL(1.0, well.getGuideRateScalingFactor(0));
    well.setGuideRateScalingFactor(4, 0.6);
    BOOST_CHECK_EQUAL(1.0, well.getGuideRateScalingFactor(3));
    BOOST_CHECK_EQUAL(0.6, well.getGuideRateScalingFactor(4));
}


BOOST_AUTO_TEST_CASE(testWellNameInWellNamePattern) {
    const std::string& wellnamePattern1 = "OP_*";
    const std::string& wellname1 = "OP_1";

    BOOST_CHECK_EQUAL(Opm::Well::wellNameInWellNamePattern(wellname1, wellnamePattern1), true);

    const std::string& wellnamePattern2 = "NONE";
    BOOST_CHECK_EQUAL(Opm::Well::wellNameInWellNamePattern(wellname1, wellnamePattern2), false);
}

namespace {
    namespace WCONHIST {
        std::string all_specified_CMODE_THP() {
            const std::string input =
                "WCONHIST\n"
                "'P' 'OPEN' 'THP' 1 2 3/\n/\n";

            return input;
        }



        std::string all_specified() {
            const std::string input =
                "WCONHIST\n"
                "'P' 'OPEN' 'ORAT' 1 2 3/\n/\n";

            return input;
        }

        std::string orat_defaulted() {
            const std::string input =
                "WCONHIST\n"
                "'P' 'OPEN' 'WRAT' 1* 2 3/\n/\n";

            return input;
        }

        std::string owrat_defaulted() {
            const std::string input =
                "WCONHIST\n"
                "'P' 'OPEN' 'GRAT' 1* 1* 3/\n/\n";

            return input;
        }

        std::string all_defaulted() {
            const std::string input =
                "WCONHIST\n"
                "'P' 'OPEN' 'LRAT'/\n/\n";

            return input;
        }

        std::string all_defaulted_with_bhp() {
            const std::string input =
                "WCONHIST\n"
                "-- 1    2     3      4-9 10\n"
                "   'P' 'OPEN' 'RESV' 6*  500/\n/\n";

            return input;
        }

        Opm::WellProductionProperties properties(const std::string& input) {
            Opm::Parser parser;

            auto deck = parser.parseString(input, Opm::ParseContext());
            const auto& record = deck.getKeyword("WCONHIST").getRecord(0);
            Opm::WellProductionProperties hist = Opm::WellProductionProperties::history( 100 , record);;

            return hist;
        }
    } // namespace WCONHIST

    namespace WCONPROD {
        std::string
        all_specified_CMODE_BHP()
        {
            const std::string input =
                "WCONPROD\n"
                "'P' 'OPEN' 'BHP' 1 2 3/\n/\n";

            return input;
        }


        Opm::WellProductionProperties
        properties(const std::string& input)
        {
            Opm::Parser parser;

            auto deck = parser.parseString(input, Opm::ParseContext());
            const auto& kwd     = deck.getKeyword("WCONHIST");
            const auto&  record = kwd.getRecord(0);
            Opm::WellProductionProperties pred = Opm::WellProductionProperties::prediction( record, false );

            return pred;
        }
    }

} // namespace anonymous

BOOST_AUTO_TEST_CASE(WCH_All_Specified_BHP_Defaulted)
{
    const Opm::WellProductionProperties& p =
        WCONHIST::properties(WCONHIST::all_specified());

    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::ORAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::WRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::GRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::LRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::RESV));

    BOOST_CHECK_EQUAL(p.controlMode , Opm::WellProducer::ORAT);

    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::BHP));
}

BOOST_AUTO_TEST_CASE(WCH_ORAT_Defaulted_BHP_Defaulted)
{
    const Opm::WellProductionProperties& p =
        WCONHIST::properties(WCONHIST::orat_defaulted());

    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::ORAT));
    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::WRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::GRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::LRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::RESV));
    BOOST_CHECK_EQUAL(p.controlMode , Opm::WellProducer::WRAT);

    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::BHP));
}

BOOST_AUTO_TEST_CASE(WCH_OWRAT_Defaulted_BHP_Defaulted)
{
    const Opm::WellProductionProperties& p =
        WCONHIST::properties(WCONHIST::owrat_defaulted());

    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::ORAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::WRAT));
    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::GRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::LRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::RESV));
    BOOST_CHECK_EQUAL(p.controlMode , Opm::WellProducer::GRAT);

    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::BHP));
}

BOOST_AUTO_TEST_CASE(WCH_Rates_Defaulted_BHP_Defaulted)
{
    const Opm::WellProductionProperties& p =
        WCONHIST::properties(WCONHIST::all_defaulted());

    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::ORAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::WRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::GRAT));
    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::LRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::RESV));
    BOOST_CHECK_EQUAL(p.controlMode , Opm::WellProducer::LRAT);

    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::BHP));
}

BOOST_AUTO_TEST_CASE(WCH_Rates_Defaulted_BHP_Specified)
{
    const Opm::WellProductionProperties& p =
        WCONHIST::properties(WCONHIST::all_defaulted_with_bhp());

    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::ORAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::WRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::GRAT));
    BOOST_CHECK( !p.hasProductionControl(Opm::WellProducer::LRAT));
    BOOST_CHECK(p.hasProductionControl(Opm::WellProducer::RESV));

    BOOST_CHECK_EQUAL(p.controlMode , Opm::WellProducer::RESV);

    BOOST_CHECK_EQUAL(true, p.hasProductionControl(Opm::WellProducer::BHP));
}


BOOST_AUTO_TEST_CASE(BHP_CMODE)
{
    BOOST_CHECK_THROW( WCONHIST::properties(WCONHIST::all_specified_CMODE_THP()) , std::invalid_argument);
    BOOST_CHECK_THROW( WCONPROD::properties(WCONPROD::all_specified_CMODE_BHP()) , std::invalid_argument);
}




BOOST_AUTO_TEST_CASE(CMODE_DEFAULT) {
    const Opm::WellProductionProperties Pproperties;
    const Opm::WellInjectionProperties Iproperties;

    BOOST_CHECK_EQUAL( Pproperties.controlMode , Opm::WellProducer::CMODE_UNDEFINED );
    BOOST_CHECK_EQUAL( Iproperties.controlMode , Opm::WellInjector::CMODE_UNDEFINED );
}
