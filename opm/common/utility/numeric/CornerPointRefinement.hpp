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

#ifndef OPM_CORNER_POINT_REFINEMENT_HPP
#define OPM_CORNER_POINT_REFINEMENT_HPP

#include <array>
#include <span>
#include <vector>

/// The corner-point description (COORD and ZCORN) of a refined box of a
/// corner-point grid. Both the LGR grids written to EGRID and the LGR grids
/// a simulation grid builds from a CARFIN box use it, so the two are the same.
namespace Opm::CornerPointRefinement {

/// How one axis of a refined box sits in its parent cells, one entry per
/// refined column: the parent column it lies in, counted from the box's first
/// parent cell, and its extent [fracLo, fracHi] inside that cell.
struct Axis {
    std::span<const int> parentOffset;
    std::span<const double> fracLo;
    std::span<const double> fracHi;
};

/// A box of a corner-point grid and how it is refined.
struct Box {
    std::array<int, 3> parentDims{};   ///< NX, NY, NZ of the parent COORD/ZCORN
    std::array<int, 3> first{};        ///< first parent cell of the box, 0-based
    std::array<Axis, 3> axes{};        ///< refined columns in I, J and K
};

/// Number of refined cells of the box in I, J and K.
std::array<int, 3> refinedDims(const Box& box);

/// COORD of the refined box, 6*(nx+1)*(ny+1) values. Each refined pillar is
/// the bilinear interpolation of its parent column's four pillar lines
/// (PillarInterpolation::bilinear).
std::vector<double> refinedCoord(const Box& box, std::span<const double> parentCoord);

/// ZCORN of the refined box, 8*nx*ny*nz values. Each refined corner depth is
/// the trilinear interpolation of the parent cell's eight corner depths.
/// Where the parent cell has zero thickness at a refined corner's lateral
/// position, the refined corners there have zero thickness too.
std::vector<double> refinedZcorn(const Box& box, std::span<const double> parentZcorn);

} // namespace Opm::CornerPointRefinement

#endif // OPM_CORNER_POINT_REFINEMENT_HPP
