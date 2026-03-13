/*
  Copyright 2015 IRIS
  Copyright 2018--2026 Equinor ASA

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

#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_MODULE NNCCollectionTest

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>


#include <memory>

using namespace Opm;


NNCCollection nnc_collection_opm_lgr_nnc_test()
{
    NNCCollection ncol;
    // Global NNCS
    {
        NNC global_nnc;
        // the first entry is invalid because the host cell is refined
        // it should be filtered out
        global_nnc.addNNC(0, 2, 0.45600000E+01);
        global_nnc.addNNC(0, 3, 0.12340000E+02);
        ncol.addNNC(global_nnc);

    }

    // different type NNCs
    {
        // grid LGR1 and GLOBAL GRID (1,0)

        //NNC sorts the cell indices, so we can add them in any order
        // FOR THIS WE REQUIRE SPECIFIC ORDER, HERE IS THE BUG
        NNCDiffGrid lgr1_nnc;
        lgr1_nnc.addNNC(0, 1,0.78900000E+03);
        lgr1_nnc.addNNC(2, 1,0.78900000E+03);
        lgr1_nnc.addNNC(1, 3,0.78900000E+03);
        lgr1_nnc.addNNC(3, 3,0.78900000E+03);
        ncol.addNNC(1, 0, lgr1_nnc);

    }
   return ncol;
}


NNCCollection nnc_collection_opm_lgr_nnc_same_diff()
{
    NNCCollection ncol;
    // Global NNCS
    {
        NNC global_nnc;
        // the first entry is invalid because the host cell is refined
        // it should be filtered out
        global_nnc.addNNC(0, 2, 0.45600000E+01);
        global_nnc.addNNC(0, 3, 0.12340000E+02);
        ncol.addNNC(global_nnc);
    }

    {
        NNC lgr1_nnc;
        lgr1_nnc.addNNC(0, 1,0.78900000E+03);
        lgr1_nnc.addNNC(2, 1,0.78900000E+03);
        lgr1_nnc.addNNC(1, 3,0.78900000E+03);
        ncol.addNNC(1, lgr1_nnc);
    }

    {
        NNC lgr2_nnc;
        lgr2_nnc.addNNC(0, 1,0.78900000E+03);
        lgr2_nnc.addNNC(2, 1,0.78900000E+03);
        lgr2_nnc.addNNC(1, 3,0.78900000E+03);
        ncol.addNNC(2, lgr2_nnc);
    }

    {
        NNC lgr3_nnc;
        lgr3_nnc.addNNC(0, 1,0.78900000E+03);
        lgr3_nnc.addNNC(2, 1,0.78900000E+03);
        lgr3_nnc.addNNC(1, 3,0.78900000E+03);
        ncol.addNNC(3, lgr3_nnc);
    }


    // different type NNCs
    {
        // NNCL NNCG
        // LGR1 and GLOBAL GRID (1,0)
        //NNC sorts the cell indices, so we can add them in any order
        // FOR THIS WE REQUIRE SPECIFIC ORDER, HERE IS THE BUG
        NNCDiffGrid lgr1_nnc;
        lgr1_nnc.addNNC(0, 1,0.78900000E+03);
        lgr1_nnc.addNNC(2, 1,0.78900000E+03);
        lgr1_nnc.addNNC(1, 3,0.78900000E+03);
        lgr1_nnc.addNNC(3, 3,0.78900000E+03);
        ncol.addNNC(1, 0, lgr1_nnc);
    }

    {
        // NNCL NNCG
        // grid LGR2 and GLOBAL GRID (2,0)
        //NNC sorts the cell indices, so we can add them in any order
        // FOR THIS WE REQUIRE SPECIFIC ORDER, HERE IS THE BUG
        NNCDiffGrid lgr2_nnc;
        lgr2_nnc.addNNC(0, 2,0.78900000E+03);
        lgr2_nnc.addNNC(2, 2,0.78900000E+03);
        lgr2_nnc.addNNC(1, 4,0.78900000E+03);
        lgr2_nnc.addNNC(3, 4,0.78900000E+03);
        ncol.addNNC(2, 0, lgr2_nnc);
    }

    {
        // NNCL NNCG
        // grid LGR2 and GLOBAL GRID (2,0)

        NNCDiffGrid lgr3_nnc;
        lgr3_nnc.addNNC(0, 3,0.78900000E+03);
        lgr3_nnc.addNNC(2, 3,0.78900000E+03);
        ncol.addNNC(3, 0, lgr3_nnc);
    }

    {
        // LGR1 LGR3
        NNCDiffGrid lgr1_lgr2_nnc;
        lgr1_lgr2_nnc.addNNC(0, 1,0.78900000E+03);
        lgr1_lgr2_nnc.addNNC(2, 1,0.78900000E+03);
        lgr1_lgr2_nnc.addNNC(1, 3,0.78900000E+03);
        lgr1_lgr2_nnc.addNNC(3, 3,0.78900000E+03);
        ncol.addNNC(1, 2, lgr1_lgr2_nnc);
    }

    {
        // LGR1 LGR2
        NNCDiffGrid lgr1_lgr3_nnc;
        lgr1_lgr3_nnc.addNNC(0, 1,0.78900000E+03);
        lgr1_lgr3_nnc.addNNC(2, 1,0.78900000E+03);
        lgr1_lgr3_nnc.addNNC(1, 3,0.78900000E+03);
        lgr1_lgr3_nnc.addNNC(3, 3,0.78900000E+03);
        ncol.addNNC(1, 3, lgr1_lgr3_nnc);
    }

   return ncol;
}




const std::string opm_lgr_nnc_test = std::string { R"(
    RUNSPEC
    TITLE
    OPM-LGR-NNC-TEST
    DIMENS
    4 1 1 /
    EQLDIMS
    /
    TABDIMS
    /
    OIL
    GAS
    WATER
    DISGAS
    FIELD
    START
    1 'JAN' 2015 /
    WELLDIMS
    8 8 8 9 /
    LGR
    10 400 10 10 20 /
    UNIFOUT
    GRID
    CARFIN
    'LGR1'  3  3  1  1  1  1  2  2  1 4/
    ENDFIN
    INIT
    DX
        4*2333 /
    DY
        4*3500 /
    DZ
        4*50 /
    TOPS
        4*8325 /
    PORO
        4*0.3 /
    PERMX
        4*500 /
    PERMY
        4*250 /
    PERMZ
        4*200 /
    NNC
    1 1 1 3 1 1 4.56 /  -- HOST CELLS 3 1 1 IS REFINED
    1 1 1 4 1 1 12.34 /  -- REGULAR NNC CELLS IS REFINED
    /
    ECHO
    PROPS
    PVTW
            4017.55 1.038 3.22E-6 0.318 0.0 /
    ROCK
    14.7 3E-6 /
    SWOF
    0.12	0    		 	1	0
    0.18	4.64876033057851E-008	1	0
    0.24	0.000000186		0.997	0
    0.3	4.18388429752066E-007	0.98	0
    0.36	7.43801652892562E-007	0.7	0
    0.42	1.16219008264463E-006	0.35	0
    0.48	1.67355371900826E-006	0.2	0
    0.54	2.27789256198347E-006	0.09	0
    0.6	2.97520661157025E-006	0.021	0
    0.66	3.7654958677686E-006	0.01	0
    0.72	4.64876033057851E-006	0.001	0
    0.78	0.000005625		0.0001	0
    0.84	6.69421487603306E-006	0	0
    0.91	8.05914256198347E-006	0	0
    1	0.00001			0	0 /
    SGOF
    0	0	1	0
    0.001	0	1	0
    0.02	0	0.997	0
    0.05	0.005	0.980	0
    0.12	0.025	0.700	0
    0.2	0.075	0.350	0
    0.25	0.125	0.200	0
    0.3	0.190	0.090	0
    0.4	0.410	0.021	0
    0.45	0.60	0.010	0
    0.5	0.72	0.001	0
    0.6	0.87	0.0001	0
    0.7	0.94	0.000	0
    0.85	0.98	0.000	0
    0.88	0.984	0.000	0 /
    DENSITY
            53.66 64.49 0.0533 /
    PVDG
    14.700	166.666	0.008000
    264.70	12.0930	0.009600
    514.70	6.27400	0.011200
    1014.7	3.19700	0.014000
    2014.7	1.61400	0.018900
    2514.7	1.29400	0.020800
    3014.7	1.08000	0.022800
    4014.7	0.81100	0.026800
    5014.7	0.64900	0.030900
    6014.7	0.58700	0.035900
    7014.7	0.45500	0.039900
    8014.7	0.41200	0.044900
    9014.7	0.38600	0.047000 /
    PVTO
    0.0010	14.7	1.0620	1.0400 /
    0.0905	264.7	1.1500	0.9750 /
    0.1800	514.7	1.2070	0.9100 /
    0.3710	1014.7	1.2950	0.8300 /
    0.6360	2014.7	1.4350	0.6950 /
    0.7750	2514.7	1.5000	0.6410 /
    0.9300	3014.7	1.5650	0.5940 /
    1.2700	4014.7	1.6950	0.5100
            5014.7	1.6650	0.5500
            6014.7	1.6250	0.6100
            7014.7	1.6050	0.6500
            8014.7	1.5850	0.6900
            9014.7	1.5790	0.7400 /
    1.6180	5014.7	1.8270	0.4490
            6014.7	1.8070	0.4890
            7014.7	1.770	0.5490
            8014.7	1.7570	0.5990
            9014.7	1.7370	0.6310 /
    /
    SOLUTION
    EQUIL
        8400 2800 8450 0 8300 0 1 0 0 /
    RSVD
    8300 1.270
    8450 1.270 /
    SUMMARY
    FOPR
    WGOR
    'PROD1'
    'PROD2'
    /
    FGOR
    WBHP
    'INJ'
    'PROD1'
    'PROD2'
    /
    WGIR
    'INJ'
    'PROD1'
    'PROD2'
    /
    WGIT
    'INJ'
    'PROD1'
    'PROD2'
    /
    WGPR
    'INJ'
    'PROD1'
    'PROD2'
    /
    WGPT
    'INJ'
    'PROD1'
    'PROD2'
    /
    WOIR
    'INJ'
    'PROD1'
    'PROD2'
    /
    WOIT
    'INJ'
    'PROD1'
    'PROD2'
    /
    WOPR
    'INJ'
    'PROD1'
    'PROD2'
    /
    WOPT
    'INJ'
    'PROD1'
    'PROD2'
    /
    WWIR
    'INJ'
    'PROD1'
    'PROD2'
    /
    WWIT
    'INJ'
    'PROD1'
    'PROD2'
    /
    WWPR
    'INJ'
    'PROD1'
    'PROD2'
    /
    WWPT
    'INJ'
    'PROD1'
    'PROD2'
    /
    SCHEDULE
    RPTSCHED
        'PRES' 'SGAS' 'RS' 'WELLS' /
    DRSDT
    0 /
    WELSPECL
        'PROD1'	'G1' 'LGR1'	2	1	8400	'OIL' /
        'PROD2'	'G3' 'LGR1'	1	1	8400	'OIL' /
    /
    WELSPECS
        'INJ'	'G2' 	2	1	8335	'GAS' /
    /
    COMPDATL
        'PROD1' 'LGR1'	2	1	1	1	'OPEN'	1*	1*	0.5 /
        'PROD2' 'LGR1'	1	1	1	1	'OPEN'	1*	1*	0.5 /
    /
    COMPDAT
        'INJ'    2	1	1	1	'OPEN'	1*	1*	0.5 /
    /
    WCONPROD
            'PROD1' 'OPEN' 'ORAT' 10000 4* 1000 /
            'PROD2' 'OPEN' 'ORAT' 10000 4* 1000 /

    /
    WCONINJE
        'INJ'	'GAS'	'OPEN'	'RATE'	100000 1* 5014 /
    /
    TSTEP
    0.1 1 31
    /
END
    )"};


static NNC make_nnc(std::size_t c1, std::size_t c2, double trans)
{
    NNC n;
    n.addNNC(c1, c2, trans);
    return n;
}

static NNCDiffGrid make_nnc_diffgrid(std::size_t c1, std::size_t c2, double trans)
{
    NNCDiffGrid n;
    n.addNNC(c1, c2, trans);
    return n;
}


Opm::Deck msw_sim(const std::string& fname)
{
    return Opm::Parser{}.parseFile(fname);
}

struct SimulationCase
{
    explicit SimulationCase(const Opm::Deck& deck)
        : es   { deck }
        , grid { es.getInputGrid() }
        , sched{ deck, es, std::make_shared<Opm::Python>() }
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    // EclipseState calls routines that initialize EclipseGrid LGR cells.
    Opm::EclipseState es;
    Opm::EclipseGrid  grid;
    Opm::Schedule     sched;
};


// ===========================================================================
// Same-grid NNC
// ===========================================================================

BOOST_AUTO_TEST_SUITE(SameGrid)

BOOST_AUTO_TEST_CASE(add_and_get)
{
    NNCCollection col;
    NNC nnc = make_nnc(1, 2, 0.5);
    col.addNNC(std::size_t{0}, nnc);

    BOOST_CHECK(col.getNNC(std::size_t{0}) == nnc);
}

BOOST_AUTO_TEST_CASE(add_populate_and_get_each)
{
    NNCCollection col;
    NNC n0 = make_nnc(1, 2, 1.0);
    NNC n1 = make_nnc(3, 4, 2.0);
    NNC n2 = make_nnc(5, 6, 3.0);

    col.addNNC(std::size_t{0}, n0);
    col.addNNC(std::size_t{1}, n1);
    col.addNNC(std::size_t{2}, n2);

    BOOST_CHECK(col.getNNC(std::size_t{0}) == n0);
    BOOST_CHECK(col.getNNC(std::size_t{1}) == n1);
    BOOST_CHECK(col.getNNC(std::size_t{2}) == n2);
}

BOOST_AUTO_TEST_CASE(check_reference_is_mutable)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, make_nnc(1, 2, 0.5));

    col.getNNC(std::size_t{0}).addNNC(7, 8, 9.9);

    BOOST_CHECK_EQUAL(col.getNNC(std::size_t{0}).input().size(), 2u);
}

BOOST_AUTO_TEST_CASE(check_duplicate_entries)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, make_nnc(1, 2, 1.0));
    BOOST_CHECK_THROW(col.addNNC(std::size_t{0}, make_nnc(3, 4, 2.0)),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(invalid_entry)
{
    NNCCollection col;
    BOOST_CHECK_THROW(col.getNNC(std::size_t{12}), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(check_absent_el)
{
    NNCCollection col;
    BOOST_CHECK(!col.hasSameGridNNC(std::size_t{0}));
}

BOOST_AUTO_TEST_CASE(check_if_inserted_el_exists)
{
    NNCCollection col;
    col.addNNC(std::size_t{5}, make_nnc(1, 2, 1.0));
    BOOST_CHECK(col.hasSameGridNNC(std::size_t{5}));
}

BOOST_AUTO_TEST_SUITE_END()


// ===========================================================================
// Global NNC  (sugar for same-grid grid-0)
// ===========================================================================

BOOST_AUTO_TEST_SUITE(GlobalNNC)

BOOST_AUTO_TEST_CASE(add_and_get)
{
    NNCCollection col;
    NNC nnc = make_nnc(10, 20, 1.5);
    col.addNNC(nnc);

    BOOST_CHECK(col.getGlobalNNC() == nnc);
}

BOOST_AUTO_TEST_CASE(global_equals_grid_zero)
{
    NNCCollection col;
    col.addNNC(make_nnc(1, 2, 3.0));

    BOOST_CHECK(col.getGlobalNNC() == col.getNNC(std::size_t{0}));
}

BOOST_AUTO_TEST_CASE(chexk_if_reference_is_mutable)
{
    NNCCollection col;
    col.addNNC(make_nnc(1, 2, 1.0));

    col.getGlobalNNC().addNNC(5, 6, 2.0);

    BOOST_CHECK_EQUAL(col.getGlobalNNC().input().size(), 2u);
}

BOOST_AUTO_TEST_CASE(check_missing_global)
{
    NNCCollection col;
    BOOST_CHECK_THROW(col.getGlobalNNC(), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()


// ===========================================================================
// Cross-grid NNC
// ===========================================================================

BOOST_AUTO_TEST_SUITE(CrossGrid)

BOOST_AUTO_TEST_CASE(add_and_get)
{
    NNCCollection col;
    NNCDiffGrid nnc = make_nnc_diffgrid(3, 4, 0.5);
    col.addNNC(std::size_t{0}, std::size_t{1}, nnc);

    BOOST_CHECK(col.getNNC(std::size_t{0}, std::size_t{1}) == nnc);
}

BOOST_AUTO_TEST_CASE(order_independent_test)
{
    // getNNC(g1,g2) and getNNC(g2,g1) must resolve to the same entry.
    NNCCollection col;
    NNCDiffGrid nnc = make_nnc_diffgrid(10, 20, 2.5);
    col.addNNC(std::size_t{2}, std::size_t{5}, nnc);

    BOOST_CHECK(col.getNNC(std::size_t{2}, std::size_t{5}) == nnc);
    BOOST_CHECK(col.getNNC(std::size_t{5}, std::size_t{2}) == nnc);
}

BOOST_AUTO_TEST_CASE(create_add_retrieve_each)
{
    NNCCollection col;
    NNCDiffGrid n01 = make_nnc_diffgrid(1, 2, 1.0);
    NNCDiffGrid n02 = make_nnc_diffgrid(3, 4, 2.0);
    NNCDiffGrid n12 = make_nnc_diffgrid(5, 6, 3.0);

    col.addNNC(std::size_t{0}, std::size_t{1}, n01);
    col.addNNC(std::size_t{0}, std::size_t{2}, n02);
    col.addNNC(std::size_t{1}, std::size_t{2}, n12);

    BOOST_CHECK(col.getNNC(std::size_t{0}, std::size_t{1}) == n01);
    BOOST_CHECK(col.getNNC(std::size_t{0}, std::size_t{2}) == n02);
    BOOST_CHECK(col.getNNC(std::size_t{1}, std::size_t{2}) == n12);
}

BOOST_AUTO_TEST_CASE(check_if_returns_mutable_reference)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc_diffgrid(3, 4, 1.0));

    col.getNNC(std::size_t{0}, std::size_t{1}).addNNC(7, 9, 5.0);

    BOOST_CHECK_EQUAL(
        col.getNNC(std::size_t{0}, std::size_t{1}).input().size(), 2u);
}

BOOST_AUTO_TEST_CASE(check_duplicate_input01)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc_diffgrid(1, 2, 1.0));
    BOOST_CHECK_THROW(
        col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc_diffgrid(3, 4, 2.0)),
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(check_duplicate_input_inverted)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc_diffgrid(1, 2, 1.0));
    BOOST_CHECK_THROW(
        col.addNNC(std::size_t{1}, std::size_t{0}, make_nnc_diffgrid(3, 4, 2.0)),
        std::runtime_error);
    BOOST_CHECK((col.getNNC(0,1) ==  col.getNNC(1,0)));
}

BOOST_AUTO_TEST_CASE(get_missing_entry)
{
    NNCCollection col;
    BOOST_CHECK_THROW(
        col.getNNC(std::size_t{99}, std::size_t{100}),
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(has_returns_false_when_absent)
{
    NNCCollection col;
    BOOST_CHECK(!col.hasCrossGridNNC(std::size_t{0}, std::size_t{1}));
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(MapAccessors)

BOOST_AUTO_TEST_CASE(same_grid_map_contains_added_entries)
{
    NNCCollection col;
    NNC n0 = make_nnc(1, 2, 1.0);
    NNC n1 = make_nnc(3, 4, 2.0);

    col.addNNC(std::size_t{0}, n0);
    col.addNNC(std::size_t{1}, n1);

    const auto& m = col.same_grid_nnc();
    BOOST_REQUIRE_EQUAL(m.size(), 2u);
    BOOST_CHECK(m.at(0) == n0);
    BOOST_CHECK(m.at(1) == n1);
}

BOOST_AUTO_TEST_CASE(diff_grid_map_contains_added_entries)
{
    NNCCollection col;
    NNCDiffGrid n01 = make_nnc_diffgrid(1, 2, 1.0);
    NNCDiffGrid n23 = make_nnc_diffgrid(3, 4, 2.0);

    col.addNNC(std::size_t{0}, std::size_t{1}, n01);
    col.addNNC(std::size_t{2}, std::size_t{3}, n23);

    const auto& m = col.diff_grid_nnc();
    BOOST_REQUIRE_EQUAL(m.size(), 2u);
    BOOST_CHECK(m.at({0, 1}) == n01);
    BOOST_CHECK(m.at({2, 3}) == n23);
}

BOOST_AUTO_TEST_CASE(empty_collection_has_empty_maps)
{
    NNCCollection col;
    BOOST_CHECK(col.same_grid_nnc().empty());
    BOOST_CHECK(col.diff_grid_nnc().empty());
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StoreIsolation)

BOOST_AUTO_TEST_CASE(cross_grid_add_does_not_affect_same_grid)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc_diffgrid(5, 6, 3.0));

    BOOST_CHECK_THROW(col.getNNC(std::size_t{0}), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(same_grid_add_does_not_affect_cross_grid)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, make_nnc(1, 2, 1.0));

    BOOST_CHECK_THROW(
        col.getNNC(std::size_t{0}, std::size_t{1}),
        std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(WriteNNCLG)

BOOST_AUTO_TEST_CASE(write_nnc_lgr_test_nnc)
{
    auto deck = Opm::Parser{}.parseString(opm_lgr_nnc_test);
    EclipseState es(deck);
    const auto& egrid = es.getInputGrid();
    auto ncc_col = Opm::NNCCollection{es.getInputNNC()};
    auto nnc_case = nnc_collection_opm_lgr_nnc_test();

    Opm::UnitSystem units(1);  // FIELD

    egrid.save("opm-test.FEGRID", true, nnc_case, units);

    Opm::EclIO::EclFile f("opm-test.FEGRID",
                           Opm::EclIO::EclFile::Formatted{true});


    BOOST_REQUIRE(f.hasKey("NNC1"));
    BOOST_REQUIRE(f.hasKey("NNC2"));

    {
        const auto nnc1 = f.get<int>("NNC1");
        const auto nnc2 = f.get<int>("NNC2");

        BOOST_REQUIRE_EQUAL(nnc1.size(), 2u);
        BOOST_REQUIRE_EQUAL(nnc2.size(), 2u);

        // First entry: cells (0, 2) → (1, 3)
        BOOST_CHECK_EQUAL(nnc1[0], 1);
        BOOST_CHECK_EQUAL(nnc2[0], 3);

        // Second entry: cells (0, 3) → (1, 4)
        BOOST_CHECK_EQUAL(nnc1[1], 1);
        BOOST_CHECK_EQUAL(nnc2[1], 4);
    }

    BOOST_REQUIRE(f.hasKey("NNCL"));
    BOOST_REQUIRE(f.hasKey("NNCG"));

    {
        const auto nncl = f.get<int>("NNCL");
        const auto nncg = f.get<int>("NNCG");

        BOOST_REQUIRE_EQUAL(nncl.size(), 4u);
        BOOST_REQUIRE_EQUAL(nncg.size(), 4u);

        // cell1=[0,2,1,3] = LGR cells → NNCL (1-indexed)
        BOOST_CHECK_EQUAL(nncl[0], 1);  // cell1=0 → 1
        BOOST_CHECK_EQUAL(nncl[1], 3);  // cell1=2 → 3
        BOOST_CHECK_EQUAL(nncl[2], 2);  // cell1=1 → 2
        BOOST_CHECK_EQUAL(nncl[3], 4);  // cell1=3 → 4

        // cell2=[1,1,3,3] = global cells → NNCG (1-indexed)
        BOOST_CHECK_EQUAL(nncg[0], 2);  // cell2=1 → 2
        BOOST_CHECK_EQUAL(nncg[1], 2);  // cell2=1 → 2
        BOOST_CHECK_EQUAL(nncg[2], 4);  // cell2=3 → 4
        BOOST_CHECK_EQUAL(nncg[3], 4);  // cell2=3 → 4
    }

    // --- NNCHEAD for the cross-grid section ---
    // save_nnc_local_global writes nnchead[0]=num_nnc (same-grid LGR count=0),
    // nnchead[1]=grid_num (=1 for LGR1).
    BOOST_REQUIRE(f.hasKey("NNCHEAD"));
    {
        const auto nnchead = f.get<int>("NNCHEAD");
        BOOST_CHECK_EQUAL(nnchead[1], 1);  // grid number = 1 (LGR1)
    }
}
BOOST_AUTO_TEST_SUITE_END()


// ===========================================================================
// Save cross-grid NNCs and verify NNCL / NNCG
// ===========================================================================

BOOST_AUTO_TEST_SUITE(SaveNNCCrossGrid)

BOOST_AUTO_TEST_CASE(local_global_insertion_normalises_to_same_output)
{
    auto deck = Opm::Parser{}.parseString(opm_lgr_nnc_test);
    EclipseState es(deck);
    const auto& egrid = es.getInputGrid();

    NNCCollection col;
    col.addNNC(NNC{});

    NNCDiffGrid cross;
    cross.addNNC(5, 2, 100.0);  // cell1=5=LGR cell, cell2=2=global cell
    // addNNC(grid1=1=LGR, grid2=0=global): grid indices normalise to (0,1),
    // cells are NOT swapped → cell1 stays associated with grid1 (LGR),
    // cell2 stays associated with grid2 (global).
    col.addNNC(std::size_t{1}, std::size_t{0}, cross);

    Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_FIELD);
    egrid.save("cross_lg.FEGRID", true, col, units);

    Opm::EclIO::EclFile f("cross_lg.FEGRID",
                           Opm::EclIO::EclFile::Formatted{true});

    BOOST_REQUIRE(f.hasKey("NNCL"));
    BOOST_REQUIRE(f.hasKey("NNCG"));

    const auto nncl = f.get<int>("NNCL");
    const auto nncg = f.get<int>("NNCG");

    BOOST_REQUIRE_EQUAL(nncl.size(), 1u);
    BOOST_REQUIRE_EQUAL(nncg.size(), 1u);

    // NNCL = cell1+1 = LGR cell 5 → 6
    // NNCG = cell2+1 = global cell 2 → 3
    BOOST_CHECK_EQUAL(nncl[0], 6);
    BOOST_CHECK_EQUAL(nncg[0], 3);
}


// no cross-grid NNC present means NNCL and NNCG in the file.
BOOST_AUTO_TEST_CASE(no_cross_grid_nnc_produces_no_nncl_nncg)
{
    EclipseGrid grid(4, 4, 4);

    NNCCollection col;
    NNC n;
    n.addNNC(0, 15, 1.0);
    col.addNNC(n);

    Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC);
    grid.save("no_cross.FEGRID", true, col, units);

    Opm::EclIO::EclFile f("no_cross.FEGRID",
                           Opm::EclIO::EclFile::Formatted{true});

    BOOST_CHECK(!f.hasKey("NNCL"));
    BOOST_CHECK(!f.hasKey("NNCG"));
}

BOOST_AUTO_TEST_CASE(nnc_collection_opm_lgr_diff)
{
    auto deck = msw_sim("LGR_BASESIM3WELLS.DATA");
    EclipseState es(deck);
    const auto& egrid = es.getInputGrid();
    auto nnc_case = nnc_collection_opm_lgr_nnc_same_diff();
    Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC);
    egrid.save("opm-test-allnnc.FEGRID", true, nnc_case, units);

    Opm::EclIO::EclFile egrid_file("opm-test-allnnc.FEGRID",
                                   Opm::EclIO::EclFile::Formatted{true});

    // Collect all array indices by keyword, in file order
    const auto& array_names = egrid_file.arrayNames();
    std::vector<int> nnchead_idx, nnc1_idx, nnc2_idx;
    std::vector<int> nncl_idx, nncg_idx;
    std::vector<int> nncheada_idx, nna1_idx, nna2_idx;

    for (int i = 0; i < static_cast<int>(array_names.size()); ++i) {
        if      (array_names[i] == "NNCHEAD")  nnchead_idx.push_back(i);
        else if (array_names[i] == "NNC1")     nnc1_idx.push_back(i);
        else if (array_names[i] == "NNC2")     nnc2_idx.push_back(i);
        else if (array_names[i] == "NNCL")     nncl_idx.push_back(i);
        else if (array_names[i] == "NNCG")     nncg_idx.push_back(i);
        else if (array_names[i] == "NNCHEADA") nncheada_idx.push_back(i);
        else if (array_names[i] == "NNA1")    nna1_idx.push_back(i);
        else if (array_names[i] == "NNA2")    nna2_idx.push_back(i);
    }

    // Expect 4 NNC1/NNC2 blocks: global + LGR1 + LGR2 + LGR3
    BOOST_REQUIRE_EQUAL(nnc1_idx.size(), 4u);
    BOOST_REQUIRE_EQUAL(nnc2_idx.size(), 4u);
    // Expect 3 NNCL/NNCG blocks: LGR1-global, LGR2-global, LGR3-global
    BOOST_REQUIRE_EQUAL(nncl_idx.size(), 3u);
    BOOST_REQUIRE_EQUAL(nncg_idx.size(), 3u);
    // Expect 2 NNA blocks: LGR1-LGR2 and LGR1-LGR3
    BOOST_REQUIRE_EQUAL(nncheada_idx.size(), 2u);
    BOOST_REQUIRE_EQUAL(nna1_idx.size(), 2u);
    BOOST_REQUIRE_EQUAL(nna2_idx.size(), 2u);

    // -----------------------------------------------------------------------
    // Global NNC (grid 0): entries (0,2) and (0,3) → 1-indexed: (1,3) (1,4)
    // -----------------------------------------------------------------------
    {
        const auto& nnc1 = egrid_file.get<int>(nnc1_idx[0]);
        const auto& nnc2 = egrid_file.get<int>(nnc2_idx[0]);
        BOOST_REQUIRE_EQUAL(nnc1.size(), 2u);
        BOOST_CHECK_EQUAL(nnc1[0], 1);  BOOST_CHECK_EQUAL(nnc2[0], 3);
        BOOST_CHECK_EQUAL(nnc1[1], 1);  BOOST_CHECK_EQUAL(nnc2[1], 4);
    }

    // -----------------------------------------------------------------------
    // LGR1 same-grid NNC (grid 1)
    // NNC sorts and enforces cell1<=cell2:
    //   addNNC(0,1)→(0,1), addNNC(2,1)→(1,2), addNNC(1,3)→(1,3)
    //   sorted: (0,1),(1,2),(1,3) → NNC1=[1,2,2], NNC2=[2,3,4]
    // -----------------------------------------------------------------------
    {
        const auto& nnc1 = egrid_file.get<int>(nnc1_idx[1]);
        const auto& nnc2 = egrid_file.get<int>(nnc2_idx[1]);
        BOOST_REQUIRE_EQUAL(nnc1.size(), 3u);
        BOOST_CHECK_EQUAL(nnc1[0], 1);  BOOST_CHECK_EQUAL(nnc2[0], 2);
        BOOST_CHECK_EQUAL(nnc1[1], 2);  BOOST_CHECK_EQUAL(nnc2[1], 3);
        BOOST_CHECK_EQUAL(nnc1[2], 2);  BOOST_CHECK_EQUAL(nnc2[2], 4);
    }

    // -----------------------------------------------------------------------
    // LGR2 same-grid NNC (grid 2) — identical structure to LGR1
    // -----------------------------------------------------------------------
    {
        const auto& nnc1 = egrid_file.get<int>(nnc1_idx[2]);
        const auto& nnc2 = egrid_file.get<int>(nnc2_idx[2]);
        BOOST_REQUIRE_EQUAL(nnc1.size(), 3u);
        BOOST_CHECK_EQUAL(nnc1[0], 1);  BOOST_CHECK_EQUAL(nnc2[0], 2);
        BOOST_CHECK_EQUAL(nnc1[1], 2);  BOOST_CHECK_EQUAL(nnc2[1], 3);
        BOOST_CHECK_EQUAL(nnc1[2], 2);  BOOST_CHECK_EQUAL(nnc2[2], 4);
    }

    // -----------------------------------------------------------------------
    // LGR3 same-grid NNC (grid 3) — identical structure to LGR1
    // -----------------------------------------------------------------------
    {
        const auto& nnc1 = egrid_file.get<int>(nnc1_idx[3]);
        const auto& nnc2 = egrid_file.get<int>(nnc2_idx[3]);
        BOOST_REQUIRE_EQUAL(nnc1.size(), 3u);
        BOOST_CHECK_EQUAL(nnc1[0], 1);  BOOST_CHECK_EQUAL(nnc2[0], 2);
        BOOST_CHECK_EQUAL(nnc1[1], 2);  BOOST_CHECK_EQUAL(nnc2[1], 3);
        BOOST_CHECK_EQUAL(nnc1[2], 2);  BOOST_CHECK_EQUAL(nnc2[2], 4);
    }

    // -----------------------------------------------------------------------
    // LGR1-GLOBAL cross-grid (NNCL/NNCG, block 0)
    // addNNC(1, 0, lgr1_nnc) with input (0,1),(2,1),(1,3),(3,3)
    // cell1=[0,2,1,3] → LGR1 cells → NNCL (1-indexed: [1,3,2,4])
    // cell2=[1,1,3,3] → global cells → NNCG (1-indexed: [2,2,4,4])
    // NNCHEAD[0]=num_same_grid_nnc=3, NNCHEAD[1]=1 (LGR1 grid number)
    // -----------------------------------------------------------------------
    {
        const auto& nnchead = egrid_file.get<int>(nnchead_idx[2]);  // 3rd NNCHEAD block
        BOOST_CHECK_EQUAL(nnchead[0], 3);
        BOOST_CHECK_EQUAL(nnchead[1], 1);

        const auto& nncl = egrid_file.get<int>(nncl_idx[0]);
        const auto& nncg = egrid_file.get<int>(nncg_idx[0]);
        BOOST_REQUIRE_EQUAL(nncl.size(), 4u);
        BOOST_CHECK_EQUAL(nncl[0], 1);  BOOST_CHECK_EQUAL(nncg[0], 2);
        BOOST_CHECK_EQUAL(nncl[1], 3);  BOOST_CHECK_EQUAL(nncg[1], 2);
        BOOST_CHECK_EQUAL(nncl[2], 2);  BOOST_CHECK_EQUAL(nncg[2], 4);
        BOOST_CHECK_EQUAL(nncl[3], 4);  BOOST_CHECK_EQUAL(nncg[3], 4);
    }

    // -----------------------------------------------------------------------
    // LGR2-GLOBAL cross-grid (NNCL/NNCG, block 1)
    // addNNC(2, 0, lgr2_nnc) with input (0,2),(2,2),(1,4),(3,4)
    // cell1=[0,2,1,3] → LGR2 cells → NNCL (1-indexed: [1,3,2,4])
    // cell2=[2,2,4,4] → global cells → NNCG (1-indexed: [3,3,5,5])
    // NNCHEAD[1]=2 (LGR2 grid number)
    // -----------------------------------------------------------------------
    {
        const auto& nnchead = egrid_file.get<int>(nnchead_idx[4]);  // 5th NNCHEAD block
        BOOST_CHECK_EQUAL(nnchead[0], 3);
        BOOST_CHECK_EQUAL(nnchead[1], 2);

        const auto& nncl = egrid_file.get<int>(nncl_idx[1]);
        const auto& nncg = egrid_file.get<int>(nncg_idx[1]);
        BOOST_REQUIRE_EQUAL(nncl.size(), 4u);
        BOOST_CHECK_EQUAL(nncl[0], 1);  BOOST_CHECK_EQUAL(nncg[0], 3);
        BOOST_CHECK_EQUAL(nncl[1], 3);  BOOST_CHECK_EQUAL(nncg[1], 3);
        BOOST_CHECK_EQUAL(nncl[2], 2);  BOOST_CHECK_EQUAL(nncg[2], 5);
        BOOST_CHECK_EQUAL(nncl[3], 4);  BOOST_CHECK_EQUAL(nncg[3], 5);
    }

    // -----------------------------------------------------------------------
    // LGR3-GLOBAL cross-grid (NNCL/NNCG, block 2)
    // addNNC(3, 0, lgr3_nnc) with input (0,3),(2,3)
    // cell1=[0,2] → LGR3 cells → NNCL (1-indexed: [1,3])
    // cell2=[3,3] → global cells → NNCG (1-indexed: [4,4])
    // NNCHEAD[1]=3 (LGR3 grid number)
    // -----------------------------------------------------------------------
    {
        const auto& nnchead = egrid_file.get<int>(nnchead_idx[6]);  // 7th NNCHEAD block
        BOOST_CHECK_EQUAL(nnchead[0], 3);
        BOOST_CHECK_EQUAL(nnchead[1], 3);

        const auto& nncl = egrid_file.get<int>(nncl_idx[2]);
        const auto& nncg = egrid_file.get<int>(nncg_idx[2]);
        BOOST_REQUIRE_EQUAL(nncl.size(), 2u);
        BOOST_CHECK_EQUAL(nncl[0], 1);  BOOST_CHECK_EQUAL(nncg[0], 4);
        BOOST_CHECK_EQUAL(nncl[1], 3);  BOOST_CHECK_EQUAL(nncg[1], 4);
    }

    // -----------------------------------------------------------------------
    // NNA: LGR1-LGR2 (first NNA block)
    // addNNC(1, 2, lgr1_lgr2_nnc) with input (0,1),(2,1),(1,3),(3,3)
    // grid1=1 < grid2=2 → no swap_adj
    // NNCHEADA[0]=1 (LGR1), NNCHEADA[1]=2 (LGR2)
    // NNA1=cell1+1=[1,3,2,4], NNA2=cell2+1=[2,2,4,4]
    // -----------------------------------------------------------------------
    {
        const auto& nncheada = egrid_file.get<int>(nncheada_idx[0]);
        BOOST_CHECK_EQUAL(nncheada[0], 1);
        BOOST_CHECK_EQUAL(nncheada[1], 2);

        const auto& nna1 = egrid_file.get<int>(nna1_idx[0]);
        const auto& nna2 = egrid_file.get<int>(nna2_idx[0]);
        BOOST_REQUIRE_EQUAL(nna1.size(), 4u);
        BOOST_CHECK_EQUAL(nna1[0], 1);  BOOST_CHECK_EQUAL(nna2[0], 2);
        BOOST_CHECK_EQUAL(nna1[1], 3);  BOOST_CHECK_EQUAL(nna2[1], 2);
        BOOST_CHECK_EQUAL(nna1[2], 2);  BOOST_CHECK_EQUAL(nna2[2], 4);
        BOOST_CHECK_EQUAL(nna1[3], 4);  BOOST_CHECK_EQUAL(nna2[3], 4);
    }

    // -----------------------------------------------------------------------
    // NNA: LGR1-LGR3 (second NNA block)
    // addNNC(1, 3, lgr1_lgr3_nnc) with input (0,1),(2,1),(1,3),(3,3)
    // grid1=1 < grid2=3 → no swap_adj
    // NNCHEADA[0]=1 (LGR1), NNCHEADA[1]=3 (LGR3)
    // NNA1=cell1+1=[1,3,2,4], NNA2=cell2+1=[2,2,4,4]
    // -----------------------------------------------------------------------
    {
        const auto& nncheada = egrid_file.get<int>(nncheada_idx[1]);
        BOOST_CHECK_EQUAL(nncheada[0], 1);
        BOOST_CHECK_EQUAL(nncheada[1], 3);

        const auto& nna1 = egrid_file.get<int>(nna1_idx[1]);
        const auto& nna2 = egrid_file.get<int>(nna2_idx[1]);
        BOOST_REQUIRE_EQUAL(nna1.size(), 4u);
        BOOST_CHECK_EQUAL(nna1[0], 1);  BOOST_CHECK_EQUAL(nna2[0], 2);
        BOOST_CHECK_EQUAL(nna1[1], 3);  BOOST_CHECK_EQUAL(nna2[1], 2);
        BOOST_CHECK_EQUAL(nna1[2], 2);  BOOST_CHECK_EQUAL(nna2[2], 4);
        BOOST_CHECK_EQUAL(nna1[3], 4);  BOOST_CHECK_EQUAL(nna2[3], 4);
    }
}


BOOST_AUTO_TEST_SUITE_END()
