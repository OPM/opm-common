/*
  Copyright 2020 Equinor ASA.

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


#include <stdexcept>

#include <opm/common/utility/Serializer.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQState.hpp>

namespace Opm {

namespace {

bool is_udq(const std::string& key) {
    if (key.size() < 2)
        return false;

    if (key[1] != 'U')
        return false;

    return true;
}

}


double UDQState::undefined_value() const {
    return this->undef_value;
}


UDQState::UDQState(double undefined) :
    undef_value(undefined)
{}

bool UDQState::has(const std::string& key) const {
    auto res_iter = this->values.find(key);
    if (res_iter == this->values.end())
        return false;

    const auto& result = res_iter->second[0];
    return result.defined();
}

bool UDQState::has_well_var(const std::string& well, const std::string& key) const {
    auto res_iter = this->values.find(key);
    if (res_iter == this->values.end())
        return false;

    for (const auto& scalar : res_iter->second) {
        if (scalar.wgname() == well)
            return scalar.defined();
    }

    return false;
}

bool UDQState::has_group_var(const std::string& group, const std::string& key) const {
    return this->has_well_var(group, key);
}


void UDQState::add(const std::string& udq_key, const UDQSet& result) {
    if (!is_udq(udq_key))
        throw std::logic_error("Key is not a UDQ variable:" + udq_key);

    auto res_iter = this->values.find(udq_key);
    if (res_iter == this->values.end())
        this->values.insert( std::make_pair( udq_key, result ));
    else
        res_iter->second = result;
}

void UDQState::add_define(const std::string& udq_key, const UDQSet& result) {
    this->add(udq_key, result);
}

void UDQState::add_assign(std::size_t report_step, const std::string& udq_key, const UDQSet& result) {
    this->assignments[udq_key] = report_step;
    this->add(udq_key, result);
}

double UDQState::get(const std::string& key) const {
    if (!is_udq(key))
        throw std::logic_error("Key is not a UDQ variable:" + key);

    const auto& result = this->values.at(key)[0];
    if (result.defined())
        return result.get();
    else
        return this->undef_value;
}

double UDQState::get_wg_var(const std::string& wgname, const std::string& key, UDQVarType var_type) const {
    auto res_iter = this->values.find(key);
    if (res_iter == this->values.end()) {
        if (is_udq(key))
            throw std::out_of_range("No such UDQ variable: " + key);
        else
            throw std::logic_error("No such UDQ variable: " + key);
    }
    const auto& result_set = res_iter->second;
    if (result_set.var_type() != var_type)
        throw std::logic_error("Incompatible query function used");

    const auto& result = result_set[wgname];
    if (result.defined())
        return result.get();
    else
        return this->undef_value;
}

double UDQState::get_well_var(const std::string& well, const std::string& key) const {
    return this->get_wg_var(well, key, UDQVarType::WELL_VAR);
}

double UDQState::get_group_var(const std::string& group, const std::string& key) const {
    return this->get_wg_var(group, key, UDQVarType::GROUP_VAR);
}

bool UDQState::operator==(const UDQState& other) const {
    return this->undef_value == other.undef_value &&
           this->values == other.values;
}



bool UDQState::assign(std::size_t report_step, const std::string& udq_key) const {
    auto assign_iter = this->assignments.find(udq_key);
    if (assign_iter == this->assignments.end())
        return true;
    else
        return report_step > assign_iter->second;
}

std::vector<char> UDQState::serialize() const {
    Serializer ser;
    ser.put(this->undef_value);
    ser.put(this->values.size());
    for (const auto& set_pair : this->values) {
        ser.put( set_pair.first );
        set_pair.second.serialize( ser );
    }
    ser.put(this->assignments);
    return ser.buffer;
}


void UDQState::deserialize(const std::vector<char>& buffer) {
    Serializer ser(buffer);
    this->undef_value = ser.get<double>();
    this->values.clear();

    {
        std::size_t size = ser.get<std::size_t>();
        for (std::size_t index = 0; index < size; index++) {
            auto key = ser.get<std::string>();
            auto udq_set = UDQSet::deserialize(ser);

            this->values.insert( std::make_pair(key, udq_set) );
        }
    }
    this->assignments = ser.get<std::string, std::size_t>();
}
}


