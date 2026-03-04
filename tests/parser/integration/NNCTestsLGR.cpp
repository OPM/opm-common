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

using namespace Opm;


static NNC make_nnc(std::size_t c1, std::size_t c2, double trans)
{
    NNC n;
    n.addNNC(c1, c2, trans);
    return n;
}

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
    NNC nnc = make_nnc(3, 4, 0.5);
    col.addNNC(std::size_t{0}, std::size_t{1}, nnc);

    BOOST_CHECK(col.getNNC(std::size_t{0}, std::size_t{1}) == nnc);
}

BOOST_AUTO_TEST_CASE(order_dependent_test)
{
    // getNNC(g1,g2) and getNNC(g2,g1) must resolve to the same entry.
    NNCCollection col;
    NNC nnc = make_nnc(10, 20, 2.5);
    col.addNNC(std::size_t{2}, std::size_t{5}, nnc);

    BOOST_CHECK(col.getNNC(std::size_t{2}, std::size_t{5}) == nnc);
    BOOST_CHECK_THROW(col.getNNC(std::size_t{5}, std::size_t{2}), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(create_add_retrieve_each)
{
    NNCCollection col;
    NNC n01 = make_nnc(1, 2, 1.0);
    NNC n02 = make_nnc(3, 4, 2.0);
    NNC n12 = make_nnc(5, 6, 3.0);

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
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc(3, 4, 1.0));

    col.getNNC(std::size_t{0}, std::size_t{1}).addNNC(7, 9, 5.0);

    BOOST_CHECK_EQUAL(
        col.getNNC(std::size_t{0}, std::size_t{1}).input().size(), 2u);
}

BOOST_AUTO_TEST_CASE(check_duplicate_input)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc(1, 2, 1.0));
    BOOST_CHECK_THROW(
        col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc(3, 4, 2.0)),
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(check_duplicate_input_inverted)
{
    NNCCollection col;
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc(1, 2, 1.0));
    col.addNNC(std::size_t{1}, std::size_t{0}, make_nnc(3, 4, 2.0));
    BOOST_CHECK(!(col.getNNC(0,1) ==  col.getNNC(1,0)));
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
    NNC n01 = make_nnc(1, 2, 1.0);
    NNC n23 = make_nnc(3, 4, 2.0);

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
    col.addNNC(std::size_t{0}, std::size_t{1}, make_nnc(5, 6, 3.0));

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
