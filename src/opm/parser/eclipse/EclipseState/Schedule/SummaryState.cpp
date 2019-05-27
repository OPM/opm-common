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


#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

namespace Opm{
namespace {

    bool is_total(const std::string& key) {
        auto sep_pos = key.find(':');

        if (sep_pos == 0)
            return false;

        if (sep_pos == std::string::npos) {
            if (key.back() == 'T')
                return true;

            return (key.compare(key.size() - 2, 2, "TH") == 0);
        } else
            return is_total(key.substr(0,sep_pos));
    }

}
    void SummaryState::update_elapsed(double delta) {
        this->elapsed += delta;
    }


    double SummaryState::get_elapsed() const {
        return this->elapsed;
    }


    void SummaryState::update(const std::string& key, double value) {
        if (is_total(key))
            this->values[key] += value;
        else
            this->values[key] = value;
    }

    void SummaryState::update(const ecl::smspec_node& node, double value) {
        if (node.get_var_type() == ECL_SMSPEC_WELL_VAR)
            this->update_well_var(node.get_wgname(),
                                  node.get_keyword(),
                                  value);
        else if (node.get_var_type() == ECL_SMSPEC_GROUP_VAR)
            this->update_group_var(node.get_wgname(),
                                   node.get_keyword(),
                                   value);
        else {
            const std::string& key = node.get_gen_key1();
            if (node.is_total())
                this->values[key] += value;
            else
                this->values[key] = value;
        }
    }


    void SummaryState::update_group_var(const std::string& group, const std::string& var, double value) {
        std::string key = var + ":" + group;
        if (is_total(var)) {
            this->values[key] += value;
            this->group_values[var][group] += value;
        } else {
            this->values[key] = value;
            this->group_values[var][group] = value;
        }
        this->m_groups.insert(group);
    }

    void SummaryState::update_well_var(const std::string& well, const std::string& var, double value) {
        std::string key = var + ":" + well;
        if (is_total(var)) {
            this->values[key] += value;
            this->well_values[var][well] += value;
        } else {
            this->values[key] = value;
            this->well_values[var][well] = value;
        }
        this->m_wells.insert(well);
    }


    void SummaryState::set(const std::string& key, double value) {
        this->values[key] = value;
    }


    bool SummaryState::has(const std::string& key) const {
        return (this->values.find(key) != this->values.end());
    }


    double SummaryState::get(const std::string& key) const {
        const auto iter = this->values.find(key);
        if (iter == this->values.end())
            throw std::out_of_range("No such key: " + key);

        return iter->second;
    }

    bool SummaryState::has_well_var(const std::string& well, const std::string& var) const {
        const auto& var_iter = this->well_values.find(var);
        if (var_iter == this->well_values.end())
            return false;

        const auto& well_iter = var_iter->second.find(well);
        if (well_iter == var_iter->second.end())
            return false;

        return true;
    }

    double SummaryState::get_well_var(const std::string& well, const std::string& var) const {
        return this->well_values.at(var).at(well);
    }

    bool SummaryState::has_group_var(const std::string& group, const std::string& var) const {
        const auto& var_iter = this->group_values.find(var);
        if (var_iter == this->group_values.end())
            return false;

        const auto& group_iter = var_iter->second.find(group);
        if (group_iter == var_iter->second.end())
            return false;

        return true;
    }

    double SummaryState::get_group_var(const std::string& group, const std::string& var) const {
        return this->group_values.at(var).at(group);
    }

    SummaryState::const_iterator SummaryState::begin() const {
        return this->values.begin();
    }


    SummaryState::const_iterator SummaryState::end() const {
        return this->values.end();
    }


    std::vector<std::string> SummaryState::wells(const std::string& var) const {
        const auto& var_iter = this->well_values.find(var);
        if (var_iter == this->well_values.end())
            return {};

        std::vector<std::string> wells;
        for (const auto& pair : var_iter->second)
            wells.push_back(pair.first);
        return wells;
    }


    std::vector<std::string> SummaryState::wells() const {
        return std::vector<std::string>(this->m_wells.begin(), this->m_wells.end());
    }


    std::vector<std::string> SummaryState::groups(const std::string& var) const {
        const auto& var_iter = this->group_values.find(var);
        if (var_iter == this->group_values.end())
            return {};

        std::vector<std::string> groups;
        for (const auto& pair : var_iter->second)
            groups.push_back(pair.first);
        return groups;
    }


    std::vector<std::string> SummaryState::groups() const {
        return std::vector<std::string>(this->m_groups.begin(), this->m_groups.end());
    }

    std::size_t SummaryState::num_wells() const {
        return this->m_wells.size();
    }

    std::size_t SummaryState::size() const {
        return this->values.size();
    }
}
