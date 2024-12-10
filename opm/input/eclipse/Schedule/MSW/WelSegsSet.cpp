/*
  Copyright 2013 Statoil ASA.

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

#include "WelSegsSet.hpp"

#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <iterator>

namespace Opm {

void WelSegsSet::insert(const std::string& well_name,
                        const KeywordLocation& location)
{
    entries_.emplace(well_name, location);
}

std::vector<WelSegsSet::Entry>
WelSegsSet::difference(const std::set<std::string>& compsegs,
                       const std::vector<Well>& wells) const
{
    std::vector<Entry> difference;
    difference.reserve(entries_.size());
    std::set_difference(entries_.begin(), entries_.end(),
                        compsegs.begin(), compsegs.end(),
                        std::back_inserter(difference),
                        PairComp());

    // Ignore wells without connections
    const auto empty_conn = [&wells](const Entry &x) {
        return std::any_of(wells.begin(), wells.end(),
                           [wname = x.first](const Well& well)
                           { return (well.name() == wname) && well.getConnections().empty(); });
    };

    difference.erase(std::remove_if(difference.begin(),
                                    difference.end(), empty_conn),
                     difference.end());

    return difference;
}

bool WelSegsSet::PairComp::
operator()(const Entry& pair, const std::string& str) const
{
    return std::get<0>(pair) < str;
}

bool WelSegsSet::PairComp::
operator()(const Entry& pair1, const Entry& pair2) const
{
    return std::get<0>(pair1) < std::get<0>(pair2);
}


bool WelSegsSet::PairComp::
operator()(const std::string& str, const Entry& pair) const
{
    return str < std::get<0>(pair);
}

} // end namespace Opm
