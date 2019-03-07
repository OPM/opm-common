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


#ifndef UDQ_CONTEXT_HPP
#define UDQ_CONTEXT_HPP

#include <vector>
#include <string>
#include <unordered_map>


namespace Opm {
    class SummaryState;
    class UDQFunctionTable;

    class UDQContext{
    public:
        UDQContext(const UDQFunctionTable& udqft, const SummaryState& summary_state);
        double get(const std::string& key) const;
        double get_well_var(const std::string& well, const std::string& var) const;
        void add(const std::string& key, double value);
        const UDQFunctionTable& function_table() const;
        std::vector<std::string> wells() const;
    private:
        const UDQFunctionTable& udqft;
        const SummaryState& summary_state;
        std::unordered_map<std::string, double> values;
    };
}



#endif
