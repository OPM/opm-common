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

#ifndef OPM_SUMMARY_PARAMETER_HPP
#define OPM_SUMMARY_PARAMETER_HPP

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Opm {
    class EclipseGrid;
    class EclipseState;
    class Schedule;
    class SummaryState;
    class UnitSystem;
} // namespace Opm

namespace Opm { namespace data {
    class WellRates;
}}

namespace Opm { namespace out {
    class RegionCache;
}} // namespace Opm::out

namespace Opm {
    /// Abstract base class for summary parameters.
    ///
    /// A collection of summary parameters defines the contents
    /// of the SMSPEC file.  Collection usually defined by class
    /// SummaryConfig.
    class SummaryParameter
    {
    public:
        /// Static objects defined by simulation model (input)
        struct InputData
        {
            /// Main run specification/configuration
            const EclipseState& es;

            /// Dynamic control/timestepping object
            const Schedule& sched;

            /// Simulation model's grid structure (active vs. inactive cells)
            const EclipseGrid& grid;

            /// Management structure for associating individual well
            /// connections to (FIP) region IDs.
            const out::RegionCache& reg;
        };

        /// Dynamic objects/values calculated by simulator
        struct SimulatorResults
        {
            /// Well solution (rates, pressures &c)
            const data::WellRates& wellSol;

            /// Values associated with the simulation process or
            /// the model as a whole (e.g., CPU time, oil-in-place)
            const std::map<std::string, double>& single;

            /// Values associated with individual regions
            const std::map<std::string, std::vector<double>>& region;

            /// Values associated with individual blocks/cells
            const std::map<std::pair<std::string, int>, double>& block;
        };

        virtual ~SummaryParameter();

        /// Calculate and store a summary parameter value update into
        /// the run's global summary state object.
        ///
        /// \param[in] reportStep ID of report step at which to
        ///    calculate value of summary parameter.
        ///
        /// \param[in] stepSize Simulated time (seconds) since previous
        ///    call to \code update() \endcode for this parameter.
        ///    Typically the size of the latest "mini step".
        ///
        /// \param[in] input Static objects describing the simulation run.
        ///
        /// \param[in] simRes Dynamic simulation results at this time.
        ///
        /// \param[in,out] st Summary state object.  On input, a fully
        ///    formed object.  On output, modified through one of its
        ///    \code update*() \endcode member functions.
        virtual void update(const std::size_t       reportStep,
                            const double            stepSize,
                            const InputData&        input,
                            const SimulatorResults& simRes,
                            SummaryState&           st) const = 0;

        /// Retrieve unique lookup key string for parameter in a
        /// \c SummaryState objec.
        virtual std::string summaryKey() const = 0;

        /// Retrieve summary parameter keyword.
        ///
        /// Common examples include "WOPR" for the oil production rate in
        /// a well, "GGIT" for the total, cumulative injected volume of
        /// gas attributed to a single group, or "FGOR" for current flowing
        /// gas/oil volume ratio aggregated across the complete field.
        virtual std::string keyword() const = 0;

        /// Retrieve name of object associated to this summary parameter.
        ///
        /// Non-trivial value for groups or wells.  Sentinel value
        /// otherwise.
        virtual std::string name() const
        {
            return ":+:+:+:+";
        }

        /// Retrieve numeric ID of object associated to this
        /// summary parameter.
        ///
        /// Non-trivial value for grid cells, regions, well connections.
        /// Sentinel value (zero) otherwise.
        virtual int num() const
        {
            return 0;
        }

        /// Retrieve display purpose unit string for this summary parameter.
        virtual std::string unit(const UnitSystem& /* usys */) const
        {
            return std::string(std::string::size_type{8}, ' ');
        }
    };
} // namespace Opm

#endif // OPM_SUMMARY_PARAMETER_HPP
