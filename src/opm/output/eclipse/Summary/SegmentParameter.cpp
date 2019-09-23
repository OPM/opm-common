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

#include <opm/output/eclipse/Summary/SegmentParameter.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/RegionCache.hpp>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
    std::string makeSegmentKey(const std::string& kw,
                               const std::string& well,
                               const int          segID)
    {
        std::ostringstream key;

        // SOFR:PROD01:5
        key << kw << ':' << well << ':' << segID;

        return key.str();
    }

    std::size_t simStep(const std::size_t reportStep)
    {
        const auto one = std::size_t{ 1 };

        return std::max(reportStep, one) - one;
    }
} // Anonymous namespace

Opm::SegmentParameter::SegmentParameter(WellName                  well,
                                        const int                 segmentID,
                                        Keyword                   keyword,
                                        UnitString                unit,
                                        const Type                type,
                                        SummaryHelpers::Evaluator eval)
    : wellname_ { std::move(well.value) }
    , keyword_  { std::move(keyword.value) }
    , unit_     { std::move(unit.value) }
    , segmentID_(segmentID)
    , type_     (type)
    , evalParam_(std::move(eval))
    , sumKey_   (makeSegmentKey(keyword_, wellname_, segmentID_))
{}

const Opm::SegmentParameter& Opm::SegmentParameter::validate() const &
{
    this->validateCore();
    return *this;
}

Opm::SegmentParameter Opm::SegmentParameter::validate() &&
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
Opm::SegmentParameter::update(const std::size_t       reportStep,
                              const double            stepSize,
                              const InputData&        input,
                              const SimulatorResults& simRes,
                              Opm::SummaryState&      st) const
{
    const auto sim_step = simStep(reportStep);

    if (! (input.sched.hasWell(this->wellname_, sim_step) &&
           input.sched.getWell2(this->wellname_, sim_step).isMultiSegment()))
    {
        // Well does not exist at this time step (or is not an MSW)
        return;
    }

    const auto wells = std::vector<std::string>{ this->wellname_ };
    auto       efac  = std::vector<std::pair<std::string, double>>{};

    const SummaryHelpers::EvaluationArguments args {
        wells, stepSize, sim_step, this->segmentID_,
        simRes.wellSol,
        input.reg, input.sched, input.grid,
        st,
        std::move(efac)
    };

    const auto& usys = input.es.getUnits();
    const auto  prm  = this->evalParam_(args);

    st.update(this->sumKey_, usys.from_si(prm.unit, prm.value));
}

void Opm::SegmentParameter::validateCore() const
{
    if (! this->isValidParamType()) {
        throw std::invalid_argument {
            "Segment parameter must be pressure or rate type"
        };
    }
}
