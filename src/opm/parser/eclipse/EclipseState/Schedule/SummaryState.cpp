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
    void SummaryState::add(const ecl::smspec_node& node, double value) {
        if (node.get_var_type() == ECL_SMSPEC_WELL_VAR)
            this->add_well_var(node.get_wgname(),
                               node.get_keyword(),
                               value);
        else
            this->add(node.get_gen_key1(), value);
    }

    void SummaryState::add(const std::string& key, double value) {
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

    void SummaryState::add_well_var(const std::string& well, const std::string& var, double value) {
        this->add(var + ":" + well, value);
        this->well_values[var][well] = value;
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

}
