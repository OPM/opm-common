//===========================================================================
//
// File: ParameterRequirement.cpp
//
// Created: Tue Jun  2 19:05:02 2009
//
// Author(s): Bård Skaflestad     <bard.skaflestad@sintef.no>
//            Atgeirr F Rasmussen <atgeirr@sintef.no>
//
// $Date$
//
// $Revision$
//
//===========================================================================

/*
  Copyright 2009, 2010 SINTEF ICT, Applied Mathematics.
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

#include <config.h>
#include <opm/common/utility/parameters/ParameterRequirement.hpp>

#include <algorithm>
#include <cassert>
#include <sstream>

namespace Opm {

std::string ParameterRequirementProbability::
operator()(double x) const
{
    if ( (x < 0.0) || (x > 1.0) ) {
        std::ostringstream stream;
        stream << "The value '" << x
               << "' is not in the interval [0, 1].";
        return stream.str();
    } else {
        return "";
    }
}

ParameterRequirementMemberOf::
ParameterRequirementMemberOf(const std::vector<std::string>& elements)
    : elements_(elements)
{
    assert(elements_.size() > 0);
}

std::string ParameterRequirementMemberOf::
operator()(const std::string& x) const
{
    if (std::find(elements_.begin(), elements_.end(), x) == elements_.end()) {
        if (elements_.size() == 1) {
            return "The string '" + x + "' is not '" + elements_[0] + "'.";
        }
        std::ostringstream stream;
        stream << "The string '" << x << "' is not among '";
        for (int i = 0; i < int(elements_.size()) - 2; ++i) {
            stream << elements_[i] << "', '";
        }
        stream << elements_[elements_.size() - 2]
               << "' and '"
               << elements_[elements_.size() - 1]
               << "'.";
        return stream.str();
    } else {
        return "";
    }
}

} // namespace Opm
