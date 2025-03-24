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

#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>

#include <opm/io/eclipse/SummaryNode.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

namespace {

    bool is_udq(std::string_view keyword)
    {
        // Does 'keyword' match one of the patterns
        //   AU*, BU*, CU*, FU*, GU*, RU*, SU*, or WU*?
        using sz_t = std::string_view::size_type;

        return (keyword.size() > sz_t{1})
            && (keyword[1] == 'U')
            && (keyword.find_first_of("WGFCRBSA") == sz_t{0});
    }

    bool is_well_udq(std::string_view keyword)
    {
        // Does 'keyword' match the pattern
        //   WU*?
        using sz_t = std::string_view::size_type;

        return (keyword.size() > sz_t{1})
            && (keyword.compare(0, 2, "WU") == 0);
    }

    bool is_group_udq(std::string_view keyword)
    {
        // Does 'keyword' match the pattern
        //   GU*?
        using sz_t = std::string_view::size_type;

        return (keyword.size() > sz_t{1})
            && (keyword.compare(0, 2, "GU") == 0);
    }

    bool is_segment_udq(std::string_view keyword)
    {
        // Does 'keyword' match the pattern
        //   SU*?
        using sz_t = std::string_view::size_type;

        return (keyword.size() > sz_t{1})
            && (keyword.compare(0, 2, "SU") == 0);
    }

    bool is_total(const std::string& key)
    {
        static const std::vector<std::string> totals = {
            "OPT"  , "GPT"  , "WPT" , "GIT", "WIT", "OPTF" , "OPTS" , "OIT"  , "OVPT" , "OVIT" , "MWT" ,
            "WVPT" , "WVIT" , "GMT"  , "GPTF" , "SGT"  , "GST" , "FGT" , "GCT" , "GIMT" ,
            "WGPT" , "WGIT" , "EGT"  , "EXGT" , "GVPT" , "GVIT" , "LPT" , "VPT" , "VIT" , "NPT" , "NIT",
            "TPT", "TIT", "CPT", "CIT", "SPT", "SIT", "EPT", "EIT", "TPTHEA", "TITHEA",
            "MMIT", "MOIT", "MUIT", "MMPT", "MOPT", "MUPT",
            "OFT", "OFT+", "OFT-", "OFTG", "OFTL",
            "GFT", "GFT+", "GFT-", "GFTG", "GFTL",
            "WFT", "WFT+", "WFT-", "GMIT", "GMPT",
        };

        auto sep_pos = key.find(':');

        // Starting with ':' - that is probably broken?!
        if (sep_pos == 0)
            return false;

        if (sep_pos == std::string::npos) {
            return std::any_of(totals.begin(), totals.end(),
                               [&key](const auto& total)
                               {
                                   return key.compare(1, total.size(), total) == 0;
                               });
        } else
            return is_total(key.substr(0,sep_pos));
    }

    template <class T>
    using map2 = std::unordered_map<std::string, std::unordered_map<std::string, T>>;

    template <class T>
    bool has_var(const map2<T>& values,
                 const std::string& var1,
                 const std::string& var2)
    {
        const auto& var1_iter = values.find(var1);
        if (var1_iter == values.end())
            return false;

        const auto& var2_iter = var1_iter->second.find(var2);
        if (var2_iter == var1_iter->second.end())
            return false;

        return true;
    }

    template <class T>
    void erase_var(map2<T>& values,
                   std::set<std::string>& var2_set,
                   const std::string& var1,
                   const std::string& var2)
    {
        const auto& var1_iter = values.find(var1);
        if (var1_iter == values.end())
            return;

        var1_iter->second.erase(var2);
        var2_set.clear();
        for (const auto& [_, var2_map] : values) {
            (void)_;
            for (const auto& [v2, __] : var2_map) {
                (void)__;
                var2_set.insert(v2);
            }
        }
    }

    template <class T>
    std::vector<std::string>
    var2_list(const map2<T>& values, const std::string& var1)
    {
        const auto& var1_iter = values.find(var1);
        if (var1_iter == values.end())
            return {};

        std::vector<std::string> l;
        std::transform(var1_iter->second.begin(), var1_iter->second.end(),
                       std::back_inserter(l),
                       [](const auto& pair) { return pair.first; });

        return l;
    }

    std::string normalise_region_set_name(const std::string& regSet)
    {
        if (regSet.empty()) {
            return "NUM";       // "" -> FIPNUM
        }

        // Discard initial "FIP" prefix if it exists.
        const auto maxchar = std::string::size_type{3};
        const auto prefix = std::string_view { "FIP" };
        const auto start = prefix.size() * (regSet.find(prefix) == std::string::size_type{0});
        return regSet.substr(start, maxchar);
    }

    std::string region_key(const std::string& variable,
                           const std::string& regSet,
                           const std::size_t  region)
    {
        auto node = Opm::EclIO::SummaryNode {
            variable, Opm::EclIO::SummaryNode::Category::Region
        };

        node.number = static_cast<int>(region);

        if (! regSet.empty() && (regSet != "NUM") && (regSet != "FIPNUM")) {
            // Generate summary vector names of the forms
            //   * RPR__ABC
            //   * ROPT_ABC
            //   * RODENABC
            // to uniquely identify vectors in the 'FIPABC' region set.
            node.keyword = fmt::format("{:_<5}{}", node.keyword,
                                       normalise_region_set_name(regSet));
        }

        return node.unique_key();
    }

} // Anonymous namespace

namespace Opm
{

    SummaryState::SummaryState(const time_point sim_start_arg,
                               const double     udqUndefined)
        : sim_start     { sim_start_arg }
        , udq_undefined { udqUndefined }
    {
        this->update_elapsed(0);
    }

    SummaryState::SummaryState(const std::time_t sim_start_arg)
        : SummaryState { TimeService::from_time_t(sim_start_arg),
                         std::numeric_limits<double>::lowest() }
    {}

    void SummaryState::set(const std::string& key, double value)
    {
        this->values.insert_or_assign(key, value);
    }

    bool SummaryState::erase(const std::string& key) {
        return (this->values.erase(key) > 0);
    }

    bool SummaryState::erase_well_var(const std::string& well, const std::string& var)
    {
        std::string key = var + ":" + well;
        if (!this->erase(key))
            return false;

        erase_var(this->well_values, this->m_wells, var, well);
        this->well_names.reset();
        return true;
    }

    bool SummaryState::erase_group_var(const std::string& group, const std::string& var)
    {
        std::string key = var + ":" + group;
        if (!this->erase(key))
            return false;

        erase_var(this->group_values, this->m_groups, var, group);
        this->group_names.reset();
        return true;
    }

    bool SummaryState::has(const std::string& key) const
    {
        return (this->values.find(key) != this->values.end()) || is_udq(key);
    }

    bool SummaryState::has_well_var(const std::string& well,
                                    const std::string& var) const
    {
        return has_var(this->well_values, var, well)
            || is_well_udq(var);
    }

    bool SummaryState::has_well_var(const std::string& var) const
    {
        return (this->well_values.count(var) != 0) || is_well_udq(var);
    }

    bool SummaryState::has_group_var(const std::string& group,
                                     const std::string& var) const
    {
        return has_var(this->group_values, var, group) || is_group_udq(var);
    }

    bool SummaryState::has_group_var(const std::string& var) const
    {
        return (this->group_values.count(var) != 0) || is_group_udq(var);
    }

    bool SummaryState::has_conn_var(const std::string& well,
                                    const std::string& var,
                                    const std::size_t  global_index) const
    {
        // Connection Values = [var][well][index] -> double

        auto varPos = this->conn_values.find(var);
        if (varPos == this->conn_values.end()) {
            return false;
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            return false;
        }

        return wellPos->second.find(global_index) != wellPos->second.end();
    }

    bool SummaryState::has_segment_var(const std::string& well,
                                       const std::string& var,
                                       const std::size_t  segment) const
    {
        // Segment Values = [var][well][segment] -> double

        auto varPos = this->segment_values.find(var);
        if (varPos == this->segment_values.end()) {
            return false;
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            return false;
        }

        return (wellPos->second.find(segment) != wellPos->second.end())
            || is_segment_udq(var);
    }

    bool SummaryState::has_region_var(const std::string& regSet,
                                      const std::string& var,
                                      const std::size_t  region) const
    {
        // Region Values = [var][regSet][region] -> double

        auto varPos = this->region_values.find(EclIO::SummaryNode::normalise_region_keyword(var));
        if (varPos == this->region_values.end()) {
            return false;
        }

        auto regSetPos = varPos->second.find(normalise_region_set_name(regSet));
        if (regSetPos == varPos->second.end()) {
            return false;
        }

        return regSetPos->second.find(region) != regSetPos->second.end();
    }

    void SummaryState::update(const std::string& key, double value)
    {
        auto& val_ref = this->values[key];

        if (is_total(key)) {
            val_ref += value;
        }
        else {
            val_ref = value;
        }
    }

    void SummaryState::update_well_var(const std::string& well,
                                       const std::string& var,
                                       const double       value)
    {
        auto& val_ref  = this->values[fmt::format("{}:{}", var, well)];
        auto& wval_ref = this->well_values[var][well];

        if (is_total(var)) {
            val_ref  += value;
            wval_ref += value;
        }
        else {
            val_ref = wval_ref = value;
        }

        if (this->m_wells.count(well) == 0) {
            this->m_wells.insert(well);
            this->well_names.reset();
        }
    }

    void SummaryState::update_group_var(const std::string& group,
                                        const std::string& var,
                                        const double       value)
    {
        auto& val_ref  = this->values[fmt::format("{}:{}", var, group)];
        auto& gval_ref = this->group_values[var][group];

        if (is_total(var)) {
            val_ref  += value;
            gval_ref += value;
        }
        else {
            val_ref = gval_ref = value;
        }

        if (this->m_groups.count(group) == 0) {
            this->m_groups.insert(group);
            this->group_names.reset();
        }
    }

    void SummaryState::update_elapsed(double delta)
    {
        this->elapsed += delta;
    }

    void SummaryState::update_udq(const UDQSet& udq_set)
    {
        const auto var_type = udq_set.var_type();
        if (var_type == UDQVarType::WELL_VAR) {
            for (const auto& udq_value : udq_set) {
                this->update_well_var(udq_value.wgname(), udq_set.name(), udq_value.value().value_or(this->udq_undefined));
            }
        }
        else if (var_type == UDQVarType::GROUP_VAR) {
            for (const auto& udq_value : udq_set) {
                this->update_group_var(udq_value.wgname(), udq_set.name(), udq_value.value().value_or(this->udq_undefined));
            }
        }
        else if (var_type == UDQVarType::SEGMENT_VAR) {
            for (const auto& udq_value : udq_set) {
                this->update_segment_var(udq_value.wgname(),
                                         udq_set.name(),
                                         udq_value.number(),
                                         udq_value.value().value_or(this->udq_undefined));
            }
        }
        else {
            const auto& udq_var = udq_set[0].value();
            this->update(udq_set.name(), udq_var.value_or(this->udq_undefined));
        }
    }

    void SummaryState::update_conn_var(const std::string& well,
                                       const std::string& var,
                                       const std::size_t  global_index,
                                       const double       value)
    {
        auto& val_ref  = this->values[fmt::format("{}:{}:{}", var, well, global_index)];
        auto& cval_ref = this->conn_values[var][well][global_index];

        if (is_total(var)) {
            val_ref  += value;
            cval_ref += value;
        }
        else {
            val_ref = cval_ref = value;
        }
    }

    void SummaryState::update_segment_var(const std::string& well,
                                          const std::string& var,
                                          const std::size_t  segment,
                                          const double       value)
    {
        auto& val_ref  = this->values[fmt::format("{}:{}:{}", var, well, segment)];
        auto& sval_ref = this->segment_values[var][well][segment];

        if (is_total(var)) {
            val_ref  += value;
            sval_ref += value;
        }
        else {
            val_ref = sval_ref = value;
        }
    }

    void SummaryState::update_region_var(const std::string& regSet,
                                         const std::string& var,
                                         const std::size_t  region,
                                         const double       value)
    {
        const auto regKw = EclIO::SummaryNode::normalise_region_keyword(var);

        auto& val_ref  = this->values[region_key(regKw, regSet, region)];
        auto& rval_ref = this->region_values[regKw][normalise_region_set_name(regSet)][region];

        if (is_total(regKw)) {
            val_ref  += value;
            rval_ref += value;
        }
        else {
            val_ref = rval_ref = value;
        }
    }

    double SummaryState::get(const std::string& key) const
    {
        auto iter = this->values.find(key);
        if (iter != this->values.end()) {
            return iter->second;
        }

        if (is_udq(key)) {
            return this->udq_undefined;
        }

        throw std::out_of_range {
            fmt::format("Summary vector {} is unknown", key)
        };
    }

    double SummaryState::get(const std::string& key,
                             const double       default_value) const
    {
        auto iter = this->values.find(key);
        if (iter != this->values.end()) {
            return iter->second;
        }

        if (is_udq(key)) {
            return this->udq_undefined;
        }

        return default_value;
    }

    double SummaryState::get_elapsed() const
    {
        return this->elapsed;
    }

    double SummaryState::get_well_var(const std::string& well,
                                      const std::string& var) const
    {
        const auto use_udq_fallback = is_well_udq(var);

        auto varPos = this->well_values.find(var);
        if (varPos == this->well_values.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist at the well level", var)
                };
            }

            return this->udq_undefined;
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist at the well level for well {}",
                                var, well)
                };
            }

            return this->udq_undefined;
        }

        return wellPos->second;
    }

    double SummaryState::get_group_var(const std::string& group,
                                       const std::string& var) const
    {
        const auto use_udq_fallback = is_group_udq(var);

        auto varPos = this->group_values.find(var);
        if (varPos == this->group_values.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist at the group level", var)
                };
            }

            return this->udq_undefined;
        }

        auto groupPos = varPos->second.find(group);
        if (groupPos == varPos->second.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist at the group level for group {}",
                                var, group)
                };
            }

            return this->udq_undefined;
        }

        return groupPos->second;
    }

    double SummaryState::get_conn_var(const std::string& well,
                                      const std::string& var,
                                      const std::size_t  global_index) const
    {
        auto varPos = this->conn_values.find(var);
        if (varPos == this->conn_values.end()) {
            throw std::invalid_argument {
                fmt::format("Summary vector {} does not "
                            "exist at the connection level", var)
            };
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            throw std::invalid_argument {
                fmt::format("Summary vector {} does not "
                            "exist at the connection "
                            "level for well {}",
                            var, well)
            };
        }

        auto connPos = wellPos->second.find(global_index);
        if (connPos == wellPos->second.end()) {
            throw std::invalid_argument {
                fmt::format("Summary vector {} does not "
                            "exist for connection {} "
                            "in well {}",
                            var, global_index, well)
            };
        }

        return connPos->second;
    }

    double SummaryState::get_segment_var(const std::string& well,
                                         const std::string& var,
                                         const std::size_t  segment) const
    {
        const auto use_udq_fallback = is_segment_udq(var);

        auto varPos = this->segment_values.find(var);
        if (varPos == this->segment_values.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist at the segment level", var)
                };
            }

            return this->udq_undefined;
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist at the segment "
                                "level for well {}",
                                var, well)
                };
            }

            return this->udq_undefined;
        }

        auto segPos = wellPos->second.find(segment);
        if (segPos == wellPos->second.end()) {
            if (! use_udq_fallback) {
                throw std::invalid_argument {
                    fmt::format("Summary vector {} does not "
                                "exist for segment {} "
                                "in well {}",
                                var, segment, well)
                };
            }

            return this->udq_undefined;
        }

        return segPos->second;
    }

    double SummaryState::get_region_var(const std::string& regSet,
                                        const std::string& var,
                                        const std::size_t  region) const
    {
        auto varPos = this->region_values.find(EclIO::SummaryNode::normalise_region_keyword(var));
        if (varPos == this->region_values.end()) {
            throw std::invalid_argument {
                fmt::format("Summary vector {} does not "
                            "exist at the region level", var)
            };
        }

        auto regSetPos = varPos->second.find(normalise_region_set_name(regSet));
        if (regSetPos == varPos->second.end()) {
            throw std::invalid_argument {
                fmt::format("Summary vector {} does not "
                            "exist at the region "
                            "level for region set {}",
                            var, regSet)
            };
        }

        auto regionPos = regSetPos->second.find(region);
        if (regionPos == regSetPos->second.end()) {
            throw std::invalid_argument {
                fmt::format("Summary vector {} does not "
                            "exist for region {} "
                            "in region set {}",
                            var, region, regSet)
            };
        }

        return regionPos->second;
    }

    double SummaryState::get_well_var(const std::string& well,
                                      const std::string& var,
                                      const double       default_value) const
    {
        const auto fallback = is_well_udq(var)
            ? this->udq_undefined
            : default_value;

        auto varPos = this->well_values.find(var);
        if (varPos == this->well_values.end()) {
            return fallback;
        }

        auto wellPos = varPos->second.find(well);
        return (wellPos == varPos->second.end())
            ? fallback
            : wellPos->second;
    }

    double SummaryState::get_group_var(const std::string& group,
                                       const std::string& var,
                                       const double       default_value) const
    {
        const auto fallback = is_group_udq(var)
            ? this->udq_undefined
            : default_value;

        auto varPos = this->group_values.find(var);
        if (varPos == this->group_values.end()) {
            return fallback;
        }

        auto groupPos = varPos->second.find(group);
        return (groupPos == varPos->second.end())
            ? fallback
            : groupPos->second;
    }

    double SummaryState::get_conn_var(const std::string& well,
                                      const std::string& var,
                                      const std::size_t  global_index,
                                      const double       default_value) const
    {
        auto varPos = this->conn_values.find(var);
        if (varPos == this->conn_values.end()) {
            return default_value;
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            return default_value;
        }

        auto connPos = wellPos->second.find(global_index);
        return (connPos == wellPos->second.end())
            ? default_value
            : connPos->second;
    }

    double SummaryState::get_segment_var(const std::string& well,
                                         const std::string& var,
                                         const std::size_t  segment,
                                         const double       default_value) const
    {
        auto varPos = this->segment_values.find(var);
        if (varPos == this->segment_values.end()) {
            return default_value;
        }

        auto wellPos = varPos->second.find(well);
        if (wellPos == varPos->second.end()) {
            return default_value;
        }

        auto valPos = wellPos->second.find(segment);
        return (valPos == wellPos->second.end())
            ? default_value
            : valPos->second;
    }

    const std::vector<std::string>& SummaryState::wells() const
    {
        if (!this->well_names.has_value()) {
            this->well_names.emplace(this->m_wells.begin(), this->m_wells.end());
        }

        return *this->well_names;
    }

    std::vector<std::string> SummaryState::wells(const std::string& var) const
    {
        return var2_list(this->well_values, var);
    }

    const std::vector<std::string>& SummaryState::groups() const
    {
        if (!this->group_names.has_value()) {
            this->group_names.emplace(this->m_groups.begin(), this->m_groups.end());
        }

        return *this->group_names;
    }

    std::vector<std::string> SummaryState::groups(const std::string& var) const
    {
        return var2_list(this->group_values, var);
    }

    void SummaryState::append(const SummaryState& buffer)
    {
        this->sim_start = buffer.sim_start;
        this->elapsed = buffer.elapsed;
        this->values = buffer.values;
        this->well_names.reset();
        this->group_names.reset();

        this->m_wells.insert(buffer.m_wells.begin(), buffer.m_wells.end());
        for (const auto& [var, vals] : buffer.well_values) {
            this->well_values.insert_or_assign(var, vals);
        }

        this->m_groups.insert(buffer.m_groups.begin(), buffer.m_groups.end());
        for (const auto& [var, vals] : buffer.group_values) {
            this->group_values.insert_or_assign(var, vals);
        }

        for (const auto& [var, vals] : buffer.conn_values) {
            this->conn_values.insert_or_assign(var, vals);
        }

        for (const auto& [var, vals] : buffer.segment_values) {
            this->segment_values.insert_or_assign(var, vals);
        }
    }

    SummaryState::const_iterator SummaryState::begin() const
    {
        return this->values.begin();
    }

    SummaryState::const_iterator SummaryState::end() const
    {
        return this->values.end();
    }

    std::size_t SummaryState::num_wells() const
    {
        return this->m_wells.size();
    }

    std::size_t SummaryState::size() const
    {
        return this->values.size();
    }

    bool SummaryState::operator==(const SummaryState& other) const
    {
        return (this->sim_start == other.sim_start)
            && (this->udq_undefined == other.udq_undefined)
            && (this->elapsed == other.elapsed)
            && (this->values == other.values)
            && (this->well_values == other.well_values)
            && (this->m_wells == other.m_wells)
            && (this->wells() == other.wells())
            && (this->group_values == other.group_values)
            && (this->m_groups == other.m_groups)
            && (this->groups() == other.groups())
            && (this->conn_values == other.conn_values)
            && (this->segment_values == other.segment_values)
            && (this->region_values == other.region_values)
            ;
    }

    SummaryState SummaryState::serializationTestObject()
    {
        auto st = SummaryState{TimeService::from_time_t(101), 1.234};

        st.elapsed = 1.0;
        st.values = {{"test1", 2.0}};
        st.well_values = {{"test2", {{"test3", 3.0}}}};
        st.m_wells = {"test4"};
        st.well_names = {"test5"};
        st.group_values = {{"test6", {{"test7", 4.0}}}},
        st.m_groups = {"test7"};
        st.group_names = {"test8"},
        st.conn_values = {{"test9", {{"test10", {{5, 6.0}}}}}};

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

        {
            auto& rval = st.region_values["ROPT"]["NUM"];
            rval.emplace(12, 34.56);
            rval.emplace(3,  14.15926);
        }

        {
            auto& rval = st.region_values["RGPR"];
            rval.try_emplace("RE2", std::unordered_map<std::size_t, double> {
                    { std::size_t{17},  29.0   },
                    { std::size_t{42}, - 1.618 },
                });
        }

        return st;
    }

    std::ostream& operator<<(std::ostream& stream, const SummaryState& st)
    {
        stream << "Simulated seconds: " << st.get_elapsed() << std::endl;
        for (const auto& value_pair : st)
            stream << std::setw(17) << value_pair.first << ": " << value_pair.second << std::endl;

        return stream;
    }

} // namespace Opm
