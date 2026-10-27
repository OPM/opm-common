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
#include <opm/common/utility/numeric/CornerPointRefinement.hpp>

#include <opm/common/utility/numeric/PillarInterpolation.hpp>

#include <cstddef>

namespace {

using Opm::CornerPointRefinement::Axis;

// Where refined line `line` of an axis lies: its parent cell (absolute index)
// and the fraction inside it. A line is the low edge of its own column, except
// the last, which closes the last column.
struct LinePosition {
    int cell;
    double fraction;
};

LinePosition linePosition(const Axis& axis, const int first, const std::size_t line)
{
    if (line == axis.parentOffset.size()) {
        return { first + axis.parentOffset.back(), axis.fracHi.back() };
    }
    return { first + axis.parentOffset[line], axis.fracLo[line] };
}

} // anonymous namespace

namespace Opm::CornerPointRefinement {

std::array<int, 3> refinedDims(const Box& box)
{
    return { static_cast<int>(box.axes[0].parentOffset.size()),
             static_cast<int>(box.axes[1].parentOffset.size()),
             static_cast<int>(box.axes[2].parentOffset.size()) };
}

std::vector<double> refinedCoord(const Box& box, std::span<const double> parentCoord)
{
    const auto [nx, ny, nz] = refinedDims(box);
    const auto parentNx = static_cast<std::size_t>(box.parentDims[0]);

    std::vector<double> coord(6 * static_cast<std::size_t>(nx + 1) * (ny + 1));
    for (int j = 0; j <= ny; ++j) {
        const auto [cj, tj] = linePosition(box.axes[1], box.first[1], j);
        for (int i = 0; i <= nx; ++i) {
            const auto [ci, ti] = linePosition(box.axes[0], box.first[0], i);
            std::array<PillarInterpolation::Pillar, 4> parent;
            for (int dj = 0; dj < 2; ++dj) {
                for (int di = 0; di < 2; ++di) {
                    const auto pillar = static_cast<std::size_t>(cj + dj) * (parentNx + 1) + ci + di;
                    parent[di + 2*dj] = PillarInterpolation::Pillar::fromCoord(&parentCoord[6 * pillar]);
                }
            }
            PillarInterpolation::bilinear(parent, ti, tj)
                .writeCoord(&coord[6 * (static_cast<std::size_t>(j) * (nx + 1) + i)]);
        }
    }
    return coord;
}

std::vector<double> refinedZcorn(const Box& box, std::span<const double> parentZcorn)
{
    const auto [nx, ny, nz] = refinedDims(box);
    const auto parentNx = static_cast<std::size_t>(box.parentDims[0]);
    const auto parentNy = static_cast<std::size_t>(box.parentDims[1]);
    const auto parentZ = [&](const int i, const int j, const int k) {
        return parentZcorn[static_cast<std::size_t>(i) + 2*parentNx*j + 4*parentNx*parentNy*k];
    };
    const auto zcornIndex = [&](const int i, const int j, const int k) {
        return static_cast<std::size_t>(i) + 2*static_cast<std::size_t>(nx)*j
             + 4*static_cast<std::size_t>(nx)*ny*k;
    };
    const auto& [axisI, axisJ, axisK] = box.axes;

    std::vector<double> zcorn(8 * static_cast<std::size_t>(nx) * ny * nz);
    for (int k = 0; k < nz; ++k) {
        const int ck = box.first[2] + axisK.parentOffset[k];
        for (int j = 0; j < ny; ++j) {
            const int cj = box.first[1] + axisJ.parentOffset[j];
            for (int i = 0; i < nx; ++i) {
                const int ci = box.first[0] + axisI.parentOffset[i];
                for (int dk = 0; dk < 2; ++dk) {
                    const double c = dk ? axisK.fracHi[k] : axisK.fracLo[k];
                    for (int dj = 0; dj < 2; ++dj) {
                        const double b = dj ? axisJ.fracHi[j] : axisJ.fracLo[j];
                        for (int di = 0; di < 2; ++di) {
                            const double a = di ? axisI.fracHi[i] : axisI.fracLo[i];

                            // Where the parent has zero thickness here, the refined
                            // corners keep zero thickness: rounding would otherwise
                            // leave some refined cells slightly thick and turn
                            // others upside down.
                            double parentDz = 0.0;
                            for (int pj = 0; pj < 2; ++pj) {
                                const double wj = (pj == 0) ? (1.0 - b) : b;
                                for (int pi = 0; pi < 2; ++pi) {
                                    const double wi = (pi == 0) ? (1.0 - a) : a;
                                    parentDz += wi*wj*(parentZ(2*ci + pi, 2*cj + pj, 2*ck + 1)
                                                       - parentZ(2*ci + pi, 2*cj + pj, 2*ck));
                                }
                            }
                            const double t = (parentDz == 0.0) ? 0.0 : c;

                            double depth = 0.0;
                            for (int pk = 0; pk < 2; ++pk) {
                                const double wk = (pk == 0) ? (1.0 - t) : t;
                                for (int pj = 0; pj < 2; ++pj) {
                                    const double wj = (pj == 0) ? (1.0 - b) : b;
                                    for (int pi = 0; pi < 2; ++pi) {
                                        const double wi = (pi == 0) ? (1.0 - a) : a;
                                        depth += wi*wj*wk*parentZ(2*ci + pi, 2*cj + pj, 2*ck + pk);
                                    }
                                }
                            }
                            zcorn[zcornIndex(2*i + di, 2*j + dj, 2*k + dk)] = depth;
                        }
                    }
                }
            }
        }
    }
    return zcorn;
}

} // namespace Opm::CornerPointRefinement
