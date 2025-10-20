/*
  Copyright (c) 2020 Equinor ASA

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

#ifndef OPM_REPORT_WELL_SPECIFICATION_HPP_INCLUDED
#define OPM_REPORT_WELL_SPECIFICATION_HPP_INCLUDED

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>

namespace Opm {

    class Schedule;

} // namespace Opm

namespace Opm::PrtFile::Reports {

    /// Call-back type for centre point depth of a grid block.
    ///
    /// Input argument is a global (Cartesian) cell index, and return value
    /// is the centre point depth of that global cell.
    using BlockDepthCallback = std::function<double(std::size_t)>;

    /// Emit textual well specification report to output stream
    ///
    /// The well specification report includes
    ///
    ///    -# well/group name, well head location, BHP reference depth,
    ///       preferred phase, shut-in instruction, &c
    ///
    ///    -# reservoir connection location, CTF, KH, skin, D-factor
    ///
    ///    -# segment/branch topology, segment properties
    ///
    /// for all wells that have structurally changed.  Furthermore, we show
    /// the current group tree and the contents of any well lists that have
    /// changed since the previous report step.
    ///
    /// \param[in] changedWells Wells that have structurally changed since
    /// the previous report step.
    ///
    /// \param[in] changedWellLists Whether or not the contents of any of
    /// the run's well lists have changed since the previous report step.
    ///
    /// \param[in] reportStep Zero-based report step index at which to look
    /// up dynamic simulation objects.
    ///
    /// \param[in] schedule Run's collection of dynamic simulation objects.
    ///
    /// \param[in] blockDepth Call-back function for retrieving centre point
    /// depths of active cells.
    ///
    /// \param[in,out] os Stream to which to emit well specification report.
    void wellSpecification(const std::vector<std::string>& changedWells,
                           const bool                      changedWellLists,
                           const std::size_t               reportStep,
                           const Schedule&                 schedule,
                           BlockDepthCallback              blockDepth,
                           std::ostream&                   os);

} // namespace Opm::PrtFile::Reports

#endif // OPM_REPORT_WELL_SPECIFICATION_HPP_INCLUDED
