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

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>


using namespace Opm;

const std::string& inputStr = "RUNSPEC\n"
                              "EQLOPTS\n"
                              "THPRES /\n "
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


static DeckPtr createDeck(const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input, ParseMode());
}


static std::shared_ptr<GridProperties<int>> getGridProperties() {
    GridPropertySupportedKeywordInfo<int> kwInfo = GridPropertySupportedKeywordInfo<int>("EQLNUM", 3, "");
    std::shared_ptr<std::vector<GridPropertySupportedKeywordInfo<int>>> supportedKeywordsVec = std::make_shared<std::vector<GridPropertySupportedKeywordInfo<int>>>();
    supportedKeywordsVec->push_back(kwInfo);
    EclipseGridConstPtr eclipseGrid = std::make_shared<const EclipseGrid>(3, 3, 3);
    std::shared_ptr<GridProperties<int>> gridProperties = std::make_shared<GridProperties<int>>(eclipseGrid, supportedKeywordsVec);
    gridProperties->addKeyword("EQLNUM");
    return gridProperties;
}


BOOST_AUTO_TEST_CASE(SimulationConfigGetThresholdPressureTableTest) {

    DeckPtr deck = createDeck(inputStr);
    SimulationConfigConstPtr simulationConfigPtr;
    BOOST_CHECK_NO_THROW(simulationConfigPtr = std::make_shared<const SimulationConfig>(deck, getGridProperties()));
    const std::vector<double>& thresholdPressureTable = simulationConfigPtr->getThresholdPressureTable();
    BOOST_CHECK(thresholdPressureTable.size() > 0);
}



