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

#include <opm/input/eclipse/Schedule/Well/WList.hpp>

#include <algorithm>

namespace Opm {

WList::WList(const std::vector<std::string>& wlist, const std::string& wlname)
    : well_list { wlist }
    , name      { wlname }
{}

std::size_t WList::size() const
{
    return this->well_list.size();
}

void WList::clear()
{
    this->well_list.clear();
}

bool WList::has(const std::string& well) const
{
    return std::find(this->well_list.begin(), this->well_list.end(), well)
        != this->well_list.end();
}

void WList::add(const std::string& well)
{
    // Add well if it is not already in the well list.
    if (! this->has(well)) {
        this->well_list.push_back(well);
    }
}

void WList::del(const std::string& well)
{
    auto end_keep = std::remove(this->well_list.begin(), this->well_list.end(), well);
    this->well_list.erase(end_keep, this->well_list.end());
}

const std::vector<std::string>& WList::wells() const
{
    return this->well_list;
}

bool WList::operator==(const WList& data) const
{
    return this->well_list == data.well_list;
}

} // namespace Opm
