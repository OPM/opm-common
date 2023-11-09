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
#define BOOST_TEST_MODULE FIPConfigTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/EclipseState/IOConfig/FIPConfig.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(FieldFoamFieldResv)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIP=1' 'FIPFOAM=1' 'FIPRESV' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::VE));
}

BOOST_AUTO_TEST_CASE(FieldFipnumFipFoamFieldFoamRegionResv)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIP=3' 'FIPFOAM=2' 'FIPVE' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::VE));
}

BOOST_AUTO_TEST_CASE(PolymerFieldPolymerRegion)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIPPLY=2' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::VE));
}

BOOST_AUTO_TEST_CASE(SurfFieldSurfRegion)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIPSURF=2' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::VE));
}

BOOST_AUTO_TEST_CASE(HeatFieldHeatRegion)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIPHEAT=2' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::VE));
}

BOOST_AUTO_TEST_CASE(TemperatureFieldTemperatureRegion)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIPTEMP=2' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::VE));
}

BOOST_AUTO_TEST_CASE(TracerFieldTracerRegion)
{
    const std::string data = R"(
SOLUTION

RPTSOL
'FIPTR=2' /
)";

    auto deck = Parser().parseString( data );
    const auto& keyword = SOLUTIONSection(deck).get<ParserKeywords::RPTSOL>().back();
    FIPConfig fipConfig(keyword);

    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIPNUM));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FIP));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::FOAM_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::POLYMER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::RESV));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SOLVENT_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::SURF_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_FIELD));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::TEMPERATURE_REGION));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::TRACER_FIELD));
    BOOST_CHECK( fipConfig.output(FIPConfig::OutputField::TRACER_REGION));
    BOOST_CHECK(!fipConfig.output(FIPConfig::OutputField::VE));
}
