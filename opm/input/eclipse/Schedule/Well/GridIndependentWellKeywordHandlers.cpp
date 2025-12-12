/*
  Copyright 2025 Equinor ASA.

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

#include "GridIndependentWellKeywordHandlers.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/MSW/Compsegs.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>


#include "../HandlerContext.hpp"

#include <fmt/format.h>

#include <unordered_set>

namespace Opm
{

namespace
{


    auto
    get_segment_geometries(HandlerContext& handlerContext,
                           const std::vector<external::WellPathCellIntersectionInfo>& intersections,
                           const external::cvf::ref<external::RigWellPath>& wellPathGeometry)
    {
        std::vector<Compsegs::TrajectorySegment> trajectory_segments;
        std::vector<std::pair<double, double>> cell_md_and_tvd;
        trajectory_segments.reserve(intersections.size());
        cell_md_and_tvd.reserve(intersections.size());
        const auto& ecl_grid = handlerContext.grid.get_grid();
        for (const auto& intersection : intersections) {
            const auto ijk = ecl_grid->getIJK(intersection.globCellIndex);
            trajectory_segments.push_back({intersection.startMD, intersection.endMD, ijk});
            double cell_md = 0.5 * (intersection.startMD + intersection.endMD);
            double cell_tvd = wellPathGeometry->interpolatedPointAlongWellPath(cell_md)[2];
            cell_md_and_tvd.emplace_back(cell_md, cell_tvd);
        }
        return std::pair {trajectory_segments, cell_md_and_tvd};
    }


    void
    process_segments(HandlerContext& handlerContext,
                     Well& well,
                     const std::vector<external::WellPathCellIntersectionInfo>& intersections,
                     const external::cvf::ref<external::RigWellPath>& wellPathGeometry,
                     double diameter)
    {
        if (well.isMultiSegment()) {
            // For now, no segments may be defined via WELSEGS, except for the top:
            if (well.getSegments().size() > 1) {
                const auto msg
                    = fmt::format("   {} already defines segments with the WELSEGS keyword", well.name());
                throw OpmInputError(msg, handlerContext.keyword.location());
            }

            auto [trajectory_segments, cell_md_and_tvd]
                = get_segment_geometries(handlerContext, intersections, wellPathGeometry);
            well.addWellSegmentsFromLengthsAndDepths(
                cell_md_and_tvd, diameter, handlerContext.keyword.location());
            auto new_connections = Compsegs::getConnectionsAndSegmentsFromTrajectory(
                trajectory_segments, well.getSegments(), well.getConnections(), handlerContext.grid);
            well.updateConnections(
                std::make_shared<WellConnections>(std::move(new_connections)), false);
            handlerContext.record_well_structure_change();
        }
    }


    void
    handleCOMPTRAJ(HandlerContext& handlerContext)
    {
        external::cvf::ref<external::cvf::BoundingBoxTree> cellSearchTree {};

        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well = handlerContext.state().wells.get(name);
                if (!well.getConnections().empty()) {
                    const auto msg = fmt::format(R"(   {} is already connected)", name);
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }

                // cellsearchTree is calculated only once and is used to
                // calculate cell intersections of the specified perforations.
                auto connections = std::make_shared<WellConnections>(well.getConnections());
                external::cvf::ref<external::RigWellPath> wellPathGeometry{new external::RigWellPath};
                const auto intersections = connections->loadCOMPTRAJ(record,
                                                                     handlerContext.grid,
                                                                     name,
                                                                     handlerContext.keyword.location(),
                                                                     cellSearchTree,
                                                                     wellPathGeometry);

                // In the case that defaults are used in WELSPECS for headI/J
                // the headI/J are calculated based on the well trajectory data
                well.updateHead(connections->getHeadI(), connections->getHeadJ());
                if (well.updateConnections(connections, handlerContext.grid)) {
                    well.updateRefDepth();
                    handlerContext.record_well_structure_change();
                }

                if (well.getConnections().empty()) {
                    const auto msg = fmt::format(R"(Problem with keyword {{keyword}}:
In {{file}} line {{line}}
Well {} has no connections to the grid. The well will remain SHUT)", name);
                    OpmLog::warning(OpmInputError::format(msg, handlerContext.keyword.location()));
                }

                const double diameter = record.getItem("DIAMETER").getSIDouble(0);
                process_segments(handlerContext, well, intersections, wellPathGeometry, diameter);

                handlerContext.state().wells.update(well);
                handlerContext.state().wellgroup_events().addEvent(name, ScheduleEvents::COMPLETION_CHANGE);
                handlerContext.comptraj_handled(name);
            }
        }

        handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
    }


    void
    handleWELTRAJ(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well = handlerContext.state().wells.get(name);
                auto connections
                    = std::make_shared<WellConnections>(WellConnections(well.getConnections()));
                connections->loadWELTRAJ(
                    record, handlerContext.grid, name, handlerContext.keyword.location());
                const auto& md = connections->getMD();
                if (md.size() > 1) {
                    const bool strictly_increasing = std::adjacent_find(
                        md.begin(), md.end(), std::greater_equal<double>()) == md.end();
                    if (!strictly_increasing) {
                        const auto msg = fmt::format(
                            "Well {} measured depth column is not strictly increasing", name);
                        throw OpmInputError(msg, handlerContext.keyword.location());
                    }
                }
                if (well.updateConnections(connections, handlerContext.grid)) {
                    handlerContext.state().wells.update(well);
                    handlerContext.record_well_structure_change();
                }
                handlerContext.state().wellgroup_events().addEvent(
                    name, ScheduleEvents::COMPLETION_CHANGE);
            }
        }
        handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
    }


} // Anonymous namespace

std::vector<std::pair<std::string, KeywordHandlers::handler_function>>
getGridIndependentWellKeywordHandlers()
{
    return {
        {"COMPTRAJ", &handleCOMPTRAJ},
        {"WELTRAJ", &handleWELTRAJ},
    };
}

} // namespace Opm
