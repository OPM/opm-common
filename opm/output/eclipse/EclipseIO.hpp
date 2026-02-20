/*
  Copyright (c) 2013 Andreas Lauser
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2014 IRIS AS

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

#ifndef OPM_ECLIPSE_WRITER_HPP
#define OPM_ECLIPSE_WRITER_HPP

#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>

#include <opm/output/data/Solution.hpp>
#include <opm/output/eclipse/RestartValue.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Opm {

class EclipseGrid;
class EclipseState;
class RestartKey;
class Schedule;
class SummaryConfig;
class SummaryState;
class UDQState;
class WellTestState;

} // namespace Opm

namespace Opm::Action {
    class State;
} // namespace Opm::Action

namespace Opm::out {
    class Summary;
} // namespace Opm::out

namespace Opm {

/// File I/O management for reservoir description and dynamic results.
class EclipseIO
{
public:
    /// Constructor.
    ///
    /// \param[in] es Run's static parameters such as region definitions.
    /// The EclipseIO object retains a reference to this object, whence the
    /// lifetime of \p es should exceed that of the EclipseIO object.
    ///
    /// \param[in] grid Run's active cells.  The EclipseIO object takes
    /// ownership of this grid object.
    ///
    /// \param[in] schedule Run's dynamic objects.  The EclipseIO object
    /// retains a reference to this object, whence the lifetime of \p
    /// schedule should exceed that of the EclipseIO object.
    ///
    /// \param[in] summary_config Run's collection of summary vectors
    /// requested in the SUMMARY section of the model description.  Used to
    /// initialise an internal \c SummaryConfig object that will
    /// additionally contain all vectors needed to evaluate the defining
    /// expressions of any user-defined quantities in the run.
    ///
    /// \param[in] basename Name of main input data file, stripped of
    /// extensions and directory names.
    ///
    /// \param[in] writeEsmry Whether or not to additionally create a
    /// "transposed" .ESMRY output file during the simulation run.  ESMRY
    /// files typically load faster into post-processing tools such as
    /// qsummary and ResInsight than traditional SMSPEC/UNSMRY files,
    /// especially if the user only needs to view a small number of vectors.
    /// On the other hand, ESMRY files typically require more memory while
    /// writing.
    EclipseIO(const EclipseState&  es,
              EclipseGrid          grid,
              const Schedule&      schedule,
              const SummaryConfig& summary_config,
              const std::string&   basename = std::string{},
              const bool           writeEsmry = false);

    /// Deleted copy constructor.
    ///
    /// There must be exactly one object of this type in any given
    /// simulation run.
    EclipseIO(const EclipseIO&) = delete;

    /// Deleted assignment operator.
    ///
    /// There must be exactly one object of this type in any given
    /// simulation run.
    EclipseIO& operator=(const EclipseIO&) = delete;

    /// Destructor.
    ///
    /// Needed for PIMPL idiom.
    ~EclipseIO();

    /// Output static properties to EGRID and INIT files.
    ///
    /// Write static property data (grid, PVT curves, etc) to disk.
    /// Per-cell static property arrays are selected as follows:
    ///
    ///    1. 3D properties which can be calculated in the output layer are
    ///       unconditionally written to the INIT file.  This collection
    ///       currently includes the DX, DY, DZ, and DEPTH properties.
    ///
    ///    2. All integer properties from the input deck are unconditionally
    ///       output to the INIT file.  This collection will include at
    ///       least the FIPNUM, MULTNUM, PVTNUM, and SATNUM region
    ///       definition arrays since these can be created in the output
    ///       layer if needed.
    ///
    ///    3. The PORV array will *always* be present in the INIT file.
    ///       Furthermore, that array will be sized according to the number
    ///       of Cartesian input cells--i.e., Nx * Ny * Nz.  All other 3D
    ///       properties, whether floating-point or integer, will be sized
    ///       according to the run's number of active cells.
    ///
    ///    4. Certain floating-point 3D property arrays from the input deck,
    ///       such as PORO, PERM* and scaled saturation function end points,
    ///       are specifically known to the INIT file writing logic.  If
    ///       available in the run, these will be output to the INIT file.
    ///
    ///    5. SimProps contains additional 3D floating-point properties from
    ///       the simulator.  Common property arrays here include the TRAN*
    ///       arrays of interface transmissibilities, but could in principle
    ///       be any floating-point property.
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
    void writeInitial(data::Solution simProps = data::Solution(),
                      std::map<std::string, std::vector<int>> int_data = {},
                      const std::vector<NNCdata>& nnc = {});

    /// Write reservoir state and summary information to disk.
    ///
    /// Calling this method is only meaningful after the first time step has
    /// been completed.
    ///
    /// The RestartValue contains fields which have been calculated by the
    /// simulator and are written to the restart file.  Examples of such
    /// fields would be the relative permeabilities KRO, KRW and KRG and
    /// fluxes.  The keywords which can be added here are represented with
    /// mnemonics in the RPTRST keyword.
    ///
    /// If the optional argument write_double is sent in as true the fields
    /// in the solution container will be written in double precision.  OPM
    /// can load and restart from files with double precision keywords, but
    /// this is non-standard, and other third party applications might choke
    /// on those.
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
    /// create output.  This is the number that gets incorporated into the
    /// file extension of "separate" restart and summary output files (e.g.,
    /// .X000n and .S000n).  Report_step=0 represents time zero.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.  We typically output summary file information only
    /// for sub-steps.
    ///
    /// \param[in] seconds_elapsed Elapsed physical (i.e., simulated) time
    /// in seconds since start of simulation.
    ///
    /// \param[in] value Collection of per-cell, per-well, per-connection,
    /// per-segment, per-group, and per-aquifer dynamic results pertaining
    /// to this time point.
    ///
    /// \param[in] write_double Whether or not to output simulation results
    /// as double precision floating-point numbers.  Compatibility
    /// considerations may dictate outputting arrays as single precision
    /// ("float") only.
    ///
    /// \param[in] time_step Current time step index.  Passing something
    /// different than nullopt will generate restart file output even for
    /// time steps that are not report steps.  This is a poor-man's
    /// approximation of the BASIC=6 setting of the RPTRST keyword.
    void writeTimeStep(const Action::State& action_state,
                       const WellTestState& wtest_state,
                       const SummaryState&  st,
                       const UDQState&      udq_state,
                       int                  report_step,
                       bool                 isSubstep,
                       double               seconds_elapsed,
                       RestartValue         value,
                       const bool           write_double = false,
                       std::optional<int>   time_step = std::nullopt);

    /// Write reservoir state and summary information to disk for runs with
    /// local grid refinement.
    ///
    /// Calling this method is only meaningful after the first time step has
    /// been completed.
    ///
    /// The RestartValue contains fields which have been calculated by the
    /// simulator and are written to the restart file.  Examples of such
    /// fields would be the relative permeabilities KRO, KRW and KRG and
    /// fluxes.  The keywords which can be added here are represented with
    /// mnemonics in the RPTRST keyword.
    ///
    /// If the optional argument write_double is sent in as true the fields
    /// in the solution container will be written in double precision.  OPM
    /// can load and restart from files with double precision keywords, but
    /// this is non-standard, and other third party applications might choke
    /// on those.
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
    /// create output.  This is the number that gets incorporated into the
    /// file extension of "separate" restart and summary output files (e.g.,
    /// .X000n and .S000n).  Report_step=0 represents time zero.
    ///
    /// \param[in] isSubstep Whether or not we're being called in the middle
    /// of a report step.  We typically output summary file information only
    /// for sub-steps.
    ///
    /// \param[in] seconds_elapsed Elapsed physical (i.e., simulated) time
    /// in seconds since start of simulation.
    ///
    /// \param[in] value Collection of per-cell, per-well, per-connection,
    /// per-segment, per-group, and per-aquifer dynamic results pertaining
    /// to this time point.  One collection per grid, with \code
    /// value.front() \endcode being results for the main/global grid and
    /// each additional element being results for a separate local grid.
    ///
    /// \param[in] write_double Whether or not to output simulation results
    /// as double precision floating-point numbers.  Compatibility
    /// considerations may dictate outputting arrays as single precision
    /// ("float") only.
    ///
    /// \param[in] time_step Current time step index.  Passing something
    /// different than nullopt will generate restart file output even for
    /// time steps that are not report steps.  This is a poor-man's
    /// approximation of the BASIC=6 setting of the RPTRST keyword.
    void writeTimeStep(const Action::State&      action_state,
                       const WellTestState&      wtest_state,
                       const SummaryState&       st,
                       const UDQState&           udq_state,
                       int                       report_step,
                       bool                      isSubstep,
                       double                    seconds_elapsed,
                       std::vector<RestartValue> value,
                       const bool                write_double = false,
                       std::optional<int>        time_step = std::nullopt);


    /// Load per-cell solution data and wellstate from restart file.
    ///
    /// Name of restart file and report step from which to restart inferred
    /// from internal IOConfig and InitConfig objects.
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
    /// The extra_keys argument can be used to request additional keywords
    /// from the restart value.  The extra vectors will be stored in the
    /// 'extra' field of the return value.  These values must have been
    /// added to the restart file previosuly with the extra argument to the
    /// writeTimeStep() method.  If the bool value in the map is true the
    /// value is required, and the output layer will throw an exception if
    /// it is missing.  Otherwise, if the bool is false missing, keywords
    /// will be ignored and there will not be an empty vector in the return
    /// value.
    ///
    /// \param[in,out] action_state Run's action system state.  On input, a
    /// valid object.  On exit, populated from restart file information.
    ///
    /// \param[in,out] summary_state Run's container of summary vector
    /// values.  On input, a valid object.  On exit, populated from restart
    /// file information.  Mostly relevant to cumulative quantities such as
    /// FOPT.
    ///
    /// \param[in] solution_keys Descriptors of requisite and optional
    /// per-cell dynamic values to load from restart file.
    ///
    /// \param[in] extra_keys Descriptors of additional dynamic values to
    /// load from restart file.  Optional.
    ///
    /// \return Collection of per-cell, per-well, per-connection,
    /// per-segment, per-group, and per-aquifer dynamic results at
    /// simulation restart time.
    RestartValue loadRestart(Action::State&                 action_state,
                             SummaryState&                  summary_state,
                             const std::vector<RestartKey>& solution_keys,
                             const std::vector<RestartKey>& extra_keys = {}) const;

    /// Load per-cell solution data from restart file at specific time.
    ///
    /// Common use case is to load the initial volumes-in-place from time
    /// zero.
    ///
    /// Name of restart file inferred from internal IOConfig and InitConfig
    /// objects.
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

    /// Access internal summary vector calculation engine.
    ///
    /// Mainly provided in order to allow callers to invoke Summary::eval().
    ///
    /// \return Read-only reference to internal summary vector calculation
    /// engine.
    const out::Summary& summary() const;

    /// Access finalised summary configuration object.
    ///
    /// Provided to enable callers to learn all summary vectors needed to
    /// evaluate defining expressions of user-defined quantities.
    ///
    /// \return Read-only reference to internal summary configuration object.
    const SummaryConfig& finalSummaryConfig() const;

private:
    /// Implementation class.
    class Impl;

    /// Pointer to implementation object.
    std::unique_ptr<Impl> impl;
};

} // namespace Opm

#endif // OPM_ECLIPSE_WRITER_HPP
