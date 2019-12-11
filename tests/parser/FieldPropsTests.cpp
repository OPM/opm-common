/*
  Copyright 2019 Equinor ASA.

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

#include <stdexcept>
#include <iostream>
#include <memory>
#include <numeric>

#define BOOST_TEST_MODULE FieldPropsTests

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include "src/opm/parser/eclipse/EclipseState/Grid/FieldProps.hpp"


using namespace Opm;

BOOST_AUTO_TEST_CASE(CreateFieldProps) {
    EclipseGrid grid(10,10,10);
    Deck deck;
    FieldPropsManager fpm(deck, grid, TableManager());
    BOOST_CHECK(!fpm.try_get<double>("PORO"));
    BOOST_CHECK(!fpm.try_get<double>("PORO"));
    BOOST_CHECK_THROW(fpm.get<double>("PORO"), std::out_of_range);
    BOOST_CHECK_THROW(fpm.get_global<double>("PORO"), std::out_of_range);
    BOOST_CHECK_THROW(fpm.try_get<int>("NOT_SUPPORTED"), std::invalid_argument);
    BOOST_CHECK_THROW(fpm.try_get<double>("NOT_SUPPORTED"), std::invalid_argument);
    BOOST_CHECK_THROW(fpm.get<int>("NOT_SUPPORTED"), std::invalid_argument);
    BOOST_CHECK_THROW(fpm.get<double>("NOT_SUPPORTED"), std::invalid_argument);

    BOOST_CHECK_THROW(fpm.get_global<double>("NO1"), std::invalid_argument);
    BOOST_CHECK_THROW(fpm.get_global<int>("NO2"), std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(CreateFieldProps2) {
    std::string deck_string = R"(
GRID

PORO
   1000*0.10 /

BOX
  1 3 1 3 1 3 /

PORV
  27*100 /

ACTNUM
   27*1 /

PERMX
  27*0.6/


)";
    std::vector<int> actnum(1000, 1);
    for (std::size_t i=0; i< 1000; i += 2)
        actnum[i] = 0;
    EclipseGrid grid(EclipseGrid(10,10,10), actnum);
    Deck deck = Parser{}.parseString(deck_string);
    FieldPropsManager fpm(deck, grid, TableManager());

    BOOST_CHECK(!fpm.has<double>("NO-PORO"));
    BOOST_CHECK(fpm.has<double>("PORO"));
    const auto& poro1 = fpm.get<double>("PORO");
    BOOST_CHECK_EQUAL(poro1.size(), grid.getNumActive());

    const auto& poro2 = fpm.try_get<double>("PORO");
    BOOST_CHECK(poro1 == *poro2);

    BOOST_CHECK(!fpm.has<double>("NO-PORO"));

    // PERMX keyword is not fully initialized
    BOOST_CHECK(!fpm.try_get<double>("PERMX"));
    BOOST_CHECK(!fpm.has<double>("PERMX"));

    {
        const auto& keys = fpm.keys<double>();
        BOOST_CHECK_EQUAL(keys.size(), 1);
        BOOST_CHECK(std::find(keys.begin(), keys.end(), "PORO")  != keys.end());
        BOOST_CHECK(std::find(keys.begin(), keys.end(), "PERMX") == keys.end());

        // The PORV property should be extracted with the special function
        // fp.porv() and not the general get<double>() functionality.
        BOOST_CHECK(std::find(keys.begin(), keys.end(), "PORV") == keys.end());
    }
    {
        const auto& keys = fpm.keys<int>();
        BOOST_CHECK_EQUAL(keys.size(), 0);

        BOOST_CHECK(std::find(keys.begin(), keys.end(), "ACTNUM") == keys.end());
    }
}


BOOST_AUTO_TEST_CASE(INVALID_COPY) {
    std::string deck_string = R"(
GRID

COPY
   PERMX PERMY /
/
)";

    EclipseGrid grid(EclipseGrid(10,10,10));
    Deck deck = Parser{}.parseString(deck_string);
    BOOST_CHECK_THROW( FieldPropsManager(deck, grid, TableManager()), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(GRID_RESET) {
    std::string deck_string = R"(
REGIONS

SATNUM
0 1 2 3 4 5 6 7 8
/
)";
    std::vector<int> actnum1 = {1,1,1,0,0,0,1,1,1};
    EclipseGrid grid(3,1,3); grid.resetACTNUM(actnum1);
    Deck deck = Parser{}.parseString(deck_string);
    FieldPropsManager fpm(deck, grid, TableManager());
    const auto& s1 = fpm.get<int>("SATNUM");
    BOOST_CHECK_EQUAL(s1.size(), 6);
    BOOST_CHECK_EQUAL(s1[0], 0);
    BOOST_CHECK_EQUAL(s1[1], 1);
    BOOST_CHECK_EQUAL(s1[2], 2);
    BOOST_CHECK_EQUAL(s1[3], 6);
    BOOST_CHECK_EQUAL(s1[4], 7);
    BOOST_CHECK_EQUAL(s1[5], 8);

    std::vector<int> actnum2 = {1,0,1,0,0,0,1,0,1};
    fpm.reset_actnum(actnum2);

    BOOST_CHECK_EQUAL(s1.size(), 4);
    BOOST_CHECK_EQUAL(s1[0], 0);
    BOOST_CHECK_EQUAL(s1[1], 2);
    BOOST_CHECK_EQUAL(s1[2], 6);
    BOOST_CHECK_EQUAL(s1[3], 8);

    BOOST_CHECK_THROW(fpm.reset_actnum(actnum1), std::logic_error);
}

BOOST_AUTO_TEST_CASE(ADDREG) {
    std::string deck_string = R"(
GRID

PORO
   6*0.1 /

MULTNUM
 2 2 2 1 1 1 /

ADDREG
  PORO 1.0 1 M /
/

)";
    std::vector<int> actnum1 = {1,1,0,0,1,1};
    EclipseGrid grid(3,2,1); grid.resetACTNUM(actnum1);
    Deck deck = Parser{}.parseString(deck_string);
    FieldPropsManager fpm(deck, grid, TableManager());
    const auto& poro = fpm.get<double>("PORO");
    BOOST_CHECK_EQUAL(poro.size(), 4);
    BOOST_CHECK_EQUAL(poro[0], 0.10);
    BOOST_CHECK_EQUAL(poro[3], 1.10);
}



BOOST_AUTO_TEST_CASE(ASSIGN) {
    FieldProps::FieldData<int> data(100);
    std::vector<int> wrong_size(50);

    BOOST_CHECK_THROW( data.default_assign( wrong_size ), std::invalid_argument );

    std::vector<int> ext_data(100);
    std::iota(ext_data.begin(), ext_data.end(), 0);
    data.default_assign( ext_data );

    BOOST_CHECK(data.valid());
    BOOST_CHECK(data.data == ext_data);
}


BOOST_AUTO_TEST_CASE(Defaulted) {
    std::string deck_string = R"(
GRID

BOX
  1 10 1 10 1 1 /

NTG
  100*2 /

)";

    EclipseGrid grid(EclipseGrid(10,10, 2));
    Deck deck = Parser{}.parseString(deck_string);
    FieldPropsManager fpm(deck, grid, TableManager());
    const auto& ntg = fpm.get<double>("NTG");
    const auto& defaulted = fpm.defaulted<double>("NTG");

    for (std::size_t g=0; g < 100; g++) {
        BOOST_CHECK_EQUAL(ntg[g], 2);
        BOOST_CHECK_EQUAL(defaulted[g], false);

        BOOST_CHECK_EQUAL(ntg[g + 100], 1);
        BOOST_CHECK_EQUAL(defaulted[g + 100], true);
    }
}

BOOST_AUTO_TEST_CASE(PORV) {
    std::string deck_string = R"(
GRID

PORO
  400*0.10 /

BOX
  1 10 1 10 2 2 /

NTG
  100*2 /

ENDBOX

EDIT

BOX
  1 10 1 10 4 4 /

MULTPV
  100*4 /


ENDBOX

BOX
  1 10 1 10 3 3 /

PORV
  100*3 /

ENDBOX

)";

    EclipseGrid grid(10,10, 4);
    Deck deck = Parser{}.parseString(deck_string);
    FieldPropsManager fpm(deck, grid, TableManager());
    const auto& poro = fpm.get<double>("PORO");
    const auto& ntg = fpm.get<double>("NTG");
    const auto& multpv = fpm.get<double>("MULTPV");
    const auto& defaulted = fpm.defaulted<double>("PORV");
    const auto& porv = fpm.porv();

    // All cells should be active for this grid
    BOOST_CHECK_EQUAL(porv.size(), grid.getNumActive());
    BOOST_CHECK_EQUAL(porv.size(), grid.getCartesianSize());

    // k = 0: poro * V
    for (std::size_t g = 0; g < 100; g++) {
        BOOST_CHECK_EQUAL(porv[g], grid.getCellVolume(g) * poro[g]);
        BOOST_CHECK_EQUAL(porv[g], 0.10);
        BOOST_CHECK_EQUAL(poro[g], 0.10);
        BOOST_CHECK_EQUAL(ntg[g], 1.0);
        BOOST_CHECK_EQUAL(multpv[g], 1.0);
    }

    // k = 1: poro * NTG * V
    for (std::size_t g = 100; g < 200; g++) {
        BOOST_CHECK_EQUAL(porv[g], grid.getCellVolume(g) * poro[g] * ntg[g]);
        BOOST_CHECK_EQUAL(porv[g], 0.20);
        BOOST_CHECK_EQUAL(poro[g], 0.10);
        BOOST_CHECK_EQUAL(ntg[g], 2.0);
        BOOST_CHECK_EQUAL(multpv[g], 1.0);
    }

    // k = 2: PORV - explicitly set
    for (std::size_t g = 200; g < 300; g++) {
        BOOST_CHECK_EQUAL(poro[g], 0.10);
        BOOST_CHECK_EQUAL(ntg[g], 1.0);
        BOOST_CHECK_EQUAL(multpv[g], 1.0);
        BOOST_CHECK_EQUAL(porv[g],3.0);
    }

    // k = 3: poro * V * multpv
    for (std::size_t g = 300; g < 400; g++) {
        BOOST_CHECK_EQUAL(porv[g], multpv[g] * grid.getCellVolume(g) * poro[g] * ntg[g]);
        BOOST_CHECK_EQUAL(porv[g], 0.40);
        BOOST_CHECK_EQUAL(poro[g], 0.10);
        BOOST_CHECK_EQUAL(ntg[g], 1.0);
        BOOST_CHECK_EQUAL(multpv[g], 4.0);
    }


    std::vector<int> actnum(400, 1);
    actnum[0] = 0;
    grid.resetACTNUM(actnum);

    fpm.reset_actnum(actnum);
    auto porv_global = fpm.porv(true);
    auto porv_active = fpm.porv(false);
    BOOST_CHECK_EQUAL( porv_active.size(), grid.getNumActive());
    BOOST_CHECK_EQUAL( porv_global.size(), grid.getCartesianSize());
    BOOST_CHECK_EQUAL( porv_global[0], 0);
    for (std::size_t g = 1; g < grid.getCartesianSize(); g++) {
        BOOST_CHECK_EQUAL(porv_active[g - 1], porv_global[g]);
        BOOST_CHECK_EQUAL(porv_global[g], porv[g]);
    }
}


BOOST_AUTO_TEST_CASE(LATE_GET_SATFUNC) {
    const char* deckString =
        "RUNSPEC\n"
        "\n"
        "OIL\n"
        "GAS\n"
        "WATER\n"
        "TABDIMS\n"
        "3 /\n"
        "\n"
        "METRIC\n"
        "\n"
        "DIMENS\n"
        "3 3 3 /\n"
        "\n"
        "GRID\n"
        "\n"
        "PERMX\n"
        " 27*1000 /\n"
        "MAXVALUE\n"
        "  PERMX 100 4* 1  1/\n"
        "/\n"
        "MINVALUE\n"
        "  PERMX 10000 4* 3  3/\n"
        "/\n"
        "ACTNUM\n"
        " 0 8*1 0 8*1 0 8*1 /\n"
        "DXV\n"
        "1 1 1 /\n"
        "\n"
        "DYV\n"
        "1 1 1 /\n"
        "\n"
        "DZV\n"
        "1 1 1 /\n"
        "\n"
        "TOPS\n"
        "9*100 /\n"
        "\n"
        "PORO \n"
        "  27*0.15 /\n"
        "PROPS\n"
        "\n"
        "SWOF\n"
        // table 1
        // S_w    k_r,w    k_r,o    p_c,ow
        "  0.1    0        1.0      2.0\n"
        "  0.15   0        0.9      1.0\n"
        "  0.2    0.01     0.5      0.5\n"
        "  0.93   0.91     0.0      0.0\n"
        "/\n"
        // table 2
        // S_w    k_r,w    k_r,o    p_c,ow
        "  0.00   0        1.0      2.0\n"
        "  0.05   0.01     1.0      2.0\n"
        "  0.10   0.02     0.9      1.0\n"
        "  0.15   0.03     0.5      0.5\n"
        "  0.852  1.00     0.0      0.0\n"
        "/\n"
        // table 3
        // S_w    k_r,w    k_r,o    p_c,ow
        "  0.00   0.00     0.9      2.0\n"
        "  0.05   0.02     0.8      1.0\n"
        "  0.10   0.03     0.5      0.5\n"
        "  0.801  1.00     0.0      0.0\n"
        "/\n"
        "\n"
        "SGOF\n"
        // table 1
        // S_g    k_r,g    k_r,o    p_c,og
        "  0.00   0.00     0.9      2.0\n"
        "  0.05   0.02     0.8      1.0\n"
        "  0.10   0.03     0.5      0.5\n"
        "  0.80   1.00     0.0      0.0\n"
        "/\n"
        // table 2
        // S_g    k_r,g    k_r,o    p_c,og
        "  0.05   0.00     1.0      2\n"
        "  0.10   0.02     0.9      1\n"
        "  0.15   0.03     0.5      0.5\n"
        "  0.85   1.00     0.0      0\n"
        "/\n"
        // table 3
        // S_g    k_r,g    k_r,o    p_c,og
        "  0.1    0        1.0      2\n"
        "  0.15   0        0.9      1\n"
        "  0.2    0.01     0.5      0.5\n"
        "  0.9    0.91     0.0      0\n"
        "/\n"
        "\n"
        "REGIONS\n"
        "\n"
        "SATNUM\n"
        "9*1 9*2 9*3 /\n"
        "\n"
        "IMBNUM\n"
        "9*3 9*2 9*1 /\n"
        "\n"
        "SOLUTION\n"
        "\n"
        "SCHEDULE\n";

    Opm::Parser parser;

    auto deck = parser.parseString(deckString);
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg(deck);
    Opm::FieldPropsManager fp(deck, eg, tm);

    const auto& fp_swu = fp.get_global<double>("SWU");
    BOOST_CHECK_EQUAL(fp_swu[1 + 0 * 3*3], 0.93);
    BOOST_CHECK_EQUAL(fp_swu[1 + 1 * 3*3], 0.852);
    BOOST_CHECK_EQUAL(fp_swu[1 + 2 * 3*3], 0.801);

    const auto& fp_sgu = fp.get_global<double>("ISGU");
    BOOST_CHECK_EQUAL(fp_sgu[1 + 0 * 3*3], 0.9);
    BOOST_CHECK_EQUAL(fp_sgu[1 + 1 * 3*3], 0.85);
    BOOST_CHECK_EQUAL(fp_sgu[1 + 2 * 3*3], 0.80);

}

BOOST_AUTO_TEST_CASE(GET_TEMP) {
    std::string deck_string = R"(
GRID

PORO
   200*0.15 /

)";

    EclipseGrid grid(10,10, 2);
    Deck deck = Parser{}.parseString(deck_string);
    std::vector<int> actnum(200, 1); actnum[0] = 0;
    grid.resetACTNUM(actnum);
    FieldPropsManager fpm(deck, grid, TableManager());

    BOOST_CHECK(!fpm.has<double>("NTG"));
    const auto& ntg = fpm.get_copy<double>("NTG");
    BOOST_CHECK(!fpm.has<double>("NTG"));
    BOOST_CHECK(ntg.size() == grid.getNumActive());


    BOOST_CHECK(fpm.has<double>("PORO"));
    const auto& poro = fpm.get_copy<double>("PORO");
    BOOST_CHECK(fpm.has<double>("PORO"));

    BOOST_CHECK(!fpm.has<int>("SATNUM"));
    const auto& satnum = fpm.get_copy<int>("SATNUM", true);
    BOOST_CHECK(!fpm.has<int>("SATNUM"));
    BOOST_CHECK(satnum.size() == grid.getCartesianSize());

    //The PERMY keyword can not be default initialized
    BOOST_CHECK_THROW(fpm.get_copy<double>("PERMY"), std::invalid_argument);
}
