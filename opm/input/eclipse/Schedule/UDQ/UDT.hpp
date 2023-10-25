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

#ifndef UDT_HPP
#define UDT_HPP

#include <vector>

namespace Opm {

class DeckRecord;

class UDT
{
public:
    enum class InterpolationType {
       NearestNeighbour, //!< Corresponds to 'NV'
       LinearClamp,      //!< Corresponds to 'LC'
       LinearExtrapolate //!< Corresponds to 'LL'
    };

    UDT() = default;

    UDT(const std::vector<double>& x_vals,
        const std::vector<double>& y_vals,
        InterpolationType interp_type);

    static UDT serializationTestObject();

    double operator()(const double x) const;

    bool operator==(const UDT& data) const;

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(xvals_);
        serializer(yvals_);
        serializer(interp_type_);
    }

private:
    std::vector<double> xvals_; //!< Data points
    std::vector<double> yvals_; //!< Data values
    InterpolationType interp_type_ = InterpolationType::LinearClamp; //!< Interpolation type
};

} // Namespace Opm

#endif // UDT_HPP
