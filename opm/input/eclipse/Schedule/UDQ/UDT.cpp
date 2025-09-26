/*
  Copyright 2023 SINTEF Digital

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

#include <opm/input/eclipse/Schedule/UDQ/UDT.hpp>

#include <opm/common/utility/numeric/linearInterpolation.hpp>

#include <algorithm>
#include <cassert>

namespace Opm {

UDT::UDT(const std::vector<double>& xvals,
         const std::vector<double>& yvals,
         InterpolationType interp_type)
    : xvals_(xvals)
    , yvals_(yvals)
    , interp_type_(interp_type)
{
}

UDT UDT::serializationTestObject()
{
    return UDT({1.0, 2.0}, {3.0, 4.0}, InterpolationType::NearestNeighbour);
}

bool UDT::operator==(const UDT& rhs) const
{
    return this->xvals_ == rhs.xvals_ &&
           this->yvals_ == rhs.yvals_ &&
           this->interp_type_ == rhs.interp_type_;
}

double UDT::operator()(const double x) const
{
    switch (interp_type_) {
    case InterpolationType::NearestNeighbour:
    {
        const int idx = Opm::tableIndex(xvals_, x);
        const double dist1 = std::abs(x - xvals_[idx]);
        const double dist2 = std::abs(x - xvals_[idx+1]);
        return dist1 < dist2 ? yvals_[idx] : yvals_[idx+1];
    }
    case InterpolationType::LinearClamp:
        return linearInterpolationNoExtrapolation(xvals_, yvals_, x);
    case InterpolationType::LinearExtrapolate:
        // TOOD: Use std::lerp when available ?
        return linearInterpolation(xvals_, yvals_, x);
    }

    assert(0); // Should be unreachable

    return 0.0;
}

} // namespace Opm
