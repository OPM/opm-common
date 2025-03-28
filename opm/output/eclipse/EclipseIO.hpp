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

namespace Opm { namespace Action {
    class State;
}} // namespace Opm::Action

namespace Opm { namespace out {
    class Summary;
}} // namespace Opm::out

namespace Opm {
/// \brief A class to write reservoir and well states of a blackoil
///        simulation to disk.
class EclipseIO
{
public:
    /// \brief Sets common attributes required to write compatible result
    /// files.
    EclipseIO(const EclipseState&  es,
              EclipseGrid          grid,
              const Schedule&      schedule,
              const SummaryConfig& summary_config,
              const std::string&   basename = "",
              const bool writeEsmry = false);

    EclipseIO(const EclipseIO&) = delete;

    ~EclipseIO();

    /// \brief Output static properties in EGRID and INIT file.
    ///
    /// Write the static ECLIPSE data (grid, PVT curves, etc) to disk, and
    /// set up additional initial properties.  For the current code the
    /// algorithm for selecting which vectors are written to the INIT file
    /// is as follows:
    ///
    ///    1. 3D properties which can be simply calculated in the output
    ///       layer are unconditionally written to disk, that currently
    ///       includes the DX, DY, DZ and DEPTH keyword.
    ///
    ///    2. All integer propeerties from the deck are written
    ///       unconditionally, that typically includes the MULTNUM, PVTNUM,
    ///       SATNUM and so on.  Observe that the keywords PVTNUM, SATNUM,
    ///       EQLNUM and FIPNUM are automatically created in the output
    ///       layer, so they will be on disk even if they are not explicitly
    ///       included in the deck.
    ///
    ///    3. The PORV keyword will *always* be present in the INIT file,
    ///       and that keyword will have nx*ny*nz elements; all other 3D
    ///       properties will only have nactive elements.
    ///
    ///    4. For floating point 3D keywords from the deck - like PORO and
    ///       PERMX there is a *hardcoded* list in the writeINIFile()
    ///       implementation of which keywords to output - if they are
    ///       available.
    ///
    ///    5. The container simProps contains additional 3D floating point
    ///       properties which have been calculated by the simulator, this
    ///       typically includes the transmissibilities TRANX, TRANY and
    ///       TRANZ but could in principle be anye floating point
    ///       property.
    ///
    ///       If you want the FOE keyword in the summary output the
    ///       simProps container must contain the initial OIP field.
    ///
    /// In addition:
    ///   - The NNC argument is distributed between the EGRID and INIT files.
    void writeInitial(data::Solution simProps = data::Solution(),
                      std::map<std::string, std::vector<int>> int_data = {},
                      const std::vector<NNCdata>& nnc = {});

    /// \brief Overwrite the initial OIP values.
    ///
    /// These are also written when we call writeInitial if the simProps
    /// contains them.  If not, these are assumed to zero and the simulator
    /// can update them with this methods.
    ///
    /// \param[in] simProps The properties containing at least OIP.
    void overwriteInitialOIP(const data::Solution& simProps);

    /// \brief Write a reservoir state and summary information to disk.
    ///
    /// Calling this method is only meaningful after the first time step has
    /// been completed.
    ///
    /// The RestartValue contains fields which have been calculated by the
    /// simulator and are written to the restart file.  Examples of such
    /// fields would be the relative permeabilities KRO, KRW and KRG and
    /// fluxes.  The keywords which can be added here are represented with
    /// mnenonics in the RPTRST keyword.
    ///
    /// If the optional argument write_double is sent in as true the fields
    /// in the solution container will be written in double precision.  OPM
    /// can load and restart from files with double precision keywords, but
    /// this is non-standard, and other third party applications might choke
    /// on those.
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

    /// Will load solution data and wellstate from the restart file.  This
    /// method will consult the IOConfig object to get filename and report
    /// step to restart from.
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
    /// The function will consult the InitConfig object in the EclipseState
    /// object to determine which file and report step to load.
    ///
    /// The extra_keys argument can be used to request additional kewyords
    /// from the restart value.  The extra vectors will be stored in the
    /// 'extra' field of the return value.  These values must have been
    /// added to the restart file previosuly with the extra argument to the
    /// writeTimeStep() method.  If the bool value in the map is true the
    /// value is required, and the output layer will throw an exception if
    /// it is missing.  Otherwise, if the bool is false missing, keywords
    /// will be ignored and there will not be an empty vector in the return
    /// value.
    RestartValue loadRestart(Action::State&                 action_state,
                             SummaryState&                  summary_state,
                             const std::vector<RestartKey>& solution_keys,
                             const std::vector<RestartKey>& extra_keys = {}) const;

    /// Will load solution data from the restart file.  This
    /// method will consult the IOConfig object to get filename.
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
    /// The function will consult the InitConfig object in the EclipseState
    /// object to determine which file and report step to load.
    ///
    data::Solution loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                       const int                      report_step) const;

    const out::Summary& summary() const;
    const SummaryConfig& finalSummaryConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Opm


#endif // OPM_ECLIPSE_WRITER_HPP
