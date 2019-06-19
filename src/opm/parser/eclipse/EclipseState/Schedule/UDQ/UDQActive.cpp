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

namespace Opm {

std::size_t UDQActive::size() const {
    return this->data.size();
}


std::string UDQActive::hash(const std::string& wgname, UDAControl control) {
  return wgname + std::to_string(static_cast<int64_t>(control));
}


int UDQActive::add(const std::string& udq, const std::string& wgname, UDAControl control) {
    auto hash_key = this->hash(wgname, control);
    const auto iter = this->keys.find( hash_key );
    if (iter == this->keys.end()) {
        auto index = this->data.size();
        this->data.push_back( {index, udq, wgname, control, true} );
        this->keys.insert( std::make_pair(hash_key, index) );
    } else {
        auto& record = this->data[iter->second];
        this->m_use_count[record.udq] -= 1;
        record.udq = udq;
    }

    this->m_use_count[udq] += 1;
    return 1;
}


int UDQActive::drop(const std::string& wgname, UDAControl control) {
    if (this->data.empty())
        return 0;

    auto hash_key = this->hash(wgname, control);
    const auto iter = this->keys.find( hash_key );
    if (iter != this->keys.end()) {
        auto index = iter->second;
        auto& record = this->data[index];

        if (record.active)
            record.active = false;

        if (this->m_use_count[record.udq] > 0)
            this->m_use_count[record.udq] -= 1;

        return 1;
    }

    return 0;
}


int UDQActive::update(const UDAValue& uda, const std::string& wgname, UDAControl control) {
    if (uda.is<std::string>())
        return this->add(uda.get<std::string>(), wgname, control);
    else
        return this->drop(wgname, control);
}

std::vector<UDQActive::Record>::const_iterator UDQActive::begin() const {
    return this->data.begin();
}

std::vector<UDQActive::Record>::const_iterator UDQActive::end() const {
    return this->data.end();
}

const UDQActive::Record& UDQActive::operator[](std::size_t index) const {
    return this->data[index];
}

std::size_t UDQActive::use_count(const std::string& udq) const {
    const auto iter = this->m_use_count.find(udq);
    if (iter == this->m_use_count.end())
        return 0;

    return iter->second;
}

}

