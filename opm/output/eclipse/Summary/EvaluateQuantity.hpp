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

#ifndef OPM_SUMMARY_EVALUATEPARAMETER_HPP
#define OPM_SUMMARY_EVALUATEPARAMETER_HPP

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Opm {
    class EclipseGrid;
    class Schedule;
    class SummaryState;
} // namespace Opm

namespace Opm { namespace data {
    class WellRates;
}} // namespace Opm::data

namespace Opm { namespace out {
    class RegionCache;
}} // namespace Opm::out

namespace Opm { namespace SummaryHelpers {

    /// Result type from evaluating a summary parameter
    struct SummaryQuantity
    {
        /// Numerical value of summary parameter
        double value;

        /// Unit of measure of summary parameter
        UnitSystem::measure unit;
    };

    /// Collection of object useful when evaluating summary parameters
    struct EvaluationArguments
    {
        /// Wells influencing this summary parameter.
        const std::vector<std::string>& schedule_wells;

        /// Time (seconds) elapsed since previous evaluation.  Typically
        /// the duration of a "mini step".
        double duration;

        /// Zero-based index of report step with which to associate
        /// evaluation of summary parameter.
        std::size_t sim_step;

        /// Entity ID of block- or region related summary parameter.
        int num;

        /// Dynamic well solution (rates, pressures &c)
        const data::WellRates& well_sol;

        /// Management structure for associating individual well
        /// connections to (FIP) region IDs.
        const out::RegionCache& region_cache;

        /// Dynamic control/timestepping object
        const Schedule& sched;

        /// Simulation model's grid structure (active vs. inactive cells)
        const EclipseGrid& grid;

        /// Current summary state object from which to retrieve previous
        /// state values (e.g., UDQ if applicable).
        const SummaryState& st;

        /// Well efficiency factors (derived from WEFAC and GEFAC).
        std::vector<std::pair<std::string, double>> eff_factors;
    };

    /// Callback type representing a summary parameter evaluation
    /// on a set of arguments.
    using Evaluator = std::function<
        SummaryQuantity(const EvaluationArguments& args)
    >;

    /// Retrieve evaluator for particular parameter keyword.
    ///
    /// Nullptr if no evaluator exists for this keyword.
    std::unique_ptr<Evaluator>
    getParameterEvaluator(const std::string& parameterKeyword);

    /// Get list of supported summary parameter keywords.
    ///
    /// Keyword strings returned in unspecified order.
    std::vector<std::string> supportedKeywords();
}} // namespace Opm::SummaryHelpers

#endif // OPM_SUMMARY_EVALUATEPARAMETER_HPP
