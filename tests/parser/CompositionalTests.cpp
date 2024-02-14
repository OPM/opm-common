/*
Copyright (C) 2024 SINTEF Digital, Mathematics and Cybernetics.

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

#define BOOST_TEST_MODULE Compositional

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Tabdims.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstddef>
#include <initializer_list>
#include <stdexcept>

using namespace Opm;

namespace {

Deck createCompositionalDeck()
{
    return Parser{}.parseString(R"(
------------------------------------------------------------------------
RUNSPEC
------------------------------------------------------------------------
TITLE
   SIMPLE CO2 CASE FOR PARSING TEST

METRIC

TABDIMS
8* 2 2/

COMPS
3 /

------------------------------------------------------------------------
PROPS
------------------------------------------------------------------------

CNAMES
DECANE
CO2
METHANE
/

ROCK
68 0 /

EOS
PR /
SRK /

BIC
0
0 0 /
1
1 1 /

ACF
0.4 0.2 0.01 /
0.5 0.3 0.03 /

PCRIT
20. 70. 40. /
21. 71. 41. /

TCRIT
600. 300. 190. /
601. 301. 191. /

MW
142.  44.  16. /
142.1 44.1 16.1 /

VCRIT
0.6  0.1  0.1 /
0.61 0.11 0.11 /


STCOND
15.0 /

END
)");
}

BOOST_AUTO_TEST_CASE(CompositionalParsingTest) {
    const Deck deck = createCompositionalDeck();
    const Runspec runspec{deck};
    const Tabdims tabdims {deck};
    const CompositionalConfig comp_config{deck, runspec};

    BOOST_CHECK(runspec.compositionalMode());
    BOOST_CHECK_EQUAL(3, runspec.numComps());

    BOOST_CHECK_EQUAL(2, tabdims.getNumEosRes());
    BOOST_CHECK_EQUAL(2, tabdims.getNumEosSur());

    BOOST_CHECK(CompositionalConfig::EOSType::PR == comp_config.eosType(0));
    BOOST_CHECK(CompositionalConfig::EOSType::SRK == comp_config.eosType(1));

    // BOOSE_CHECK_EQUAL()


}

}
