/*
  Copyright 2019 Equinor ASA.

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
#include <opm/common/utility/shmatch.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>

#include <algorithm>
#include <utility>

namespace Opm {


WellMatcher::WellMatcher(const NameOrder& well_order) :
    m_well_order(well_order)
{
}

WellMatcher::WellMatcher(std::initializer_list<std::string> wells) :
    m_well_order(wells)
{
}

WellMatcher::WellMatcher(const std::vector<std::string>& wells) :
    m_well_order(wells)
{
}

WellMatcher::WellMatcher(const NameOrder& well_order, const WListManager &wlm) :
    m_well_order(well_order),
    m_wlm(wlm)
{
}

std::vector<std::string> WellMatcher::sort(std::vector<std::string> wells) const {
    return this->m_well_order.sort(std::move(wells));
}

const std::vector<std::string>& WellMatcher::wells() const {
    return this->m_well_order.names();
}


std::vector<std::string> WellMatcher::wells(const std::string& pattern) const {
    if (pattern.size() == 0)
        return {};

    // WLIST
    if (pattern[0] == '*' && pattern.size() > 1)
        return this->sort( this->m_wlm.wells(pattern) );

    // Normal pattern matching
    auto star_pos = pattern.find('*');
    if (star_pos != std::string::npos) {
        std::vector<std::string> names;
        std::copy_if(this->m_well_order.begin(), this->m_well_order.end(),
                     std::back_inserter(names),
                     [&pattern](const auto& wname)
                     {
                         return shmatch(pattern, wname);
                     });
        return names;
    }

    if (this->m_well_order.has(pattern))
        return { pattern };

    return {};
}
}
