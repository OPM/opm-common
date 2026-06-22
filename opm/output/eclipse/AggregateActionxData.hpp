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
    /// Integer descriptors for ACTIONX.
    ///
    /// Nine (9) integers per ACTIONX keyword.
    WindowedArray<int> iACT_;

    /// Floating-point descriptors for ACTIONX.
    ///
    /// Five (5) floats per ACTIONX keyword.
    WindowedArray<float> sACT_;

    /// Character descriptors for ACTIONX.
    ///
    /// One (1) string of 8 characters, the action name, per ACTIONX keyword.
    WindowedArray<EclIO::PaddedOutputString<8>> zACT_;

    /// Linearised action block keywords for all ACTIONX keywords.
    ///
    /// At most 16 eight-character strings per ACTIONX keyword line, with
    /// a (common) runtime-configured number of lines per ACTIONX keyword.
    WindowedArray<EclIO::PaddedOutputString<8>> zLACT_;

    /// Integer descriptors for all ACTIONX conditions.
    ///
    /// 26 integers per ACTIONX condition, with a (common) runtime-configured
    /// number of conditions per ACTIONX keyword.  Not all integer descriptors
    /// are used for all conditions, but the restart file format is fixed.
    WindowedMatrix<int> iACN_;

    /// Floating-point descriptors for all ACTIONX conditions.
    ///
    /// 16 items per ACTIONX condition, with a (common) runtime-configured
    /// number of conditions per ACTIONX keyword.  Not all floating-point
    /// descriptors are used for all conditions, but the restart file format
    /// is fixed.
    ///
    /// Note: Type 'double' despite the S* name.
    WindowedMatrix<double> sACN_;

    /// String descriptors for all ACTIONX conditions.
    ///
    /// 13 eight-character strings per ACTIONX condition, with a (common)
    /// runtime-configured number of conditions per ACTIONX keyword.  Not all
    /// string descriptors are used for all conditions, but the restart file
    /// format is fixed.
    WindowedMatrix<EclIO::PaddedOutputString<8>> zACN_;
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

#endif // OPM_AGGREGATE_Actionx_DATA_HPP
