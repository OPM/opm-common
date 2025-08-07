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


    void add_segments(HandlerContext& handlerContext,
                      const std::string& name,
                      const std::vector<std::pair<double, double>>& cell_md_and_tvd,
                      double diameter)
    {
        auto well = handlerContext.state().wells.get(name);
        well.addWellSegmentsFromLengthsAndDepths(
            cell_md_and_tvd, diameter, handlerContext.keyword.location());
        handlerContext.state().wells.update(std::move(well));
        handlerContext.record_well_structure_change();
    }


    void
    process_segment_connections(HandlerContext& handlerContext,
                                const std::string& name,
                                const std::vector<Compsegs::TrajectorySegment>& trajectory_segments)
    {
        auto well = handlerContext.state().wells.get(name);
        auto new_connections = Compsegs::getConnectionsAndSegmentsFromTrajectory(
            trajectory_segments, well.getSegments(), well.getConnections(), handlerContext.grid);
        well.updateConnections(std::make_shared<WellConnections>(std::move(new_connections)),
                               false);
        handlerContext.state().wells.update(std::move(well));
        handlerContext.record_well_structure_change();
    }


    void process_segments(HandlerContext& handlerContext,
                          const std::string& name,
                          const std::vector<external::WellPathCellIntersectionInfo>& intersections,
                          const external::cvf::ref<external::RigWellPath>& wellPathGeometry,
                          double diameter)
    {
        const auto& well = handlerContext.state().wells.get(name);
        if (well.isMultiSegment()) {
            // For now, no segments may be defined via WELSEGS, except for the top:
            if (well.getSegments().size() > 1) {
                const auto msg
                    = fmt::format("   {} already defines segments with the WELSEGS keyword", name);
                throw Opm::OpmInputError(msg, handlerContext.keyword.location());
            }

            auto [trajectory_segments, cell_md_and_tvd]
                = get_segment_geometries(handlerContext, intersections, wellPathGeometry);
            add_segments(handlerContext, name, cell_md_and_tvd, diameter);
            process_segment_connections(handlerContext, name, trajectory_segments);
        }
    }


    void handleCOMPTRAJ(HandlerContext& handlerContext)
    {
        // Keyword WELTRAJ must be read first
        std::unordered_set<std::string> wells;
        external::cvf::ref<external::cvf::BoundingBoxTree> cellSearchTree {};

        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well2 = handlerContext.state().wells.get(name);
                auto connections = std::make_shared<WellConnections>(well2.getConnections());
                external::cvf::ref<external::RigWellPath> wellPathGeometry {
                    new external::RigWellPath};

                // cellsearchTree is calculated only once and is used to
                // calculated cell intersections of the perforations
                // specified in COMPTRAJ
                const auto intersections
                    = connections->loadCOMPTRAJ(record,
                                                handlerContext.grid,
                                                name,
                                                handlerContext.keyword.location(),
                                                cellSearchTree,
                                                wellPathGeometry);

                // In the case that defaults are used in WELSPECS for
                // headI/J the headI/J are calculated based on the well
                // trajectory data
                well2.updateHead(connections->getHeadI(), connections->getHeadJ());
                if (well2.updateConnections(connections, handlerContext.grid)) {
                    handlerContext.state().wells.update(well2);
                    wells.insert(name);
                }

                if (connections->empty() && well2.getConnections().empty()) {
                    const auto& location = handlerContext.keyword.location();
                    const auto msg = fmt::format(R"(Problem with COMPTRAJ/{}
In {} line {}
Well {} is not connected to grid - will remain SHUT)",
                                                 name,
                                                 location.filename,
                                                 location.lineno,
                                                 name);
                    OpmLog::warning(msg);
                }

                double diameter = record.getItem("DIAMETER").getSIDouble(0);
                process_segments(handlerContext, name, intersections, wellPathGeometry, diameter);

                handlerContext.state().wellgroup_events().addEvent(
                    name, ScheduleEvents::COMPLETION_CHANGE);
            }
        }

        handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);

        // In the case the wells reference depth has been defaulted in the
        // WELSPECS keyword we need to force a calculation of the wells
        // reference depth exactly when the COMPTRAJ keyword has been
        // completely processed.
        for (const auto& wname : wells) {
            auto well = handlerContext.state().wells.get(wname);
            well.updateRefDepth();

            handlerContext.state().wells.update(std::move(well));
            handlerContext.comptraj_handled(wname);
        }

        if (!wells.empty()) {
            handlerContext.record_well_structure_change();
        }
    }


    void handleWELTRAJ(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well2 = handlerContext.state().wells.get(name);
                auto connections
                    = std::make_shared<WellConnections>(WellConnections(well2.getConnections()));
                connections->loadWELTRAJ(
                    record, handlerContext.grid, name, handlerContext.keyword.location());
                if (well2.updateConnections(connections, handlerContext.grid)) {
                    handlerContext.state().wells.update(well2);
                    handlerContext.record_well_structure_change();
                }
                handlerContext.state().wellgroup_events().addEvent(
                    name, ScheduleEvents::COMPLETION_CHANGE);
                const auto& md = connections->getMD();
                if (!std::is_sorted(std::begin(md), std::end(md))) {
                    auto msg = fmt::format(
                        "Well {} measured depth column is not strictly increasing", name);
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }
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
