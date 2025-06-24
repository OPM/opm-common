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
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.
    void add_timestep(const SummaryState& st,
                      const int           report_step,
                      const bool          isSubstep);

    /// Calculate summary vector values.
    ///
    /// \param[in,out] summary_state Summary vector values.  On exit, holds
    /// updated values for all vectors that are not user-defined quantities.
    /// UDQs are calculated in UDQConfig::eval() which should be called
    /// shortly after calling Summary::eval().
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  This is the number that gets incorporated into the
    /// file extension of "separate" summary output files (i.e., .S000n).
    /// Report_step=0 represents time zero.
    ///
    /// \param[in] secs_elapsed Elapsed physical time in seconds since start
    /// of simulation.
    ///
    /// \param[in] well_solution Collection of per-well, per-connection, and
    /// per-segment solution variables.
    ///
    /// \param[in] wbp Well-block average pressures inferred from WPAVE (or
    /// WWPAVE) settings.
    ///
    /// \param[in] group_and_nwrk_solution Constraints, guide rates and
    /// nodal pressures for the extended network model.
    ///
    /// \param[in] single_values named scalar quantities such as field-wide
    /// pressures, rates, and volumes, as well as performance related
    /// quantities such as CPU time, number of linear iterations &c.
    ///
    /// \param[in] initial_inplace Volumes initially in place.  Needed to
    /// calculate recovery factors.  Nullopt if such initial volumes are not
    /// available.
    ///
    /// \param[in] inplace Current volumes in place.
    ///
    /// \param[in] region_values Per-region quantities.  Empty if such
    /// values do not exist--typically in unit tests or if per-region
    /// summary output has not been requested.
    ///
    /// \param[in] block_values Per-block (cell) quantities.  Empty if such
    /// values do not exist (e.g., in unit tests) or if no per-block summary
    /// vectors have been requested.
    ///
    /// \param[in] aquifers_values Flow rates, cumulatives, and pressures
    /// attributed to aquifers--both analytic and numerical aquifers.  Empty
    /// if such values do not exist (e.g., in unit tests) or if per-aquifer
    /// summary vectors have not been requested.
    ///
    /// \param[in] interreg_flows Inter-region flows (rates and
    /// cumulatives).  Empty if no such values exist (e.g., in unit tests)
    /// or if no such summary vectors have been requested.
    void eval(SummaryState&                          summary_state,
              const int                              report_step,
              const double                           secs_elapsed,
              const data::Wells&                     well_solution,
              const data::WellBlockAveragePressures& wbp,
              const data::GroupAndNetworkValues&     group_and_nwrk_solution,
              const GlobalProcessParameters&         single_values,
              const std::optional<Inplace>&          initial_inplace,
              const Inplace&                         inplace,
              const RegionParameters&                region_values = {},
              const BlockValues&                     block_values  = {},
              const data::Aquifers&                  aquifers_values = {},
              const InterRegFlowValues&              interreg_flows = {}) const;

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
