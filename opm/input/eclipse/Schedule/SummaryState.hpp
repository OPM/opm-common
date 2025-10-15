/*
  Copyright 2016 Statoil ASA.

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

#ifndef SUMMARY_STATE_H
#define SUMMARY_STATE_H

#include <opm/common/utility/TimeService.hpp>

#include <cstddef>
#include <ctime>
#include <iosfwd>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

class UDQSet;

} // namespace Opm

// The purpose of the SummaryState class is to serve as a small container
// object for computed, ready to use summary values.  The values will
// typically be used by the UDQ, WTEST and ACTIONX calculations.  Observe
// that all values are stored in the run's output unit conventions.
//
// The main key used to access the content of this container is the eclipse
// style colon separated string - i.e. 'WWCT:OPX' to get the watercut in
// well 'OPX'.  The main usage of the SummaryState class is a temporary
// holding ground while assembling data for the summary output, but it is
// also used as a context object when evaulating the condition in ACTIONX
// keywords. For that reason some of the data is duplicated both in the
// general structure and a specialized structure:
//
//     SummaryState st { start, udqUndefined };
//
//     st.add_well_var("OPX", "WWCT", 0.75);
//     st.add("WGOR:OPY", 120);
//
//     // The WWCT:OPX key has been added with the specialized add_well_var()
//     // method and this data is available both with the general
//     // st.has("WWCT:OPX") and the specialized st.has_well_var("OPX", "WWCT");
//     st.has("WWCT:OPX") => True
//     st.has_well_var("OPX", "WWCT") => True
//
//     // The WGOR:OPY key is added with the general add("WGOR:OPY") and is *not*
//     // accessible through the specialized st.has_well_var("OPY", "WGOR").
//     st.has("WGOR:OPY") => True
//     st.has_well_var("OPY", "WGOR") => False

namespace Opm {

class SummaryState
{
public:
    using const_iterator = std::unordered_map<std::string, double>::const_iterator;

    explicit SummaryState(time_point sim_start_arg, double udqUndefined);

    // The std::time_t constructor is only for export to Python
    explicit SummaryState(std::time_t sim_start_arg);

    // Only used for testing purposes.
    SummaryState() : SummaryState(std::time_t{0}) {}
    ~SummaryState() = default;

    // The canonical way to update the SummaryState is through the
    // update_xxx() methods which will inspect the variable and either
    // accumulate or just assign, depending on whether it represents a total
    // or not. The set() method is low level and unconditionally do an
    // assignment.
    void set(const std::string& key, double value);

    bool erase(const std::string& key);
    bool erase_well_var(const std::string& well, const std::string& var);
    bool erase_group_var(const std::string& group, const std::string& var);

    bool has(const std::string& key) const;
    bool has_well_var(const std::string& well, const std::string& var) const;
    bool has_well_var(const std::string& var) const;
    bool has_group_var(const std::string& group, const std::string& var) const;
    bool has_group_var(const std::string& var) const;
    bool has_conn_var(const std::string& well, const std::string& var, std::size_t global_index) const;
    bool has_segment_var(const std::string& well, const std::string& var, std::size_t segment) const;
    bool has_region_var(const std::string& regSet, const std::string& var, std::size_t region) const;

    void update(const std::string& key, double value);
    void update_well_var(const std::string& well, const std::string& var, double value);
    void update_group_var(const std::string& group, const std::string& var, double value);
    void update_elapsed(double delta);
    void update_udq(const UDQSet& udq_set);
    void update_conn_var(const std::string& well, const std::string& var, std::size_t global_index, double value);
    void update_segment_var(const std::string& well, const std::string& var, std::size_t segment, double value);
    void update_region_var(const std::string& regSet, const std::string& var, std::size_t region, double value);

    double get(const std::string&) const;
    double get(const std::string&, double) const;
    double get_elapsed() const;
    double get_well_var(const std::string& well, const std::string& var) const;
    double get_group_var(const std::string& group, const std::string& var) const;
    double get_conn_var(const std::string& conn, const std::string& var, std::size_t global_index) const;
    double get_segment_var(const std::string& well, const std::string& var, std::size_t segment) const;
    double get_region_var(const std::string& regSet, const std::string& var, std::size_t region) const;
    double get_well_var(const std::string& well, const std::string& var, double) const;
    double get_group_var(const std::string& group, const std::string& var, double) const;
    double get_conn_var(const std::string& conn, const std::string& var, std::size_t global_index, double) const;
    double get_segment_var(const std::string& well, const std::string& var, std::size_t segment, double) const;
    double get_region_var(const std::string& regSet, const std::string& var, std::size_t region, double) const;
    double get_udq_undefined() const { return udq_undefined; }

    bool is_undefined_value(const double val) const { return val == udq_undefined; }

    const std::vector<std::string>& wells() const;
    std::vector<std::string> wells(const std::string& var) const;
    const std::vector<std::string>& groups() const;
    std::vector<std::string> groups(const std::string& var) const;
    void append(const SummaryState& buffer);
    const_iterator begin() const;
    const_iterator end() const;
    std::size_t num_wells() const;
    std::size_t size() const;
    bool operator==(const SummaryState& other) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(sim_start);
        serializer(this->udq_undefined);
        serializer(elapsed);
        serializer(values);
        serializer(well_values);
        serializer(m_wells);
        serializer(well_names);
        serializer(group_values);
        serializer(m_groups);
        serializer(group_names);
        serializer(conn_values);
        serializer(segment_values);
        serializer(this->region_values);
    }

    static SummaryState serializationTestObject();

private:
    time_point sim_start;
    double udq_undefined{};
    double elapsed = 0;
    std::unordered_map<std::string,double> values;

    // The first key is the variable and the second key is the well.
    std::unordered_map<std::string, std::unordered_map<std::string, double>> well_values;
    std::set<std::string> m_wells;
    mutable std::optional<std::vector<std::string>> well_names;

    // The first key is the variable and the second key is the group.
    std::unordered_map<std::string, std::unordered_map<std::string, double>> group_values;
    std::set<std::string> m_groups;
    mutable std::optional<std::vector<std::string>> group_names;

    // The first key is the variable and the second key is the well and the
    // third is the global index. NB: The global_index has offset 1!
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::size_t, double>>> conn_values;

    // The first key is the variable and the second key is the well and the
    // third is the one-based segment number.
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::size_t, double>>> segment_values;

    // First key is variable (e.g., ROIP), second key is region set (e.g.,
    // FIPNUM, FIPABC), and the third key is the one-based region number.
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::size_t, double>>> region_values;
};

std::ostream& operator<<(std::ostream& stream, const SummaryState& st);

} // namespace Opm

#endif  // SUMMARY_STATE_H
