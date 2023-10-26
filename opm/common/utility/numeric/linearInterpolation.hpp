/*
  Copyright 2009, 2010, 2013 SINTEF ICT, Applied Mathematics.
  Copyright 2009, 2010 Statoil ASA.

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

#ifndef OPM_LINEARINTERPOLATION_HEADER_INCLUDED
#define OPM_LINEARINTERPOLATION_HEADER_INCLUDED


#include <algorithm>
#include <tuple>
#include <vector>

namespace Opm
{

//! \brief Returns an index in an ordered table such that x is between
//!        table[j] and table[j+1].
//! \details If x is out of bounds, it returns a clamped index
inline int tableIndex(const std::vector<double>& table, double x)
{
    if (table.size() < 2)
        return 0;

    const auto lower = std::lower_bound(table.begin(), table.end(), x);

    if (lower == table.end())
        return table.size()-2;
    else if (lower == table.begin())
        return 0;
    else
      return std::distance(table.begin(), lower)-1;
}

inline std::pair<double, int>
linearInterpolationSlope(const std::vector<double>& xv,
                         const std::vector<double>& yv,
                         const double x)
{
    const auto i = Opm::tableIndex(xv, x);
    return { (yv[i + 1] - yv[i]) / (xv[i + 1] - xv[i]), i };
}


inline double linearInterpolationDerivative(const std::vector<double>& xv,
                                            const std::vector<double>& yv, double x)
{
    // Extrapolates if x is outside xv
    return linearInterpolationSlope(xv, yv, x).first;
}

inline double linearInterpolation(const std::vector<double>& xv,
                                  const std::vector<double>& yv, double x)
{
    // Extrapolates if x is outside xv
    const auto& [t, i] = linearInterpolationSlope(xv, yv, x);
    return t * (x - xv[i]) + yv[i];
}

inline double linearInterpolationNoExtrapolation(const std::vector<double>& xv,
                                                 const std::vector<double>& yv, double x)
{
    // Return end values if x is outside xv
    if (x < xv.front()) {
        return yv.front();
    }
    if (x > xv.back()) {
        return yv.back();
    }

    return linearInterpolation(xv, yv, x);
}

inline double linearInterpolation(const std::vector<double>& xv,
                                  const std::vector<double>& yv,
                                  double x, int& ix1)
{
    // Extrapolates if x is outside xv
    double t;
    std::tie(t, ix1) = linearInterpolationSlope(xv, yv, x);
    return t * (x - xv[ix1]) + yv[ix1];
}

} // namespace Opm

#endif // OPM_LINEARINTERPOLATION_HEADER_INCLUDED
