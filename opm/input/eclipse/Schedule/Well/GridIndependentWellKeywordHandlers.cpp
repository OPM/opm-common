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
#include <opm/common/utility/numeric/linearInterpolation.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/MSW/Compsegs.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>

#include "../HandlerContext.hpp"
#include "WellTrajInfo.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm
{

namespace
{

    void
    process_segments(HandlerContext&                                            handlerContext,
                     Well&                                                      well,
                     const int                                                  branch,
                     const std::vector<external::WellPathCellIntersectionInfo>& intersections,
                     const external::cvf::ref<external::RigWellPath>&           wellPathGeometry)
    {
        if (!well.isMultiSegment()) {
            return;
        }

        std::vector<Compsegs::TrajectoryConnection> trajectory_connections {};
        trajectory_connections.reserve(intersections.size());
        const auto& ecl_grid = handlerContext.grid.get_grid();
        for (const auto& intersection : intersections) {
            const double center_tvd = wellPathGeometry->interpolatedPointAlongWellPath(
                0.5 * (intersection.startMD + intersection.endMD))[2];
            trajectory_connections.push_back({intersection.startMD,
                                              intersection.endMD,
                                              center_tvd,
                                              ecl_grid->getIJK(intersection.globCellIndex)});
        }

        auto new_connections = Compsegs::getConnectionsToSegmentsFromTrajectory
            (well.name(), 
            branch,
            trajectory_connections,
            well.getSegments(),
            well.getConnections(),
            handlerContext.grid,
            handlerContext.keyword.location(),
            handlerContext.parseContext,
            handlerContext.errors);

        well.updateConnections
            (std::make_shared<WellConnections>(std::move(new_connections)), false);

        handlerContext.record_well_structure_change();
    }


    void
    handleCOMPTRAJ(HandlerContext& handlerContext)
    {
        using Kw = ParserKeywords::COMPTRAJ;

        WellTrajInfo wellTraj;
        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well = handlerContext.state().wells.get(name);

                if (!well.getConnections().empty()) {
                    const auto msg = fmt::format(R"(   {} is already connected)", name);
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }

                // cellsearchTree is calculated only once and is used to
                // calculate cell intersections of the specified perforations.
                wellTraj.intersections.clear();
                wellTraj.wellPathGeometry = new external::RigWellPath;

                auto connections = std::make_shared<WellConnections>(well.getConnections());
                connections->loadCOMPTRAJ(record, name, handlerContext.grid,
                                          handlerContext.keyword.location(),
                                          wellTraj);

                // In the case that defaults are used in WELSPECS for headI/J
                // the headI/J are calculated based on the well trajectory data
                well.updateHead(connections->getHeadI(), connections->getHeadJ());
                if (well.updateConnections(std::move(connections), handlerContext.grid)) {
                    well.updateRefDepth();
                    handlerContext.record_well_structure_change();
                }

                if (well.getConnections().empty()) {
                    const auto msg = fmt::format(R"(Problem with keyword {{keyword}}:
In {{file}} line {{line}}
Well {} has no connections to the grid. The well will remain SHUT)", name);

                    OpmLog::warning(OpmInputError::format(msg, handlerContext.keyword.location()));
                }

                process_segments(handlerContext, well,
                                 record.getItem<Kw::BRANCH_NUMBER>().get<int>(0),
                                 wellTraj.intersections,
                                 wellTraj.wellPathGeometry);

                handlerContext.state().wells.update(std::move(well));

                handlerContext.state().wellgroup_events()
                    .addEvent(name, ScheduleEvents::COMPLETION_CHANGE);

                handlerContext.comptraj_handled(name);
            }
        }

        handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
    }


    void
    handleWELTRAJ(HandlerContext& handlerContext)
    {
        using Kw = ParserKeywords::WELTRAJ;

        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well = handlerContext.state().wells.get(name);

                if (well.isMultiSegment()) {
                    const auto msg = fmt::format(
                        "Well {} is a segmented grid-independent well, but its WELSEGS keyword "
                        "must be defined after the corresponding WELTRAJ keyword. Please check "
                        "the order of the keywords in the input file.",
                        name);
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }

                auto connections = std::make_shared<WellConnections>(well.getConnections());
                connections->loadWELTRAJ(record, name, handlerContext.grid,
                                         handlerContext.keyword.location());
                for (auto const& [branch, md] : connections->getMD()) {
                    if (md.size() > 1) {
                        const bool strictly_increasing = std::adjacent_find
                            (md.begin(), md.end(), std::greater_equal<>{}) == md.end();

                        if (!strictly_increasing) {
                            const auto msg = fmt::format("Well {} measured depth column "
                                                         "is not strictly increasing", name);

                            throw OpmInputError(msg, handlerContext.keyword.location());
                        }
                    }
                }

                if (well.updateConnections(std::move(connections), handlerContext.grid)) {
                    handlerContext.state().wells.update(std::move(well));
                    handlerContext.record_well_structure_change();
                }

                handlerContext.state().wellgroup_events()
                    .addEvent(name, ScheduleEvents::COMPLETION_CHANGE);
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

void initWellPathGeometry(
    external::cvf::ref<external::RigWellPath>& wellPathGeometry,         
    const std::array<std::vector<double>, 3>& coords, const std::vector<double>& mds,
    std::optional<double> top_opt, std::optional<double> bot_opt
) {
    std::vector<external::cvf::Vec3d> points;
    std::vector<double> measured_depths;

    const double top = top_opt.value_or(mds.front());
    const double bot = bot_opt.value_or(mds.back());

    // Calulate the x,y,z coordinates of the begin and end of a perforation
    if (top < mds.front() || bot > mds.back()) {
        throw std::logic_error(fmt::format("Perforation interval [{}, {}] is outside of the well path range [{}, {}]",
            top, bot, mds.front(), mds.back()));
    }
        
    external::cvf::Vec3d p_top, p_bot;
    for (std::size_t i = 0; i < 3 ; ++i) {
        p_top[i] = linearInterpolation(mds, coords[i], top);
        p_bot[i] = linearInterpolation(mds, coords[i], bot);
    }
    points.push_back(p_top);
    measured_depths.push_back(top);

    points.reserve(coords[0].size());
    measured_depths.reserve(coords[0].size());
    for (std::size_t i = 0; i < coords[0].size(); ++i) {
        if (mds[i] > top and mds[i] < bot) {
            points.push_back(external::cvf::Vec3d(coords[0][i], coords[1][i], coords[2][i]));
            measured_depths.push_back(mds[i]);
        }
    }

    points.push_back(p_bot);
    measured_depths.push_back(bot);

    wellPathGeometry->setWellPathPoints(points);
    wellPathGeometry->setMeasuredDepths(measured_depths);
}

} // namespace Opm
