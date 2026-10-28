/*
  Copyright 2026 Equinor ASA.

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

#include <config.h>

#define BOOST_TEST_MODULE CornerPointRefinement
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/numeric/CornerPointRefinement.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace {

using Opm::CornerPointRefinement::Box;

// Refined columns of one axis: `parents` parent cells, each split into `factor` equal columns.
struct Columns {
    std::vector<int> parentOffset;
    std::vector<double> fracLo;
    std::vector<double> fracHi;

    Columns(const int parents, const int factor)
    {
        for (int parent = 0; parent < parents; ++parent) {
            for (int sub = 0; sub < factor; ++sub) {
                parentOffset.push_back(parent);
                fracLo.push_back(static_cast<double>(sub) / factor);
                fracHi.push_back(static_cast<double>(sub + 1) / factor);
            }
        }
    }
};

Box makeBox(const std::array<int, 3>& parentDims, const std::array<int, 3>& first,
            const std::array<Columns, 3>& columns)
{
    Box box { parentDims, first, {} };
    for (std::size_t dim = 0; dim < 3; ++dim) {
        box.axes[dim] = { columns[dim].parentOffset, columns[dim].fracLo, columns[dim].fracHi };
    }
    return box;
}

// Vertical pillars on a regular 100 m lattice of nx * ny cells.
std::vector<double> verticalCoord(const int nx, const int ny, const double top, const double bottom)
{
    std::vector<double> coord;
    for (int j = 0; j <= ny; ++j) {
        for (int i = 0; i <= nx; ++i) {
            coord.insert(coord.end(), { 100.0*i, 100.0*j, top, 100.0*i, 100.0*j, bottom });
        }
    }
    return coord;
}

// ZCORN value of corner (i, j, k), each in 0 .. 2*n - 1, of a grid with nx * ny cells per layer.
double zcornAt(const std::vector<double>& zcorn, const int nx, const int ny, const int i, const int j, const int k)
{
    return zcorn[static_cast<std::size_t>(i) + 2*static_cast<std::size_t>(nx)*j
                 + 4*static_cast<std::size_t>(nx)*ny*k];
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(UniformRefinementOfAFlatCell)
{
    const std::array<Columns, 3> columns { Columns(1, 2), Columns(1, 2), Columns(1, 2) };
    const auto box = makeBox({1, 1, 1}, {0, 0, 0}, columns);
    const auto coord = Opm::CornerPointRefinement::refinedCoord(box, verticalCoord(1, 1, 1000.0, 1010.0));
    const std::vector<double> parentZcorn { 1000, 1000, 1000, 1000, 1010, 1010, 1010, 1010 };
    const auto zcorn = Opm::CornerPointRefinement::refinedZcorn(box, parentZcorn);

    BOOST_CHECK((Opm::CornerPointRefinement::refinedDims(box) == std::array<int, 3>{2, 2, 2}));
    BOOST_REQUIRE_EQUAL(coord.size(), 6u * 3 * 3);
    // The middle refined pillar.
    const std::array<double, 6> middle { 50.0, 50.0, 1000.0, 50.0, 50.0, 1010.0 };
    for (std::size_t n = 0; n < middle.size(); ++n) {
        BOOST_CHECK_EQUAL(coord[6*4 + n], middle[n]);
    }
    BOOST_REQUIRE_EQUAL(zcorn.size(), 8u * 2 * 2 * 2);
    for (int k = 0; k < 4; ++k) {
        const double expected = 1000.0 + 5.0*((k + 1)/2);
        for (int j = 0; j < 4; ++j) {
            for (int i = 0; i < 4; ++i) {
                BOOST_CHECK_EQUAL(zcornAt(zcorn, 2, 2, i, j, k), expected);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(ZeroThicknessParentGivesFlatRefinedCells)
{
    // A tilted parent of zero thickness, refined three times per direction:
    // the fractions do not round exactly, yet every refined cell must be flat.
    const std::array<double, 4> top { 2075.438530, 2094.930120, 2011.741428, 2089.191318 };
    std::vector<double> parentZcorn(top.begin(), top.end());
    parentZcorn.insert(parentZcorn.end(), top.begin(), top.end());

    const std::array<Columns, 3> columns { Columns(1, 3), Columns(1, 3), Columns(1, 3) };
    const auto zcorn = Opm::CornerPointRefinement::refinedZcorn(makeBox({1, 1, 1}, {0, 0, 0}, columns), parentZcorn);

    for (int j = 0; j < 6; ++j) {
        for (int i = 0; i < 6; ++i) {
            for (int k = 1; k < 6; ++k) {
                BOOST_TEST_CONTEXT("corner " << i << ',' << j << " level " << k) {
                    BOOST_CHECK_EQUAL(zcornAt(zcorn, 3, 3, i, j, k), zcornAt(zcorn, 3, 3, i, j, 0));
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(SharedFaceIsTheSameFromBothBoxes)
{
    // Two parent cells side by side in I with sloping depths that match on
    // their shared face. Each is refined as a box of its own; the refined
    // corners on the shared face must agree bit for bit.
    const std::vector<double> coord = verticalCoord(2, 1, 2000.0, 2100.0);
    std::vector<double> parentZcorn(16);
    const std::array<double, 3> topAtPillar { 2011.741428, 2043.775731, 2089.191318 };
    for (int k = 0; k < 2; ++k) {
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < 4; ++i) {
                // Depths belong to pillars, so both cells see the same depths on their shared face.
                const int pillar = (i + 1)/2;
                const double depth = topAtPillar[pillar] + 0.37*j + k*(17.31 + 0.11*pillar);
                parentZcorn[static_cast<std::size_t>(i) + 4*j + 8*k] = depth;
            }
        }
    }
    const std::array<Columns, 3> columns { Columns(1, 3), Columns(1, 3), Columns(1, 3) };
    const auto left = makeBox({2, 1, 1}, {0, 0, 0}, columns);
    const auto right = makeBox({2, 1, 1}, {1, 0, 0}, columns);
    const auto zLeft = Opm::CornerPointRefinement::refinedZcorn(left, parentZcorn);
    const auto zRight = Opm::CornerPointRefinement::refinedZcorn(right, parentZcorn);
    const auto cLeft = Opm::CornerPointRefinement::refinedCoord(left, coord);
    const auto cRight = Opm::CornerPointRefinement::refinedCoord(right, coord);

    for (int k = 0; k < 6; ++k) {
        for (int j = 0; j < 6; ++j) {
            BOOST_CHECK_EQUAL(zcornAt(zLeft, 3, 3, 5, j, k), zcornAt(zRight, 3, 3, 0, j, k));
        }
    }
    for (int j = 0; j <= 3; ++j) {
        for (int n = 0; n < 6; ++n) {
            BOOST_CHECK_EQUAL(cLeft[6*(4*j + 3) + n], cRight[6*(4*j + 0) + n]);
        }
    }
}
