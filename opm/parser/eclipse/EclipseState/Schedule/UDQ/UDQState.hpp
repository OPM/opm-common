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

#ifndef UDQSTATE_HPP_
#define UDQSTATE_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQSet.hpp>


namespace Opm {

namespace RestartIO {
    struct RstState;
}

class UDQState {
public:
    UDQState(double undefined);

    bool has(const std::string& key) const;
    void load_rst(const RestartIO::RstState& rst_state);

    bool has_well_var(const std::string& well, const std::string& key) const;
    bool has_group_var(const std::string& group, const std::string& key) const;

    double get(const std::string& key) const;
    double get_group_var(const std::string& well, const std::string& var) const;
    double get_well_var(const std::string& well, const std::string& var) const;
    void add_define(std::size_t report_step, const std::string& udq_key, const UDQSet& result);
    void add_assign(std::size_t report_step, const std::string& udq_key, const UDQSet& result);
    bool assign(std::size_t report_step, const std::string& udq_key) const;
    bool define(const std::string& udq_key, std::pair<UDQUpdate, std::size_t> update_status) const;
    double undefined_value() const;

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& buffer);
    bool operator==(const UDQState& other) const;
private:
    void add(const std::string& udq_key, const UDQSet& result);
    double get_wg_var(const std::string& well, const std::string& key, UDQVarType var_type) const;
    double undef_value;
    std::unordered_map<std::string, double> scalar_values;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> well_values;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> group_values;
    std::unordered_map<std::string, std::size_t> assignments;
    std::unordered_map<std::string, std::size_t> defines;
};
}

#endif
