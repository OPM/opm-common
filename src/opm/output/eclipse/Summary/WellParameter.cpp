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

#include <opm/output/eclipse/Summary/WellParameter.hpp>

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
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    std::size_t simStep(const std::size_t reportStep)
    {
        const auto one = std::size_t{ 1 };

        return std::max(reportStep, one) - one;
    }

    struct EfficiencyFactor
    {
        std::vector<std::pair<std::string, double>> fact;

        void calculateCumulative(const std::string&   wname,
                                 const Opm::Schedule& sched,
                                 const std::size_t    sim_step);
    };

    void EfficiencyFactor::calculateCumulative(const std::string&   wname,
                                               const Opm::Schedule& sched,
                                               const std::size_t    sim_step)
    {
        if (! sched.hasWell(wname, sim_step)) { return; }

        const auto& well = sched.getWell2(wname, sim_step);
        this->fact.push_back({wname, well.getEfficiencyFactor()});

        auto& f = this->fact.back().second;
        for (auto prnt = well.groupName(); prnt != "";) {
            const auto& grp = sched.getGroup2(prnt, sim_step);

            f *= grp.getGroupEfficiencyFactor();

            prnt = grp.parent();
        }
    }
}

Opm::WellParameter::WellParameter(WellName                  wellname,
                                  Keyword                   keyword,
                                  UnitString                unit,
                                  SummaryHelpers::Evaluator eval)
    : wellname_ (std::move(wellname.value))
    , keyword_  (std::move(keyword.value))
    , unit_     (std::move(unit.value))
    , evalParam_(std::move(eval))
    , sumKey_   (keyword_ + ':' + wellname_)
{}

Opm::WellParameter&
Opm::WellParameter::flowType(const FlowType type)
{
    if (this->isPressure()) {
        throw std::invalid_argument {
            "Cannot assign flow type to pressure-related parameter"
        };
    }

    switch (type) {
    case FlowType::Rate:
        this->setFlag(Flag::Rate, {Flag::Total, Flag::Ratio});
        break;

    case FlowType::Total:
        this->setFlag(Flag::Total, {Flag::Rate, Flag::Ratio});
        break;

    case FlowType::Ratio:
        this->setFlag(Flag::Ratio, {Flag::Rate, Flag::Total});
        break;
    }

    return *this;
}

Opm::WellParameter&
Opm::WellParameter::pressure(const Pressure type)
{
    if (this->isFlow()) {
        throw std::invalid_argument {
            "Cannot assign pressure type to flow-related parameter"
        };
    }

    switch (type) {
    case Pressure::BHP:
        this->setFlag(Flag::BHP, {Flag::THP});
        break;

    case Pressure::THP:
        this->setFlag(Flag::THP, {Flag::BHP});
        break;
    }

    return *this;
}

const Opm::WellParameter& Opm::WellParameter::validate() const &
{
    this->validateCore();
    return *this;
}

Opm::WellParameter Opm::WellParameter::validate() &&
{
    this->validateCore();
    return *this;
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

void
Opm::WellParameter::update(const std::size_t       reportStep,
                           const double            stepSize,
                           const InputData&        input,
                           const SimulatorResults& simRes,
                           SummaryState&           st) const
{
    const auto sim_step = simStep(reportStep);

    if (! input.sched.hasWell(this->wellname_, sim_step)) {
        return;
    }

    const auto wells = std::vector<std::string>{ this->wellname_ };
    const auto num   = 0;

    // Unit (i.e., 1.0) efficiency factor for well rates and pressures.
    auto efac = EfficiencyFactor{};
    if (this->isTotal()) {
        // Discount total production/injection by explicit shut-in
        // of well and all its parent groups (including FIELD).
        efac.calculateCumulative(this->wellname_, input.sched, sim_step);
    }

    const SummaryHelpers::EvaluationArguments args {
        wells, stepSize, sim_step, num,
        simRes.wellSol,
        input.reg, input.sched, input.grid,
        st,
        std::move(efac.fact)
    };

    const auto& usys = input.es.getUnits();
    const auto  prm  = this->evalParam_(args);

    st.update_well_var(this->wellname_, this->keyword_,
                       usys.from_si(prm.unit, prm.value));
}

void
Opm::WellParameter::setFlag(const Flag                  f,
                            std::initializer_list<Flag> conflicts)
{
    for (const auto& conflict : conflicts) {
        if (this->isSet(conflict)) {
            throw std::invalid_argument {
                "Flag '" + this->flagName(f) + "' conflicts with "
                "existing flag '" + this->flagName(conflict) + '\''
            };
        }
    }

    this->typeFlags_[static_cast<std::size_t>(f)] = true;
}

std::string Opm::WellParameter::flagName(const Flag f) const
{
    switch (f) {
    case Flag::Rate:  return "Rate";
    case Flag::Total: return "Total";
    case Flag::BHP:   return "BHP";
    case Flag::THP:   return "THP";
    }

    throw std::invalid_argument {
        "Unkown Flag Value: " +
        std::to_string(static_cast<std::size_t>(f))
    };
}

void Opm::WellParameter::validateCore() const
{
    if (! this->isValidParamType()) {
        throw std::invalid_argument {
            "Well parameter must be pressure or flow type"
        };
    }
}
