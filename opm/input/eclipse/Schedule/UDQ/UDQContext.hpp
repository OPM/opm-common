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

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

    class SegmentMatcher;
    class SegmentSet;
    class SummaryState;
    class UDQFunctionTable;
    class UDQSet;
    class UDQState;
    class UDT;
    class WellMatcher;

} // namespace Opm

namespace Opm {

    class UDQContext
    {
    public:
        using SegmentMatcherFactory = std::function<std::unique_ptr<SegmentMatcher>()>;

        UDQContext(const UDQFunctionTable& udqft,
                   const WellMatcher&      wm,
                   const std::unordered_map<std::string, UDT>& tables,
                   SegmentMatcherFactory   create_segment_matcher,
                   SummaryState&           summary_state,
                   UDQState&               udq_state);

        std::optional<double> get(const std::string& key) const;
        std::optional<double> get_well_var(const std::string& well, const std::string& var) const;
        std::optional<double> get_group_var(const std::string& group, const std::string& var) const;
        std::optional<double> get_segment_var(const std::string& well, const std::string& var, std::size_t segment) const;
        const UDT& get_udt(const std::string& name) const;

        void add(const std::string& key, double value);
        void update_assign(const std::string& keyword, const UDQSet& udq_result);
        void update_define(std::size_t report_step, const std::string& keyword, const UDQSet& udq_result);

        const UDQFunctionTable& function_table() const;

        std::vector<std::string> wells() const;
        std::vector<std::string> wells(const std::string& pattern) const;
        std::vector<std::string> groups() const;
        SegmentSet segments() const;
        SegmentSet segments(const std::vector<std::string>& set_descriptor) const;

    private:
        const UDQFunctionTable& udqft;
        const WellMatcher& well_matcher;
        const std::unordered_map<std::string, UDT>& udt;

        SegmentMatcherFactory create_segment_matcher;
        mutable std::unique_ptr<SegmentMatcher> segment_matcher;
        SummaryState& summary_state;
        UDQState& udq_state;

        //std::unordered_map<std::string, UDQSet> udq_results;
        std::unordered_map<std::string, double> values;

        void ensure_segment_matcher_exists() const;
    };

} // namespace Opm

#endif // UDQ_CONTEXT_HPP
