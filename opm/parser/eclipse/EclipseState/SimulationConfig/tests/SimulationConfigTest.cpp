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



#define BOOST_TEST_MODULE SimulationConfigTests

#include <opm/common/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <opm/common/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>


using namespace Opm;

const std::string& inputStr = "RUNSPEC\n"
                              "EQLOPTS\n"
                              "THPRES /\n"
                              "DIMENS\n"
                              "10 3 4 /\n"
                              "\n"
                              "GRID\n"
                              "REGIONS\n"
                              "EQLNUM\n"
                              "10*1 10*2 100*3 /\n "
                              "\n"

                              "SOLUTION\n"
                              "THPRES\n"
                              "1 2 12.0/\n"
                              "1 3 5.0/\n"
                              "2 3 7.0/\n"
                              "/\n"
                              "\n";

const std::string& inputStr_noTHPRES = "RUNSPEC\n"
                                       "EQLOPTS\n"
                                       "DIMENS\n"
                                       "10 3 4 /\n"
                                       "\n"
                                       "GRID\n"
                                       "REGIONS\n"
                                       "EQLNUM\n"
                                       "10*1 10*2 100*3 /\n "
                                       "\n"
                                       "SOLUTION\n"
                                       "\n";

const std::string& inputStr_cpr = "RUNSPEC\n"
    "CPR\n"
    "/\n"
    "SUMMARY\n";


const std::string& inputStr_INVALID = "RUNSPEC\n"
    "CPR\n"
    "WEll 10 10 17/"
    "/\n"
    "SUMMARY\n";



const std::string& inputStr_cpr_in_SUMMARY = "SUMMARY\n"
        "CPR\n"
        "well1 10 27 10/\n/\n";

const std::string& inputStr_cpr_BOTH = "RUNSPEC\n"
    "CPR\n"
    "/\n"
    "SUMMARY\n"
    "CPR\n"
    "well1 10 20 30/\n/\n";

const std::string& inputStr_vap_dis = "RUNSPEC\n"
                                      "VAPOIL\n"
                                      "DISGAS\n"
                                      "DIMENS\n"
                                      "10 3 4 /\n"
                                      "\n"
                                      "GRID\n"
                                      "REGIONS\n"
                                      "\n";

static DeckPtr createDeck(const ParseMode& parseMode , const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input, parseMode);
}


static std::shared_ptr<GridProperties<int>> getGridProperties() {
    GridPropertySupportedKeywordInfo<int> kwInfo = GridPropertySupportedKeywordInfo<int>("EQLNUM", 3, "");
    std::vector<GridPropertySupportedKeywordInfo<int>> supportedKeywordsVec;
    supportedKeywordsVec.push_back(kwInfo);
    EclipseGridConstPtr eclipseGrid = std::make_shared<const EclipseGrid>(3, 3, 3);
    std::shared_ptr<GridProperties<int>> gridProperties = std::make_shared<GridProperties<int>>(eclipseGrid, std::move(supportedKeywordsVec));
    gridProperties->addKeyword("EQLNUM");
    return gridProperties;
}


BOOST_AUTO_TEST_CASE(SimulationConfigGetThresholdPressureTableTest) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr);
    SimulationConfigConstPtr simulationConfigPtr;
    BOOST_CHECK_NO_THROW(simulationConfigPtr = std::make_shared<const SimulationConfig>(parseMode , deck, getGridProperties()));
}


BOOST_AUTO_TEST_CASE(SimulationConfigNOTHPRES) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr_noTHPRES);
    SimulationConfig simulationConfig(parseMode , deck, getGridProperties());
    BOOST_CHECK_EQUAL( false , simulationConfig.hasThresholdPressure());
}

BOOST_AUTO_TEST_CASE(SimulationConfigCPRNotUsed) {
        ParseMode parseMode;
        DeckPtr deck = createDeck(parseMode , inputStr_noTHPRES);
        SimulationConfig simulationConfig(parseMode , deck, getGridProperties());
        BOOST_CHECK_EQUAL( false , simulationConfig.useCPR());
}

BOOST_AUTO_TEST_CASE(SimulationConfigCPRUsed) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr_cpr);
    SUMMARYSection summary(*deck);
    SimulationConfig simulationConfig(parseMode , deck, getGridProperties());
    BOOST_CHECK_EQUAL( true , simulationConfig.useCPR());
    BOOST_CHECK_EQUAL( false , summary.hasKeyword("CPR"));
}


BOOST_AUTO_TEST_CASE(SimulationConfigCPRInSUMMARYSection) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr_cpr_in_SUMMARY);
    SUMMARYSection summary(*deck);
    SimulationConfig simulationConfig(parseMode , deck, getGridProperties());
    BOOST_CHECK_EQUAL( false , simulationConfig.useCPR());
    BOOST_CHECK_EQUAL( true , summary.hasKeyword("CPR"));
}


BOOST_AUTO_TEST_CASE(SimulationConfigCPRBoth) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr_cpr_BOTH);
    SUMMARYSection summary(*deck);
    SimulationConfig simulationConfig(parseMode , deck, getGridProperties());
    BOOST_CHECK_EQUAL( true , simulationConfig.useCPR());
    BOOST_CHECK_EQUAL( true , summary.hasKeyword("CPR"));

    const auto& cpr = summary.getKeyword<ParserKeywords::CPR>();
    const auto& record = cpr.getRecord(0);
    BOOST_CHECK_EQUAL( 1 , cpr.size());
    BOOST_CHECK_EQUAL( record.getItem<ParserKeywords::CPR::WELL>().get< std::string >(0) , "well1");
    BOOST_CHECK_EQUAL( record.getItem<ParserKeywords::CPR::I>().get< int >(0) , 10);
    BOOST_CHECK_EQUAL( record.getItem<ParserKeywords::CPR::J>().get< int >(0) , 20);
    BOOST_CHECK_EQUAL( record.getItem<ParserKeywords::CPR::K>().get< int >(0) , 30);
}


BOOST_AUTO_TEST_CASE(SimulationConfigCPRRUnspecWithData) {
    ParseMode parseMode;
    BOOST_CHECK_THROW( createDeck(parseMode , inputStr_INVALID) , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(SimulationConfig_VAPOIL_DISGAS) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr);
    SimulationConfig simulationConfig(parseMode , deck, getGridProperties());
    BOOST_CHECK_EQUAL( false , simulationConfig.hasDISGAS());
    BOOST_CHECK_EQUAL( false , simulationConfig.hasVAPOIL());

    DeckPtr deck_vd = createDeck(parseMode, inputStr_vap_dis);
    SimulationConfig simulationConfig_vd(parseMode , deck_vd, getGridProperties());
    BOOST_CHECK_EQUAL( true , simulationConfig_vd.hasDISGAS());
    BOOST_CHECK_EQUAL( true , simulationConfig_vd.hasVAPOIL());
}
