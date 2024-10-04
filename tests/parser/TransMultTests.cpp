/*
  Copyright 2014 Statoil ASA.

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

#define BOOST_TEST_MODULE EclipseGridTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/TransMult.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>
#include <opm/input/eclipse/EclipseState/Grid/TransMult.hpp>

#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(Empty)
{
    Opm::EclipseGrid grid(10,10,10);
    Opm::FieldPropsManager fp(Opm::Deck(), Opm::Phases{true, true, true}, grid, Opm::TableManager());
    Opm::TransMult transMult(grid ,{} , fp);

    BOOST_CHECK_THROW( transMult.getMultiplier(12,10,10 , Opm::FaceDir::XPlus) , std::invalid_argument );
    BOOST_CHECK_THROW( transMult.getMultiplier(1000 , Opm::FaceDir::XPlus) , std::invalid_argument );

    BOOST_CHECK_EQUAL( transMult.getMultiplier(9,9,9, Opm::FaceDir::YPlus) , 1.0 );
    BOOST_CHECK_EQUAL( transMult.getMultiplier(100 , Opm::FaceDir::ZPlus) , 1.0 );

    BOOST_CHECK_EQUAL( transMult.getMultiplier(9,9,9, Opm::FaceDir::YMinus) , 1.0 );
    BOOST_CHECK_EQUAL( transMult.getMultiplier(100 , Opm::FaceDir::ZMinus) , 1.0 );
}


BOOST_AUTO_TEST_CASE(GridAndEdit)
{
    const std::string deck_string = R"(
RUNSPEC
GRIDOPTS
  'YES'  2 /

DIMENS
 5 5 5 /
GRID
MULTZ
  125*2 /
EDIT
MULTZ
  125*2 /
)";

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::TableManager tables(deck);
    Opm::EclipseGrid grid(5,5,5);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, grid, tables);
    Opm::TransMult transMult(grid, deck, fp);

    transMult.applyMULT(fp.get_global_double("MULTZ"), Opm::FaceDir::ZPlus);
    BOOST_CHECK_EQUAL( transMult.getMultiplier(0,0,0 , Opm::FaceDir::ZPlus) , 4.0 );
}

BOOST_AUTO_TEST_SUITE_END() // Basic_Operations

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(EqualReg_MultX)

namespace {
    double getMultiplier(std::string_view tailMultSpec,
                         std::string_view headMultSpec = "")
    {
        return Opm::EclipseState {
            Opm::Parser{}.parseString(fmt::format(R"(RUNSPEC
DIMENS
  1 5 1 /
GRID
DXV
  100.0 /
DYV
  5*100.0 /
DZV
  5.0 /
DEPTHZ
  12*2000.0 /
PERMX
  5*100.0 /
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
MULTIPLY
  PERMZ 0.1 /
/
PORO
  5*0.3 /
MULTNUM
  1 1 2 2 3 /
FLUXNUM
  1 2 3 4 5 /
FAULTS
  'T' 1 1  2 2  1 1 'Y' /
/
{}
MULTFLT
  'T' 0.123 /
/
{}
END
)", headMultSpec, tailMultSpec))
        }
            .getTransMult()
            .getMultiplier(0, 1, 0, Opm::FaceDir::YPlus);
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(FaultMultiplier_Only)
{
    const auto tmult = getMultiplier("");

    BOOST_CHECK_CLOSE(tmult, 0.123, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Explicit_MultY)
{
    const auto tmult = getMultiplier(R"(
MULTY
  5*0.1 /
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.1, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(EqualReg_MultY)
{
    const auto tmult = getMultiplier(R"(
EQUALREG
  'MULTY' 0.15 2 'F' /
/
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.15, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(EqualReg_MultY_Reordered)
{
    // Same as EqualReg_MultY, except EQUALREG happens before MULTFLT (tail
    // = "", head = R"(...)")
    const auto tmult = getMultiplier("", R"(
EQUALREG
  'MULTY' 0.15 2 'F' /
/
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.15, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(EqualReg_Overrides_Multiply)
{
    const auto tmult = getMultiplier(R"(
MULTIPLY
  'MULTY' 5.2 /
/
EQUALREG
  'MULTY' 0.25 2 'F' /
/
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.25, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(EqualReg_Compounds_Multiply)
{
    const auto tmult = getMultiplier(R"(
EQUALREG
  'MULTY' 0.25 2 'F' /
/
MULTIPLY
  'MULTY' 5.2 /
/
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.25 * 5.2, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(EqualReg_Twice)
{
    const auto tmult = getMultiplier(R"(
EQUALREG
  'MULTY' 0.25 2 'F' /
/
MULTIPLY
  'MULTY' 5.2 /
/
EQUALREG
  'MULTY' 0.42 1 'M' /
/
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.42, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(EqualReg_Twice_MultY_Overrides)
{
    const auto tmult = getMultiplier(R"(
EQUALREG
  'MULTY' 0.25 2 'F' /
/
MULTIPLY
  'MULTY' 5.2 /
/
EQUALREG
  'MULTY' 0.42 1 'M' /
/
MULTY
  5*0.32 /
)");

    BOOST_CHECK_CLOSE(tmult, 0.123 * 0.32, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // EqualReg_MultX
