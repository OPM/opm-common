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

#include <opm/parser/eclipse/Deck/UDAValue.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>

namespace Opm {

std::size_t UDQActive::active_size() const {
    std::size_t as = 0;
    for (const auto& record : this->data) {
        if (record.active)
            as += 1;
    }
    return as;
}

std::size_t UDQActive::size() const {
    return this->data.size();
}

UDQActive::operator bool() const {
    return this->data.size() > 0;
}

std::string UDQActive::hash(const std::string& udq, UDAControl control) {
  return udq + std::to_string(static_cast<int64_t>(control));
}


int UDQActive::add(const UDQConfig& udq_config, const std::string& udq, const std::string& wgname, UDAControl control) {
    auto hash_key = this->hash(udq, control);
    const auto iter = this->keys.find( hash_key );
    if (iter == this->keys.end()) {
        const auto& udq_input = udq_config[udq];
        auto udq_index = udq_input.index.insert_index;
        std::size_t use_index = 1;
        for (const auto& rec : this->data)
            use_index += rec.use_count;

        this->keys.insert( std::make_pair(hash_key, this->data.size()) );
        this->data.emplace_back(udq, udq_index, use_index, wgname, control);
    } else {
        auto data_index = iter->second;
        auto& record = this->data[data_index];
        record.use_count += 1;

        for (std::size_t index = data_index + 1; index < this->data.size(); index++)
            this->data[index].use_index += 1;
    }
    return 1;
}


int UDQActive::drop(const std::string& udq, UDAControl control) {
    auto hash_key = this->hash(udq, control);
    const auto iter = this->keys.find( hash_key );
    if (iter != this->keys.end()) {
        auto index = iter->second;
        auto& record = this->data[index];

        if (record.use_count > 0) {
            record.use_count -= 1;
            if (record.use_count == 0)
                record.active = false;
        }

        return 1;
    }

    return 0;
}


int UDQActive::update(const UDQConfig& udq_config, const UDAValue& uda, const std::string& wgname, UDAControl control) {
    if (uda.is<std::string>())
        return this->add(udq_config, uda.get<std::string>(), wgname, control);
    else {
        if (this->data.empty())
            return 0;
        /*
          The situation where a certain control is given by a UDA, and then in a
          later timestep is just given with a numerical value is just not
          supported in the current code; consider the situation below:

             -- The ORAT is given by the UDA 'WUOPRU'
             WCONDPROD
                'PROD'  'OPEN' 'GRUP' WUOPRU  /
             /

             TSTEP
                 10 /

             -- The ORAT is given by as the plain number 123
             WCONDPROD
                'PROD'  'OPEN' 'GRUP' 123 /
             /

          For the second WCONDPROD keyword we should ideally reduce the use
          count of the 'WUOPRU' UDQ, but when evaluating this WCONPROD keyword
          we do not know which UDQ was previously active.
        */
        std::string udq = "UNKNWON";
        return this->drop(udq, control);
    }
}


UDQActive::Record UDQActive::get(const std::string& udq, UDAControl control) {
    auto hash_key = this->hash(udq, control);
    auto index = this->keys.at(hash_key);
    return this->operator[](index);
}

UDQActive::Record UDQActive::operator[](std::size_t index) const {
    Record data_record = this->data[index];

    return data_record;
}


}

