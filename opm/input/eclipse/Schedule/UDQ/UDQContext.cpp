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

#include <opm/input/eclipse/Schedule/UDQ/UDQContext.hpp>

#include <opm/input/eclipse/EclipseState/Grid/RegionSetMatcher.hpp>

#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDT.hpp>
#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {

bool is_udq(const std::string& key)
{
    return (key.size() >= std::string::size_type{2})
        && (key[1] == 'U');
}

} // Anonymous namespace

namespace Opm {

    UDQContext::UDQContext(const UDQFunctionTable& udqft_arg,
                           const WellMatcher&      wm,
                           const GroupOrder&       go,
                           const std::unordered_map<std::string, UDT>& tables,
                           MatcherFactories        create_matchers,
                           SummaryState&           summary_state_arg,
                           UDQState&               udq_state_arg)
        : udqft           (udqft_arg)
        , well_matcher    (wm)
        , group_order_    (go)
        , udt             (tables)
        , summary_state   (summary_state_arg)
        , udq_state       (udq_state_arg)
        , create_matchers_(std::move(create_matchers))
    {
        for (const auto& [month, index] : TimeService::eclipseMonthIndices()) {
            this->add(month, index);
        }

        // Simulator performance keywords which are expected to be available
        // for UDQ keywords; probably better to guarantee that they are
        // present in the underlying summary state object.

        this->add("MSUMLINS", 0.0);
        this->add("MSUMNEWT", 0.0);
        this->add("NEWTON", 0.0);
        this->add("TCPU", 0.0);
    }

    void UDQContext::add(const std::string& key, double value)
    {
        this->values.insert_or_assign(key, value);
    }

    std::optional<double>
    UDQContext::get(const std::string& key) const
    {
        if (is_udq(key)) {
            if (this->udq_state.has(key)) {
                return this->udq_state.get(key);
            }

            return std::nullopt;
        }

        auto pair_ptr = this->values.find(key);
        if (pair_ptr != this->values.end()) {
            return pair_ptr->second;
        }

        return this->summary_state.get(key);
    }

    std::optional<double>
    UDQContext::get_well_var(const std::string& well,
                             const std::string& var) const
    {
        if (is_udq(var)) {
            if (this->udq_state.has_well_var(well, var)) {
                return this->udq_state.get_well_var(well, var);
            }

            return std::nullopt;
        }

        if (this->summary_state.has_well_var(var)) {
            if (this->summary_state.has_well_var(well, var)) {
                return this->summary_state.get_well_var(well, var);
            }

            return std::nullopt;
        }

        throw std::logic_error {
            fmt::format("Summary well variable: {} not registered", var)
        };
    }

    std::optional<double>
    UDQContext::get_group_var(const std::string& group,
                              const std::string& var) const
    {
        if (is_udq(var)) {
            if (this->udq_state.has_group_var(group, var)) {
                return this->udq_state.get_group_var(group, var);
            }

            return std::nullopt;
        }

        if (this->summary_state.has_group_var(var)) {
            if (this->summary_state.has_group_var(group, var)) {
                return this->summary_state.get_group_var(group, var);
            }

            return std::nullopt;
        }

        throw std::logic_error {
            fmt::format("Summary group variable: {} not registered", var)
        };
    }

    std::optional<double>
    UDQContext::get_segment_var(const std::string& well,
                                const std::string& var,
                                std::size_t        segment) const
    {
        if (is_udq(var)) {
            if (this->udq_state.has_segment_var(well, var, segment)) {
                return this->udq_state.get_segment_var(well, var, segment);
            }

            return std::nullopt;
        }

        if (this->summary_state.has_segment_var(well, var, segment)) {
            return this->summary_state.get_segment_var(well, var, segment);
        }

        throw std::logic_error {
            fmt::format("Segment summary variable {} not "
                        "registered for segment {} in well {}",
                        var, segment, well)
        };
    }

    std::optional<double>
    UDQContext::get_region_var(const std::string& regSet,
                               const std::string& var,
                               const std::size_t region) const
    {
        if (this->summary_state.has_region_var(regSet, var, region)) {
            return this->summary_state.get_region_var(regSet, var, region);
        }

        throw std::logic_error {
            fmt::format("Region summary variable {} not "
                        "registered/supported for region "
                        "{} in region set {}",
                        var, region, regSet)
        };
    }

    const UDT&
    UDQContext::get_udt(const std::string& name) const
    {
        const auto it = udt.find(name);
        if (it == udt.end()) {
            throw std::logic_error {
              fmt::format("Not such UDT defined: {}", name)
            };
        }
        return it->second;
    }

    const std::vector<std::string>& UDQContext::wells() const
    {
        return this->well_matcher.wells();
    }

    std::vector<std::string> UDQContext::wells(const std::string& pattern) const
    {
        return this->well_matcher.wells(pattern);
    }

    std::vector<std::string> UDQContext::nonFieldGroups() const
    {
        auto rgroups = std::vector<std::string>{};

        const auto& allGroups = this->group_order_.names();

        rgroups.reserve(allGroups.size());
        std::copy_if(allGroups.begin(), allGroups.end(),
                     std::back_inserter(rgroups),
                     [](const std::string& gname)
                     { return gname != "FIELD"; });

        return rgroups;
    }

    std::vector<std::string> UDQContext::groups(const std::string& pattern) const
    {
        return this->group_order_.names(pattern);
    }

    SegmentSet UDQContext::segments() const
    {
        // Empty descriptor matches all segments in all existing MS wells.

        this->ensure_segment_matcher_exists();
        return this->matchers_.segments->findSegments(SegmentMatcher::SetDescriptor{});
    }

    SegmentSet UDQContext::segments(const std::vector<std::string>& set_descriptor) const
    {
        assert (! set_descriptor.empty() &&
                "Internal error passing empty segment set "
                "descriptor to filtered segment set query");

        auto desc = SegmentMatcher::SetDescriptor{}
            .wellNames(set_descriptor.front());

        if (set_descriptor.size() > std::vector<std::string>::size_type{1}) {
            desc.segmentNumber(set_descriptor[1]);
        }

        this->ensure_segment_matcher_exists();
        return this->matchers_.segments->findSegments(desc);
    }

    RegionSetMatchResult UDQContext::regions() const
    {
        // Empty descriptor matches all regions in all region sets.

        this->ensure_region_matcher_exists();
        return this->matchers_.regions->findRegions(RegionSetMatcher::SetDescriptor{});
    }

    RegionSetMatchResult
    UDQContext::regions(const std::string&              vector_name,
                        const std::vector<std::string>& set_descriptor) const
    {
        auto desc = RegionSetMatcher::SetDescriptor{}
            .vectorName(vector_name);

        if (! set_descriptor.empty()) {
            desc.regionID(set_descriptor.front());
        }

        this->ensure_region_matcher_exists();
        return this->matchers_.regions->findRegions(desc);
    }

    const UDQFunctionTable& UDQContext::function_table() const
    {
        return this->udqft;
    }

    void UDQContext::update_assign(const std::string& keyword,
                                   const UDQSet&      udq_result)
    {
        this->udq_state.add_assign(keyword, udq_result);
        this->summary_state.update_udq(udq_result);
    }

    void UDQContext::update_define(const std::size_t  report_step,
                                   const std::string& keyword,
                                   const UDQSet&      udq_result)
    {
        this->udq_state.add_define(report_step, keyword, udq_result);
        this->summary_state.update_udq(udq_result);
    }

    void UDQContext::ensure_segment_matcher_exists() const
    {
        if (this->matchers_.segments == nullptr) {
            this->matchers_.segments = this->create_matchers_.segments();
        }
    }

    void UDQContext::ensure_region_matcher_exists() const
    {
        if (this->matchers_.regions == nullptr) {
            this->matchers_.regions = this->create_matchers_.regions();
        }
    }
}
