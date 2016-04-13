/*
  Copyright 2015 Statoil ASA.

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



#include <algorithm>

#define BOOST_TEST_MODULE ThresholdPressureTests

#include <opm/common/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <opm/common/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/ThresholdPressure.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>


using namespace Opm;

const std::string& inputStr = "RUNSPEC\n"
                              "EQLOPTS\n"
                              "THPRES /\n "
                              "\n"

                              "SOLUTION\n"
                              "THPRES\n"
                              "1 2 12.0/\n"
                              "1 3 5.0/\n"
                              "2 3 33.0 /\n"
                              "2 3 7.0/\n"
                              "/\n"
                              "\n";

const std::string& inputStrWithEqlNum =
                              "RUNSPEC\n"
                              "EQLOPTS\n"
                              "THPRES /\n "
                              "\n"
                              "REGIONS\n"
                              "EQLNUM\n"
                              "   27*3 /\n"
                              "SOLUTION\n"
                              "THPRES\n"
                              "1 2 12.0/\n"
                              "1 3 5.0/\n"
                              "2 3 33.0 /\n"
                              "2 3 7.0/\n"
                              "/\n"
                              "\n";


const std::string& inputStrWithEqlNumall0 =
                              "RUNSPEC\n"
                              "EQLOPTS\n"
                              "THPRES /\n "
                              "\n"
                              "REGIONS\n"
                              "EQLNUM\n"
                              "   27*0 /\n"
                              "SOLUTION\n"
                              "THPRES\n"
                              "1 2 12.0/\n"
                              "1 3 5.0/\n"
                              "2 3 33.0 /\n"
                              "2 3 7.0/\n"
                              "/\n"
                              "\n";


const std::string& inputStrNoSolutionSection =  "RUNSPEC\n"
                                                "EQLOPTS\n"
                                                "THPRES /\n "
                                                "\n";


const std::string& inputStrNoTHPRESinSolutionNorRUNSPEC = "RUNSPEC\n"
                                                          "\n"
                                                          "SOLUTION\n"
                                                          "\n"
                                                          "SCHEDULE\n";

const std::string& inputStrTHPRESinRUNSPECnotSoultion = "RUNSPEC\n"
                                                        "EQLOPTS\n"
                                                        "ss /\n "
                                                        "\n"
                                                        "SOLUTION\n"
                                                        "\n";


const std::string& inputStrIrrevers = "RUNSPEC\n"
                                      "EQLOPTS\n"
                                      "THPRES IRREVERS/\n "
                                      "\n"

                                      "SOLUTION\n"
                                      "THPRES\n"
                                      "/\n"
                                      "\n";

const std::string& inputStrInconsistency =  "RUNSPEC\n"
                                            "EQLOPTS\n"
                                            "THPRES /\n "
                                            "\n"

                                            "SOLUTION\n"
                                            "\n";

const std::string& inputStrTooHighRegionNumbers = "RUNSPEC\n"
                                                  "EQLOPTS\n"
                                                  "THPRES /\n "
                                                  "\n"

                                                  "SOLUTION\n"
                                                  "THPRES\n"
                                                  "1 2 12.0/\n"
                                                  "4 3 5.0/\n"
                                                  "2 3 7.0/\n"
                                                  "/\n"
                                                  "\n";


const std::string& inputStrMissingData = "RUNSPEC\n"
                                         "EQLOPTS\n"
                                         "THPRES /\n "
                                         "\n"

                                         "SOLUTION\n"
                                         "THPRES\n"
                                         "1 2 12.0/\n"
                                         "2 3 5.0/\n"
                                         "1 /\n"
                                         "/\n"
                                         "\n";


const std::string& inputStrMissingPressure = "RUNSPEC\n"
                                         "EQLOPTS\n"
                                         "THPRES /\n "
                                         "\n"
                                         "REGIONS\n"
                                         "EQLNUM\n"
                                         "   27*3 /\n"
                                         "SOLUTION\n"
                                         "THPRES\n"
                                         "1 2 12.0/\n"
                                         "2 3 5.0/\n"
                                         "2 3 /\n"
                                         "/\n"
                                         "\n";




static DeckPtr createDeck(const ParseContext& parseContext , const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input , parseContext);
}

static Eclipse3DProperties getEclipseProperties(const Deck& deck)
{
    static const EclipseGrid eclipseGrid( 3, 3, 3 );
    return Eclipse3DProperties( deck, TableManager( deck ), eclipseGrid );
}


BOOST_AUTO_TEST_CASE(ThresholdPressureDeckHasEqlnum) {
    ParseContext parseContext;
    DeckPtr deck = createDeck(parseContext , inputStrWithEqlNum);
    auto ep = getEclipseProperties( *deck );

    BOOST_CHECK (ep.hasDeckIntGridProperty("EQLNUM"));
}


BOOST_AUTO_TEST_CASE(ThresholdPressureTest) {
    ParseContext parseContext;
    DeckPtr deck = createDeck(parseContext , inputStrWithEqlNum);
    auto ep = getEclipseProperties( *deck );
    ThresholdPressureConstPtr thp = std::make_shared<ThresholdPressure>(parseContext , *deck, ep);

    BOOST_CHECK_EQUAL( thp->getThresholdPressure(1,2) , 1200000.0 );
    BOOST_CHECK_EQUAL( thp->getThresholdPressure(2,1) , 1200000.0 );
    BOOST_CHECK_EQUAL( thp->getThresholdPressure(1,3) , 500000.0 );
    BOOST_CHECK_EQUAL( thp->getThresholdPressure(3,1) , 500000.0 );
    BOOST_CHECK_EQUAL( thp->getThresholdPressure(2,3) , 700000.0 );
    BOOST_CHECK_EQUAL( thp->getThresholdPressure(3,2) , 700000.0 );

}


BOOST_AUTO_TEST_CASE(ThresholdPressureEmptyTest) {
    ParseContext parseContext;
    DeckPtr deck = createDeck(parseContext , inputStrNoSolutionSection);
    auto ep = getEclipseProperties(*deck);
    ThresholdPressureConstPtr thresholdPressurePtr = std::make_shared<ThresholdPressure>(parseContext , *deck, ep);
    BOOST_CHECK_EQUAL(0, thresholdPressurePtr->size());
}


BOOST_AUTO_TEST_CASE(ThresholdPressureNoTHPREStest) {
    ParseContext parseContext;
    DeckPtr deck_no_thpres = createDeck(parseContext , inputStrNoTHPRESinSolutionNorRUNSPEC);
    DeckPtr deck_no_thpres2 = createDeck(parseContext , inputStrTHPRESinRUNSPECnotSoultion);
    auto ep = getEclipseProperties(*deck_no_thpres);
    auto ep2 = getEclipseProperties(*deck_no_thpres2);

    ThresholdPressureConstPtr thresholdPressurePtr;
    BOOST_CHECK_NO_THROW(thresholdPressurePtr = std::make_shared<ThresholdPressure>(parseContext , *deck_no_thpres, ep));
    ThresholdPressureConstPtr thresholdPressurePtr2;
    BOOST_CHECK_NO_THROW(thresholdPressurePtr2 = std::make_shared<ThresholdPressure>(parseContext , *deck_no_thpres2, ep2));

    BOOST_CHECK_EQUAL(0, thresholdPressurePtr->size());
    BOOST_CHECK_EQUAL(0, thresholdPressurePtr2->size());
}


BOOST_AUTO_TEST_CASE(ThresholdPressureThrowTest) {
    ParseContext parseContext;
    DeckPtr deck                 = createDeck(parseContext , inputStr);
    DeckPtr deck_all0            = createDeck(parseContext , inputStrWithEqlNumall0);
    DeckPtr deck_irrevers        = createDeck(parseContext , inputStrIrrevers);
    DeckPtr deck_inconsistency   = createDeck(parseContext , inputStrInconsistency);
    DeckPtr deck_highRegNum      = createDeck(parseContext , inputStrTooHighRegionNumbers);
    DeckPtr deck_missingData     = createDeck(parseContext , inputStrMissingData);
    DeckPtr deck_missingPressure = createDeck(parseContext , inputStrMissingPressure);

    auto ep                 = getEclipseProperties(*deck                );
    auto ep_irrevers        = getEclipseProperties(*deck_irrevers       );
    auto ep_inconsistency   = getEclipseProperties(*deck_inconsistency  );
    auto ep_highRegNum      = getEclipseProperties(*deck_highRegNum     );
    auto ep_missingData     = getEclipseProperties(*deck_missingData    );
    auto ep_missingPressure = getEclipseProperties(*deck_missingPressure);

    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseContext,*deck_irrevers, ep_irrevers), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseContext,*deck_inconsistency, ep_inconsistency), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseContext,*deck_highRegNum, ep_highRegNum), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseContext,*deck_missingData, ep_missingData), std::runtime_error);

    {
        auto gridPropertiesEQLNUMkeywordNotAdded = getEclipseProperties( *deck);
        BOOST_CHECK_THROW(
                std::make_shared<ThresholdPressure>( parseContext, *deck, gridPropertiesEQLNUMkeywordNotAdded ),
                std::runtime_error );
    }
    {
        auto gridPropertiesEQLNUMall0 = getEclipseProperties(*deck_all0);
        BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseContext , *deck, gridPropertiesEQLNUMall0), std::runtime_error);
    }

    parseContext.update( ParseContext::UNSUPPORTED_INITIAL_THPRES , InputError::IGNORE );
    BOOST_CHECK_NO_THROW(ThresholdPressure(parseContext,*deck_missingPressure, ep_missingPressure));


    {
        ThresholdPressure thp(parseContext , *deck_missingPressure , ep_missingPressure );

        BOOST_CHECK_EQUAL( true  , thp.hasRegionBarrier(2,3));
        BOOST_CHECK_EQUAL( false , thp.hasThresholdPressure(2,3));

        parseContext.update( ParseContext::INTERNAL_ERROR_UNINITIALIZED_THPRES , InputError::THROW_EXCEPTION );
        BOOST_CHECK_THROW( thp.getThresholdPressure(2, 3) , std::invalid_argument);

        parseContext.update( ParseContext::INTERNAL_ERROR_UNINITIALIZED_THPRES , InputError::IGNORE );
        BOOST_CHECK_EQUAL( 0.0 , thp.getThresholdPressure(2, 3));
    }
}


BOOST_AUTO_TEST_CASE(HasPair) {
    ParseContext      parseContext;
    DeckPtr deck    = createDeck(parseContext , inputStrWithEqlNum);
    auto ep         = getEclipseProperties(*deck);
    ThresholdPressure thp(parseContext , *deck , ep);

    BOOST_CHECK_EQUAL( true , thp.hasRegionBarrier( 1 , 2 ));
    BOOST_CHECK_EQUAL( false , thp.hasRegionBarrier( 1 , 7 ));
    BOOST_CHECK_EQUAL( true , thp.hasThresholdPressure( 1, 2 ));
    BOOST_CHECK_EQUAL( false , thp.hasThresholdPressure( 1, 7 ));
    BOOST_CHECK_EQUAL( 1200000.0 , thp.getThresholdPressure( 1,2 ));

}

