/*
  Copyright 2015 SINTEF ICT, Applied Mathematics.

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

#ifndef COMPSEGS_HPP_
#define COMPSEGS_HPP_

#include <array>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Opm {

    class Connection;
    class DeckKeyword;
    class ErrorGuard;
    class KeywordLocation;
    class ParseContext;
    class ScheduleGrid;
    class Segment;
    class WellConnections;
    class WellSegments;

} // namespace Opm

namespace Opm::RestartIO {

    struct RstWell;

} // namespace Opm::RestartIO

namespace Opm::Compsegs {

    /// Allocate well/reservoir connections to well segments.
    ///
    /// The COMPSEGS keyword defines a link between reservoir connections
    /// and well segments.  This link is bidirectional as some segment
    /// information is embedded in the connection data, and some connection
    /// information is stored in the segment data.  This function completes
    /// the connection data for a single well by incorporating connection
    /// related information from the segment structure.
    ///
    /// \param[in] compsegs Raw input segment-to-connection links from the
    /// COMPSEGS keyword.  Pertains to a single well referenced in the first
    /// record.
    ///
    /// \param[in] input_connections Preliminary well connection objects
    /// from COMPDAT.
    ///
    /// \param[in] input_segments All well segments defined for the well
    /// referenced in the first record of \p compsegs.
    ///
    /// \param[in] grid Run's active cells.
    ///
    /// \param[in] parseContext Error handling behaviour.
    ///
    /// \param[in,out] errors Run's failure condition container.  On exit,
    /// also contains any failure conditions encountered while processing \p
    /// compsegs.
    ///
    /// \return Input connections \p input_connections amended to include
    /// well segment allocation.
    WellConnections
    processCOMPSEGS(const DeckKeyword&     compsegs,
                    const WellConnections& input_connections,
                    const WellSegments&    input_segments,
                    const ScheduleGrid&    grid,
                    const ParseContext&    parseContext,
                    ErrorGuard&            errors);

    /// Single well segment of a grid-independent well (WELTRAJ/COMPTRAJ
    /// keywords).
    struct TrajectorySegment
    {
        /// Measured depth along well bore at the start of the segment.
        double startMD{};

        /// Measured depth along well bore at the end of the segment.
        double endMD{};

        /// Cartesian IJK tuple of the cell intersected by this segment.
        std::array<int, 3> ijk{};
    };

    /// Allocate well/reservoir connections to well segements.
    ///
    /// \param[in] well_name Named well for which to allocate connections to
    /// well segments.
    ///
    /// \param[in] trajectory_segments Cells intersected by the well bore
    /// trajectory.
    ///
    /// \param[in] segments All well segments defined for well \p well_name,
    /// from the WELSEGS keyword.
    ///
    /// \param[in] input_connections Preliminary well connection objects
    /// from COMPDAT.
    ///
    /// \param[in] grid Run's active cells.
    ///
    /// \param[in] location of COMPTRAJ keyword that triggered this
    /// connection allocation.
    ///
    /// \param[in] parseContext Error handling behaviour.
    ///
    /// \param[in,out] errors Run's failure condition container.  On exit,
    /// also contains any failure conditions encountered while processing \p
    /// compsegs.
    ///
    /// \return Input connections \p input_connections amended to include
    /// well segment allocation.
    WellConnections
    getConnectionsAndSegmentsFromTrajectory(std::string_view                      well_name,
                                            const std::vector<TrajectorySegment>& trajectory_segments,
                                            const WellSegments&                   segments,
                                            const WellConnections&                input_connections,
                                            const ScheduleGrid&                   grid,
                                            const KeywordLocation&                location,
                                            const ParseContext&                   parseContext,
                                            ErrorGuard&                           errors);

    /// Form connection and segment structures for a single well from
    /// restart file information.
    ///
    /// \param[in] rst_well Restart file's notion of a single well.
    ///
    /// \param[in] input_connections Preliminary well connection objects
    /// from restart file.
    ///
    /// \param[in] input_segments Preliminary well segment objects from
    /// restart file.
    ///
    /// \return Fully formed and linked well connections and well segments
    /// from restart file information.
    std::pair<WellConnections, WellSegments>
    rstUpdate(const RestartIO::RstWell&               rst_well,
              std::vector<Connection>                 input_connections,
              const std::unordered_map<int, Segment>& input_segments);

} // namespace Opm::Compsegs

#endif // COMPSEGS_HPP_
