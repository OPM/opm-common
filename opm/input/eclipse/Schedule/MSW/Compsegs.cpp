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

#include <opm/input/eclipse/Schedule/MSW/Compsegs.hpp>

#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/io/eclipse/rst/well.hpp>

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {

    struct Record
    {
        Record(int i_in, int j_in, int k_in,
               int branch_number_in,
               double distance_start_in,
               double distance_end_in,
               Opm::Connection::Direction dir_in,
               double center_depth_in,
               int segment_number_in,
               std::size_t seqIndex_in);

        int m_i;
        int m_j;
        int m_k;

        // The branch number on the main stem is always 1.  Subordinate
        // branches must have branch number higher than their
        // kick-off/parent branch.
        int m_branch_number;
        double m_distance_start;
        double m_distance_end;
        Opm::Connection::Direction m_dir;

        double center_depth;
        // we do not handle thermal length for the moment
        // double m_thermal_length;
        int segment_number;
        std::size_t m_seqIndex;

        void calculateCenterDepthWithSegments(const Opm::WellSegments& segment_set);
    };

    Record::Record(const int i_in, const int j_in, const int k_in,
                   const int branch_number_in,
                   const double distance_start_in,
                   const double distance_end_in,
                   const Opm::Connection::Direction dir_in,
                   const double center_depth_in,
                   const int segment_number_in,
                   const std::size_t seqIndex_in)
        : m_i              { i_in }
        , m_j              { j_in }
        , m_k              { k_in }
        , m_branch_number  { branch_number_in }
        , m_distance_start { distance_start_in }
        , m_distance_end   { distance_end_in }
        , m_dir            { dir_in }
        , center_depth     { center_depth_in }
        , segment_number   { segment_number_in }
        , m_seqIndex       { seqIndex_in }
    {}

    void Record::calculateCenterDepthWithSegments(const Opm::WellSegments& segment_set)
    {
        // the depth and distance of the segment to the well head
        const Opm::Segment& segment = segment_set
            .getFromSegmentNumber(this->segment_number);

        const double segment_depth = segment.depth();
        const double segment_distance = segment.totalLength();

        // Using top segment depth may lead to depths outside of the
        // perforated grid cell, so simply stick to grid cell center in this case
        if (segment_number == 1) {
            center_depth = -1.0;
            return;
        }

        // for other cases, interpolation between two segments is needed.
        // looking for the other segment needed for interpolation
        // by default, it uses the outlet segment to do the interpolation

        const double center_distance = (m_distance_start + m_distance_end) / 2.0;

        int interpolation_segment_number = segment.outletSegment();

        // if the perforation is further than the segment and the segment
        // has inlet segments in the same branch we use the inlet segment to
        // do the interpolation
        if (center_distance > segment_distance) {
            for (const int inlet : segment.inletSegments()) {
                const int inlet_index = segment_set.segmentNumberToIndex(inlet);
                if (segment_set[inlet_index].branchNumber() == m_branch_number) {
                    interpolation_segment_number = inlet;
                    break;
                }
            }
        }

        if (interpolation_segment_number == 0) {
            throw std::runtime_error {
                fmt::format("Failed in finding a segment to do the "
                            "interpolation with segment {}", this->segment_number)
            };
        }

        // performing the interpolation
        const Opm::Segment& interpolation_segment = segment_set
            .getFromSegmentNumber(interpolation_segment_number);

        const double interpolation_detph = interpolation_segment.depth();
        const double interpolation_distance = interpolation_segment.totalLength();

        const double depth_change_segment = segment_depth - interpolation_detph;
        const double segment_length = segment_distance - interpolation_distance;

        // Use segment depth if length of sement is 0
        if (segment_length == 0.) {
            center_depth = segment_depth;
        }
        else {
            center_depth = segment_depth +
                (center_distance - segment_distance) / segment_length * depth_change_segment;
        }
    }

    // ---------------------------------------------------------------------------

    int connectionSegmentFromMeasuredDepth(const Record&            connection,
                                           const Opm::WellSegments& segment_set)
    {
        const auto center_distance =
            (connection.m_distance_start + connection.m_distance_end) / 2.0;

        const auto segmentPos = std::min_element
            (segment_set.begin(), segment_set.end(),
             [center_distance, branch_num = connection.m_branch_number]
             (const Opm::Segment& s1, const Opm::Segment& s2)
             {
                 if (s1.branchNumber() != branch_num) { return false; }
                 if (s2.branchNumber() != branch_num) { return true;  }

                 return std::abs(center_distance - s1.totalLength())
                     <  std::abs(center_distance - s2.totalLength());
             });

        return ((segmentPos != segment_set.end()) &&
                (segmentPos->branchNumber() == connection.m_branch_number))
            ? segmentPos->segmentNumber()
            : 0;
    }

    void processCOMPSEGS__(std::string_view         well_name,
                           const Opm::WellSegments& segment_set,
                           std::vector<Record>&     compsegs)
    {
        // for the current cases we have at the moment, the distance
        // information is specified explicitly, while the depth information
        // is defaulted though, which need to be obtained from the related
        // segment
        for (auto& compseg : compsegs) {
            // need to determine the related segment number first, if not defined
            if (compseg.segment_number == 0) {
                const auto segment_number =
                    connectionSegmentFromMeasuredDepth(compseg, segment_set);

                if (segment_number == 0) {
                    throw std::runtime_error {
                        fmt::format("Connection ({},{},{}) for well {} cannot "
                                    "be allocated to a well segment based on MD",
                                    compseg.m_i + 1,
                                    compseg.m_j + 1,
                                    compseg.m_k + 1,
                                    well_name)
                    };
                }

                compseg.segment_number = segment_number;
            }

            // when depth is default or zero, we obtain the depth of the
            // connection based on the information of the related segments
            if (compseg.center_depth == 0.0) {
                compseg.calculateCenterDepthWithSegments(segment_set);
            }
        }
    }

    // ---------------------------------------------------------------------------

    std::vector<Record>
    compsegsFromCOMPSEGSKeyword(std::string_view         well_name,
                                const Opm::DeckKeyword&  compsegsKeyword,
                                const Opm::WellSegments& segments,
                                const Opm::ScheduleGrid& grid,
                                const Opm::ParseContext& parseContext,
                                Opm::ErrorGuard&         errors)
    {
        using Kw = Opm::ParserKeywords::COMPSEGS;

        auto compsegs = std::vector<Record> {};
        compsegs.reserve(compsegsKeyword.size() - 1); // -1 since first record is well name.

        const auto& location = compsegsKeyword.location();

        // The first record in the keyword only contains the well name
        // looping from the second record in the keyword
        for (std::size_t recordIndex = 1; recordIndex < compsegsKeyword.size(); ++recordIndex) {
            const auto& record = compsegsKeyword.getRecord(recordIndex);

            // following the coordinate rule for connections
            const int I = record.getItem<Kw::I>().get<int>(0) - 1;
            const int J = record.getItem<Kw::J>().get<int>(0) - 1;
            const int K = record.getItem<Kw::K>().get<int>(0) - 1;
            const int branch = record.getItem<Kw::BRANCH>().get<int>(0);

            // A defaulted or explicitly zeroed segment number will be
            // replaced later in the process.
            const int segment_number = record.getItem<Kw::SEGMENT_NUMBER>().hasValue(0)
                ? record.getItem<Kw::SEGMENT_NUMBER>().get<int>(0)
                : 0;

            if ((segment_number > 0) && (segments.segmentNumberToIndex(segment_number) < 0)) {
                // COMPSEGS references a nonexistent segment.  This segment
                // should have been entered in WELSEGS ahead of COMPSEGS.

                const auto msg = fmt::format("Segment {} on branch {} has not "
                                             "been defined in WELSEGS for well "
                                             "{}, connection ({},{},{}).",
                                             segment_number, branch, well_name,
                                             I + 1, J + 1, K + 1);

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID,
                                         msg, location, errors);

                continue;
            }

            double distance_start = 0.0;
            if (record.getItem<Kw::DISTANCE_START>().hasValue(0)) {
                distance_start = record.getItem<Kw::DISTANCE_START>().getSIDouble(0);
            }
            else if (recordIndex != 1) {
                // TODO: the end of the previous connection or range
                // 'previous' should be in term of the input order since
                // basically no specific order for the connections
                const auto msg_fmt = std::string {
                    R"(Must specify start of segment in item 5 in {keyword}
In {file} line {line})"
                };

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED,
                                         msg_fmt, location, errors);
            }

            double distance_end = -1.0;
            if (record.getItem<Kw::DISTANCE_END>().hasValue(0)) {
                distance_end = record.getItem<Kw::DISTANCE_END>().getSIDouble(0);
            }
            else {
                // TODO: the distance_start plus the thickness of the grid block
                const auto msg_fmt = std::string {
                    R"(Must specify end of segment in item 6 in {keyword}
In {file} line {line})"
                };

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED,
                                         msg_fmt, location, errors);
            }

            if (distance_end < distance_start) {
                const auto msg_fmt = fmt::format(R"(Problems with {{keyword}}
In {{file}} line {{line}}
The end of the perforation must be below the start for well {} connection ({},{},{}))",
                                                 well_name, I+1, J+1, K+1);

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID,
                                         msg_fmt, location, errors);
            }

            const auto& dirItem = record.getItem<Kw::DIRECTION>();

            if (!dirItem.hasValue(0) &&
                !record.getItem<Kw::DISTANCE_END>().hasValue(0))
            {
                const auto msg_fmt = fmt::format(R"(Problems with {{keyword}}
In {{file}} line {{line}}
The direction must be specified when DISTANCE_END is defaulted. Well: {})", well_name);

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID,
                                         msg_fmt, location, errors);
            }

            if (record.getItem<Kw::END_IJK>().hasValue(0) &&
                !dirItem.hasValue(0))
            {
                const auto msg_fmt = fmt::format(R"(Problems with {{keyword}}
In {{file}} line {{line}}
The direction must be specified when END_IJK is specified. Well: {})", well_name);

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID,
                                         msg_fmt, location, errors);
            }

            // 0.0 is also the defaulted value which is used to indicate to
            // obtain the final value through related segment
            const auto center_depth = !record.getItem<Kw::CENTER_DEPTH>().defaultApplied(0)
                ? record.getItem<Kw::CENTER_DEPTH>().getSIDouble(0)
                : 0.0;

            if (center_depth < 0.0) {
                // TODO: get the depth from COMPDAT data.
                const auto msg_fmt = fmt::format(R"(Problems with {{keyword}}
In {{file}} line {{line}}
The use of negative center depth in item 9 is not supported. Well: {})", well_name);

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED,
                                         msg_fmt, location, errors);
            }

            // Direction must be defined if the record applies to a range of
            // connections or if the DISTANCE_END item is set.  Otherwise,
            // this value is ignored and we use 'X' as a placeholder.
            const auto direction = dirItem.hasValue(0)
                ? Opm::Connection::DirectionFromString(dirItem.getTrimmedString(0))
                : Opm::Connection::Direction::X;

            if (! record.getItem<Kw::END_IJK>().hasValue(0)) { // only one compsegs
                if (grid.get_cell(I, J, K).is_active()) {
                    const std::size_t seqIndex = compsegs.size();
                    compsegs.emplace_back(I, J, K,
                                          branch,
                                          distance_start, distance_end,
                                          direction,
                                          center_depth,
                                          segment_number,
                                          seqIndex);
                }
            }
            else {
                // Input applies to a range of connections, so generate a
                // sequence of Record objects.
                const auto msg_fmt = fmt::format(R"(Problem with {{keyword}}"
In {{file}} line {{line}}
Entering COMPEGS with a range of connections is not yet supported
Well: {}, connection: ({},{},{}))", well_name, I+1, J+1 , K+1);

                parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_NOT_SUPPORTED,
                                         msg_fmt, location, errors);
            }
        }

        processCOMPSEGS__(well_name, segments, compsegs);

        return compsegs;
    }

    std::vector<Record>
    compsegsFromTrajectory(std::string_view                                     well_name,
                           const std::vector<Opm::Compsegs::TrajectorySegment>& trajectory_segments,
                           const Opm::WellSegments&                             segments)
    {
        auto compsegs = std::vector<Record> {};

        compsegs.reserve(trajectory_segments.size());

        for (const auto& trajectory_point : trajectory_segments) {
            // Defaulted values:
            const auto direction = Opm::Connection::Direction::X;
            const auto center_depth = 0.0;
            const auto segment_number = 0;
            const auto branch = 1;

            const auto seqIndex = compsegs.size();

            compsegs.emplace_back(trajectory_point.ijk[0],
                                  trajectory_point.ijk[1],
                                  trajectory_point.ijk[2],
                                  branch,
                                  trajectory_point.startMD, trajectory_point.endMD,
                                  direction,
                                  center_depth,
                                  segment_number, seqIndex);
        }

        processCOMPSEGS__(well_name, segments, compsegs);

        return compsegs;
    }

    // ---------------------------------------------------------------------------

    void identifyUnattachedConnections(std::string_view            well_name,
                                       const Opm::WellConnections& new_connection_set,
                                       const Opm::KeywordLocation& location,
                                       const Opm::ParseContext&    parseContext,
                                       Opm::ErrorGuard&            errors)
    {
        auto noSeg = std::vector<Opm::Connection>{};

        std::copy_if(new_connection_set.begin(), new_connection_set.end(),
                     std::back_inserter(noSeg),
                     [](const auto& conn) { return !conn.attachedToSegment(); });

        if (noSeg.empty()) {
            return;
        }

        const auto* pl1 = (noSeg.size() == 1) ? ""   : "s";
        const auto* pl2 = (noSeg.size() == 1) ? "is" : "are";

        auto msg_fmt = fmt::format("Well {} connection{} that {} "
                                   "not attached to a segment:",
                                   well_name, pl1, pl2);

        for (const auto& conn : noSeg) {
            msg_fmt += fmt::format("\n  * ({},{},{})",
                                   conn.getI() + 1,
                                   conn.getJ() + 1,
                                   conn.getK() + 1);
        }

        parseContext.handleError(Opm::ParseContext::SCHEDULE_COMPSEGS_INVALID,
                                 msg_fmt, location, errors);
    }

    Opm::WellConnections
    process_compsegs_records(std::string_view            well_name,
                             const std::vector<Record>&  compsegs_vector,
                             const Opm::WellConnections& input_connections,
                             const Opm::ScheduleGrid&    grid,
                             const Opm::KeywordLocation& location,
                             const Opm::ParseContext&    parseContext,
                             Opm::ErrorGuard&            errors)
    {
        auto new_connection_set = input_connections; // Intentional copy.

        for (const auto& compseg : compsegs_vector) {
            const int i = compseg.m_i;
            const int j = compseg.m_j;
            const int k = compseg.m_k;

            if (const auto& cell = grid.get_cell(i, j, k); cell.is_active()) {
                // Negative values to indicate cell depths should be used
                const double cdepth = (compseg.center_depth >= 0.0)
                    ? compseg.center_depth
                    : cell.depth;

                new_connection_set.getFromIJK(i, j, k)
                    .updateSegment(compseg.segment_number,
                                   cdepth,
                                   compseg.m_seqIndex,
                                   std::make_pair(compseg.m_distance_start,
                                                  compseg.m_distance_end));
            }
        }

        identifyUnattachedConnections(well_name, new_connection_set,
                                      location, parseContext, errors);

        return new_connection_set;
    }

    // ---------------------------------------------------------------------------

    // Duplicated from Well.cpp
    Opm::Connection::Order order_from_int(int int_value)
    {
        switch (int_value) {
        case 0: return Opm::Connection::Order::TRACK;
        case 1: return Opm::Connection::Order::DEPTH;
        case 2: return Opm::Connection::Order::INPUT;

        default:
            throw std::invalid_argument {
                fmt::format("Invalid integer value: {} encountered "
                            "when determining connection ordering", int_value)
            };
        }
    }

    Opm::WellSegments::CompPressureDrop pressure_drop_from_int(int ecl_id)
    {
        using PLM = Opm::RestartIO::Helpers::VectorItems::IWell::Value::PLossMod;

        switch (ecl_id) {
        case PLM::HFA:
            return Opm::WellSegments::CompPressureDrop::HFA;

        case PLM::HF_:
            return Opm::WellSegments::CompPressureDrop::HF_;

        case PLM::H__:
            return Opm::WellSegments::CompPressureDrop::H__;

        default:
            throw std::logic_error("Converting to pressure loss value failed");
        }
    }

} // Anonymous namespace

namespace Opm::Compsegs {

    WellConnections
    processCOMPSEGS(const DeckKeyword&     compsegs,
                    const WellConnections& input_connections,
                    const WellSegments&    input_segments,
                    const ScheduleGrid&    grid,
                    const ParseContext&    parseContext,
                    ErrorGuard&            errors)
    {
        const auto well_name = compsegs.getRecord(0)
            .getItem<ParserKeywords::COMPSEGS::WELL>()
            .getTrimmedString(0);

        const auto compsegs_vector =
            compsegsFromCOMPSEGSKeyword(well_name, compsegs, input_segments,
                                        grid, parseContext, errors);

        return process_compsegs_records(well_name, compsegs_vector, input_connections,
                                        grid, compsegs.location(), parseContext, errors);
    }

    WellConnections
    getConnectionsAndSegmentsFromTrajectory(std::string_view                      well_name,
                                            const std::vector<TrajectorySegment>& trajectory_segments,
                                            const WellSegments&                   segments,
                                            const WellConnections&                input_connections,
                                            const ScheduleGrid&                   grid,
                                            const KeywordLocation&                location,
                                            const ParseContext&                   parseContext,
                                            ErrorGuard&                           errors)
    {
        const auto compsegs_vector =
            compsegsFromTrajectory(well_name, trajectory_segments, segments);

        return process_compsegs_records(well_name, compsegs_vector, input_connections,
                                        grid, location, parseContext, errors);
    }

    std::pair<WellConnections, WellSegments>
    rstUpdate(const RestartIO::RstWell&               rst_well,
              std::vector<Connection>                 rst_connections,
              const std::unordered_map<int, Segment>& rst_segments)
    {
        for (auto& connection : rst_connections) {
            const auto segment_id = connection.segment();
            if (segment_id > 0) {
                const auto& segment = rst_segments.at(segment_id);
                connection.updateSegmentRST(segment.segmentNumber(),
                                            segment.depth());
            }
        }

        auto connections = WellConnections {
            order_from_int(rst_well.completion_ordering),
            rst_well.ij[0], rst_well.ij[1], rst_connections
        };

        auto segments_list = std::vector<Segment> {};
        segments_list.reserve(rst_segments.size());

        // The ordering of the segments in the WellSegments structure seems
        // a bit random.  In some parts of the code, the segment number
        // seems to be treated like a random integer ID, whereas in other
        // parts it seems to be treated like a running index.  Here the
        // segments in WellSegments are sorted according to the segment
        // number.  Observe that this is somewhat important because the top
        // segment--segment number 1--is treated differently from the other
        // segments.
        std::ranges::transform(rst_segments, std::back_inserter(segments_list),
                               [](const auto& segment_pair) { return segment_pair.second; });

        std::ranges::sort(segments_list,
                          [](const Segment& seg1, const Segment& seg2)
                          { return seg1.segmentNumber() < seg2.segmentNumber(); });

        auto comp_pressure_drop = pressure_drop_from_int(rst_well.msw_pressure_drop_model);

        WellSegments segments( comp_pressure_drop, segments_list);

        return std::make_pair( std::move(connections), std::move(segments));
    }

} // namespace Opm::Compsegs
