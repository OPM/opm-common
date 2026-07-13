/*
  Copyright (c) 2026 Equinor ASA

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

#define BOOST_TEST_MODULE data_RegionVariableView

#include <boost/test/unit_test.hpp>

#include <opm/output/data/RegionVariableView.hpp>

#include <opm/output/data/RegionsetVariableDescriptor.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

    template <typename Elm>
    std::vector<Elm> iota(const std::size_t n)
    {
        auto v = std::vector<Elm>(n);
        std::iota(v.begin(), v.end(), Elm{});
        return v;
    }

    template <typename Elm>
    std::vector<Elm> reverse(std::vector<Elm>&& v)
    {
        std::reverse(v.begin(), v.end());
        return v;
    }

} // namespace anonymous

BOOST_AUTO_TEST_SUITE(Single_RegSet)

namespace {

    std::vector<int> fipabc()
    {
        return { 1, 2, 3, 3, 2, 1, 1, 1, 1, 1, };
    }

    Opm::data::RegionsetVariableDescriptor descriptor(const int maxID)
    {
        auto descr = Opm::data::RegionsetVariableDescriptor{};

        descr.prepareDescriptorSet();
        descr.addRegionSet(maxID);
        descr.finaliseDescriptorSet();

        return descr;
    }

    Opm::data::RegionsetVariableDescriptor
    descriptor(const std::vector<int>& regset)
    {
        auto descr = Opm::data::RegionsetVariableDescriptor{};

        descr.prepareDescriptorSet();
        descr.addRegionSet(0, regset);
        descr.finaliseDescriptorSet();

        return descr;
    }

} // namespace anonymous

BOOST_AUTO_TEST_CASE(Read_Only_Float)
{
    const auto descr = descriptor(5);
    const auto x = iota<float>(descr.numVariableSlots());

    const auto vals = Opm::data::RegionVariableView { std::span{x}, descr };

    BOOST_CHECK_CLOSE(vals.element(0, 0), 0.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 1), 1.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 2), 2.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 3), 3.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 4), 4.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 5), 5.0f, 1.0e-6f);
}

BOOST_AUTO_TEST_CASE(Read_Only_Double)
{
    const auto descr = descriptor(fipabc());

    const auto x = reverse(iota<double>(descr.numVariableSlots()));

    const auto vals = Opm::data::RegionVariableView {
        std::span{x}, descr
    };

    BOOST_CHECK_CLOSE(vals.element(0, 0), 3.0, 1.0e-8);
    BOOST_CHECK_CLOSE(vals.element(0, 1), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(vals.element(0, 2), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(vals.element(0, 3), 0.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Read_Only_Integer)
{
    const auto descr = descriptor(10);
    const auto x = iota<std::size_t>(descr.numVariableSlots());

    const auto vals = Opm::data::RegionVariableView {
        std::span{x}, descr
    };

    BOOST_CHECK_EQUAL(vals.element(0, 0), std::size_t{ 0});
    BOOST_CHECK_EQUAL(vals.element(0, 1), std::size_t{ 1});
    BOOST_CHECK_EQUAL(vals.element(0, 2), std::size_t{ 2});
    BOOST_CHECK_EQUAL(vals.element(0, 3), std::size_t{ 3});
    BOOST_CHECK_EQUAL(vals.element(0, 4), std::size_t{ 4});
    BOOST_CHECK_EQUAL(vals.element(0, 5), std::size_t{ 5});
    BOOST_CHECK_EQUAL(vals.element(0, 6), std::size_t{ 6});
    BOOST_CHECK_EQUAL(vals.element(0, 7), std::size_t{ 7});
    BOOST_CHECK_EQUAL(vals.element(0, 8), std::size_t{ 8});
    BOOST_CHECK_EQUAL(vals.element(0, 9), std::size_t{ 9});
    BOOST_CHECK_EQUAL(vals.element(0,10), std::size_t{10});
}

BOOST_AUTO_TEST_CASE(Read_Write_Double)
{
    const auto descr = descriptor(fipabc());

    auto x = reverse(iota<double>(descr.numVariableSlots()));

    BOOST_CHECK_CLOSE(x[3], 0.0, 1.0e-8);

    auto vals = Opm::data::RegionVariableView {
        std::span{x}, descr
    };

    vals.element(0, 3) = 42.1729;

    BOOST_CHECK_CLOSE(vals.element(0, 0), 3.0, 1.0e-8);
    BOOST_CHECK_CLOSE(vals.element(0, 1), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(vals.element(0, 2), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(vals.element(0, 3), 42.1729, 1.0e-8);

    BOOST_CHECK_CLOSE(x[3], 42.1729, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Size_Mismatch)
{
    const auto descr = descriptor(5);
    const auto x = iota<std::size_t>(2 * descr.numVariableSlots());

    BOOST_CHECK_THROW(Opm::data::RegionVariableView(std::span{x}, descr),
                      std::logic_error);
}

BOOST_AUTO_TEST_CASE(Size_Mismatch_2)
{
    const auto descr = descriptor(5);
    const auto x = iota<std::size_t>(descr.numVariableSlots());

    BOOST_CHECK_THROW(Opm::data::RegionVariableView(std::span{x}.subspan(0, 1), descr),
                      std::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()     // Single_RegSet

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multiple_Regsets)

namespace {

    Opm::data::RegionsetVariableDescriptor descriptor(const int maxID)
    {
        auto descr = Opm::data::RegionsetVariableDescriptor{};

        descr.prepareDescriptorSet();
        descr.addRegionSet(maxID / 2);
        descr.addRegionSet(maxID);
        descr.finaliseDescriptorSet();

        return descr;
    }

    template <typename Elm>
    std::vector<Elm> concat(std::vector<Elm>&& a, std::vector<Elm>&& b)
    {
        a.insert(a.end(),
                 std::make_move_iterator(b.begin()),
                 std::make_move_iterator(b.end()));

        return a;
    }
}

BOOST_AUTO_TEST_CASE(Read_Only_Float)
{
    const auto descr = descriptor(5); // { Max ID 2, Max ID 5 }

    const auto x = concat(iota<float>(2 + 1), reverse(iota<float>(5 + 1)));

    const auto vals = Opm::data::RegionVariableView {
        std::span{x}, descr
    };

    BOOST_CHECK_CLOSE(vals.element(0, 0), 0.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 1), 1.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(0, 2), 2.0f, 1.0e-6f);

    BOOST_CHECK_CLOSE(vals.element(1, 0), 5.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(1, 1), 4.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(1, 2), 3.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(1, 3), 2.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(1, 4), 1.0f, 1.0e-6f);
    BOOST_CHECK_CLOSE(vals.element(1, 5), 0.0f, 1.0e-6f);
}

BOOST_AUTO_TEST_CASE(Read_Write_Integer)
{
    const auto descr = descriptor(6); // { Max ID 3, Max ID 6 }

    auto x = std::vector {
        271828, -31, 41592, 1729,
        1, -2, 3, -4, 5, -6, -42,
    };

    auto vals = Opm::data::RegionVariableView {
        std::span{x}, descr
    };

    BOOST_CHECK_EQUAL(vals.element(0, 0),  271828);
    BOOST_CHECK_EQUAL(vals.element(0, 1), -    31);
    BOOST_CHECK_EQUAL(vals.element(0, 2),   41592);
    BOOST_CHECK_EQUAL(vals.element(0, 3),    1729);

    BOOST_CHECK_EQUAL(vals.element(1, 0),   1);
    BOOST_CHECK_EQUAL(vals.element(1, 1), - 2);
    BOOST_CHECK_EQUAL(vals.element(1, 2),   3);
    BOOST_CHECK_EQUAL(vals.element(1, 3), - 4);
    BOOST_CHECK_EQUAL(vals.element(1, 4),   5);
    BOOST_CHECK_EQUAL(vals.element(1, 5), - 6);
    BOOST_CHECK_EQUAL(vals.element(1, 6), -42);

    // --------------------------------------------------

    vals.element(0, 2) = 112233;
    vals.element(1, 0) = 445566;
    vals.element(0, 1) =   1618;

    const auto expect = std::vector {
        271828, 1618, 112233, 1729,
        445566, -2, 3, -4, 5, -6, -42,
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(x     .begin(), x     .end(),
                                  expect.begin(), expect.end());
}

BOOST_AUTO_TEST_SUITE_END()     // Multiple_Regsets
