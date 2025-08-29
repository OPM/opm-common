// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2025 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
#define BOOST_TEST_MODULE WellSpeciesTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTracerProperties.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

using namespace Opm;

namespace {

Deck createDeckWithOutSpecies()
{
    return Parser{}.parseString(R"(
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
1000*0.3 /

SCHEDULE
WELSPECS
     'W_1'        'OP'   2   2  1*       'OIL'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
     'W_1' 'WATER' 'OPEN' 'BHP' 1 2 3/
/
END
)");
}

Deck createDeckWithDynamicWSPECIES()
{
    return Parser{}.parseString(R"(
START             -- 0
1 JAN 2000 /
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
1000*0.3 /

SCHEDULE
WELSPECS
     'W_1'        'OP'   1   1  1*       'GAS'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONINJE
     'W_1' 'GAS' 'OPEN' 'BHP' 1 2 3/
/
DATES             -- 1
 1  MAY 2000 /
/
WSPECIES
     'W_1' 'CA'       1 /
     'W_1' 'K'        1 /
/
DATES             -- 2, 3
 1  JUL 2000 /
 1  AUG 2000 /
/
WSPECIES
     'W_1' 'CA'       0 /
/
DATES             -- 4
 1  SEP 2000 /
/

END
)");
}

Deck createDeckWithSpeciesInProducer()
{
    return Parser{}.parseString(R"(
START             -- 0
1 JAN 2000 /
GRID
PERMX
   1000*0.25/
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
  1000*0.3 /

SCHEDULE
WELSPECS
     'W_1'        'OP'   1   1  1*       'GAS'  7* /
/
COMPDAT
 'W_1'  2*  1   1 'OPEN' /
/
WCONPROD
   'W_1' 'OPEN' 'ORAT' 20000  4* 1000 /
WSPECIES
     'W_1' 'CA'       1 /
     'W_1' 'K'        1 /
/
END
)");
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TestNoSpecies)
{
    const auto deck = createDeckWithOutSpecies();

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };

    BOOST_CHECK(!deck.hasKeyword("WSPECIES"));
}


BOOST_AUTO_TEST_CASE(TestDynamicWSPECIES)
{
    const auto deck = createDeckWithDynamicWSPECIES();

    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    const Schedule schedule {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };
    SummaryState st(TimeService::now(), 0.0);

    BOOST_CHECK(deck.hasKeyword("WSPECIES"));

    const auto& keyword = deck["WSPECIES"].back();
    BOOST_CHECK_EQUAL(keyword.size(),1U);

    const auto& record = keyword.getRecord(0);
    const std::string& well_name = record.getItem("WELL").getTrimmedString(0);
    const auto well = WellTracerProperties::Well{"W_1"};
    const auto ca_species = WellTracerProperties::Tracer{"CA"};
    const auto k_species = WellTracerProperties::Tracer{"K"};
    BOOST_CHECK_EQUAL(well_name, "W_1");
    BOOST_CHECK_EQUAL(schedule.getWell(well_name, 0).getSpeciesProperties().getConcentration(well, ca_species, st), 0);
    BOOST_CHECK_EQUAL(schedule.getWell(well_name, 0).getSpeciesProperties().getConcentration(well, k_species, st), 0);
    BOOST_CHECK_EQUAL(schedule.getWell(well_name, 1).getSpeciesProperties().getConcentration(well, ca_species, st), 1);
    BOOST_CHECK_EQUAL(schedule.getWell(well_name, 2).getSpeciesProperties().getConcentration(well, ca_species, st), 1);
    BOOST_CHECK_EQUAL(schedule.getWell(well_name, 4).getSpeciesProperties().getConcentration(well, ca_species, st), 0);
    BOOST_CHECK_EQUAL(schedule.getWell(well_name, 4).getSpeciesProperties().getConcentration(well, k_species, st), 1);
}


BOOST_AUTO_TEST_CASE(TestSpeciesInProducerTHROW)
{
    const auto deck = createDeckWithSpeciesInProducer();
    EclipseGrid grid(10,10,10);
    const TableManager table (deck);
    const FieldPropsManager fp(deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);

    BOOST_CHECK_THROW(Schedule(deck, grid, fp, NumericalAquifers{}, runspec, std::make_shared<Python>()), OpmInputError);
}
