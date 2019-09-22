/*
  Copyright (c) 2019 Equinor ASA

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

#include <opm/output/eclipse/Summary/GroupParameter.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/RegionCache.hpp>
#include <opm/output/eclipse/Summary/EvaluateQuantity.hpp>
#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    using EfficiencyFactor = std::pair<std::string, double>;
    using EFacCollection = std::vector<EfficiencyFactor>;
    using WellOp = std::function<void(const Opm::Well2&)>;
    using GroupOp = std::function<void(const Opm::Group2&)>;

    std::size_t simStep(const std::size_t reportStep)
    {
        const auto one = std::size_t{ 1 };

        return std::max(reportStep, one) - one;
    }

    double parentEfficiencyFactor(const Opm::Group2&   group,
                                  const std::size_t    timeStep,
                                  const Opm::Schedule& sched)
    {
        auto efac = 1.0;

        for (auto parentname = group.parent(); parentname != "";) {
            const auto& parent = sched.getGroup2(parentname, timeStep);

            efac *= parent.getGroupEfficiencyFactor();

            parentname = parent.parent();
        }

        return efac;
    }

    std::vector<std::string>
    wellsFromEFac(const EFacCollection& efac)
    {
        auto wname = std::vector<std::string>{};
        wname.reserve(efac.size());

        std::transform(efac.begin(), efac.end(), std::back_inserter(wname),
            [](const EfficiencyFactor& factor)
        {
            return factor.first;
        });

        return wname;
    }

    void groupTreeTraversal(std::string          root,
                            const std::size_t    timeStep,
                            const Opm::Schedule& sched,
                            GroupOp              groupOp,
                            WellOp               wellOp)
    {
        auto groups = std::queue<std::string>{};

        for (groups.emplace(std::move(root)); !groups.empty(); groups.pop()) {
            const auto& group = sched.getGroup2(groups.front(), timeStep);

            groupOp(group);

            if (group.wellgroup()) {
                for (const auto& well : group.wells()) {
                    wellOp(sched.getWell2(well, timeStep));
                }
            }
            else {
                // Node group.  Insert child groups at end of group queue
                // in order to visit these groups in later iterations.
                for (const auto& gname : group.groups()) {
                    groups.emplace(gname);
                }
            }
        }
    }
} // Anonymous namespace

Opm::GroupParameter::GroupParameter(GroupName                 groupname,
                                    Keyword                   keyword,
                                    UnitString                unit,
                                    const Type                type,
                                    SummaryHelpers::Evaluator eval)
    : groupname_(std::move(groupname.value))
    , keyword_  (std::move(keyword.value))
    , unit_     (std::move(unit.value))
    , type_     (type)
    , evalParam_(std::move(eval))
    , sumKey_   { (groupname_ == "FIELD")
        ?  keyword_
        : (keyword_ + ':' + groupname_)
      }
{}

const Opm::GroupParameter& Opm::GroupParameter::validate() const &
{
    this->validateCore();
    return *this;
}

Opm::GroupParameter Opm::GroupParameter::validate() &&
{
    this->validateCore();
    return *this;
}

void
Opm::GroupParameter::update(const std::size_t       reportStep,
                            const double            stepSize,
                            const InputData&        input,
                            const SimulatorResults& simRes,
                            SummaryState&           st) const
{
    if (! input.sched.hasGroup(this->groupname_, simStep(reportStep))) {
        return;
    }

    st.update_group_var(this->groupname_, this->keyword_,
                        this->parameterValue(reportStep, stepSize,
                                             input, simRes, st));
}

#if 0
    struct EvaluationArguments
    {
        const vector<string>& schedule_wells;

        double duration;
        size_t sim_step;
        int num;

        const data::WellRates& well_sol;
        const out::RegionCache& region_cache;
        const Schedule& sched;
        const EclipseGrid& grid;
        const SummaryState& st;

        vector<pair<string, double>> eff_factors;
    };
#endif

double
Opm::GroupParameter::parameterValue(const std::size_t       reportStep,
                                    const double            stepSize,
                                    const InputData&        input,
                                    const SimulatorResults& simRes,
                                    const SummaryState&     st) const
{
    const auto sim_step = simStep(reportStep);
    const auto num      = 0;

    auto efac = this->efficiencyFactors(sim_step, input.sched);

    const auto wnames = efac.empty()
        ? this->wells(sim_step, input.sched)
        : wellsFromEFac(efac);

    const SummaryHelpers::EvaluationArguments args {
        wnames, stepSize, sim_step, num,
        simRes.wellSol,
        input.reg, input.sched, input.grid,
        st,
        std::move(efac)
    };

    const auto prm  = this->evalParam_(args);

    return input.es.getUnits().from_si(prm.unit, prm.value);
}

void Opm::GroupParameter::validateCore() const
{
    if (! (this->isCount() || this->isFlow())) {
        throw std::invalid_argument {
            "Group parameter must be count or flow type"
        };
    }
}

std::vector<std::pair<std::string, double>>
Opm::GroupParameter::efficiencyFactors(const std::size_t sim_step,
                                       const Schedule&   sched) const
{
    auto wefac = EFacCollection{};

    if (this->isFlow() && !this->isRatio()) {
        // Shared mutable state for communicating bewtween the group
        // and well operations (visitGroup (r/w) and vistWell (r/o)).
        auto gefac = std::unordered_map<std::string, double>{};

        const auto& grp = sched.getGroup2(this->groupname_, sim_step);

        gefac[grp.parent()] = this->isTotal()
            ? parentEfficiencyFactor(grp, sim_step, sched)
            : 1.0;

        if (this->isRate()) {
            // Don't include this group's efficiency factor in the case of
            // rate-type parameters.  These parameter types only incorporate
            // efficiency factors from subordinate group tree levels.
            gefac[grp.name()] = 1.0;
        }

        auto visitGroup = [&gefac](const Group2& group) -> void
        {
            const auto efac = gefac.at(group.parent())
                * group.getGroupEfficiencyFactor();

            // .emplace()--instead of operator[]--to prevent overwriting this
            // group's entry (= 1.0) in the case of a rate-type parameter.
            gefac.emplace(group.name(), efac);
        };

        auto visitWell = [&gefac, &wefac](const Well2& well) -> void
        {
            const auto efac = gefac.at(well.groupName())
                * well.getEfficiencyFactor();

            wefac.emplace_back(well.name(), efac);
        };

        groupTreeTraversal(grp.name(), sim_step, sched,
                           visitGroup, visitWell);
    }

    return wefac;
}

std::vector<std::string>
Opm::GroupParameter::wells(const std::size_t sim_step,
                           const Schedule&   sched) const
{
    auto wlist = std::vector<std::string>{};

    auto visitGroup = [](const Group2&) { /* No-op */ };
    auto visitWell  = [&wlist](const Well2& well)
    {
        wlist.push_back(well.name());
    };

    groupTreeTraversal(this->groupname_, sim_step, sched,
                       visitGroup, visitWell);

    return wlist;
}

// =====================================================================

Opm::FieldParameter::FieldParameter(Keyword                   keyword,
                                    UnitString                unit,
                                    const Type                type,
                                    SummaryHelpers::Evaluator eval)
    : GroupParameter {
        ::Opm::GroupParameter::GroupName{ "FIELD" },
        std::move(keyword), std::move(unit), type, std::move(eval)
    }
{}

void
Opm::FieldParameter::update(const std::size_t       reportStep,
                            const double            stepSize,
                            const InputData&        input,
                            const SimulatorResults& simRes,
                            SummaryState&           st) const
{
    // FIELD always exists.  No checking needed here.
    st.update(this->keywordNoCopy(),
              this->parameterValue(reportStep, stepSize,
                                   input, simRes, st));
}

const Opm::FieldParameter& Opm::FieldParameter::validate() const &
{
    this->validateCore();
    return *this;
}

Opm::FieldParameter Opm::FieldParameter::validate() &&
{
    this->validateCore();
    return *this;
}

std::vector<std::string>
Opm::FieldParameter::wells(const std::size_t sim_step,
                           const Schedule&   sched) const
{
    return sched.wellNames(sim_step);
}
