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
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    class RegionWells
    {
    public:
        using RegConn = std::pair<std::string, std::size_t>;
        using ConnCollection = std::vector<RegConn>;

        explicit RegionWells(const Opm::Schedule& sched,
                             const std::size_t    sim_step)
            : sched_   (sched)
            , sim_step_(sim_step)
        {}

        std::vector<std::string>
        uniqueWells(const ConnCollection& regConns)
        {
            this->indices_.clear();
            this->regConns_ = &regConns;

            if (! regConns.empty()) {
                this->extractKnownWells();
                this->extractUniqueWells();
            }

            return this->getUniqueWells();
        }

    private:
        using Index = ConnCollection::size_type;

        const Opm::Schedule& sched_;
        std::size_t          sim_step_;

        const ConnCollection* regConns_{nullptr};
        std::vector<Index> indices_{};

        void extractKnownWells();
        void extractUniqueWells();

        std::vector<std::string> getUniqueWells() const;
    };

    void RegionWells::extractKnownWells()
    {
        auto ix = Index{0};

        for (const auto& regConn : *this->regConns_) {
            if (this->sched_.hasWell(regConn.first, this->sim_step_)) {
                this->indices_.push_back(ix);
            }

            ++ix;
        }
    }

    void RegionWells::extractUniqueWells()
    {
        const auto& rc = *this->regConns_;

        // Possibly stable_sort() here.
        std::sort(this->indices_.begin(), this->indices_.end(),
            [&rc](const Index i1, const Index i2) -> bool
        {
            return rc[i1].first < rc[i2].first;
        });

        auto u = std::unique(this->indices_.begin(), this->indices_.end(),
            [&rc](const Index i1, const Index i2) -> bool
        {
            return rc[i1].first == rc[i2].first;
        });

        if (u != this->indices_.end()) {
            this->indices_.erase(u + 1, this->indices_.end());
        }
    }

    std::vector<std::string>
    RegionWells::getUniqueWells() const
    {
        auto wells = std::vector<std::string>{};

        if (this->indices_.empty()) {
            return wells;
        }

        wells.reserve(this->indices_.size());

        for (const auto& index : this->indices_) {
            wells.push_back((*this->regConns_)[index].first);
        }

        return wells;
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

    std::size_t simStep(const std::size_t reportStep)
    {
        const auto one = std::size_t{ 1 };

        return std::max(reportStep, one) - one;
    }

    std::vector<std::string>
    regionWells(const Opm::out::RegionCache& region_cache,
                const int                    regionID,
                const Opm::Schedule&         sched,
                const std::size_t            sim_step)
    {
        RegionWells rw { sched, sim_step };

        return rw.uniqueWells(region_cache.connections(regionID));
    }

    std::string makeRegionKey(const std::string& keyword, const int regionID)
    {
        std::ostringstream key;

        // ROPT:17
        key << keyword << ':' << regionID;

        return key.str();
    }
}

Opm::WellParameter::WellParameter(WellName                  wellname,
                                  Keyword                   keyword,
                                  UnitString                unit,
                                  SummaryHelpers::Evaluator eval,
                                  const bool                is_udq)
    : wellname_     (std::move(wellname.value))
    , keyword_      (std::move(keyword.value))
    , unit_         (std::move(unit.value))
    , isUserDefined_(is_udq)
    , evalParam_    (std::move(eval))
    , sumKey_       (keyword_ + ':' + wellname_)
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
    if (this->isUserDefined_) {
        // Defer to separate calculation
        return;
    }

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
    if (f == Flag::Rate)  { return "Rate"; }
    if (f == Flag::Ratio) { return "Ratio"; }
    if (f == Flag::Total) { return "Total"; }
    if (f == Flag::BHP)   { return "BHP"; }
    if (f == Flag::THP)   { return "THP"; }

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

// =====================================================================

Opm::WellAggregateRegionParameter::
WellAggregateRegionParameter(const int                 regionID,
                             Keyword                   keyword,
                             const Type                type,
                             UnitString                unit,
                             SummaryHelpers::Evaluator eval)
    : keyword_  { std::move(keyword.value) }
    , unit_     { std::move(unit.value) }
    , regionID_ (regionID)
    , type_     (type)
    , evalParam_{ std::move(eval) }
    , sumKey_   (makeRegionKey(keyword_, regionID_))
{}

const Opm::WellAggregateRegionParameter&
Opm::WellAggregateRegionParameter::validate() const &
{
    this->validateCore();
    return *this;
}

Opm::WellAggregateRegionParameter
Opm::WellAggregateRegionParameter::validate() &&
{
    this->validateCore();
    return *this;
}

void
Opm::WellAggregateRegionParameter::
update(const std::size_t       reportStep,
       const double            stepSize,
       const InputData&        input,
       const SimulatorResults& simRes,
       SummaryState&           st) const
{
    const auto sim_step = simStep(reportStep);
    const auto wells    = regionWells(input.reg, this->regionID_,
                                      input.sched, sim_step);

    if (wells.empty()) {
        return;
    }

    auto efac = EfficiencyFactor{};
    if (this->isTotal()) {
        for (const auto& well : wells) {
            efac.calculateCumulative(well, input.sched, sim_step);
        }
    }

    const SummaryHelpers::EvaluationArguments args {
        wells, stepSize, sim_step, this->regionID_,
        simRes.wellSol,
        input.reg, input.sched, input.grid,
        st,
        std::move(efac.fact)
    };

    const auto& usys = input.es.getUnits();
    const auto  prm  = this->evalParam_(args);

    st.update(this->sumKey_, usys.from_si(prm.unit, prm.value));
}

void Opm::WellAggregateRegionParameter::validateCore() const
{
    if (! (this->isRate() || this->isTotal())) {
        throw std::invalid_argument {
            "Well-dependent region parameter must be "
            "flow rate or cumulative total"
        };
    }
}
