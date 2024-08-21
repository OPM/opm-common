/*
  Copyright 2016 Statoil ASA.

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

#include <opm/output/data/Solution.hpp>

#include <opm/output/data/Cells.hpp>

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Opm { namespace data {

Solution::Solution(const bool init_si)
    : si(init_si)
{}

bool Solution::has(const std::string& keyword) const
{
    return this->find(keyword) != this->end();
}

template<class T>
std::vector<T>&
Solution::data(const std::string& keyword)
{
    return this->at(keyword).data<T>();
}

template<class T>
const std::vector<T>&
Solution::data(const std::string& keyword) const
{
    return this->at(keyword).data<T>();
}

std::pair<Solution::iterator, bool>
Solution::insert(std::string               name,
                 const UnitSystem::measure m,
                 std::vector<double>       xs,
                 const TargetType          type)
{
    return this->emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::move(name)),
                         std::forward_as_tuple(m, std::move(xs), type));
}


std::pair<Solution::iterator, bool>
Solution::insert(std::string               name,
                 const UnitSystem::measure m,
                 std::vector<float>        xs,
                 const TargetType          type)
{
    std::vector<double> xsd(xs.begin(), xs.end());
    return this->emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::move(name)),
                         std::forward_as_tuple(m, std::move(xsd), type));
}


std::pair<Solution::iterator, bool>
Solution::insert(std::string               name,
                 std::vector<int>          xs,
                 const TargetType          type)
{
    return this->emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::move(name)),
                         std::forward_as_tuple(std::move(xs), type));
}

void data::Solution::convertToSI(const UnitSystem& units)
{
    if (this->si) {
        return;
    }

    for (auto& elm : *this) {
        const auto dim = elm.second.dim;

        if (dim != UnitSystem::measure::identity) {
            units.to_si(dim, (elm.second.data<double>()));
        }
    }

    this->si = true;
}

void data::Solution::convertFromSI(const UnitSystem& units)
{
    if (!this->si) {
        return;
    }

    for (auto& elm : *this) {
        const auto dim = elm.second.dim;

        if (dim != UnitSystem::measure::identity) {
            units.from_si(dim, elm.second.data<double>());
        }
    }

    this->si = false;
}

template std::vector<double>& data::Solution::data<double>(const std::string&);
template const std::vector<double>& data::Solution::data<double>(const std::string&) const;
template std::vector<int>& data::Solution::data<int>(const std::string&);
template const std::vector<int>& data::Solution::data<int>(const std::string&) const;

}} // namespace Opm::data
