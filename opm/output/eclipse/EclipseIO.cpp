/*
  Copyright (c) 2016 Statoil ASA
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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

#include "config.h"

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/output/eclipse/AggregateAquiferData.hpp>
#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>
#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/WriteInit.hpp>
#include <opm/output/eclipse/WriteRFT.hpp>

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>     // unique_ptr
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>    // move
#include <vector>

namespace {

/// Create directory if it does not already exist
///
/// Intended primarily for the run's output directory.
///
/// \param[in] odir Directory name.  This function will create the named
/// directory if it does not already exist.  If, on the other hand, a
/// filesystem element with the name \p odir already exists but is not a
/// directory, then this function will throw an exception of type \code
/// std::runtime_error \endcode.  Finally, if the function is unable to
/// create the named directory, e.g., because of insufficient privileges,
/// then the function will also throw an exception of type \code
/// std::runtime_error \endcode.
void ensure_directory_exists(const std::filesystem::path& odir)
{
    namespace fs = std::filesystem;

    if (fs::exists(odir) && !fs::is_directory(odir)) {
        throw std::runtime_error {
            "Filesystem element '" + odir.generic_string()
            + "' already exists but is not a directory"
        };
    }

    std::error_code ec{};
    if (! fs::exists(odir)) {
        fs::create_directories(odir, ec);
    }

    if (ec) {
        std::ostringstream msg;

        msg << "Failed to create output directory '"
            << odir.generic_string()
            << "\nSystem reports: " << ec << '\n';

        throw std::runtime_error { msg.str() };
    }
}

std::vector<Opm::time_point> reportStepStartTimes(const Opm::Schedule& sched)
{
    auto rptStepStart = std::vector<Opm::time_point>{};
    rptStepStart.reserve(sched.size());

    std::ranges::transform(sched, std::back_inserter(rptStepStart),
                           [](const Opm::ScheduleState& state)
                           { return state.start_time(); });

    return rptStepStart;
}

} // Anonymous namespace

/// Internal implementation class for EclipseIO public interface.
class Opm::EclipseIO::Impl
{
public:
    /// Constructor.
    ///
    /// Invoked only by containing class EclipseIO.
    ///
    /// \param[in] eclipseState Run's static parameters such as region
    /// definitions.  The EclipseIO object retains a reference to this
    /// object, whence the lifetime of \p eclipseState should exceed that of
    /// the EclipseIO object.
    ///
    /// \param[in] grid Run's active cells.  The EclipseIO object takes
    /// ownership of this grid object.
    ///
    /// \param[in] schedule Run's dynamic objects.  The EclipseIO object
    /// retains a reference to this object, whence the lifetime of \p
    /// schedule should exceed that of the EclipseIO object.
    ///
    /// \param[in] summryConfig Run's collection of summary vectors
    /// requested in the SUMMARY section of the model description.  Used to
    /// initialise an internal \c SummaryConfig object that will
    /// additionally contain all vectors needed to evaluate the defining
    /// expressions of any user-defined quantities in the run.
    ///
    /// \param[in] baseName Name of main input data file, stripped of
    /// extensions and directory names.
    ///
    /// \param[in] writeEsmry Whether or not to additionally create a
    /// "transposed" .ESMRY output file during the simulation run.  ESMRY
    /// files typically load faster into post-processing tools such as
    /// qsummary and ResInsight than traditional SMSPEC/UNSMRY files,
    /// especially if the user only needs to view a small number of vectors.
    /// On the other hand, ESMRY files typically require more memory while
    /// writing.
    Impl(const EclipseState&  eclipseState,
         EclipseGrid          grid,
         const Schedule&      schedule,
         const SummaryConfig& summryConfig,
         const std::string&   baseName,
         const bool           writeEsmry);

    /// Whether or not run requests file output.
    bool outputEnabled() const { return this->output_enabled_; }

    /// Whether or not run requests RFT file output at this time.
    ///
    /// \param[in] report_step One-based report step index.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.  We typically do not output RFT files for
    /// sub-steps.
    ///
    /// \return RFT file output status.  First member is whether or not to
    /// output RFT information at this time.  Second is whether or not an
    /// RFT file already exists.
    std::pair<bool, bool>
    wantRFTOutput(const int report_step, const bool isSubstep) const;

    /// Whether or not to output summary file information at this time.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \param[in] time_step Current time step index.  Passing something
    /// different than nullopt will typically generate summary file output
    /// for all times.
    ///
    /// \return Whether or not to output summary file information at this
    /// time.
    bool wantSummaryOutput(const int          report_step,
                           const bool         isSubstep,
                           const double       secs_elapsed,
                           std::optional<int> time_step) const;

    /// Whether or not to output restart file information at this time.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.
    ///
    /// \param[in] time_step Current time step index.  Passing something
    /// different than nullopt will typically generate summary file output
    /// for all times.
    ///
    /// \return Whether or not to output restart file information at this
    /// time.
    bool wantRestartOutput(const int          report_step,
                           const bool         isSubstep,
                           std::optional<int> time_step) const;

    /// Whether or not this is the run's final report step.
    ///
    /// Simplifies checking for whether or not to to create an RSM file.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \return Whether or not \p report_step is the run's final report
    /// step.
    bool isFinalStep(const int report_step) const;

    /// Name of run's output directory.
    const std::string& outputDir() const { return this->outputDir_; }

    /// Run's summary vector calculation engine.
    const out::Summary& summary() const { return this->summary_; }

    /// Run's complete summary configuration object, including those vectors
    /// that are needed to evaluate the defining expressions of any
    /// user-defined quantities.
    const SummaryConfig& summaryConfig() const { return this->summaryConfig_; }

    /// Load per-cell solution data and wellstate from restart file.
    ///
    /// Name of restart file and report step from which to restart inferred
    /// from internal IOConfig object.
    ///
    /// \param[in] solution_keys Descriptors of requisite and optional
    /// per-cell dynamic values to load from restart file.
    ///
    /// \param[in] extra_keys Descriptors of additional dynamic values to
    /// load from restart file.  Optional.
    ///
    /// \param[in,out] action_state Run's action system state.  On input, a
    /// valid object.  On exit, populated from restart file information.
    ///
    /// \param[in,out] summary_state Run's container of summary vector
    /// values.  On input, a valid object.  On exit, populated from restart
    /// file information.  Mostly relevant to cumulative quantities such as
    /// FOPT.
    ///
    /// \return Collection of per-cell, per-well, per-connection,
    /// per-segment, per-group, and per-aquifer dynamic results at
    /// simulation restart time.
    RestartValue loadRestart(const std::vector<RestartKey>& solution_keys,
                             const std::vector<RestartKey>& extra_keys,
                             Action::State&                 action_state,
                             SummaryState&                  summary_state) const;

    /// Load per-cell solution data from restart file at specific time.
    ///
    /// Common use case is to load the initial volumes-in-place from time
    /// zero.
    ///
    /// Name of restart file inferred from internal IOConfig object.
    ///
    /// The map keys should be a map of keyword names and their
    /// corresponding dimension object.  In other words, loading the state
    /// from a simple two phase simulation you would pass:
    ///
    ///    keys = {
    ///        {"PRESSURE" , UnitSystem::measure::pressure },
    ///        {"SWAT"     , UnitSystem::measure::identity },
    ///    }
    ///
    /// For a three phase black oil simulation you would add pairs for SGAS,
    /// RS and RV.  If you request keys which are not found in the restart
    /// file an exception will be raised.  This also happens if the size of
    /// a vector does not match the expected size.
    ///
    /// \param[in] solution_keys Descriptors of requisite and optional
    /// per-cell dynamic values to load from restart file.
    ///
    /// \param[in] report_step One-based report step index for which load
    /// restart file information.
    ///
    /// \return Collection of per-cell results at \p report_step.
    data::Solution loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                       const int                      report_step) const;

    /// Output static properties to EGRID and INIT files.
    ///
    /// \param[in] simProps Initial per-cell properties such as
    /// transmissibilities.  Will be output to the INIT file.
    ///
    /// \param[in] int_data Additional integer arrays defined by simulator.
    /// May contain things like the MPI partition arrays.  Will be output to
    /// the INIT file.
    ///
    /// \param[in] nnc Run's non-neighbouring connections.  Includes those
    /// connections that are derived from corner-point grid processing and
    /// those connections that are explicitly entered using keywords like
    /// NNC, EDITNNC, or EDITNNCR.  The cell pairs will be output to the
    /// EGRID file while the associate transmissibility will be output to
    /// the INIT file.
    void writeInitial(data::Solution                          simProps,
                      std::map<std::string, std::vector<int>> int_data,
                      const std::vector<NNCdata>&             nnc) const;

    /// Create summary file output.
    ///
    /// Calls Summary::add_timestep() and Summary::write().
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
    /// \param[in] time_step Zero-based time step ID.  Nullopt if the
    /// sequence number should be the same as the report step.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.
    void writeSummaryFile(const SummaryState&      st,
                          const int                report_step,
                          const std::optional<int> time_step,
                          const double             secs_elapsed,
                          const bool               isSubstep);

    /// Create restart file output.
    ///
    /// Calls RestartIO::save().
    ///
    /// \param[in] action_state Run's current action system state.  Expected
    /// to hold current values for the number of times each action has run
    /// and the time of each action's last run.
    ///
    /// \param[in] wtest_state Run's current WTEST information.  Expected to
    /// hold information about those wells that have been closed due to
    /// various runtime conditions.
    ///
    /// \param[in] st Summary values from most recent call to
    /// Summary::eval().  Source object from which to retrieve the values
    /// that go into the output buffer.
    ///
    /// \param[in] udq_state Run's current UDQ values.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \param[in] time_step Current time step index.  Nullopt if the
    /// sequence number should be the same as the report step index.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \param[in] write_double Whether or not to output simulation results
    /// as double precision floating-point numbers.
    ///
    /// \param[in] value Collection of per-cell, per-well, per-connection,
    /// per-segment, per-group, and per-aquifer dynamic results pertaining
    /// to this time point.
    void writeRestartFile(const Action::State& action_state,
                          const WellTestState& wtest_state,
                          const SummaryState&  st,
                          const UDQState&      udq_state,
                          const int            report_step,
                          std::optional<int>   time_step,
                          const double         secs_elapsed,
                          const bool           write_double,
                          RestartValue&&       value);

    /// Create restart file output for simulation runs with local grids.
    ///
    /// Calls RestartIO::save().
    ///
    /// \param[in] action_state Run's current action system state.  Expected
    /// to hold current values for the number of times each action has run
    /// and the time of each action's last run.
    ///
    /// \param[in] wtest_state Run's current WTEST information.  Expected to
    /// hold information about those wells that have been closed due to
    /// various runtime conditions.
    ///
    /// \param[in] st Summary values from most recent call to
    /// Summary::eval().  Source object from which to retrieve the values
    /// that go into the output buffer.
    ///
    /// \param[in] udq_state Run's current UDQ values.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \param[in] time_step Current time step index.  Nullopt if the
    /// sequence number should be the same as the report step index.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \param[in] write_double Whether or not to output simulation results
    /// as double precision floating-point numbers.
    ///
    /// \param[in] value Collection of per-cell, per-well, per-connection,
    /// per-segment, per-group, and per-aquifer dynamic results pertaining
    /// to this time point.  One collection per grid, with \code
    /// value.front() \endcode representing the main/global grid.
    void writeRestartFile(const Action::State&        action_state,
                          const WellTestState&        wtest_state,
                          const SummaryState&         st,
                          const UDQState&             udq_state,
                          const int                   report_step,
                          std::optional<int>          time_step,
                          const double                secs_elapsed,
                          const bool                  write_double,
                          std::vector<RestartValue>&& value);

    /// Create RSM file.
    ///
    /// Loads the run's summary output files and writes the "transposed"
    /// file output.  Should typically be called only at the end of the
    /// simulation run, and only if specifically requested through the
    /// RUNSUM keyword.
    void writeRunSummary() const;

    /// Create RFT file output.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero, although this is
    /// typically not applicable to this function.
    ///
    /// \param[in] haveExistingRFT Whether or not the run's RFT file already
    /// exists.
    ///
    /// \param[in] wellSol Per-well, per-connection, and per-segment dynamic
    /// result values.
    void writeRftFile(const double       secs_elapsed,
                      const int          report_step,
                      const bool         haveExistingRFT,
                      const data::Wells& wellSol) const;

    /// Record full processing of a complete time step.
    void countTimeStep() { ++this->miniStepId_; }

private:
    /// Run's static properties.
    std::reference_wrapper<const EclipseState> es_;

    /// Run's dynamic objects.
    std::reference_wrapper<const Schedule> schedule_;

    /// Run's active cells.
    EclipseGrid grid_;

    /// Run's output directory.
    std::string outputDir_;

    /// Run's base name.  Input DATA file without extensions or directories.
    std::string baseName_;

    /// Run's complete summary configuration object.
    SummaryConfig summaryConfig_;

    /// Run's summary vector calculation engine.
    out::Summary summary_;

    /// Whether or not run requests any kind of file output.
    ///
    /// Typically 'true', although 'false' may be useful in performance
    /// tests.
    bool output_enabled_{false};

    /// Cached copy of ScheduleState::start_time() to avoid race conditions
    /// in elapsedTimeAccepted().
    std::vector<time_point> rptStepStart_{};

    /// Run's current time step ID.
    int miniStepId_{0};

    /// Static aquifer descriptions for restart file output.
    ///
    /// Nullopt unless run includes aquifers.
    std::optional<RestartIO::Helpers::AggregateAquiferData> aquiferData_{std::nullopt};

    /// Whether or not SUMTHIN is currently active.
    mutable bool sumthin_active_{false};

    /// Whether or not sufficient time has passed since last summary file
    /// output in the context of SUMTHIN.
    ///
    /// Applicable only if sumthin_active_ is true.
    mutable bool sumthin_triggered_{false};

    /// Simulated time of last summary file output triggered by SUMTHIN.
    ///
    /// Applicable only if sumthin_active_ is true.
    double last_sumthin_output_{std::numeric_limits<double>::lowest()};

    /// Elapsed/simulated time at last summary file output event.
    ///
    /// Stored as \c float to mimic the summary file's TIME vector.
    float last_summary_output_{std::numeric_limits<float>::lowest()};

    /// Output static properties to INIT file.
    ///
    /// \param[in] simProps Initial per-cell properties such as
    /// transmissibilities.  Will be output to the INIT file.
    ///
    /// \param[in] int_data Additional integer arrays defined by simulator.
    /// May contain things like the MPI partition arrays.  Will be output to
    /// the INIT file.
    ///
    /// \param[in] nnc Run's non-neighbouring connections.  Includes those
    /// connections that are derived from corner-point grid processing and
    /// those connections that are explicitly entered using keywords like
    /// NNC, EDITNNC, or EDITNNCR.  This function uses only the
    /// transmissibility information.
    void writeInitFile(data::Solution                          simProps,
                       std::map<std::string, std::vector<int>> int_data,
                       const std::vector<NNCdata>&             nnc) const;

    /// Output static geometry to EGRID file.
    ///
    /// \param[in] nnc Run's non-neighbouring connections.  Includes those
    /// connections that are derived from corner-point grid processing and
    /// those connections that are explicitly entered using keywords like
    /// NNC, EDITNNC, or EDITNNCR.  This function uses only the cell pairs.
    void writeEGridFile(const std::vector<NNCdata>& nnc) const;

    /// Record a summary file output event.
    ///
    /// Handles details of SUMTHIN triggers if applicable.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    void recordSummaryOutput(const double secs_elapsed);

    /// Compute report sequence number.
    ///
    /// Typically equal to the report step index.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \param[in] time_step Zero-based time step ID.  Nullopt if the
    /// sequence number should be the same as the report step.
    int reportIndex(const int report_step, const std::optional<int> time_step) const;

    /// Decide whether or not SUMTHIN triggers summary file output.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \return Whether or not summary file output is triggered by SUMTHIN.
    /// False if SUMTHIN is not currently active or if time since last
    /// SUMTHIN output event does not exceed current SUMTHIN interval.  This
    /// predicate value is also stored in the sumthin_triggered_ data
    /// member.
    bool checkAndRecordIfSumthinTriggered(const int report_step,
                                          const double secs_elapsed) const;

    /// Whether or not to output summary file information only at report
    /// steps.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero.
    ///
    /// \return Run's RPTONLY setting at \p report_step.  Typically false.
    bool summaryAtRptOnly(const int report_step) const;

    /// Whether or not to enable summary file output at particular time.
    ///
    /// This is special case handling to ensure that the summary file's TIME
    /// vector is strictly increasing at all summary file output times.
    /// Since TIME is stored in single precision ("float"), the minimum time
    /// between summary file output events must typically be at least
    ///
    ///     numeric_limits<float>::epsilon() * elapsed
    ///
    /// which grows with the total simulated time.  For common float
    /// implementations, this means that the minimum interval between
    /// summary file output events may increase by as much as a minute for
    /// each decade of simulated time.
    ///
    /// This special case handling is nevertheless applicable only to
    /// sub-steps.  We always create summary file output for report steps.
    ///
    /// \param[in] report_step One-based report step index for which to
    /// create output.  Report_step=0 represents time zero--the start of the
    /// simulation.
    ///
    /// \param[in] secs_elapsed Elapsed physical (i.e., simulated) time in
    /// seconds since start of simulation.
    ///
    /// \return Whether or not \p secs_elapsed, when treated as a \c float,
    /// is strictly between the previous summary file output time
    /// (last_summary_output_) and the next report step time.
    bool elapsedTimeAccepted(const int report_step, const double secs_elapsed) const;
};

Opm::EclipseIO::Impl::Impl(const EclipseState&  eclipseState,
                           EclipseGrid          grid,
                           const Schedule&      schedule,
                           const SummaryConfig& summaryConfig,
                           const std::string&   base_name,
                           const bool           writeEsmry)
    : es_            (std::cref(eclipseState))
    , schedule_      (std::cref(schedule))
    , grid_          (std::move(grid))
    , outputDir_     (eclipseState.getIOConfig().getOutputDir())
    , baseName_      (uppercase(eclipseState.getIOConfig().getBaseName()))
    , summaryConfig_ (summaryConfig)
    , summary_       (summaryConfig_, es_, grid_, schedule, base_name, writeEsmry)
    , output_enabled_(eclipseState.getIOConfig().getOutputEnabled())
    , rptStepStart_  (reportStepStartTimes(schedule))
{
    if (const auto& aqConfig = this->es_.get().aquifer();
        aqConfig.connections().active() || aqConfig.hasNumericalAquifer())
    {
        this->aquiferData_
            .emplace(RestartIO::inferAquiferDimensions(this->es_),
                     aqConfig, this->grid_);
    }
}

std::pair<bool, bool>
Opm::EclipseIO::Impl::wantRFTOutput(const int  report_step,
                                    const bool isSubstep) const
{
    if (isSubstep) {
        return { false, false };
    }

    const auto first_rft = this->schedule_.get().first_RFT();
    if (! first_rft.has_value()) {
        return { false, false };
    }

    const auto first_rft_out = first_rft.value();
    const auto step = static_cast<std::size_t>(report_step);

    return {
        step >= first_rft_out,  // wantRFT
        step >  first_rft_out   // haveExistingRFT
    };
}

bool Opm::EclipseIO::Impl::isFinalStep(const int report_step) const
{
    return report_step == static_cast<int>(this->schedule_.get().size()) - 1;
}

bool Opm::EclipseIO::Impl::wantSummaryOutput(const int          report_step,
                                             const bool         isSubstep,
                                             const double       secs_elapsed,
                                             std::optional<int> time_step) const
{
    if (isSubstep && !this->elapsedTimeAccepted(report_step, secs_elapsed)) {
        // Time step too short or too close to end of report step.  This
        // would lead to the summary file's single precision (float) TIME
        // vector not being strictly increasing which is known to cause
        // problems for at least some post-processing tools.  Don't write
        // summary file output for this time step.
        //
        // Note: This special provision applies only to sub-steps.  We
        // *always* emit summary file information at the end of a report
        // step.
        return false;
    }

    if (time_step.has_value()) {
        return true;
    }

    if (report_step <= 0) {
        return false;
    }

    // Check this condition first because the end of a SUMTHIN interval
    // might coincide with a report step.  In that case we also need to
    // reset the interval starting point even if the primary reason for
    // generating summary output is the report step.
    this->checkAndRecordIfSumthinTriggered(report_step, secs_elapsed);

    return !isSubstep
        || (!this->summaryAtRptOnly(report_step)
            && (!this->sumthin_active_ || this->sumthin_triggered_));
}

bool Opm::EclipseIO::Impl::wantRestartOutput(const int          report_step,
                                             const bool         isSubstep,
                                             std::optional<int> time_step) const
{
    return (time_step.has_value() && (*time_step > 0))
        || (!isSubstep && this->schedule_.get().write_rst_file(report_step));
}

Opm::RestartValue
Opm::EclipseIO::Impl::loadRestart(const std::vector<RestartKey>& solution_keys,
                                  const std::vector<RestartKey>& extra_keys,
                                  Action::State&                 action_state,
                                  SummaryState&                  summary_state) const
{
    const auto& initConfig = this->es_.get().getInitConfig();

    const auto report_step = initConfig.getRestartStep();
    const auto filename    = this->es_.get().cfg().io()
        .getRestartFileName(initConfig.getRestartRootName(),
                            report_step,
                            /* for file writing output = */ false);

    return RestartIO::load(filename,
                           report_step,
                           action_state,
                           summary_state,
                           solution_keys,
                           this->es_,
                           this->grid_,
                           this->schedule_,
                           extra_keys);
}

Opm::data::Solution
Opm::EclipseIO::Impl::loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                          const int                      report_step) const
{
    const auto& initConfig  = this->es_.get().getInitConfig();
    const auto  filename    = this->es_.get().getIOConfig()
        .getRestartFileName(initConfig.getRestartRootName(), report_step, false);

    return RestartIO::load_solution_only(filename, report_step, solution_keys,
                                         this->es_, this->grid_);
}

void Opm::EclipseIO::Impl::writeInitial(data::Solution                          simProps,
                                        std::map<std::string, std::vector<int>> int_data,
                                        const std::vector<NNCdata>&             nnc) const
{
    if (this->es_.get().cfg().io().getWriteINITFile()) {
        this->writeInitFile(std::move(simProps), std::move(int_data), nnc);
    }

    if (this->es_.get().cfg().io().getWriteEGRIDFile()) {
        this->writeEGridFile(nnc);
    }
}

void Opm::EclipseIO::Impl::writeSummaryFile(const SummaryState&      st,
                                            const int                report_step,
                                            const std::optional<int> time_step,
                                            const double             secs_elapsed,
                                            const bool               isSubstep)
{
    this->summary_.add_timestep(st, this->reportIndex(report_step, time_step),
                                this->miniStepId_,
                                !time_step.has_value() || isSubstep);

    const auto is_final_summary =
        this->isFinalStep(report_step) && !isSubstep;

    this->summary_.write(is_final_summary);

    this->recordSummaryOutput(secs_elapsed);
}

void Opm::EclipseIO::Impl::writeRestartFile(const Action::State& action_state,
                                            const WellTestState& wtest_state,
                                            const SummaryState&  st,
                                            const UDQState&      udq_state,
                                            const int            report_step,
                                            std::optional<int>   time_step,
                                            const double         secs_elapsed,
                                            const bool           write_double,
                                            RestartValue&&       value)
{
    EclIO::OutputStream::Restart rstFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        this->reportIndex(report_step, time_step),
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() },
        EclIO::OutputStream::Unified   { this->es_.get().cfg().io().getUNIFOUT() }
    };

    RestartIO::save(rstFile, report_step, secs_elapsed,
                    std::move(value),
                    this->es_, this->grid_, this->schedule_,
                    action_state, wtest_state, st,
                    udq_state, this->aquiferData_, write_double);
}

void Opm::EclipseIO::Impl::writeRestartFile(const Action::State&        action_state,
                                            const WellTestState&        wtest_state,
                                            const SummaryState&         st,
                                            const UDQState&             udq_state,
                                            const int                   report_step,
                                            std::optional<int>          time_step,
                                            const double                secs_elapsed,
                                            const bool                  write_double,
                                            std::vector<RestartValue>&& value)
{
    EclIO::OutputStream::Restart rstFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        this->reportIndex(report_step, time_step),
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() },
        EclIO::OutputStream::Unified   { this->es_.get().cfg().io().getUNIFOUT() }
    };

    RestartIO::save(rstFile, report_step, secs_elapsed,
                    std::move(value),
                    this->es_, this->grid_, this->schedule_,
                    action_state, wtest_state, st,
                    udq_state, this->aquiferData_, write_double);
}

void Opm::EclipseIO::Impl::writeRunSummary() const
{
    const auto formatted = this->es_.get().cfg().io().getFMTOUT();

    const auto ext = '.'
        + (formatted ? std::string{"F"} : std::string{})
        + "SMSPEC";

    const auto rset = EclIO::OutputStream::ResultSet {
        this->outputDir_, this->baseName_
    };

    const auto smspec = EclIO::OutputStream::outputFileName(rset, ext);

    EclIO::ESmry { smspec }.write_rsm_file();
}

void Opm::EclipseIO::Impl::writeRftFile(const double       secs_elapsed,
                                        const int          report_step,
                                        const bool         haveExistingRFT,
                                        const data::Wells& wellSol) const
{
    // Open existing RFT file if report step is after first RFT event.
    const auto openExisting = EclIO::OutputStream::RFT::OpenExisting {
        haveExistingRFT
    };

    EclIO::OutputStream::RFT rftFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() },
        openExisting
    };

    RftIO::write(report_step,
                 secs_elapsed,
                 this->es_.get().getUnits(),
                 this->grid_,
                 this->schedule_,
                 wellSol,
                 rftFile);
}

// ---------------------------------------------------------------------------

void Opm::EclipseIO::Impl::writeInitFile(data::Solution                          simProps,
                                         std::map<std::string, std::vector<int>> int_data,
                                         const std::vector<NNCdata>&             nnc) const
{
    EclIO::OutputStream::Init initFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() }
    };

    simProps.convertFromSI(this->es_.get().getUnits());

    InitIO::write(this->es_, this->grid_, this->schedule_,
                  simProps, std::move(int_data), nnc, initFile);
}

void Opm::EclipseIO::Impl::writeEGridFile(const std::vector<NNCdata>& nnc) const
{
    const auto formatted = this->es_.get().cfg().io().getFMTOUT();

    const auto ext = '.'
        + (formatted ? std::string{"F"} : std::string{})
        + "EGRID";

    const auto rset = EclIO::OutputStream::ResultSet {
        this->outputDir_, this->baseName_
    };

    const auto egridFile = EclIO::OutputStream::outputFileName(rset, ext);

    this->grid_.save(egridFile, formatted, nnc, this->es_.get().getDeckUnitSystem());
}

void Opm::EclipseIO::Impl::recordSummaryOutput(const double secs_elapsed)
{
    if (this->sumthin_triggered_) {
        this->last_sumthin_output_ = secs_elapsed;
    }

    this->last_summary_output_ = static_cast<float>(secs_elapsed);
}

int Opm::EclipseIO::Impl::reportIndex(const int                report_step,
                                      const std::optional<int> time_step) const
{
    return time_step.has_value()
        ?  (*time_step + 1)
        :  report_step;
}

bool Opm::EclipseIO::Impl::checkAndRecordIfSumthinTriggered(const int    report_step,
                                                            const double secs_elapsed) const
{
    const auto& sumthin = this->schedule_.get()[report_step - 1].sumthin();

    this->sumthin_active_ = sumthin.has_value(); // True if SUMTHIN value strictly positive.

    return this->sumthin_triggered_ = this->sumthin_active_
        && ! (secs_elapsed < this->last_sumthin_output_ + sumthin.value());
}

bool Opm::EclipseIO::Impl::summaryAtRptOnly(const int report_step) const
{
    return this->schedule_.get()[report_step - 1].rptonly();
}

bool Opm::EclipseIO::Impl::elapsedTimeAccepted(const int    report_step,
                                               const double secs_elapsed) const
{
    const auto float_elapsed = static_cast<float>(secs_elapsed);

    // Recall: 'report_step' is a one-based index, so using this directly as
    // a subscript in rptStepStart_ means we get the start time of the
    // *next* report step.
    const auto float_nextrpt = std::chrono::duration
        <float, std::chrono::seconds::period> {
        this->rptStepStart_[report_step] - this->rptStepStart_.front()
    }.count();

    // We accept the elapsed time if, when treated as a float, it is
    // *strictly* between the previous summary file output time and the end
    // of the current report step (i.e., the start of the next report step).
    return (float_elapsed > this->last_summary_output_)
        && (float_elapsed < float_nextrpt);
}

// ===========================================================================

Opm::EclipseIO::EclipseIO(const EclipseState&  es,
                          EclipseGrid          grid,
                          const Schedule&      schedule,
                          const SummaryConfig& summary_config,
                          const std::string&   baseName,
                          const bool           writeEsmry)
    : impl { std::make_unique<Impl>(es, std::move(grid),
                                    schedule, summary_config,
                                    baseName, writeEsmry) }
{
    if (! this->impl->outputEnabled()) {
        return;
    }

    ensure_directory_exists(this->impl->outputDir());
}

Opm::EclipseIO::~EclipseIO() = default;

void Opm::EclipseIO::writeInitial(data::Solution                          simProps,
                                  std::map<std::string, std::vector<int>> int_data,
                                  const std::vector<NNCdata>&             nnc)
{
    if (! this->impl->outputEnabled()) {
        return;
    }

    this->impl->writeInitial(std::move(simProps), std::move(int_data), nnc);
}

void Opm::EclipseIO::writeTimeStep(const Action::State& action_state,
                                   const WellTestState& wtest_state,
                                   const SummaryState&  st,
                                   const UDQState&      udq_state,
                                   const int            report_step,
                                   const bool           isSubstep,
                                   const double         secs_elapsed,
                                   RestartValue         value,
                                   const bool           write_double,
                                   std::optional<int>   time_step)
{
    if (! this->impl->outputEnabled()) {
        // Run does not request any output.  Uncommon, but might be useful
        // in the case of performance testing.
        return;
    }

    // RFT file written only if requested and never for substeps.
    if (const auto& [wantRFT, haveExistingRFT] =
        this->impl->wantRFTOutput(report_step, isSubstep);
        wantRFT)
    {
        this->impl->writeRftFile(secs_elapsed, report_step,
                                 haveExistingRFT, value.wells);
    }

    if (this->impl->wantSummaryOutput(report_step, isSubstep, secs_elapsed, time_step)) {
        this->impl->writeSummaryFile(st, report_step, time_step,
                                     secs_elapsed, isSubstep);
    }

    if (this->impl->wantRestartOutput(report_step, isSubstep, time_step)) {
        // Restart file output (RPTRST &c).
        this->impl->writeRestartFile(action_state, wtest_state, st, udq_state,
                                     report_step, time_step, secs_elapsed,
                                     write_double, std::move(value));
    }

    if (! isSubstep &&
        this->impl->isFinalStep(report_step) &&
        this->impl->summaryConfig().createRunSummary())
    {
        // Write RSM file at end of simulation.
        this->impl->writeRunSummary();
    }

    this->impl->countTimeStep();
}


void Opm::EclipseIO::writeTimeStep(const Action::State&      action_state,
                                   const WellTestState&      wtest_state,
                                   const SummaryState&       st,
                                   const UDQState&           udq_state,
                                   const int                 report_step,
                                   const bool                isSubstep,
                                   const double              secs_elapsed,
                                   std::vector<RestartValue> value,
                                   const bool                write_double,
                                   std::optional<int>        time_step)
{
    if (! this->impl->outputEnabled()) {
        // Run does not request any output.  Uncommon, but might be useful
        // in the case of performance testing.
        return;
    }

    // RFT file is currently skipped for LGR grids.

    if (this->impl->wantSummaryOutput(report_step, isSubstep, secs_elapsed, time_step)) {
        this->impl->writeSummaryFile(st, report_step, time_step,
                                     secs_elapsed, isSubstep);
    }

    if (this->impl->wantRestartOutput(report_step, isSubstep, time_step)) {
        // Restart file output (RPTRST &c).
        this->impl->writeRestartFile(action_state, wtest_state, st, udq_state,
                                     report_step, time_step, secs_elapsed,
                                     write_double, std::move(value));
    }

    if (! isSubstep &&
        this->impl->isFinalStep(report_step) &&
        this->impl->summaryConfig().createRunSummary())
    {
        // Write RSM file at end of simulation.
        this->impl->writeRunSummary();
    }

    this->impl->countTimeStep();
}


Opm::RestartValue
Opm::EclipseIO::loadRestart(Action::State&                 action_state,
                            SummaryState&                  summary_state,
                            const std::vector<RestartKey>& solution_keys,
                            const std::vector<RestartKey>& extra_keys) const
{
    return this->impl->loadRestart(solution_keys, extra_keys,
                                   action_state, summary_state);
}

Opm::data::Solution
Opm::EclipseIO::loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                    const int                      report_step) const
{
    return this->impl->loadRestartSolution(solution_keys, report_step);
}

const Opm::out::Summary& Opm::EclipseIO::summary() const
{
    return this->impl->summary();
}

const Opm::SummaryConfig& Opm::EclipseIO::finalSummaryConfig() const
{
    return this->impl->summaryConfig();
}
