/*
  Copyright 2016 Statoil ASA.

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

#ifndef OPM_OUTPUT_SUMMARY_HPP
#define OPM_OUTPUT_SUMMARY_HPP

#include <opm/output/data/Aquifer.hpp>
#include <opm/output/data/InterRegFlowMap.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Opm {
    class EclipseGrid;
    class EclipseState;
    class Inplace;
    class Schedule;
    class SummaryConfig;
    class SummaryState;
} // namespace Opm

namespace Opm::data {
    class GroupAndNetworkValues;
    class InterRegFlowMap;
    struct WellBlockAveragePressures;
    class Wells;
} // namespace Opm::data

namespace Opm::out {

/// Computational engine for calculating summary vectors (2D curves as a
/// function of time) and writing these values to the summary file.
///
/// Supports formatted and unformatted, unified and separate summary files.
class Summary
{
public:
    /// Layer of indirection for transferring dynamic state objects into
    /// vector calculation engine.
    ///
    /// These values are typically computed by the simulator and the engine
    /// derives vectors from the values.
    struct DynamicSimulatorState
    {
        /// Collection of named scalar quantities such as field-wide pressures,
        /// rates, and volumes, as well as performance related quantities such
        /// as CPU time, number of linear iterations &c.
        using GlobalProcessParameters = std::map<std::string, double>;

        /// Collection of named per-region quantities.  Name may or may not
        /// include a region set identifier.
        using RegionParameters = std::map<std::string, std::vector<double>>;

        /// Collection of per-block (cell) quantities.
        ///
        /// Identifier associates a summary keyword and a block ID (linearised
        /// Cartesian cell index).
        using BlockValues = std::map<std::pair<std::string, int>, double>;

        /// Collection of named inter-region flows (rates and cumulatives)
        ///
        /// Name may or may not include a region set identifier.
        using InterRegFlowValues = std::unordered_map<std::string, data::InterRegFlowMap>;

        /// Volumes of fluids-in-place.
        struct VolumeInPlace
        {
            /// Current fluids-in-place.
            ///
            /// Nullptr if unavailable.
            const Inplace* current {nullptr};

            /// Initial fluids-in-place.
            ///
            /// Nullptr if unavailable.
            const Inplace* initial {nullptr};
        };

        /// Dynamic state variables at the well, connection, and segment levels.
        ///
        /// Nullptr if unavailable.
        const data::Wells* well_solution {nullptr};

        /// Well-block averaged pressures.
        ///
        /// Goes into the WBP* and WPI* summary quantities.
        ///
        /// Nullptr if unavailable.
        const data::WellBlockAveragePressures* wbp {nullptr};

        /// Dynamic state at the group and network levels (e.g., mode of
        /// control and node pressures).
        ///
        /// Nullptr if unavailable.
        const data::GroupAndNetworkValues* group_and_nwrk_solution {nullptr};

        /// Aggregate information about the simulation process such as
        /// number of linear and non-linear iterations or CPU time.
        ///
        /// Nullptr if unavailable.
        const GlobalProcessParameters* single_values {nullptr};

        /// Per region dynamic state such as pressures.
        ///
        /// Nullptr if unavailable.
        const RegionParameters* region_values {nullptr};

        /// Block (cell) level dynamic state values.
        ///
        /// Selection configured by the SummaryConfig object.  Nullptr if
        /// unavailable.
        const BlockValues* block_values {nullptr};

        /// Aquifer level dynamic state values.
        ///
        /// Pressures, flow rates and cumulatives.  Nullptr if unavailable.
        const data::Aquifers* aquifer_values {nullptr};

        /// Inter-region flows (rates and cumulatives).
        ///
        /// Nullptr if unavailable.
        const InterRegFlowValues* interreg_flows {nullptr};

        /// Fluid phase volumes in place at the field and region levels.
        ///
        /// Selection configured by SummaryConfig object.
        VolumeInPlace inplace{};
    };

    using DynamicConns = std::vector<std::pair<std::string, std::vector<std::size_t>>>;

    /// Constructor
    ///
    /// \param[in,out] sumcfg On input, the full collection of summary
    /// vectors requested in the run's SUMMARY section.  On exit, also
    /// contains those additional summary vectors needed to evaluate any UDQ
    /// defining expressions.
    ///
    /// \param[in] es Run's static parameters such as region definitions.
    /// The Summary object retains a reference to this object, so its
    /// lifetime should not exceed that of the EclipseState object.
    ///
    /// \param[in] grid Run's active cells.  The Summary object retains a
    /// reference to this object, so its lifetime should not exceed that of
    /// the EclipseGrid object.
    ///
    /// \param[in] sched Run's dynamic objects.  The Summary object retains
    /// a reference to this object, so its lifetime should not exceed that
    /// of the Schedule object.
    ///
    /// \param[in] basename Run's base name.  Needed to create names of
    /// summary output files.
    ///
    /// \param[in] writeEsmry Whether or not to additionally create a
    /// "transposed" .ESMRY output file during the simulation run.  ESMRY
    /// files typically load faster into post-processing tools such as
    /// qsummary and ResInsight than traditional SMSPEC/UNSMRY files,
    /// especially if the user only needs to view a small number of vectors.
    /// On the other hand, ESMRY files typically require more memory while
    /// writing.
    Summary(SummaryConfig&      sumcfg,
            const EclipseState& es,
            const EclipseGrid&  grid,
            const Schedule&     sched,
            const std::string&  basename = "",
            const bool          writeEsmry = false);

    /// Destructor.
    ///
    /// Needed for PIMPL idiom.
    ~Summary();

    /// Linearise summary values into internal buffer for output purposes.
    ///
    /// \param[in] st Summary values from most recent call to eval().
    /// Source object from which to retrieve the values that go into the
    /// output buffer.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  This is the number that gets incorporated into the
    /// file extension of "separate" summary output files (i.e., .S000n).
    /// Report_step=0 represents time zero.
    ///
    /// \param[in] ministep_id Zero based count of time steps performed.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.
    void add_timestep(const SummaryState& st,
                      const int           report_step,
                      const int           ministep_id,
                      const bool          isSubstep);

    void recordNewDynamicWellConns(const DynamicConns& newConns);

    /// Calculate summary vector values.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  This is the number that gets incorporated into the
    /// file extension of "separate" summary output files (i.e., .S000n).
    /// Report_step=0 represents time zero.
    ///
    /// \param[in] secs_elapsed Elapsed physical time in seconds since start
    /// of simulation.
    ///
    /// \param[in] values Dynamic state values from simulator.
    ///
    /// \param[in,out] summary_state Summary vector values.  On exit, holds
    /// updated values for all vectors that are not user-defined quantities.
    /// UDQs are calculated in UDQConfig::eval() which should be called
    /// shortly after calling Summary::eval().
    void eval(const int                    report_step,
              const double                 secs_elapsed,
              const DynamicSimulatorState& values,
              SummaryState&                summary_state) const;

    /// Write all current summary vector buffers to output files.
    ///
    /// \param[in] is_final_summary Whether or not this is the final summary
    /// output request.  When set to true, this guarantees that runs which
    /// request the creation of a "transposed" .ESMRY output file will create
    /// ESMRY file output containing all summary vector values.
    void write(const bool is_final_summary = false) const;

private:
    /// Implementation type.
    class SummaryImplementation;

    /// Pointer to class implementation object.
    std::unique_ptr<SummaryImplementation> pImpl_;
};

} // namespace Opm::out

#endif // OPM_OUTPUT_SUMMARY_HPP
