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

#ifndef OPM_WRITE_RESTART_HELPERS_HPP
#define OPM_WRITE_RESTART_HELPERS_HPP

#include <cstddef>
#include <vector>

// Forward declarations

namespace Opm {

    class Runspec;
    class EclipseGrid;
    class EclipseState;
    class Schedule;
    class ScheduleState;
    class Well;
    class UnitSystem;
    class UDQActive;
    class Actdims;

} // namespace Opm

namespace Opm::Action {
    class Actions;
} // namespace Opm::Action

namespace Opm::RestartIO::Helpers {

    std::vector<double>
    createDoubHead(const EclipseState& es,
                   const Schedule&     sched,
                   const std::size_t   sim_step,
                   const std::size_t   report_step,
                   const double        simTime,
                   const double        nextTimeStep);

    std::vector<int>
    createInteHead(const EclipseState& es,
                   const EclipseGrid&  grid,
                   const Schedule&     sched,
                   const double        simTime,
                   const int           num_solver_steps,
                   const int           report_step,
                   const int           lookup_step);

    std::vector<int>
    createLgrHeadi(const EclipseState& es,
                   const int           lgr_index);

    std::vector<bool>
    createLgrHeadq(const EclipseState& es);

    std::vector<double>
    createLgrHeadd();

    /// Static flags, mostly for INIT file purposes.
    ///
    /// \param[in] es Static properties and run settings.
    ///
    /// \return Output file's logical flag settings.
    std::vector<bool>
    createLogiHead(const EclipseState& es);

    /// Dynamic flags, mostly for restart file purposes.
    ///
    /// \param[in] es Static properties and run settings.
    ///
    /// \param[in] sched Dynamic simulation objects and settings.
    ///
    /// \return Output file's logical flag settings.
    std::vector<bool>
    createLogiHead(const EclipseState& es, const ScheduleState& sched);

    /// Number of elements per action in IACT.
    constexpr std::size_t entriesPerIACT() noexcept { return std::size_t{9}; }

    /// Number of elements per action in SACT.
    constexpr std::size_t entriesPerSACT() noexcept { return std::size_t{5}; }

    /// Number of elements per action in ZACT.
    constexpr std::size_t entriesPerZACT() noexcept { return std::size_t{4}; }

    /// Compute number of elements per line in ZLACT.
    ///
    /// \param[in] actdims Action dimensioning information.
    ///
    /// \return Number of elements per line in ZLACT.
    std::size_t entriesPerLine(const Actdims& actdims);

    /// Compute number of elements per action in ZLACT.
    ///
    /// \param[in] actdims Action dimensioning information.
    ///
    /// \param[in] acts Actions information.
    ///
    /// \return Number of elements per action in ZLACT.
    std::size_t entriesPerZLACT(const Actdims& actdims,
                                const Action::Actions& acts);

    /// Compute number of elements per action in IACN.
    ///
    /// \param[in] actdims Action dimensioning information.
    ///
    /// \return Number of elements per action in IACN.
    std::size_t entriesPerIACN(const Actdims& actdims);

    /// Compute number of elements per action in SACN.
    ///
    /// \param[in] actdims Action dimensioning information.
    ///
    /// \return Number of elements per action in SACN.
    std::size_t entriesPerSACN(const Actdims& actdims);

    /// Compute number of elements per action in ZACN.
    ///
    /// \param[in] actdims Action dimensioning information.
    ///
    /// \return Number of elements per action in ZACN.
    std::size_t entriesPerZACN(const Actdims& actdims);

    [[deprecated("Restart file action dimensions represented as a linear array are deprecated. Use entriesPer*() instead.")]]
    std::vector<int>
    createActionRSTDims(const Schedule&     sched,
                        const std::size_t   simStep);

} // namespace Opm::RestartIO::Helpers

#endif // OPM_WRITE_RESTART_HELPERS_HPP
