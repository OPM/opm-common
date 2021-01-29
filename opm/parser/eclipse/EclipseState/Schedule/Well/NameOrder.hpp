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
#ifndef WELL_ORDER_HPP
#define WELL_ORDER_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

/*
  The purpose of this small class is to ensure that well and group name always
  come in the order they are defined in the deck.
*/



class NameOrder {
public:
    using Map = std::unordered_map<std::string, std::size_t>;
    NameOrder() = default;
    explicit NameOrder(const std::vector<std::string>& names);
    void add(const std::string& name);
    std::vector<std::string> sort(std::vector<std::string> names) const;
    const std::vector<std::string>& names() const;
    bool has(const std::string& wname) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer) {
        serializer.template map<Map, false>(m_names1);
        serializer(m_names2);
    }

    static NameOrder serializeObject();

    bool operator==(const NameOrder& other) const;
    std::vector<std::string>::const_iterator begin() const;
    std::vector<std::string>::const_iterator end() const;

private:
    Map m_names1;
    std::vector<std::string> m_names2;
};


class GroupOrder : public NameOrder {
public:
    GroupOrder();
    std::vector<std::string> restart_groups() const;
};

}
#endif
