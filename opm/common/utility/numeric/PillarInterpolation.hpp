/*
  Copyright 2026 Equinor ASA.

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

#ifndef OPM_PILLAR_INTERPOLATION_HPP
#define OPM_PILLAR_INTERPOLATION_HPP

#include <array>

namespace Opm::PillarInterpolation {

/// An infinite pillar line, represented by two endpoints.
struct Pillar {
    std::array<double, 3> top;
    std::array<double, 3> bottom;

    /// Read six COORD values: (x_top, y_top, z_top, x_bottom, y_bottom, z_bottom).
    static Pillar fromCoord(const double* coord);

    /// Write six COORD values to the supplied storage.
    void writeCoord(double* coord) const;

    /// Intersect the line with a horizontal plane, allowing extrapolation.
    /// Equal endpoint depths use a vertical line through top, matching
    /// EclipseGrid's convention for degenerate COORD pillars.
    std::array<double, 3> atDepth(double z) const;
};

/// Bilinearly interpolate four infinite pillar lines in horizontal planes.
///
/// The input order is (0,0), (1,0), (0,1), (1,1). At each depth, the
/// interpolated x and y are the bilinear combination of the four lines'
/// intersections with that horizontal plane. Thus the resulting line is
/// independent of the choice of endpoints representing each input line.
///
/// The returned endpoints are at the minimum top depth and maximum bottom
/// depth of the input pillars with nonzero interpolation weight. On a shared
/// face, only its two pillars contribute, so adjacent columns compute the
/// same endpoint representation. The depths describe a practical extent only;
/// interpolation along a pillar is not restricted to that extent.
Pillar bilinear(const std::array<Pillar, 4>& pillars, double u, double v);

} // namespace Opm::PillarInterpolation

#endif // OPM_PILLAR_INTERPOLATION_HPP
