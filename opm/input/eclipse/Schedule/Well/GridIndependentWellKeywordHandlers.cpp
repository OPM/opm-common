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
#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>

#include "../HandlerContext.hpp"
#include "WellTrajInfo.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm
{

namespace
{

    /// Amend \p msg with a hint about restart support when the run was
    /// resumed from a restart file.
    ///
    /// A restart-restored connection carries no branch information, so it is
    /// indistinguishable from a COMPDAT connection and continuing the well
    /// with COMPTRAJ/WELTRAJ trips the mixing guard. Say so, rather than
    /// leaving the user with a rule they appear not to have broken.
    std::string appendRestartHint(std::string msg, const HandlerContext& handlerContext)
    {
        const auto restart_step = handlerContext.static_schedule().rst_info.report_step;

        if (restart_step != 0) {
            msg += fmt::format(" This may be caused by continuing this well after a restart "
                               "(resumed at report step {}) - restarting is not supported "
                               "for trajectory-based (COMPTRAJ/WELTRAJ) wells.", restart_step);
        }

        return msg;
    }

    using PerforationInterval = Connection::PerforationInterval;

    /// Reject a COMPTRAJ perforation interval overlapping another interval
    /// of the same branch of a well.
    ///
    /// The intervals of a branch given in one keyword must not overlap,
    /// although they may touch.  An interval given by an earlier keyword may
    /// be re-specified, which replaces it, but only as a whole, so a later
    /// interval must either match it exactly or not overlap it at all.
    ///
    /// \param[in] interval Perforation interval of the current record.
    ///
    /// \param[in] keyword_intervals Perforation intervals of the well given
    /// by the preceding records of the current keyword.
    ///
    /// \param[in] connections Current connections of the well, recording
    /// the perforation intervals perforating their cells.
    ///
    /// \param[in] well_name Name of the well.
    ///
    /// \param[in] handlerContext Context of the current keyword.
    void checkPerforationInterval(const PerforationInterval&              interval,
                                  const std::vector<PerforationInterval>& keyword_intervals,
                                  const WellConnections&                  connections,
                                  const std::string&                      well_name,
                                  const HandlerContext&                   handlerContext)
    {
        const auto& units = handlerContext.static_schedule().m_unit_system;
        const auto toDeckLength = [&units](const double si)
        { return units.from_si(UnitSystem::measure::length, si); };

        const auto overlaps = [&interval](const PerforationInterval& other)
        {
            return (other.branch == interval.branch)
                && (std::max(other.top, interval.top) < std::min(other.bottom, interval.bottom));
        };

        for (const auto& other : keyword_intervals) {
            if (overlaps(other)) {
                const auto msg = fmt::format(
                    "The perforation interval from {:g} to {:g} of branch {} of well {} "
                    "overlaps the interval from {:g} to {:g} given earlier in the same "
                    "keyword. The perforation intervals of a branch must not overlap.",
                    toDeckLength(interval.top), toDeckLength(interval.bottom), interval.branch,
                    well_name, toDeckLength(other.top), toDeckLength(other.bottom));

                throw OpmInputError(msg, handlerContext.keyword.location());
            }
        }

        // The intervals of the current keyword do not overlap, so any
        // overlapping interval found here was given by an earlier keyword.
        for (const auto& connection : connections) {
            for (const auto& contribution : connection.intervalContributions()) {
                const auto& other = contribution.interval;

                if ((other != interval) && overlaps(other)) {
                    const auto msg = fmt::format(
                        "The perforation interval from {:g} to {:g} of branch {} of well {} "
                        "overlaps the interval from {:g} to {:g} given by an earlier COMPTRAJ "
                        "keyword. An earlier interval can only be re-specified as a whole, with "
                        "the same top and bottom, which replaces it.",
                        toDeckLength(interval.top), toDeckLength(interval.bottom), interval.branch,
                        well_name, toDeckLength(other.top), toDeckLength(other.bottom));

                    throw OpmInputError(msg, handlerContext.keyword.location());
                }
            }
        }
    }


    void
    handleCOMPTRAJ(HandlerContext& handlerContext)
    {
        using Kw = ParserKeywords::COMPTRAJ;

        // Wells in the order in which the keyword first names them.
        auto comptraj_wells = std::vector<std::string>{};

        // Perforation intervals given by the keyword, per well.
        auto keyword_intervals = std::map<std::string, std::vector<PerforationInterval>>{};

        WellTrajInfo wellTraj;
        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well = handlerContext.state().wells.get(name);

                const auto& existing_connections = well.getConnections();
                if (!existing_connections.empty() && !existing_connections[0].fromTrajectory()) {
                    const auto msg = appendRestartHint(
                        fmt::format("Well {} already has COMPDAT-defined connections and cannot "
                                    "also be defined using COMPTRAJ. A well must use either "
                                    "COMPDAT or COMPTRAJ, never both.", name),
                        handlerContext);

                    throw OpmInputError(msg, handlerContext.keyword.location());
                }

                const auto interval = PerforationInterval {
                    .branch = record.getItem<Kw::BRANCH_NUMBER>().get<int>(0),
                    .top = record.getItem<Kw::PERF_TOP>().getSIDouble(0),
                    .bottom = record.getItem<Kw::PERF_BOT>().getSIDouble(0),
                };

                auto& well_intervals = keyword_intervals[name];
                checkPerforationInterval(interval, well_intervals, existing_connections,
                                         name, handlerContext);
                well_intervals.push_back(interval);

                wellTraj.wellPathGeometry = new external::RigWellPath;

                auto connections = std::make_shared<WellConnections>(well.getConnections());
                auto sequence_index_offset = std::size_t{0};
                if (well.isMultiSegment()) {
                    for (const auto& connection : *connections) {
                        sequence_index_offset =
                            std::max(sequence_index_offset, connection.sort_value() + 1);
                    }
                }

                connections->loadCOMPTRAJ(record, name, handlerContext.grid,
                                          handlerContext.keyword.location(),
                                          wellTraj);

                if (well.isMultiSegment()) {
                    if (well.getSegments().size() <= 1) {
                        const auto msg = fmt::format(
                            "Well {} is a segmented grid-independent well, but its WELSEGS keyword "
                            "only defines the top segment. The segment structure of a COMPTRAJ well "
                            "is taken from WELSEGS and must be given explicitly.", well.name());

                        throw OpmInputError(msg, handlerContext.keyword.location());
                    }

                    auto segmented_connections =
                        Compsegs::getConnectionsToSegmentsFromTrajectory
                            (name,
                             wellTraj.trajectoryConnections,
                             sequence_index_offset,
                             well.getSegments(),
                             *connections,
                             handlerContext.grid,
                             handlerContext.keyword.location(),
                             handlerContext.parseContext,
                             handlerContext.errors);

                    connections = std::make_shared<WellConnections>
                        (std::move(segmented_connections));
                }

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

                handlerContext.state().wells.update(std::move(well));

                handlerContext.state().wellgroup_events()
                    .addEvent(name, ScheduleEvents::COMPLETION_CHANGE);

                handlerContext.comptraj_handled(name);

                if (std::ranges::find(comptraj_wells, name) == comptraj_wells.end()) {
                    comptraj_wells.push_back(name);
                }
            }
        }

        for (const auto& name : comptraj_wells) {
            const auto& well = handlerContext.state().wells.get(name);

            // Reported once per keyword, rather than once per record.
            if ((well.getHeadI() < 0) || (well.getHeadJ() < 0)) {
                const auto reason = well.getConnections().getCoord().contains(1)
                    ? "the trajectory of branch 1 does not enter any active cell"
                    : "the well has no WELTRAJ trajectory for branch 1";

                const auto msg = fmt::format(R"(Problem with keyword {{keyword}}:
In {{file}} line {{line}}
The head location of well {} is defaulted in WELSPECS and cannot be derived from its main branch, since {}. The head location remains undefined)", name, reason);

                OpmLog::warning(OpmInputError::format(msg, handlerContext.keyword.location()));
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

                const auto& existing_connections = well.getConnections();
                if (!existing_connections.empty() && !existing_connections[0].fromTrajectory()) {
                    const auto msg = appendRestartHint(
                        fmt::format("Well {} already has COMPDAT-defined connections and cannot "
                                    "also be defined using WELTRAJ/COMPTRAJ. A well must use "
                                    "either COMPDAT or COMPTRAJ, never both.", name),
                        handlerContext);

                    throw OpmInputError(msg, handlerContext.keyword.location());
                }

                if (well.isMultiSegment()) {
                    const auto msg = fmt::format(
                        "Well {} is a segmented grid-independent well, but its WELSEGS keyword "
                        "must be defined after the corresponding WELTRAJ keyword. Please check "
                        "the order of the keywords in the input file.", name);

                    throw OpmInputError(msg, handlerContext.keyword.location());
                }

                auto connections = std::make_shared<WellConnections>(well.getConnections());
                connections->loadWELTRAJ(record, name, handlerContext.grid,
                                         handlerContext.keyword.location());

                for (const auto& [branch, md] : connections->getMD()) {
                    if (md.size() < 2) {
                        continue;
                    }

                    const bool strictly_increasing = std::adjacent_find
                        (md.begin(), md.end(), std::greater_equal<>{}) == md.end();

                    if (!strictly_increasing) {
                        const auto msg = fmt::format("Well {} measured depth column "
                                                     "for branch {} is not strictly increasing",
                                                     name, branch);

                        throw OpmInputError(msg, handlerContext.keyword.location());
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

void initWellPathGeometry(external::cvf::ref<external::RigWellPath>& wellPathGeometry,
                          const std::array<std::vector<double>, 3>&  coords,
                          const std::vector<double>&                 mds,
                          std::optional<double>                      top_opt,
                          std::optional<double>                      bot_opt)
{
    const double top = top_opt.value_or(mds.front());
    const double bot = bot_opt.value_or(mds.back());

    if ((top < mds.front()) || (bot > mds.back()) || (bot < top)) {
        // linearInterpolation() clamps to the end points, so an interval
        // reaching past the trajectory would be silently truncated and one
        // that runs backwards would collapse to a point.
        throw std::logic_error {
            fmt::format("Perforation interval {} to {} is not contained in the "
                        "branch's trajectory, which covers measured depths "
                        "{} to {}.", top, bot, mds.front(), mds.back())
        };
    }

    std::vector<external::cvf::Vec3d> points;
    std::vector<double> measured_depths;

    points.reserve(coords[0].size());
    measured_depths.reserve(coords[0].size());

    // Calculate the x,y,z coordinates of the begin and end of the interval
    external::cvf::Vec3d p_top, p_bot;
    for (std::size_t i = 0; i < 3; ++i) {
        p_top[i] = linearInterpolation(mds, coords[i], top);
        p_bot[i] = linearInterpolation(mds, coords[i], bot);
    }

    points.push_back(p_top);
    measured_depths.push_back(top);

    for (std::size_t i = 0; i < coords[0].size(); ++i) {
        if ((mds[i] > top) && (mds[i] < bot)) {
            points.push_back(external::cvf::Vec3d(coords[0][i], coords[1][i], coords[2][i]));
            measured_depths.push_back(mds[i]);
        }
    }

    points.push_back(p_bot);
    measured_depths.push_back(bot);

    wellPathGeometry->setWellPathPoints(points, measured_depths);
}

} // namespace Opm
