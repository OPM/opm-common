/*
  Copyright 2025 Equinor ASA.

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

#define BOOST_TEST_MODULE OperateTests
#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstdio>
#include <stdexcept>

using namespace Opm;

std::string setDeck() {
    std::string deck_string = R"(RUNSPEC
METRIC
DIMENS
14 1 1 /
GRID
DX
14*1 /
DY
14*1 /
DZ
14*1 /
TOPS
14*0 /
PORO
14*0.25 /
PERMX
14*5 /
COPY
PERMX PERMY /
PERMX PERMZ /
/
OPERNUM
1 2 3 4 5 6 7 8 9 10 11 12 13 14 /
OPERATER
PERMX  1 MULTA    PORO 2 3 /
PERMX  2 POLY     PORO 4 5 /
PERMX  3 SLOG     PORO 6 7 /
PERMX  4 LOG10    PORO /
PERMX  5 LOGE     PORO /
PERMX  6 INV      PORO /
PERMX  7 MULTX    PORO 8 /
PERMX  8 ADDX     PORO 9 /
PERMX  9 COPY     PORO /
PERMX 10 MAXLIM   PORO 2 /
PERMX 11 MINLIM   PORO 3 /
PERMX 12 MULTP    PORO 4 5 /
PERMX 13 ABS      PORO /
PERMX 14 MULTIPLY PORO /
/
EDIT
PORV
14*2 /
OPERATER
PORV  1 MULTA    PERMY 2 3 /
PORV  2 POLY     PERMY 4 5 /
PORV  3 SLOG     PERMY 6 7 /
PORV  4 LOG10    PERMY /
PORV  5 LOGE     PERMY /
PORV  6 INV      PERMY /
PORV  7 MULTX    PERMY 8 /
PORV  8 ADDX     PERMY 9 /
PORV  9 COPY     PERMY /
PORV 10 MAXLIM   PERMY 2 /
PORV 11 MINLIM   PERMY 3 /
PORV 12 MULTP    PERMY 4 5 /
PORV 13 ABS      PERMY /
PORV 14 MULTIPLY PERMY /
/
SOLUTION
PRESSURE
14*3 /
OPERATER
PRESSURE  1 MULTA    PERMY 2 3 /
PRESSURE  2 POLY     PERMY 4 5 /
PRESSURE  3 SLOG     PERMY 6 7 /
PRESSURE  4 LOG10    PERMY /
PRESSURE  5 LOGE     PERMY /
PRESSURE  6 INV      PERMY /
PRESSURE  7 MULTX    PERMY 8 /
PRESSURE  8 ADDX     PERMY 9 /
PRESSURE  9 COPY     PERMY /
PRESSURE 10 MAXLIM   PERMY 2 /
PRESSURE 11 MINLIM   PERMY 3 /
PRESSURE 12 MULTP    PERMY 4 5 /
PRESSURE 13 ABS      PERMY /
PRESSURE 14 MULTIPLY PERMY /
/
)";
    return deck_string;
}

BOOST_AUTO_TEST_CASE(Test_metric) {
    UnitSystem unit_system(UnitSystem::UnitType::UNIT_TYPE_METRIC);
    const std::string deck_string = setDeck();
    Parser parser;
    Deck deck = parser.parseString(deck_string);
    TableManager tables(deck);
    EclipseGrid grid(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, tables);

    const auto& porv = fp.porv();
    const auto& permx = fp.get_double("PERMX");
    const auto& permy = fp.get_double("PERMY");
    const auto& poro = fp.get_double("PORO");
    const auto& pressure = fp.get_double("PRESSURE");

    auto perm_to_si = [&unit_system](double raw_value) { return unit_system.to_si(UnitSystem::measure::permeability, raw_value); };
    auto perm_from_si = [&unit_system](double raw_value) { return unit_system.from_si(UnitSystem::measure::permeability, raw_value); };
    auto pres_to_si = [&unit_system](double raw_value) { return unit_system.to_si(UnitSystem::measure::pressure, raw_value); };
    auto volm_to_si = [&unit_system](double raw_value) { return unit_system.to_si(UnitSystem::measure::volume, raw_value); };

    BOOST_CHECK_EQUAL(permx[0],  perm_to_si(2.0 * poro[0] + 3.0));
    BOOST_CHECK_EQUAL(permx[1],  perm_to_si(5.0 + 4.0 * pow(poro[1], 5.0)));
    BOOST_CHECK_EQUAL(permx[2],  perm_to_si(pow(10, 6.0 + 7.0 * poro[2])));
    BOOST_CHECK_EQUAL(permx[3],  perm_to_si(log10(poro[3])));
    BOOST_CHECK_EQUAL(permx[4],  perm_to_si(log(poro[4])));
    BOOST_CHECK_EQUAL(permx[5],  perm_to_si(1.0 / poro[5]));
    BOOST_CHECK_EQUAL(permx[6],  perm_to_si(8.0 * poro[6]));
    BOOST_CHECK_EQUAL(permx[7],  perm_to_si(9.0 + poro[7]));
    BOOST_CHECK_EQUAL(permx[8],  perm_to_si(poro[8]));
    BOOST_CHECK_EQUAL(permx[9],  perm_to_si(std::min(2.0, poro[9])));
    BOOST_CHECK_EQUAL(permx[10], perm_to_si(std::max(3.0, poro[10])));
    BOOST_CHECK_EQUAL(permx[11], perm_to_si(4.0 * pow(poro[11], 5.0)));
    BOOST_CHECK_EQUAL(permx[12], perm_to_si(std::abs(poro[12])));
    BOOST_CHECK_EQUAL(permx[13], perm_to_si(5.0 * poro[13]));

    BOOST_CHECK_EQUAL(porv[0],  volm_to_si(2.0 * perm_from_si(permy[0]) + 3.0));
    BOOST_CHECK_EQUAL(porv[1],  volm_to_si(2.0 + 4.0 * pow(perm_from_si(permy[1]), 5.0)));
    BOOST_CHECK_EQUAL(porv[2],  volm_to_si(pow(10, 6.0 + 7.0 * perm_from_si(permy[2]))));
    BOOST_CHECK_EQUAL(porv[3],  volm_to_si(log10(perm_from_si(permy[3]))));
    BOOST_CHECK_EQUAL(porv[4],  volm_to_si(log(perm_from_si(permy[4]))));
    BOOST_CHECK_EQUAL(porv[5],  volm_to_si(1.0 / perm_from_si(permy[5])));
    BOOST_CHECK_EQUAL(porv[6],  volm_to_si(8.0 * perm_from_si(permy[6])));
    BOOST_CHECK_EQUAL(porv[7],  volm_to_si(9.0 + perm_from_si(permy[7])));
    BOOST_CHECK_EQUAL(porv[8],  volm_to_si(perm_from_si(permy[8])));
    BOOST_CHECK_EQUAL(porv[9],  volm_to_si(std::min(2.0, perm_from_si(permy[9]))));
    BOOST_CHECK_EQUAL(porv[10], volm_to_si(std::max(3.0, perm_from_si(permy[10]))));
    BOOST_CHECK_EQUAL(porv[11], volm_to_si(4.0 * pow(perm_from_si(permy[11]), 5.0)));
    BOOST_CHECK_EQUAL(porv[12], volm_to_si(std::abs(perm_from_si(permy[12]))));
    BOOST_CHECK_EQUAL(porv[13], volm_to_si(2.0 * perm_from_si(permy[13])));

    BOOST_CHECK_EQUAL(pressure[0],  pres_to_si(2.0 * perm_from_si(permy[0]) + 3.0));
    BOOST_CHECK_EQUAL(pressure[1],  pres_to_si(3.0 + 4.0 * pow(perm_from_si(permy[1]), 5.0)));
    BOOST_CHECK_EQUAL(pressure[2],  pres_to_si(pow(10, 6.0 + 7.0 * perm_from_si(permy[2]))));
    BOOST_CHECK_EQUAL(pressure[3],  pres_to_si(log10(perm_from_si(permy[3]))));
    BOOST_CHECK_EQUAL(pressure[4],  pres_to_si(log(perm_from_si(permy[4]))));
    BOOST_CHECK_EQUAL(pressure[5],  pres_to_si(1.0 / perm_from_si(permy[5])));
    BOOST_CHECK_EQUAL(pressure[6],  pres_to_si(8.0 * perm_from_si(permy[6])));
    BOOST_CHECK_EQUAL(pressure[7],  pres_to_si(9.0 + perm_from_si(permy[7])));
    BOOST_CHECK_EQUAL(pressure[8],  pres_to_si(perm_from_si(permy[8])));
    BOOST_CHECK_EQUAL(pressure[9],  pres_to_si(std::min(2.0, perm_from_si(permy[9]))));
    BOOST_CHECK_EQUAL(pressure[10], pres_to_si(std::max(3.0, perm_from_si(permy[10]))));
    BOOST_CHECK_EQUAL(pressure[11], pres_to_si(4.0 * pow(perm_from_si(permy[11]), 5.0)));
    BOOST_CHECK_EQUAL(pressure[12], pres_to_si(std::abs(perm_from_si(permy[12]))));
    BOOST_CHECK_CLOSE(pressure[13], pres_to_si(3.0 * perm_from_si(permy[13])), 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Test_field) {
    UnitSystem unit_system(UnitSystem::UnitType::UNIT_TYPE_FIELD);
    std::string deck_string = setDeck();
    deck_string.replace(8, 6, "FIELD");
    Parser parser;
    Deck deck = parser.parseString(deck_string);
    TableManager tables(deck);
    EclipseGrid grid(deck);
    FieldPropsManager fp(deck, Phases{true, true, true}, grid, tables);

    const auto& porv = fp.porv();
    const auto& permx = fp.get_double("PERMX");
    const auto& permy = fp.get_double("PERMY");
    const auto& poro = fp.get_double("PORO");
    const auto& pressure = fp.get_double("PRESSURE");

    auto perm_to_si = [&unit_system](double raw_value) { return unit_system.to_si(UnitSystem::measure::permeability, raw_value); };
    auto perm_from_si = [&unit_system](double raw_value) { return unit_system.from_si(UnitSystem::measure::permeability, raw_value); };
    auto pres_to_si = [&unit_system](double raw_value) { return unit_system.to_si(UnitSystem::measure::pressure, raw_value); };
    auto volm_to_si = [&unit_system](double raw_value) { return unit_system.to_si(UnitSystem::measure::volume, raw_value); };

    BOOST_CHECK_EQUAL(permx[0],  perm_to_si(2.0 * poro[0] + 3.0));
    BOOST_CHECK_EQUAL(permx[1],  perm_to_si(5.0 + 4.0 * pow(poro[1], 5.0)));
    BOOST_CHECK_EQUAL(permx[2],  perm_to_si(pow(10, 6.0 + 7.0 * poro[2])));
    BOOST_CHECK_EQUAL(permx[3],  perm_to_si(log10(poro[3])));
    BOOST_CHECK_EQUAL(permx[4],  perm_to_si(log(poro[4])));
    BOOST_CHECK_EQUAL(permx[5],  perm_to_si(1.0 / poro[5]));
    BOOST_CHECK_EQUAL(permx[6],  perm_to_si(8.0 * poro[6]));
    BOOST_CHECK_EQUAL(permx[7],  perm_to_si(9.0 + poro[7]));
    BOOST_CHECK_EQUAL(permx[8],  perm_to_si(poro[8]));
    BOOST_CHECK_EQUAL(permx[9],  perm_to_si(std::min(2.0, poro[9])));
    BOOST_CHECK_EQUAL(permx[10], perm_to_si(std::max(3.0, poro[10])));
    BOOST_CHECK_EQUAL(permx[11], perm_to_si(4.0 * pow(poro[11], 5.0)));
    BOOST_CHECK_EQUAL(permx[12], perm_to_si(std::abs(poro[12])));
    BOOST_CHECK_EQUAL(permx[13], perm_to_si(5.0 * poro[13]));

    BOOST_CHECK_EQUAL(porv[0],  volm_to_si(2.0 * perm_from_si(permy[0]) + 3.0));
    BOOST_CHECK_EQUAL(porv[1],  volm_to_si(2.0 + 4.0 * pow(perm_from_si(permy[1]), 5.0)));
    BOOST_CHECK_EQUAL(porv[2],  volm_to_si(pow(10, 6.0 + 7.0 * perm_from_si(permy[2]))));
    BOOST_CHECK_EQUAL(porv[3],  volm_to_si(log10(perm_from_si(permy[3]))));
    BOOST_CHECK_EQUAL(porv[4],  volm_to_si(log(perm_from_si(permy[4]))));
    BOOST_CHECK_EQUAL(porv[5],  volm_to_si(1.0 / perm_from_si(permy[5])));
    BOOST_CHECK_EQUAL(porv[6],  volm_to_si(8.0 * perm_from_si(permy[6])));
    BOOST_CHECK_EQUAL(porv[7],  volm_to_si(9.0 + perm_from_si(permy[7])));
    BOOST_CHECK_EQUAL(porv[8],  volm_to_si(perm_from_si(permy[8])));
    BOOST_CHECK_EQUAL(porv[9],  volm_to_si(std::min(2.0, perm_from_si(permy[9]))));
    BOOST_CHECK_EQUAL(porv[10], volm_to_si(std::max(3.0, perm_from_si(permy[10]))));
    BOOST_CHECK_EQUAL(porv[11], volm_to_si(4.0 * pow(perm_from_si(permy[11]), 5.0)));
    BOOST_CHECK_EQUAL(porv[12], volm_to_si(std::abs(perm_from_si(permy[12]))));
    BOOST_CHECK_EQUAL(porv[13], volm_to_si(2.0 * perm_from_si(permy[13])));

    BOOST_CHECK_EQUAL(pressure[0],  pres_to_si(2.0 * perm_from_si(permy[0]) + 3.0));
    BOOST_CHECK_EQUAL(pressure[1],  pres_to_si(3.0 + 4.0 * pow(perm_from_si(permy[1]), 5.0)));
    BOOST_CHECK_EQUAL(pressure[2],  pres_to_si(pow(10, 6.0 + 7.0 * perm_from_si(permy[2]))));
    BOOST_CHECK_EQUAL(pressure[3],  pres_to_si(log10(perm_from_si(permy[3]))));
    BOOST_CHECK_EQUAL(pressure[4],  pres_to_si(log(perm_from_si(permy[4]))));
    BOOST_CHECK_EQUAL(pressure[5],  pres_to_si(1.0 / perm_from_si(permy[5])));
    BOOST_CHECK_EQUAL(pressure[6],  pres_to_si(8.0 * perm_from_si(permy[6])));
    BOOST_CHECK_EQUAL(pressure[7],  pres_to_si(9.0 + perm_from_si(permy[7])));
    BOOST_CHECK_EQUAL(pressure[8],  pres_to_si(perm_from_si(permy[8])));
    BOOST_CHECK_EQUAL(pressure[9],  pres_to_si(std::min(2.0, perm_from_si(permy[9]))));
    BOOST_CHECK_EQUAL(pressure[10], pres_to_si(std::max(3.0, perm_from_si(permy[10]))));
    BOOST_CHECK_EQUAL(pressure[11], pres_to_si(4.0 * pow(perm_from_si(permy[11]), 5.0)));
    BOOST_CHECK_EQUAL(pressure[12], pres_to_si(std::abs(perm_from_si(permy[12]))));
    BOOST_CHECK_CLOSE(pressure[13], pres_to_si(3.0 * perm_from_si(permy[13])), 1.0e-10);
}
