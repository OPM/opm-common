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
#define BOOST_TEST_MODULE GeochemistryTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Geochemistry/SpeciesConfig.hpp>
#include <opm/input/eclipse/EclipseState/Geochemistry/MineralConfig.hpp>
#include <opm/input/eclipse/EclipseState/Geochemistry/IonExchangeConfig.hpp>

using namespace Opm;

static Deck createGeochemDeck()
{
    return Parser{}.parseString(R"(
        RUNSPEC
        GEOCHEM
        test.json 1e-3 1e-4 CHARGE 10 /
        )");
}

static Deck createSpeciesDeck()
{
    return Parser{}.parseString(R"(
        GEOCHEM
        /
        DIMENS
        3 3 3/
        TABDIMS
        /
        EQLDIMS
        /

        GRID
        DX
        27*1.0 /
        DY
        27*1.0 /
        DZ
        27*1.0 /
        TOPS
        9*10.0 /

        PROPS
        SPECIES
        CA NA /

        SOLUTION
        SBLKCA
        27*1e-3 /
        SVDPNA
        5.0  1e-4
        10.0 1e-4 /
        )");
}

static Deck createMineralDeck()
{
    return Parser{}.parseString(R"(
        GEOCHEM
        /
        DIMENS
        3 3 3/
        TABDIMS
        /
        EQLDIMS
        /

        GRID
        DX
        27*1.0 /
        DY
        27*1.0 /
        DZ
        27*1.0 /
        TOPS
        9*10.0 /

        PROPS
        MINERAL
        --calcite   magnesite
        CALCITE     MGS /

        SOLUTION
        MBLKCALCITE
        27*0.75 /
        MVDPMGS
        5.0  0.25
        10.0 0.25 /
        )");
}

static Deck createIonExchangeDeck()
{
    return Parser{}.parseString(R"(
        GEOCHEM
        /
        DIMENS
        3 3 3/
        TABDIMS
        /
        EQLDIMS
        /

        GRID
        DX
        27*1.0 /
        DY
        27*1.0 /
        DZ
        27*1.0 /
        TOPS
        9*10.0 /

        PROPS
        IONEX
        X Y /

        SOLUTION
        IBLKX
        27*3e-3 /
        IVDPY
        5.0  3e-4
        10.0 3e-4 /
        )");
}

static Deck createSpeciesNameCheckDeck()
{
    return Parser{}.parseString(R"(
        GEOCHEM
        /
        DIMENS
        3 3 3/
        TABDIMS
        /
        EQLDIMS
        /

        GRID
        DX
        27*1.0 /
        DY
        27*1.0 /
        DZ
        27*1.0 /
        TOPS
        9*10.0 /

        PROPS
        SPECIES
        CA CA /

        MINERAL
        MAGNESITE /

        IONEX
        XCHNGR XCHNGER /
        )");
}

BOOST_AUTO_TEST_CASE(GeochemDeck) {
    const auto deck = createGeochemDeck();
    Runspec runspec(deck);

    const auto& geochem = runspec.geochem();
    const std::string file_name = "test.json";
    BOOST_CHECK(geochem.enabled());
    BOOST_CHECK_EQUAL(geochem.geochem_file_name(), file_name);
    BOOST_CHECK_CLOSE(geochem.mbal_tol(), 1e-3, 1e-8);
    BOOST_CHECK_CLOSE(geochem.ph_tol(), 1e-4, 1e-8);
    BOOST_CHECK_EQUAL(geochem.splay_tree_resolution(), 10);
    BOOST_CHECK(geochem.charge_balance());
    BOOST_CHECK(geochem.enabled());
}

BOOST_AUTO_TEST_CASE(SpeciesConfigDeck) {
    const auto deck = createSpeciesDeck();
    EclipseState state(deck);

    const SpeciesConfig& species = state.species();
    BOOST_CHECK_EQUAL(species.size(), 2U);

    const auto& ca_species = species["CA"];
    BOOST_CHECK_EQUAL(ca_species.name, "CA");
    BOOST_CHECK(ca_species.concentration.has_value());
    BOOST_CHECK(!ca_species.svdp.has_value());
    for (const auto& elem : ca_species.concentration.value()) {
        BOOST_CHECK_CLOSE(elem, 1e-3, 1e-5);
    }

    const auto& na_species = species[1];
    double zPos = 7.5;
    BOOST_CHECK_EQUAL(na_species.name, "NA");
    BOOST_CHECK(!na_species.concentration.has_value());
    BOOST_CHECK(na_species.svdp.has_value());
    BOOST_CHECK_EQUAL(na_species.svdp.value().numColumns(), 2U);
    BOOST_CHECK_CLOSE(na_species.svdp.value().evaluate("SPECIES_CONCENTRATION", zPos), 1e-4, 1e-5);
}

BOOST_AUTO_TEST_CASE(MineralConfigDeck) {
    const auto deck = createMineralDeck();
    EclipseState state(deck);

    const MineralConfig& mineral = state.mineral();
    BOOST_CHECK_EQUAL(mineral.size(), 2U);

    const auto& calcite = mineral["CALCITE"];
    BOOST_CHECK_EQUAL(calcite.name, "CALCITE");
    BOOST_CHECK(calcite.concentration.has_value());
    BOOST_CHECK(!calcite.svdp.has_value());
    for (const auto& elem : calcite.concentration.value()) {
        BOOST_CHECK_CLOSE(elem, 0.75, 1e-5);
    }

    const auto& magnesite = mineral[1];
    double zPos = 7.5;
    BOOST_CHECK_EQUAL(magnesite.name, "MGS");
    BOOST_CHECK(!magnesite.concentration.has_value());
    BOOST_CHECK(magnesite.svdp.has_value());
    BOOST_CHECK_EQUAL(magnesite.svdp.value().numColumns(), 2U);
    BOOST_CHECK_CLOSE(magnesite.svdp.value().evaluate("SPECIES_CONCENTRATION", zPos), 0.25, 1e-5);
}

BOOST_AUTO_TEST_CASE(IonExchangeConfigDeck) {
    const auto deck = createIonExchangeDeck();
    EclipseState state(deck);

    const IonExchangeConfig& ionex = state.ionExchange();
    BOOST_CHECK_EQUAL(ionex.size(), 2U);

    const auto& x = ionex["X"];
    BOOST_CHECK_EQUAL(x.name, "X");
    BOOST_CHECK(x.concentration.has_value());
    BOOST_CHECK(!x.svdp.has_value());
    for (const auto& elem : x.concentration.value()) {
        BOOST_CHECK_CLOSE(elem, 3e-3, 1e-5);
    }

    const auto& y = ionex[1];
    double zPos = 7.5;
    BOOST_CHECK_EQUAL(y.name, "Y");
    BOOST_CHECK(!y.concentration.has_value());
    BOOST_CHECK(y.svdp.has_value());
    BOOST_CHECK_EQUAL(y.svdp.value().numColumns(), 2U);
    BOOST_CHECK_CLOSE(y.svdp.value().evaluate("SPECIES_CONCENTRATION", zPos), 3e-4, 1e-5);
}

BOOST_AUTO_TEST_CASE(SpeciesNameCheck) {
    const auto deck = createSpeciesNameCheckDeck();

    // Duplicate error
    BOOST_CHECK_THROW(SpeciesConfig{deck}, std::runtime_error);

    // Long item name error
    BOOST_CHECK_THROW(MineralConfig{deck}, std::runtime_error);

    // Same first four character error
    BOOST_CHECK_THROW(IonExchangeConfig{deck}, std::runtime_error);

}
