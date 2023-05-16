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

#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Opm { namespace RestartIO {
    struct RstState;
}} // namespace Opm::RestartIO

namespace Opm {

class UDQState
{
public:
    UDQState() = default;
    explicit UDQState(double undefined);

    bool has(const std::string& key) const;
    void load_rst(const RestartIO::RstState& rst_state);

    bool has_well_var(const std::string& well, const std::string& key) const;
    bool has_group_var(const std::string& group, const std::string& key) const;
    bool has_segment_var(const std::string& well, const std::string& key, const std::size_t segment) const;

    double get(const std::string& key) const;
    double get_group_var(const std::string& well, const std::string& var) const;
    double get_well_var(const std::string& well, const std::string& var) const;
    double get_segment_var(const std::string& well, const std::string& var, const std::size_t segment) const;

    void add_define(std::size_t report_step, const std::string& udq_key, const UDQSet& result);
    void add_assign(std::size_t report_step, const std::string& udq_key, const UDQSet& result);
    bool assign(std::size_t report_step, const std::string& udq_key) const;
    bool define(const std::string& udq_key, const std::pair<UDQUpdate, std::size_t>& update_status) const;
    double undefined_value() const;

    bool operator==(const UDQState& other) const;

    static UDQState serializationTestObject();

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->undef_value);
        serializer(this->scalar_values);
        serializer(this->well_values);
        serializer(this->group_values);
        serializer(this->segment_values);
        serializer(this->assignments);
        serializer(this->defines);
    }

private:
    double undef_value;
    std::unordered_map<std::string, double> scalar_values{};

    // [var][well] -> double
    std::unordered_map<std::string, std::unordered_map<std::string, double>> well_values{};

    // [var][group] -> double
    std::unordered_map<std::string, std::unordered_map<std::string, double>> group_values{};

    // [var][well][segment] -> double
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::size_t, double>>> segment_values{};

    std::unordered_map<std::string, std::size_t> assignments;
    std::unordered_map<std::string, std::size_t> defines;

    void add(const std::string& udq_key, const UDQSet& result);
    double get_wg_var(const std::string& well, const std::string& key, UDQVarType var_type) const;
};

} // namespace Opm

#endif  // UDQSTATE_HPP_
