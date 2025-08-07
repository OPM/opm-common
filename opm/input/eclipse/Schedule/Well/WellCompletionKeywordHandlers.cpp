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

#include "WellCompletionKeywordHandlers.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/Action/WGNames.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/MSW/Compsegs.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>

#include <external/resinsight/ReservoirDataModel/RigWellLogExtractor.h>
#include <external/resinsight/ReservoirDataModel/RigWellPath.h>


#include "../HandlerContext.hpp"

#include <fmt/format.h>

#include <unordered_set>

namespace Opm {

namespace {

using LoadConnectionMethod = void (WellConnections::*)(
    const DeckRecord&,
    const ScheduleGrid&,
    const std::string&,
    const WDFAC&,
    const KeywordLocation&
);

void handleCOMPDATX(HandlerContext& handlerContext, LoadConnectionMethod loadMethod)
{
    std::unordered_set<std::string> wells;
    std::unordered_map<std::string, bool> well_connected;
    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto wellnames = handlerContext.wellNames(wellNamePattern);

        for (const auto& name : wellnames) {
            auto well2 = handlerContext.state().wells.get(name);

            auto connections = std::make_shared<WellConnections>(well2.getConnections());
            const auto origWellConnSetIsEmpty = connections->empty();
            std::invoke(loadMethod, connections, record, handlerContext.grid,
                             name, well2.getWDFAC(), handlerContext.keyword.location());

            const auto isConnected = !origWellConnSetIsEmpty || !connections->empty();
            if (well_connected.count(name))
                well_connected[name] = (isConnected || well_connected[name]);
            else
                well_connected[name] = isConnected;

            if (well2.updateConnections(std::move(connections), handlerContext.grid)) {
                auto wdfac = std::make_shared<WDFAC>(well2.getWDFAC());
                wdfac->updateWDFACType(well2.getConnections());

                well2.updateWDFAC(std::move(wdfac));
                handlerContext.state().wells.update( well2 );

                wells.insert(name);
            }

            handlerContext.state().wellgroup_events()
                .addEvent(name, ScheduleEvents::COMPLETION_CHANGE);
        }
    }
    // Output warning messages per well/keyword (not per COMPDAT record..)
    for (const auto& [wname, connected] : well_connected) {
        if (!connected) {
            const auto& location = handlerContext.keyword.location();

            const auto msg = fmt::format(R"(Potential problem with COMPDAT/{}
In {} line {}
Well {} is not connected to grid - will remain SHUT)",
                                         wname, location.filename,
                                         location.lineno, wname);

            OpmLog::warning(msg);
        }
    }

    handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);

    // In the case the wells reference depth has been defaulted in the
    // WELSPECS keyword we need to force a calculation of the wells
    // reference depth exactly when the COMPDAT keyword has been completely
    // processed.
    for (const auto& wname : wells) {
        auto well = handlerContext.state().wells.get( wname );
        well.updateRefDepth();

        handlerContext.state().wells.update(std::move(well));
    }

    if (! wells.empty()) {
        handlerContext.record_well_structure_change();
    }
}

void handleCOMPDAT(HandlerContext& handlerContext)
{
    handleCOMPDATX(handlerContext, &WellConnections::loadCOMPDAT);
}

void handleCOMPDATL(HandlerContext& handlerContext)
{
    handleCOMPDATX(handlerContext, &WellConnections::loadCOMPDATL);
}



void handleCOMPLUMP(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);

        for (const auto& wname : well_names) {
            auto well = handlerContext.state().wells.get(wname);
            if (well.handleCOMPLUMP(record)) {
                handlerContext.state().wells.update( std::move(well) );

                handlerContext.record_well_structure_change();
            }
        }
    }
}

// The COMPORD keyword is handled together with the WELSPECS keyword in the
// handleWELSPECS() function.
void handleCOMPORD(HandlerContext&)
{}

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
            
            if (handlerContext.state().wells.get(name).isMultiSegment()) {
                // Retrieve the well path coordinates:
                std::vector<std::tuple<double, double, std::array<int, 3>>> intersections_md_and_ijk;
                std::vector<std::pair<double, double>> cell_md_and_tvd;
                intersections_md_and_ijk.reserve(intersections.size());
                cell_md_and_tvd.reserve(intersections.size());
                const auto& ecl_grid = handlerContext.grid.get_grid();
                for (const auto& intersection: intersections) {
                    const auto ijk = ecl_grid->getIJK(intersection.globCellIndex);
                    intersections_md_and_ijk.emplace_back(intersection.startMD, intersection.endMD, ijk);
                    double cell_md = 0.5 * (intersection.startMD + intersection.endMD);
                    double cell_tvd =  wellPathGeometry->interpolatedPointAlongWellPath(cell_md)[2];
                    cell_md_and_tvd.emplace_back(cell_md, cell_tvd);
                }

                auto well = handlerContext.state().wells.get(name);
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
                // Generate WELSEGS data:
                const auto& diameter = record.getItem("DIAMETER").getSIDouble(0);
                well.addWellSegmentsFromLengthsAndDepths(cell_md_and_tvd, diameter);
                handlerContext.state().wells.update(std::move(well));
                handlerContext.record_well_structure_change();

                // Generate COMPSEGS data:
                well = handlerContext.state().wells.get(name);
                auto [new_connections, new_segments] = Compsegs::processCOMPSEGS(
                    intersections_md_and_ijk, well.getSegments(), well.getConnections(), well.getSegments(), handlerContext.grid
                );
                well.updateConnections(std::make_shared<WellConnections>(std::move(new_connections)), false);
                well.updateSegments(std::make_shared<WellSegments>(std::move(new_segments)));
                handlerContext.state().wells.update(std::move(well));
                handlerContext.record_well_structure_change();
            }

            handlerContext.state().wellgroup_events()
                .addEvent(name, ScheduleEvents::COMPLETION_CHANGE);
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


void handleCSKIN(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::CSKIN;

    // Loop over records in CSKIN
    for (const auto& record : handlerContext.keyword) {
        // Get well names
        const auto wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        // Loop over well(s) in record
        for (const auto& wname : well_names) {
            // Get well information, modify connection skin factor, and
            // update well.
            auto well = handlerContext.state().wells.get(wname);

            if (well.handleCSKIN(record, handlerContext.keyword.location())) {
                handlerContext.state().wells.update(std::move(well));
            }
        }
    }
}

} // Anonymous namespace

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getWellCompletionHandlers()
{
    return {
        { "COMPDAT" , &handleCOMPDAT  },
        { "COMPDATL", &handleCOMPDATL },
        { "COMPLUMP", &handleCOMPLUMP },
        { "COMPORD" , &handleCOMPORD  },
        { "COMPTRAJ", &handleCOMPTRAJ },
        { "CSKIN",    &handleCSKIN    },
    };
}

} // namespace Opm
