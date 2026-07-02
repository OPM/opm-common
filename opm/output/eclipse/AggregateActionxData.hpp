/*
  Copyright (c) 2018 Statoil ASA

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

#ifndef OPM_AGGREGATE_Actionx_DATA_HPP
#define OPM_AGGREGATE_Actionx_DATA_HPP

#include <opm/output/eclipse/WindowedArray.hpp>

#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <cstddef>
#include <ctime>
#include <span>
#include <string>
#include <vector>

namespace Opm {
    class Actdims;
    class Schedule;
    class SummaryState;
    class UnitSystem;
    class UDQInput;
    class WListManager;
} // namespace Opm

namespace Opm::Action {
    class ActionX;
    class State;
} // namespace Opm::Action

namespace Opm::RestartIO::Helpers {

struct AggregateActionxRuntimeContext
{
    std::time_t startTime;
    std::time_t simTime;
    const UnitSystem& units;
    std::span<const std::string> wellNames;
    const WListManager& wlistManager;
};

class AggregateActionxData
{
public:
    AggregateActionxData(std::span<const Action::ActionX>      actions,
                         const Actdims&                        actdims,
                         const Action::State&                  action_state,
                         const SummaryState&                   st,
                         const AggregateActionxRuntimeContext& runtime);

    // Compatibility constructor. Prefer createAggregateActionxData() or the
    // constructor accepting explicit actions and runtime context.
    [[deprecated("Use createAggregateActionxData() or the span-based constructor")]]
    AggregateActionxData(const Schedule&      sched,
                         const Action::State& action_state,
                         const SummaryState&  st,
                         const std::size_t    simStep);

    const std::vector<int>& getIACT() const
    {
        return this->iACT_.data();
    }

    const std::vector<float>& getSACT() const
    {
        return this->sACT_.data();
    }

    const std::vector<EclIO::PaddedOutputString<8>>& getZACT() const
    {
        return this->zACT_.data();
    }

    const std::vector<EclIO::PaddedOutputString<8>>& getZLACT() const
    {
        return this->zLACT_.data();
    }

    const std::vector<EclIO::PaddedOutputString<8>>& getZACN() const
    {
        return this->zACN_.data();
    }

    const std::vector<int>& getIACN() const
    {
        return this->iACN_.data();
    }

    // Note: Type 'double' despite the S* name.
    const std::vector<double>& getSACN() const
    {
        return this->sACN_.data();
    }

private:
    /// Aggregate 'IACT' array (Integer) for all ACTIONX data  (9 integers pr UDQ)
    WindowedArray<int> iACT_;

    /// Aggregate 'SACT' array (Integer) for all ACTIONX data  (5 integers pr ACTIONX - currently all zero - meaning unknown)
    WindowedArray<float> sACT_;

    /// Aggregate 'ZACT' array (Character) for all ACTIONX data. (4 * 8 chars pr ACIONX keyword - name of Action)
    WindowedArray<EclIO::PaddedOutputString<8>> zACT_;

    /// Aggregate 'ZLACT' array (Character) for all Actionx data.  (max 16 * 8 characters pr line (default 80 chars pr line)
    WindowedArray<EclIO::PaddedOutputString<8>> zLACT_;

    /// Aggregate 'ZACN' array (Character) for all Actionx data  (length equal to max no of conditions pr Actionx * the number of Actiox kwords)
    WindowedArray<EclIO::PaddedOutputString<8>> zACN_;

    /// Aggregate 'IACN' array (Integer) for all Actionx data  (length 26* the max number of conditoins pr Actionx * the number of Actionx kwords)
    WindowedArray<int> iACN_;

    /// Aggregate 'SACN' array (double precision floating-point) for all Actionx data  (16 * max number of Actionx conditions)
    WindowedMatrix<double> sACN_;

};

AggregateActionxData
createAggregateActionxData(std::span<const Action::ActionX>      actions,
                           const Actdims&                        actdims,
                           const Action::State&                  action_state,
                           const SummaryState&                   st,
                           const AggregateActionxRuntimeContext& runtime);

AggregateActionxData
createAggregateActionxData(const Schedule&      sched,
                           const Action::State& action_state,
                           const SummaryState&  st,
                           std::size_t          simStep);

} // namespace Opm::RestartIO::Helpers

#endif //OPM_AGGREGATE_WELL_DATA_HPP
