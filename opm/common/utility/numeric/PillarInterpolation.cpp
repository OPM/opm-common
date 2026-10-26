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
    double top = pillars[0].top[2];
    double bottom = pillars[0].bottom[2];
    for (const auto& pillar : pillars) {
        top = std::min(top, pillar.top[2]);
        bottom = std::max(bottom, pillar.bottom[2]);
    }

    const std::array<double, 4> weights {
        (1.0 - u)*(1.0 - v), u*(1.0 - v), (1.0 - u)*v, u*v
    };
    auto interpolateAtDepth = [&](double depth) {
        std::array<double, 3> point {0.0, 0.0, depth};
        for (std::size_t i = 0; i < pillars.size(); ++i) {
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
