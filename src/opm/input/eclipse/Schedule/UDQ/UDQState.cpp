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

#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>

#include <opm/io/eclipse/rst/state.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <unordered_map>

#include <fmt/format.h>

namespace {

template <typename K, typename V>
using UMap = std::unordered_map<K, V>;

template <typename V>
using SMap = UMap<std::string, V>;

template <typename T>
using S2Map = SMap<SMap<T>>;

template <typename K, typename V>
using SKMap = SMap<UMap<K, V>>;

template <typename K, typename V>
using S2KMap = S2Map<UMap<K, V>>;

bool is_udq(const std::string& key)
{
    return (key.size() >= std::string::size_type{2})
        && (key[1] == 'U');
}

bool has_var(const S2Map<double>& values,
             const std::string&   wgname,
             const std::string&   udq_key)
{
    auto res_iter = values.find(udq_key);
    return (res_iter != values.end())
        && (res_iter->second.count(wgname) > 0);
}

void undefine_results(const Opm::UDQScalar& result,
                      SMap<double>&         values)
{
    values.erase(result.wgname());
}

void undefine_results(const Opm::UDQScalar&       result,
                      SKMap<std::size_t, double>& values)
{
    auto wellPos = values.find(result.wgname());
    if (wellPos == values.end()) {
        // No results for this well.  Nothing to do.
        return;
    }

    wellPos->second.erase(result.number());
}

void add_defined_results(const Opm::UDQScalar& result,
                         SMap<double>&         values)
{
    values.insert_or_assign(result.wgname(), result.get());
}

void add_defined_results(const Opm::UDQScalar&       result,
                         SKMap<std::size_t, double>& values)
{
    auto wellPos = values.find(result.wgname());
    if (wellPos == values.end()) {
        values.insert_or_assign(result.wgname(), std::unordered_map<std::size_t, double>
                                {{ result.number(), result.get() }});

        return;
    }

    wellPos->second.insert_or_assign(result.number(), result.get());
}

void add_results(const std::string& udq_key,
                 const Opm::UDQSet& result,
                 S2Map<double>&     values)
{
    auto& udq_values = values[udq_key];
    for (const auto& res1 : result) {
        if (! res1.defined()) {
            undefine_results(res1, udq_values);
        }
        else {
            add_defined_results(res1, udq_values);
        }
    }
}

void add_results(const std::string&           udq_key,
                 const Opm::UDQSet&           result,
                 S2KMap<std::size_t, double>& values)
{
    auto& udq_values = values[udq_key];
    for (const auto& res1 : result) {
        if (! res1.defined()) {
            undefine_results(res1, udq_values);
        }
        else {
            add_defined_results(res1, udq_values);
        }
    }
}

double get_scalar(const SMap<double>& values,
                  const std::string&  udq_key,
                  const double        undef_value)
{
    auto iter = values.find(udq_key);
    if (iter == values.end()) {
        return undef_value;
    }

    return iter->second;
}

double get_wg(const S2Map<double>& values,
              const std::string&   wgname,
              const std::string&   udq_key,
              const double         undef_value)
{
    auto res_iter = values.find(udq_key);
    if (res_iter == values.end()) {
        if (is_udq(udq_key)) {
            throw std::out_of_range("No such UDQ variable: " + udq_key);
        }
        else {
            throw std::logic_error("No such UDQ variable: " + udq_key);
        }
    }

    return get_scalar(res_iter->second, wgname, undef_value);
}

} // Anonymous namespace

namespace Opm {

void UDQState::load_rst(const RestartIO::RstState& rst_state)
{
    for (const auto& udq : rst_state.udqs) {
        if (udq.is_define()) {
            if (udq.var_type == UDQVarType::WELL_VAR) {
                auto& values = this->well_values[udq.name];
                for (const auto& [wname, value] : udq.values()) {
                    values.insert_or_assign(wname, value);
                }
            }

            if (udq.var_type == UDQVarType::GROUP_VAR) {
                auto& values = this->group_values[udq.name];
                for (const auto& [gname, value] : udq.values()) {
                    values.insert_or_assign(gname, value);
                }
            }

            if (const auto& field_value = udq.field_value(); field_value.has_value()) {
                this->scalar_values[udq.name] = field_value.value();
            }
        }
        else {
            const auto value = udq.assign_value();

            if ((udq.var_type == UDQVarType::WELL_VAR) &&
                ! udq.assign_selector().empty())
            {
                auto& values = this->well_values[udq.name];
                for (const auto& wname : udq.assign_selector()) {
                    values.insert_or_assign(wname, value);
                }
            }

            if (udq.var_type == UDQVarType::GROUP_VAR) {
                auto& values = this->group_values[udq.name];
                for (const auto& gname : udq.assign_selector()) {
                    values.insert_or_assign(gname, value);
                }
            }

            if (udq.var_type == UDQVarType::FIELD_VAR) {
                this->scalar_values[udq.name] = value;
            }
        }
    }
}

double UDQState::undefined_value() const
{
    return this->undef_value;
}

UDQState::UDQState(double undefined)
    : undef_value(undefined)
{}

bool UDQState::has(const std::string& key) const
{
    return this->scalar_values.count(key);
}

bool UDQState::has_well_var(const std::string& well, const std::string& key) const
{
    return has_var(this->well_values, well, key);
}

bool UDQState::has_group_var(const std::string& group, const std::string& key) const
{
    return has_var(this->group_values, group, key);
}

bool UDQState::has_segment_var(const std::string& well,
                               const std::string& key,
                               const std::size_t  segment) const
{
    auto varPos = this->segment_values.find(key);
    if (varPos == this->segment_values.end()) {
        return false;
    }

    auto wellPos = varPos->second.find(well);
    if (wellPos == varPos->second.end()) {
        return false;
    }

    return wellPos->second.find(segment) != wellPos->second.end();
}

void UDQState::add(const std::string& udq_key, const UDQSet& result)
{
    if (!is_udq(udq_key)) {
        throw std::logic_error {
            fmt::format("{} is not a UDQ variable", udq_key)
        };
    }

    switch (result.var_type()) {
    case UDQVarType::WELL_VAR:
        add_results(udq_key, result, this->well_values);
        break;

    case UDQVarType::GROUP_VAR:
        add_results(udq_key, result, this->group_values);
        break;

    case UDQVarType::SEGMENT_VAR:
        add_results(udq_key, result, this->segment_values);
        break;

    default:
        // Scalar
        if (const auto& scalar = result[0]; scalar.defined()) {
            this->scalar_values.insert_or_assign(udq_key, scalar.get());
        }
        else {
            this->scalar_values.erase(udq_key);
        }
        break;
    }
}

void UDQState::add_define(std::size_t report_step, const std::string& udq_key, const UDQSet& result)
{
    this->defines[udq_key] = report_step;
    this->add(udq_key, result);
}

void UDQState::add_assign(std::size_t report_step, const std::string& udq_key, const UDQSet& result)
{
    this->assignments[udq_key] = report_step;
    this->add(udq_key, result);
}

double UDQState::get(const std::string& key) const
{
    if (!is_udq(key)) {
        throw std::logic_error("Key is not a UDQ variable:" + key);
    }

    auto iter = this->scalar_values.find(key);
    if (iter == this->scalar_values.end())
        throw std::out_of_range("Invalid key: " + key);

    return iter->second;
}

double UDQState::get_group_var(const std::string& group, const std::string& key) const
{
    return get_wg(this->group_values, group, key, this->undef_value);
}

double UDQState::get_well_var(const std::string& well, const std::string& key) const
{
    return get_wg(this->well_values, well, key, this->undef_value);
}

double UDQState::get_segment_var(const std::string& well,
                                 const std::string& var,
                                 const std::size_t  segment) const
{
    if (! is_udq(var)) {
        throw std::logic_error {
            fmt::format("Cannot evaluate non-UDQ variable '{}'", var)
        };
    }

    auto varPos = this->segment_values.find(var);
    if (varPos == this->segment_values.end()) {
        throw std::out_of_range {
            fmt::format("'{}' is not a valid segment UDQ variable", var)
        };
    }

    auto wellPos = varPos->second.find(well);
    if (wellPos == varPos->second.end()) {
        throw std::out_of_range {
            fmt::format("'{}' is not a valid segment UDQ "
                        "variable for well '{}'", var, well)
        };
    }

    auto valPos = wellPos->second.find(segment);
    if (valPos == wellPos->second.end()) {
        throw std::invalid_argument {
            fmt::format("'{}' is not a valid segment UDQ "
                        "variable for segment {} in well '{}'",
                        var, segment, well)
        };
    }

    return valPos->second;
}

bool UDQState::operator==(const UDQState& other) const
{
    return (this->undef_value == other.undef_value)
        && (this->scalar_values == other.scalar_values)
        && (this->well_values == other.well_values)
        && (this->group_values == other.group_values)
        && (this->segment_values == other.segment_values)
        && (this->assignments == other.assignments)
        && (this->defines == other.defines);
}

UDQState UDQState::serializationTestObject()
{
    UDQState st;
    st.undef_value = 78;
    st.scalar_values = {{"FU1", 100}, {"FU2", 200}};
    st.assignments = {{"GU1", 99}, {"GU2", 199}};
    st.defines = {{"DU1", 299}, {"DU2", 399}};

    st.well_values.emplace("W1", std::unordered_map<std::string, double>{{"U1", 100}, {"U2", 200}});
    st.well_values.emplace("W2", std::unordered_map<std::string, double>{{"U1", 700}, {"32", 600}});

    st.group_values.emplace("G1", std::unordered_map<std::string, double>{{"U1", 100}, {"U2", 200}});
    st.group_values.emplace("G2", std::unordered_map<std::string, double>{{"U1", 700}, {"32", 600}});

    {
        auto& sval = st.segment_values["SU1"];
        sval.emplace("W1", std::unordered_map<std::size_t, double> {
                { std::size_t{ 1},  123.456   },
                { std::size_t{ 2},   17.29    },
                { std::size_t{10}, -  2.71828 },
            });

        sval.emplace("W6", std::unordered_map<std::size_t, double> {
                { std::size_t{ 7}, 3.1415926535 },
            });
    }

    {
        auto& sval = st.segment_values["SUVIS"];
        sval.emplace("I2", std::unordered_map<std::size_t, double> {
                { std::size_t{17},  29.0   },
                { std::size_t{42}, - 1.618 },
            });
    }

    // Deliberately creating an element with an empty value.  Not likely to
    // occur in a real run, but we should be able to handle that case too.
    st.segment_values["SUSPECT"];

    return st;
}

bool UDQState::assign(std::size_t report_step, const std::string& udq_key) const
{
    auto assign_iter = this->assignments.find(udq_key);

    return (assign_iter == this->assignments.end())
        || (report_step > assign_iter->second);
}

bool UDQState::define(const std::string&                       udq_key,
                      const std::pair<UDQUpdate, std::size_t>& update_status) const
{
    if (update_status.first == UDQUpdate::ON) {
        return true;
    }

    if (update_status.first == UDQUpdate::OFF) {
        return false;
    }

    auto define_iter = this->defines.find(udq_key);
    return (define_iter == this->defines.end())
        || (define_iter->second < update_status.second);
}

} // namespace Opm
