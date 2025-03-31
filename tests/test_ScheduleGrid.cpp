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

#define BOOST_TEST_MODULE Schedule_Grid

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <optional>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(Main_Grid_Only)

BOOST_AUTO_TEST_SUITE(No_New_Cell_Objects)

BOOST_AUTO_TEST_CASE(Empty_Cell_Collection)
{
    // Recall: CompletedCells collection must be mutable and outlive the
    // ScheduleGrid.
    auto cc = Opm::CompletedCells { 10, 10, 3 };
    const auto grid = Opm::ScheduleGrid { cc };

    BOOST_CHECK_THROW(grid.get_cell(0, 0, 0), std::out_of_range);

    BOOST_CHECK_EQUAL(grid.get_grid(), nullptr);
}

BOOST_AUTO_TEST_CASE(Empty_Cell_Collection_Unknown_LGR)
{
    // Recall: CompletedCells collection must be mutable and outlive the
    // ScheduleGrid.
    auto cc = Opm::CompletedCells { 10, 10, 3 };
    const auto grid = Opm::ScheduleGrid { cc };

    BOOST_CHECK_THROW(grid.get_cell(0, 0, 0, "LGR1"), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(NonEmpty_Cell_Collection)
{
    // Recall: CompletedCells collection must be mutable and outlive the
    // ScheduleGrid.
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

    const auto grid = Opm::ScheduleGrid { cc };

    const auto& cell = grid.get_cell(0, 0, 0);

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

BOOST_AUTO_TEST_SUITE_END()     // No_New_Cell_Objects

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(With_Existing_Property_Data)

namespace {
    Opm::Deck singleStrip()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
1 5 2 /
GRID
DXV
100 /
DYV
5*50 /
DZV
2*10 /
TOPS
5*2000 /
PERMX
  10 20 30 40  50
  60 70 80 90 100 /
PERMY
  100 200 300 400  500
  600 700 800 900 1000 /
PERMZ
  10 9 8 7 6
   5 4 3 2 1 /
PORO
  0.1 1   0.2 0.9 0.3
  0.8 0.4 0.7 0.5 0.6 /
END
)");
    }

    Opm::Deck singleStripWithAquifer()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
1 5 2 /
GRID
DXV
100 /
DYV
5*50 /
DZV
2*10 /
TOPS
5*2000 /
PERMX
  10 20 30 40  50
  60 70 80 90 100 /
PERMY
  100 200 300 400  500
  600 700 800 900 1000 /
PERMZ
  10 9 8 7 6
   5 4 3 2 1 /
PORO
  0.1 1   0.2 0.9 0.3
  0.8 0.4 0.7 0.5 0.6 /
AQUNUM
--aqnr I  J  K     A       L     PHI     K   DEPTH
    1  1  3  2  3000000  25000  0.1243  8000  2115.53 /
/
END
)");
    }

    struct Case
    {
        Case(const Opm::Deck& deck)
            : es { deck }
        {}

        Opm::EclipseState es{};
    };

    auto mD() { return Opm::prefix::milli*Opm::unit::darcy; }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Insert_New_Cell)
{
    const auto cse = Case { singleStrip() };

    auto cc = Opm::CompletedCells { cse.es.getInputGrid() };
    auto grid = Opm::ScheduleGrid {
        cse.es.getInputGrid(),
        cse.es.fieldProps(),
        cc
    };

    const auto& cell = grid.get_cell(0, 2, 1);

    BOOST_CHECK_MESSAGE(cell.props.has_value(),
                        "Existing cell object must have property data");

    BOOST_CHECK_MESSAGE(cell.is_active(), "Existing cell object must be active");

    BOOST_CHECK_EQUAL(cell.global_index, std::size_t{7});
    BOOST_CHECK_EQUAL(cell.i,            std::size_t{0});
    BOOST_CHECK_EQUAL(cell.j,            std::size_t{2});
    BOOST_CHECK_EQUAL(cell.k,            std::size_t{1});

    BOOST_CHECK_CLOSE(cell.dimensions[0], 100.0, 1.0e-8);
    BOOST_CHECK_CLOSE(cell.dimensions[1],  50.0, 1.0e-8);
    BOOST_CHECK_CLOSE(cell.dimensions[2],  10.0, 1.0e-8);

    BOOST_CHECK_CLOSE(cell.depth, 2015.0, 1.0e-8);

    BOOST_CHECK_EQUAL(cell.active_index(), 7);
    BOOST_CHECK_EQUAL(cell.props->active_index, 7);

    BOOST_CHECK_CLOSE(cell.props->permx,  80.0*mD(), 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->permy, 800.0*mD(), 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->permz,   3.0*mD(), 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->poro ,   0.7     , 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->ntg  ,   1.0     , 1.0e-8);

    BOOST_CHECK_EQUAL(cell.props->satnum, 1);
    BOOST_CHECK_EQUAL(cell.props->pvtnum, 1);
}

BOOST_AUTO_TEST_CASE(Insert_New_Cell_In_Aquifer)
{
    const auto cse = Case { singleStripWithAquifer() };

    auto cc = Opm::CompletedCells { cse.es.getInputGrid() };
    auto grid = Opm::ScheduleGrid {
        cse.es.getInputGrid(),
        cse.es.fieldProps(),
        cc
    };

    grid.include_numerical_aquifers(cse.es.aquifer().numericalAquifers());

    const auto& cell = grid.get_cell(0, 2, 1);

    BOOST_CHECK_MESSAGE(cell.props.has_value(),
                        "Existing cell object must have property data");

    BOOST_CHECK_MESSAGE(cell.is_active(), "Existing cell object must be active");

    BOOST_CHECK_EQUAL(cell.global_index, std::size_t{7});
    BOOST_CHECK_EQUAL(cell.i,            std::size_t{0});
    BOOST_CHECK_EQUAL(cell.j,            std::size_t{2});
    BOOST_CHECK_EQUAL(cell.k,            std::size_t{1});

    BOOST_CHECK_CLOSE(cell.dimensions[0], 100.0, 1.0e-8);
    BOOST_CHECK_CLOSE(cell.dimensions[1],  50.0, 1.0e-8);
    BOOST_CHECK_CLOSE(cell.dimensions[2],  10.0, 1.0e-8);

    BOOST_CHECK_CLOSE(cell.depth, 2115.53, 1.0e-8);

    BOOST_CHECK_EQUAL(cell.active_index(), 7);
    BOOST_CHECK_EQUAL(cell.props->active_index, 7);

    BOOST_CHECK_CLOSE(cell.props->permx, 8000.0*mD(), 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->permy, 8000.0*mD(), 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->permz, 8000.0*mD(), 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->poro ,    0.1243  , 1.0e-8);
    BOOST_CHECK_CLOSE(cell.props->ntg  ,    1.0     , 1.0e-8);

    BOOST_CHECK_EQUAL(cell.props->satnum, 1);
    BOOST_CHECK_EQUAL(cell.props->pvtnum, 1);
}

BOOST_AUTO_TEST_SUITE_END()     // With_Existing_Property_Data

BOOST_AUTO_TEST_SUITE_END()     // Main_Grid_Only
