/*
  Copyright 2023 SINTEF Digital

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

#define BOOST_TEST_MODULE UDTTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDT.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(UDT_NV)
{
    UDT udt({1.0, 4.0, 5.0}, {5.0, 10.0, 11.0}, UDT::InterpolationType::NearestNeighbour);

    BOOST_CHECK_EQUAL(udt(0.0), 5.0);
    BOOST_CHECK_EQUAL(udt(1.5), 5.0);
    BOOST_CHECK_EQUAL(udt(4.0), 10.0);
    BOOST_CHECK_EQUAL(udt(4.7), 11.0);
    BOOST_CHECK_EQUAL(udt(5.2), 11.0);
}

BOOST_AUTO_TEST_CASE(UDT_LC)
{
    UDT udt({1.0, 4.0, 5.0}, {5.0, 10.0, 11.0}, UDT::InterpolationType::LinearClamp);

    BOOST_CHECK_EQUAL(udt(0.0), 5.0);
    BOOST_CHECK_EQUAL(udt(1.5), 5.0 + (10.0 - 5.0) * (1.5 - 1.0) / (4.0 - 1.0));
    BOOST_CHECK_EQUAL(udt(4.0), 10.0);
    BOOST_CHECK_EQUAL(udt(4.7), 10.0 + (11.0 - 10.0) * (4.7 - 4.0) / (5.0 - 4.0));
    BOOST_CHECK_EQUAL(udt(5.2), 11.0);
}

BOOST_AUTO_TEST_CASE(UDT_LL)
{
    UDT udt({1.0, 4.0, 5.0}, {5.0, 10.0, 11.0}, UDT::InterpolationType::LinearExtrapolate);

    BOOST_CHECK_EQUAL(udt(0.0), 5.0 + (10.0 - 5.0) * (0.0 - 1.0) / (4.0 - 1.0));
    BOOST_CHECK_EQUAL(udt(1.5), 5.0 + (10.0 - 5.0) * (1.5 - 1.0) / (4.0 - 1.0));
    BOOST_CHECK_EQUAL(udt(4.0), 10.0);
    BOOST_CHECK_EQUAL(udt(4.7), 10.0 + (11.0 - 10.0) * (4.7 - 4.0) / (5.0 - 4.0));
    BOOST_CHECK_EQUAL(udt(5.2), 10.0 + (11.0 - 10.0) * (5.2 - 4.0) / (5.0 - 4.0));
}

BOOST_AUTO_TEST_CASE(ParseUDT_NV)
{
    const std::string input = R"(
RUNSPEC
UDTDIMS
  1 10 10 1 /
SCHEDULE
UDT
  'TEST1' 1/
  'NV' 100.0 500.0 600.0 /
  100.0 180.0 90.0 /
/
/
)";
    Parser parser;
    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec(deck);
    BOOST_CHECK_NO_THROW(Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()));
}

BOOST_AUTO_TEST_CASE(ParseUDT_LC)
{
    const std::string input = R"(
RUNSPEC
UDTDIMS
  1 10 10 1 /
SCHEDULE
UDT
  'TEST1' 1/
  'LC' 100.0 500.0 /
  100.0 180.0 /
/
/
)";
    Parser parser;
    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec(deck);
    BOOST_CHECK_NO_THROW(Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()));
}

BOOST_AUTO_TEST_CASE(ParseUDT_LL)
{
    const std::string input = R"(
RUNSPEC
UDTDIMS
  1 10 10 1 /
SCHEDULE
UDT
  'TEST1' 1/
  'LL' 100.0 500.0 /
  100.0 180.0 /
/
/
)";
    Parser parser;
    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec(deck);
    BOOST_CHECK_NO_THROW(Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()));
}

BOOST_AUTO_TEST_CASE(ParseUDT_NonAscending)
{
    const std::string input = R"(
RUNSPEC
UDTDIMS
  1 10 10 1 /
SCHEDULE
UDT
  'TEST1' 1/
  'LL' 100.0 500.0 200.0 /
  100.0 180.0 13.0 /
/
/
)";
    Parser parser;
    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec(deck);
    BOOST_CHECK_THROW(Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()),
                      Opm::OpmInputError);
}

BOOST_AUTO_TEST_CASE(ParseUDT_SizeMismatch)
{
    const std::string input = R"(
RUNSPEC
UDTDIMS
  1 10 10 1 /
SCHEDULE
UDT
  'TEST1' 1/
  'LL' 100.0 500.0 200.0 /
  100.0 180.0 13.0 15.0 /
/
/
)";
    Parser parser;
    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec(deck);
    BOOST_CHECK_THROW(Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()),
                      Opm::OpmInputError);
}

BOOST_AUTO_TEST_CASE(ParseUDT_Duplicate)
{
    const std::string input = R"(
RUNSPEC
UDTDIMS
  1 10 10 1 /
SCHEDULE
UDT
  'TEST1' 1/
  'LL' 100.0 500.0 500.0 /
  100.0 180.0 13.0 15.0 /
/
/
)";
    Parser parser;
    auto deck = parser.parseString(input);
    EclipseGrid grid(10,10,10);
    TableManager table(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    Runspec runspec(deck);
    BOOST_CHECK_THROW(Schedule schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()),
                      Opm::OpmInputError);
}
