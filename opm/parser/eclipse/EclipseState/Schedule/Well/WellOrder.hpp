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
  The purpose of this small class is to ensure that wellnames always come in the
  order they are defined in the deck.
*/



class WellOrder {
public:
    using Map = std::unordered_map<std::string, std::size_t>;
    using Vector = std::vector<std::string>;
    WellOrder() = default;
    explicit WellOrder(const std::vector<std::string>& wells);
    void add(const std::string& well);
    std::vector<std::string> sort(std::vector<std::string> wells) const;
    const std::vector<std::string>& wells() const;
    bool has(const std::string& wname) const;


    static WellOrder serializeObject();

    bool operator==(const WellOrder& other) const;
    Vector::const_iterator begin() const;
    Vector::const_iterator end() const;

private:
    Map m_wells1;
    Vector m_wells2;
};

}
#endif
