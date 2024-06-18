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

#define BOOST_TEST_MODULE MultiRegTests
#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <stdexcept>
#include <cstdio>

namespace {

Opm::Deck createDeckInvalidArray()
{
    return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 10 10 10 /
GRID
MULTIREG
  MISSING 10 10 M /
/

EDIT
)");
}

Opm::Deck createDeckInvalidRegion()
{
    return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 10 10 10 /
GRID
REGIONS
SATNUM
  1000*1 /
MULTIREG
  SATNUM 10 10 MX /
/
EDIT
)");
}

Opm::Deck createDeckInvalidValue()
{
    return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 10 10 10 /
GRID
REGIONS
SATNUM
  1000*1 /
MULTIREG
  SATNUM 0.2 10 M /
/
EDIT
)");
}

Opm::Deck createDeckMissingVector()
{
    return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 10 10 10 /
GRID
REGIONS
SATNUM
  1000*1 /
MULTIREG
  SATNUM 2 10 M /
/
EDIT
)");
}

Opm::Deck createDeckUnInitialized()
{
    return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 10 10 10 /
GRID
REGIONS
MULTIREG
  SATNUM 2 10 M /
/
EDIT
)");
}

Opm::Deck createValidIntDeck()
{
    return Opm::Parser{}.parseString(R"(RUNSPEC
GRIDOPTS
  'YES'  2 /

DIMENS
 5 5 1 /
GRID
DX
25*1.0 /
DY
25*1.0 /
DZ
25*1.0 /
TOPS
25*0.25 /
PERMY
   25*1.0 /
PERMX
   25*1.0 /
PORO
   25*1.0 /
MULTNUM
1  1  2  2 2
1  1  2  2 2
1  1  2  2 2
1  1  2  2 2
1  1  2  2 2
/
REGIONS
SATNUM
1  1  2  2 2
1  1  2  2 2
1  1  2  2 2
1  1  2  2 2
1  1  2  2 2
/
OPERNUM
1  2  3   4  5
6  7  8   9 10
11 12 13 14 15
16 17 18 19 20
21 22 23 24 25
/
OPERATER
 PERMX 1 MULTX  PERMY 0.50 /
 PERMX 2 COPY   PERMY /
 PORV 1 'MULTX' PORV 0.50 /
/
MULTIREG
  SATNUM 11 1    M /
  SATNUM 20 2      /
/
EDIT
)");
}

} // Anonymous namespace

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(InvalidArrayThrows)
{
    BOOST_CHECK_THROW(Opm::EclipseState{createDeckInvalidArray()}, std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(InvalidRegionThrows)
{
    BOOST_CHECK_THROW(Opm::EclipseState{createDeckInvalidRegion()}, std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ExpectedIntThrows)
{
    BOOST_CHECK_THROW(Opm::EclipseState{createDeckInvalidValue()}, std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(MissingRegionVectorThrows)
{
    BOOST_CHECK_THROW(Opm::EclipseState{createDeckMissingVector()}, std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(UnInitializedVectorThrows)
{
    BOOST_CHECK_THROW(Opm::EclipseState{createDeckUnInitialized()}, std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Test_OPERATER)
{
    const auto deck = createValidIntDeck();

    // Note: the EclipseGrid object must be mutable.
    auto eg = Opm::EclipseGrid { deck };

    const auto tm = Opm::TableManager { deck };
    const auto fp = Opm::FieldPropsManager {
        deck, Opm::Phases{true, true, true}, eg, tm
    };

    const auto& porv  = fp.porv(true);
    const auto& permx = fp.get_global_double("PERMX");
    const auto& permy = fp.get_global_double("PERMY");

    BOOST_CHECK_CLOSE(porv[0], 0.50, 1.0e-8);
    BOOST_CHECK_CLOSE(porv[1], 1.00, 1.0e-8);
    BOOST_CHECK_CLOSE(permx[0] / permy[0], 0.50, 1.0e-8);
    BOOST_CHECK_CLOSE(permx[1], permy[1], 1.0e-8);
}
