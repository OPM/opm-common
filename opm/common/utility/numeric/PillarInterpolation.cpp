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

#include <opm/common/utility/numeric/PillarInterpolation.hpp>

#include <algorithm>
#include <limits>

namespace Opm::PillarInterpolation {

Pillar Pillar::fromCoord(const double* coord)
{
    return {{coord[0], coord[1], coord[2]}, {coord[3], coord[4], coord[5]}};
}

void Pillar::writeCoord(double* coord) const
{
    std::copy(top.begin(), top.end(), coord);
    std::copy(bottom.begin(), bottom.end(), coord + top.size());
}

std::array<double, 3> Pillar::atDepth(double z) const
{
    const double dz = bottom[2] - top[2];
    const double t = dz != 0.0 ? (z - top[2])/dz : 0.0;
    return {top[0] + t*(bottom[0] - top[0]),
            top[1] + t*(bottom[1] - top[1]), z};
}

Pillar bilinear(const std::array<Pillar, 4>& pillars, double u, double v)
{
    const std::array<double, 4> weights {
        (1.0 - u)*(1.0 - v), u*(1.0 - v), (1.0 - u)*v, u*v
    };
    double top = std::numeric_limits<double>::max();
    double bottom = std::numeric_limits<double>::lowest();
    for (std::size_t i = 0; i < pillars.size(); ++i) {
        if (weights[i] != 0.0) {
            top = std::min(top, pillars[i].top[2]);
            bottom = std::max(bottom, pillars[i].bottom[2]);
        }
    }

    auto interpolateAtDepth = [&](double depth) {
        std::array<double, 3> point {0.0, 0.0, depth};
        for (std::size_t i = 0; i < pillars.size(); ++i) {
            if (weights[i] == 0.0) {
                continue;
            }
            const auto coordinate = pillars[i].atDepth(depth);
            for (int dim = 0; dim < 2; ++dim) {
                point[dim] += weights[i]*coordinate[dim];
            }
        }
        return point;
    };
    return {interpolateAtDepth(top), interpolateAtDepth(bottom)};
}

} // namespace Opm::PillarInterpolation
