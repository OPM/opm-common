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

#include <opm/core/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <opm/core/utility/platform_dependent/reenable_warnings.h>

#include <opm/parser/eclipse/EclipseState/SimulationConfig/ThresholdPressure.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
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

                                         "SOLUTION\n"
                                         "THPRES\n"
                                         "1 2 12.0/\n"
                                         "2 3 5.0/\n"
                                         "2 3 /\n"
                                         "/\n"
                                         "\n";




static DeckPtr createDeck(const ParseMode& parseMode , const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input , parseMode);
}


static std::shared_ptr<GridProperties<int>> getGridProperties(int defaultEqlnum = 3, bool addKeyword = true) {
    GridPropertySupportedKeywordInfo<int> kwInfo = GridPropertySupportedKeywordInfo<int>("EQLNUM", defaultEqlnum, "");
    std::shared_ptr<std::vector<GridPropertySupportedKeywordInfo<int>>> supportedKeywordsVec = std::make_shared<std::vector<GridPropertySupportedKeywordInfo<int>>>();
    supportedKeywordsVec->push_back(kwInfo);
    EclipseGridConstPtr eclipseGrid = std::make_shared<const EclipseGrid>(3, 3, 3);
    std::shared_ptr<GridProperties<int>> gridProperties = std::make_shared<GridProperties<int>>(eclipseGrid, supportedKeywordsVec);
    if (addKeyword) {
        gridProperties->addKeyword("EQLNUM");
    }
    return gridProperties;
}


BOOST_AUTO_TEST_CASE(ThresholdPressureTest) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStr);
    static std::shared_ptr<GridProperties<int>> gridProperties = getGridProperties();
    ThresholdPressureConstPtr tresholdPressurePtr = std::make_shared<ThresholdPressure>(parseMode , deck, gridProperties);

    const std::vector<double>& thresholdPressureTable = tresholdPressurePtr->getThresholdPressureTable();

    double pressureList[] = {0.0, 1200000.0, 500000.0, 1200000.0, 0.0, 700000.0, 500000.0, 700000.0, 0.0};
    std::vector<double> wantedResultVec(pressureList, pressureList + sizeof(pressureList) / sizeof(double));

    BOOST_CHECK_EQUAL(thresholdPressureTable.size(), wantedResultVec.size());
    BOOST_CHECK(std::equal(thresholdPressureTable.begin(), thresholdPressureTable.end(), wantedResultVec.begin()));
}


BOOST_AUTO_TEST_CASE(ThresholdPressureEmptyTest) {
    ParseMode parseMode;
    DeckPtr deck = createDeck(parseMode , inputStrNoSolutionSection);
    static std::shared_ptr<GridProperties<int>> gridProperties = getGridProperties();
    ThresholdPressureConstPtr tresholdPressurePtr = std::make_shared<ThresholdPressure>(parseMode , deck, gridProperties);
    const std::vector<double>& thresholdPressureTable = tresholdPressurePtr->getThresholdPressureTable();

    BOOST_CHECK_EQUAL(0, thresholdPressureTable.size());
}


BOOST_AUTO_TEST_CASE(ThresholdPressureNoTHPREStest) {
    ParseMode parseMode;
    DeckPtr deck_no_thpres = createDeck(parseMode , inputStrNoTHPRESinSolutionNorRUNSPEC);
    DeckPtr deck_no_thpres2 = createDeck(parseMode , inputStrTHPRESinRUNSPECnotSoultion);
    static std::shared_ptr<GridProperties<int>> gridProperties = getGridProperties();

    ThresholdPressureConstPtr tresholdPressurePtr;
    BOOST_CHECK_NO_THROW(tresholdPressurePtr = std::make_shared<ThresholdPressure>(parseMode , deck_no_thpres, gridProperties));
    ThresholdPressureConstPtr tresholdPressurePtr2;
    BOOST_CHECK_NO_THROW(tresholdPressurePtr2 = std::make_shared<ThresholdPressure>(parseMode , deck_no_thpres2, gridProperties));

    const std::vector<double>& thresholdPressureTable = tresholdPressurePtr->getThresholdPressureTable();
    BOOST_CHECK_EQUAL(0, thresholdPressureTable.size());

    const std::vector<double>& thresholdPressureTable2 = tresholdPressurePtr2->getThresholdPressureTable();
    BOOST_CHECK_EQUAL(0, thresholdPressureTable2.size());
}


BOOST_AUTO_TEST_CASE(ThresholdPressureThrowTest) {
    ParseMode parseMode;
    DeckPtr deck                 = createDeck(parseMode , inputStr);
    DeckPtr deck_irrevers        = createDeck(parseMode , inputStrIrrevers);
    DeckPtr deck_inconsistency   = createDeck(parseMode , inputStrInconsistency);
    DeckPtr deck_highRegNum      = createDeck(parseMode , inputStrTooHighRegionNumbers);
    DeckPtr deck_missingData     = createDeck(parseMode , inputStrMissingData);
    DeckPtr deck_missingPressure = createDeck(parseMode , inputStrMissingPressure);
    static std::shared_ptr<GridProperties<int>> gridProperties = getGridProperties();

    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode,deck_irrevers, gridProperties), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode,deck_inconsistency, gridProperties), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode,deck_highRegNum, gridProperties), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode,deck_missingData, gridProperties), std::runtime_error);
    BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode,deck_missingPressure, gridProperties), std::invalid_argument);

    {
        static std::shared_ptr<GridProperties<int>> gridPropertiesEQLNUMkeywordNotAdded = getGridProperties(3, false);
        BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode , deck, gridPropertiesEQLNUMkeywordNotAdded), std::runtime_error);
    }
    {
        static std::shared_ptr<GridProperties<int>> gridPropertiesEQLNUMall0 = getGridProperties(0);
        BOOST_CHECK_THROW(std::make_shared<ThresholdPressure>(parseMode , deck, gridPropertiesEQLNUMall0), std::runtime_error);
    }

    parseMode.update( ParseMode::UNSUPPORTED_INITIAL_THPRES , InputError::IGNORE );
    BOOST_CHECK_NO_THROW(ThresholdPressure(parseMode,deck_missingPressure, gridProperties));
}

