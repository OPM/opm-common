// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:

/*
  Copyright 2025 Equinor

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

#define BOOST_TEST_MODULE Completed_Cells_Collection

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>              // std::pair<>

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(Try_Get_Insert_New)
{
    auto cc = Opm::CompletedCells { 10, 10, 3 };

    auto [cellPtr, is_existing_cell] = cc.try_get(0, 0, 0);
    BOOST_REQUIRE_MESSAGE(cellPtr != nullptr, "Try_get() must generate cell object");

    BOOST_CHECK_MESSAGE(! is_existing_cell, "New cell must not be existing cell");

    BOOST_CHECK_EQUAL(cellPtr->global_index, std::size_t{0});
    BOOST_CHECK_EQUAL(cellPtr->i, std::size_t{0});
    BOOST_CHECK_EQUAL(cellPtr->j, std::size_t{0});
    BOOST_CHECK_EQUAL(cellPtr->k, std::size_t{0});

    BOOST_CHECK_CLOSE(cellPtr->depth, 0.0, 1.0e-8);

    BOOST_CHECK_CLOSE(cellPtr->dimensions[0], 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(cellPtr->dimensions[1], 0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(cellPtr->dimensions[2], 0.0, 1.0e-8);

    BOOST_CHECK_MESSAGE(! cellPtr->props.has_value(),
                        "New cell object must not have property data");

    BOOST_CHECK_MESSAGE(! cellPtr->is_active(), "New cell object must not be active");
}

BOOST_AUTO_TEST_CASE(Try_Get_Existing)
{
    auto cc = Opm::CompletedCells { 10, 10, 3 };

    {
        auto cellPair = cc.try_get(0, 0, 0);

        auto& props = cellPair.first->props
            .emplace(Opm::CompletedCells::Cell::Props{});

        props.active_index = 1729;
        props.permx = 1.1;
        props.permy = 2.2;
        props.permz = 3.3;
        props.poro = -0.123;
        props.ntg = 5;

        props.satnum = 42;
        props.pvtnum = -1;
    }

    auto [cellPtr, is_existing_cell] = cc.try_get(0, 0, 0);
    BOOST_REQUIRE_MESSAGE(cellPtr != nullptr, "Try_get() existing must generate cell object");

    BOOST_CHECK_MESSAGE(is_existing_cell, "Existing cell must be tagged as such");

    BOOST_CHECK_MESSAGE(cellPtr->props.has_value(),
                        "Existing cell object must have property data");

    BOOST_CHECK_MESSAGE(cellPtr->is_active(), "Existing cell object must be active");

    BOOST_CHECK_EQUAL(cellPtr->active_index(), 1729);
    BOOST_CHECK_EQUAL(cellPtr->props->active_index, 1729);

    BOOST_CHECK_CLOSE(cellPtr->props->permx,  1.1  , 1.0e-8);
    BOOST_CHECK_CLOSE(cellPtr->props->permy,  2.2  , 1.0e-8);
    BOOST_CHECK_CLOSE(cellPtr->props->permz,  3.3  , 1.0e-8);
    BOOST_CHECK_CLOSE(cellPtr->props->poro , -0.123, 1.0e-8);
    BOOST_CHECK_CLOSE(cellPtr->props->ntg  ,  5.0  , 1.0e-8);

    BOOST_CHECK_EQUAL(cellPtr->props->satnum, 42);
    BOOST_CHECK_EQUAL(cellPtr->props->pvtnum, -1);
}

BOOST_AUTO_TEST_CASE(Get_Existing)
{
    auto cc = Opm::CompletedCells { 10, 10, 3 };

    {
        auto cellPair = cc.try_get(0, 0, 0);

        auto& props = cellPair.first->props
            .emplace(Opm::CompletedCells::Cell::Props{});

        props.active_index = 1729;
        props.permx = 1.1;
        props.permy = 2.2;
        props.permz = 3.3;
        props.poro = -0.123;
        props.ntg = 5;

        props.satnum = 42;
        props.pvtnum = -1;
    }

    const auto& cell = cc.get(0, 0, 0);

    BOOST_CHECK_MESSAGE(cell.props.has_value(),
                        "Existing cell object must have property data");

    BOOST_CHECK_MESSAGE(cell.is_active(), "Existing cell object must be active");

    BOOST_CHECK_EQUAL(cell.active_index(), 1729);
    BOOST_CHECK_EQUAL(cell.props->active_index, 1729);

    BOOST_CHECK_CLOSE(cell.props->permx,  1.1  , 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->permy,  2.2  , 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->permz,  3.3  , 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->poro , -0.123, 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->ntg  ,  5.0  , 1.0e-8);

    BOOST_CHECK_EQUAL(cell.props->satnum, 42);
    BOOST_CHECK_EQUAL(cell.props->pvtnum, -1);
}

BOOST_AUTO_TEST_CASE(Get_Non_Existing)
{
    const auto cc = Opm::CompletedCells { 10, 10, 3 };

    BOOST_CHECK_THROW(cc.get(9, 9, 2), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()     // Basic_Operations
