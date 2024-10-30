/*
  Copyright 2021 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

void NameOrder::add(const std::string& name)
{
    const auto emplaceResult = this->m_index_map
        .emplace(name, this->m_name_list.size());

    if (emplaceResult.second) {
        // New element inserted.  Update name list.
        this->m_name_list.push_back(name);
    }
}

NameOrder::NameOrder(const std::vector<std::string>& names)
{
    for (const auto& w : names) {
        this->add(w);
    }
}

NameOrder::NameOrder(std::initializer_list<std::string> names)
{
    for (const auto& w : names) {
        this->add(w);
    }
}

bool NameOrder::has(const std::string& wname) const
{
    return this->m_index_map.find(wname) != this->m_index_map.end();
}

const std::vector<std::string>& NameOrder::names() const
{
    return this->m_name_list;
}

std::vector<std::string>
NameOrder::sort(std::vector<std::string> names) const
{
    std::sort(names.begin(), names.end(),
        [this](const std::string& w1, const std::string& w2) -> bool
    {
        return this->m_index_map.at(w1) < this->m_index_map.at(w2);
    });

    return names;
}

NameOrder NameOrder::serializationTestObject()
{
    NameOrder wo;
    wo.add("W1");
    wo.add("W2");
    wo.add("W3");
    return wo;
}

bool NameOrder::operator==(const NameOrder& other) const
{
    return (this->m_index_map == other.m_index_map)
        && (this->m_name_list == other.m_name_list);
}

const std::string& NameOrder::operator[](std::size_t index) const
{
    return this->m_name_list.at(index);
}

// --------------------------------------------------------------------------------

GroupOrder::GroupOrder(const std::size_t max_groups)
    : m_max_groups { max_groups }
{
    this->add("FIELD");
}

GroupOrder GroupOrder::serializationTestObject()
{
    GroupOrder go(123);

    go.add("G1");
    go.add("G2");

    return go;
}

void GroupOrder::add(const std::string& gname)
{
    auto iter = std::find(this->m_name_list.begin(),
                          this->m_name_list.end(), gname);
    if (iter == this->m_name_list.end()) {
        this->m_name_list.push_back(gname);
    }
}

bool GroupOrder::has(const std::string& gname) const
{
    auto iter = std::find(this->m_name_list.begin(),
                          this->m_name_list.end(), gname);
    return iter != this->m_name_list.end();
}

const std::vector<std::string>& GroupOrder::names() const
{
    return this->m_name_list;
}

std::vector<std::optional<std::string>> GroupOrder::restart_groups() const
{
    std::vector<std::optional<std::string>> groups(this->m_max_groups + 1);

    const auto& input_groups = this->names();

    std::copy(input_groups.begin() + 1, input_groups.end(), groups.begin());
    groups.back() = input_groups.front();

    return groups;
}

bool GroupOrder::operator==(const GroupOrder& other) const
{
    return (this->m_max_groups == other.m_max_groups)
        && (this->m_name_list == other.m_name_list);
}

} // namespace Opm
