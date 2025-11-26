/*
  Copyright 2020 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/MSW/Compsegs.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>


#include "../HandlerContext.hpp"

#include <fmt/format.h>

#include <unordered_set>

namespace Opm {

namespace {


auto get_segment_geometries(HandlerContext& handlerContext,
                            const std::vector<external::WellPathCellIntersectionInfo>& intersections, 
                            const external::cvf::ref<external::RigWellPath>& wellPathGeometry)
{
    std::vector<std::tuple<double, double, std::array<int, 3>>> intersections_md_and_ijk;
    std::vector<std::pair<double, double>> cell_md_and_tvd;
    intersections_md_and_ijk.reserve(intersections.size());
    cell_md_and_tvd.reserve(intersections.size());
    const auto& ecl_grid = handlerContext.grid.get_grid();
    for (const auto& intersection: intersections) {
        const auto ijk = ecl_grid->getIJK(intersection.globCellIndex);
        intersections_md_and_ijk.emplace_back(intersection.startMD, intersection.endMD, ijk);
        double cell_md = 0.5 * (intersection.startMD + intersection.endMD);
        double cell_tvd = wellPathGeometry->interpolatedPointAlongWellPath(cell_md)[2];
        cell_md_and_tvd.emplace_back(cell_md, cell_tvd);
    }
    return std::pair{intersections_md_and_ijk, cell_md_and_tvd};
}


void add_segments(HandlerContext& handlerContext,
                  const std::string& name,
                  const std::vector<std::pair<double, double>>& cell_md_and_tvd,
                  double diameter)
{
    auto well = handlerContext.state().wells.get(name);
    well.addWellSegmentsFromLengthsAndDepths(cell_md_and_tvd, diameter, handlerContext.keyword.location());
    handlerContext.state().wells.update(std::move(well));
    handlerContext.record_well_structure_change();
}


void process_connections(HandlerContext& handlerContext,
                         const std::string& name,
                         const std::vector<std::tuple<double, double, std::array<int, 3>>> intersections_md_and_ijk)
{
    auto well = handlerContext.state().wells.get(name);
    auto [new_connections, new_segments] = Compsegs::getConnectionsAndSegmentsFromTrajectory(
        intersections_md_and_ijk, well.getSegments(), well.getConnections(), well.getSegments(), handlerContext.grid
    );
    well.updateConnections(std::make_shared<WellConnections>(std::move(new_connections)), false);
    well.updateSegments(std::make_shared<WellSegments>(std::move(new_segments)));
    handlerContext.state().wells.update(std::move(well));
    handlerContext.record_well_structure_change();
}


void process_segments(HandlerContext& handlerContext, const std::string& name,
                      const std::vector<external::WellPathCellIntersectionInfo>& intersections, 
                      const external::cvf::ref<external::RigWellPath>& wellPathGeometry,
                      double diameter)
{
    const auto& well = handlerContext.state().wells.get(name);
    if (well.isMultiSegment()) {
        // COMPTRAJ is in absolute units, INC in WELSEGS is not supported:
        if (well.getSegments().getLengthDepthType() == WellSegments::LengthDepth::INC) {
            const auto msg = fmt::format("   WELSEGS/{} defines segments as incremental (INC): only ABS allowed", name);
            throw Opm::OpmInputError(msg, handlerContext.keyword.location());
        }
        // For now, no segments may be defined via WELSEGS, except for the top:
        if (well.getSegments().size() > 1) {
            const auto msg = fmt::format("   {} already defines segments with the WELSEGS keyword", name);
            throw Opm::OpmInputError(msg, handlerContext.keyword.location());
        }

        auto [intersections_md_and_ijk, cell_md_and_tvd] = get_segment_geometries(handlerContext, intersections, wellPathGeometry);
        add_segments(handlerContext, name, cell_md_and_tvd, diameter);
        process_connections(handlerContext, name, intersections_md_and_ijk);
    }

}


void handleCOMPTRAJ(HandlerContext& handlerContext)
{
    // Keyword WELTRAJ must be read first
    std::unordered_set<std::string> wells;
    external::cvf::ref<external::cvf::BoundingBoxTree> cellSearchTree{};

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& name : wellnames) {
            auto well2 = handlerContext.state().wells.get(name);
            auto connections = std::make_shared<WellConnections>(well2.getConnections());
            external::cvf::ref<external::RigWellPath> wellPathGeometry { new external::RigWellPath };

            // cellsearchTree is calculated only once and is used to
            // calculated cell intersections of the perforations
            // specified in COMPTRAJ
            auto intersections = connections->loadCOMPTRAJ(
                record, handlerContext.grid, name, handlerContext.keyword.location(), cellSearchTree, wellPathGeometry
            );

            // In the case that defaults are used in WELSPECS for
            // headI/J the headI/J are calculated based on the well
            // trajectory data
            well2.updateHead(connections->getHeadI(), connections->getHeadJ());
            if (well2.updateConnections(connections, handlerContext.grid)) {
                handlerContext.state().wells.update( well2 );
                wells.insert( name );
            }

            if (connections->empty() && well2.getConnections().empty()) {
                const auto& location = handlerContext.keyword.location();
                const auto msg = fmt::format(R"(Problem with COMPTRAJ/{}
In {} line {}
Well {} is not connected to grid - will remain SHUT)",
                                             name, location.filename,
                                             location.lineno, name);
                OpmLog::warning(msg);
            }
            
            double diameter = record.getItem("DIAMETER").getSIDouble(0);
            process_segments(handlerContext, name, intersections, wellPathGeometry, diameter);

            handlerContext.state().wellgroup_events().addEvent(name, ScheduleEvents::COMPLETION_CHANGE);
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

    if (! wells.empty()) {
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
            auto connections = std::make_shared<WellConnections>(WellConnections(well2.getConnections()));
            connections->loadWELTRAJ(record, handlerContext.grid, name, handlerContext.keyword.location());
            if (well2.updateConnections(connections, handlerContext.grid)) {
                handlerContext.state().wells.update( well2 );
                handlerContext.record_well_structure_change();
            }
            handlerContext.state().wellgroup_events().addEvent( name, ScheduleEvents::COMPLETION_CHANGE);
            const auto& md = connections->getMD();
            if (!std::is_sorted(std::begin(md), std::end(md))) {
                auto msg = fmt::format("Well {} measured depth column is not strictly increasing", name);
                throw OpmInputError(msg, handlerContext.keyword.location());
            }
        }
    }
    handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
}


} // Anonymous namespace

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getGridIndependentWellKeywordHandlers()
{
    return {
        { "COMPTRAJ", &handleCOMPTRAJ },
        { "WELTRAJ" , &handleWELTRAJ  },
    };
}

} // namespace Opm
