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


#ifndef UDQ_USAGE_HPP
#define UDQ_USAGE_HPP

#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

namespace Opm {

class UDAValue;
class UDQActive {
public:
    struct Record{
        std::size_t index;
        std::string udq;
        std::string wgname;
        UDAControl  control;
        bool active;
    };

    int update(const UDAValue& uda, const std::string& wgname, UDAControl control);

    std::size_t size() const;
    std::size_t use_count(const std::string& udq) const;

    explicit operator bool() const;
    const Record& operator[](std::size_t index) const;
    std::vector<Record>::const_iterator begin() const;
    std::vector<Record>::const_iterator end() const;
private:
    std::string hash(const std::string& wgname, UDAControl control);
    int add(const std::string& udq, const std::string& wgname, UDAControl control);
    int drop(const std::string& wgname, UDAControl control);

    std::vector<Record> data;
    std::unordered_map<std::string, std::size_t> keys;
    std::unordered_map<std::string, std::size_t> m_use_count;
};

}

#endif
