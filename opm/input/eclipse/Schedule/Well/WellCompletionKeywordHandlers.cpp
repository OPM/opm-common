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

#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>

#include "../HandlerContext.hpp"

#include <fmt/format.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using namespace Opm;

template <typename CompdatKwHandler>
void handleCOMPDATX(HandlerContext&    handlerContext,
                    CompdatKwHandler&& compdatKwHandler)
{
    using Kw = ParserKeywords::COMPDATX;

    auto pending_connections =
        std::unordered_map<std::string, std::shared_ptr<WellConnections>> {};

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);
        const auto wellnames = handlerContext.wellNames(wellNamePattern);

        for (const auto& wname : wellnames) {
            const auto& well = handlerContext.state().wells(wname);

            const auto& [connPos, newConnsInserted] =
                pending_connections.try_emplace(wname);

            if (newConnsInserted) {
                connPos->second = std::make_shared<WellConnections>
                    (well.getConnections());
            }

            // Connections opened/shut by this record; used to raise/clear
            // REQUEST_OPEN_COMPLETION events below.
            auto requested_open_complnums = std::vector<int>{};
            auto requested_shut_complnums = std::vector<int>{};

            std::invoke(compdatKwHandler, *connPos->second,
                        record, wname, well.getWDFAC(),
                        handlerContext.grid,
                        handlerContext.keyword.location(),
                        handlerContext.parseContext,
                        handlerContext.errors,
                        requested_open_complnums,
                        requested_shut_complnums);

            for (const auto& complnum : requested_open_complnums) {
                handlerContext.state().wellcompletion_events()
                    .addEvent(wname, complnum, ScheduleEvents::REQUEST_OPEN_COMPLETION);
            }

            // A connection shut by this record cancels any
            // REQUEST_OPEN_COMPLETION raised for the same connection
            // earlier in this keyword.
            for (const auto& complnum : requested_shut_complnums) {
                handlerContext.state().wellcompletion_events()
                    .clearEvent(wname, complnum, ScheduleEvents::REQUEST_OPEN_COMPLETION);
            }
        }
    }

    auto connections_updated = false;
    for (auto& [wname, connections] : pending_connections) {
        auto well = handlerContext.state().wells(wname);

        const auto is_connected = !well.getConnections().empty()
            || !connections->empty();

        if (well.updateConnections(std::move(connections), handlerContext.grid)) {
            auto wdfac = std::make_shared<WDFAC>(well.getWDFAC());
            wdfac->updateWDFACType(well.getConnections());

            well.updateWDFAC(std::move(wdfac));

            // If the well's reference depth has been defaulted in WELSPECS,
            // we need to calculate that reference depth when completing
            // COMPDAT processing for this well.
            well.updateRefDepth();

            handlerContext.state().wellgroup_events()
                .addEvent(wname, ScheduleEvents::COMPLETION_CHANGE);

            handlerContext.state().wells.update(std::move(well));

            connections_updated = true;
        }

        if (! is_connected) {
            const auto& location = handlerContext.keyword.location();

            const auto msg = fmt::format(R"(Potential problem with {}/{}
In {} line {}
Well {} is not connected to grid - will remain SHUT)",
                                         location.keyword, wname,
                                         location.filename,
                                         location.lineno, wname);

            OpmLog::warning(msg);
        }
    }

    if (connections_updated) {
        handlerContext.state().events()
            .addEvent(ScheduleEvents::COMPLETION_CHANGE);

        handlerContext.record_well_structure_change();
    }
}

// A connection is built in the grid its keyword names (COMPDAT: the global
// grid, COMPDATL: the local grid in the record), but placed in the well's own
// grid (WELSPECS: the global grid, WELSPECL: its local grid), so the two must
// be the same.
void checkConnectionGrid(HandlerContext&                   handlerContext,
                         const std::string&                wname,
                         const std::optional<std::string>& connection_lgr)
{
    const auto well_lgr = handlerContext.state().wells(wname).get_lgr_well_tag();
    if (well_lgr == connection_lgr) {
        return;
    }

    const auto grid = [](const std::optional<std::string>& lgr)
    {
        return lgr.has_value() ? fmt::format("local grid {}", *lgr) : std::string { "the global grid" };
    };

    const auto msg = fmt::format("Well {} is in {}, but {} gives its connection in {}",
                                 wname, grid(well_lgr), handlerContext.keyword.name(),
                                 grid(connection_lgr));
    throw OpmInputError(msg, handlerContext.keyword.location());
}

void handleCOMPDAT(HandlerContext& handlerContext)
{
    handleCOMPDATX(handlerContext,
                   [&handlerContext](WellConnections&   connections,
                                     const DeckRecord&  record,
                                     const std::string& wname,
                                     auto&&...          args)
    {
        checkConnectionGrid(handlerContext, wname, std::nullopt);
        connections.loadCOMPDAT(record, wname, std::forward<decltype(args)>(args)...);
    });
}

void handleCOMPDATL(HandlerContext& handlerContext)
{
    handleCOMPDATX(handlerContext,
                   [&handlerContext](WellConnections&   connections,
                                     const DeckRecord&  record,
                                     const std::string& wname,
                                     auto&&...          args)
    {
        checkConnectionGrid(handlerContext, wname,
                            record.getItem<ParserKeywords::COMPDATX::LGR>().getTrimmedString(0));
        connections.loadCOMPDATL(record, wname, std::forward<decltype(args)>(args)...);
    });
}

void handleCOMPLUMP(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::COMPLUMP;

    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);
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

void handleCECON(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::CECON;

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<Kw::WELLNAME>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& wname : well_names) {
            auto well = handlerContext.state().wells.get(wname);

            if (well.handleCECON(record, handlerContext.keyword.location(),
                                 handlerContext.parseContext, handlerContext.errors))
            {
                handlerContext.state().wells.update(std::move(well));
            }
        }
    }
}

} // Anonymous namespace

namespace Opm {

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getWellCompletionHandlers()
{
    return {
        { "CECON",    &handleCECON    },
        { "COMPDAT" , &handleCOMPDAT  },
        { "COMPDATL", &handleCOMPDATL },
        { "COMPLUMP", &handleCOMPLUMP },
        { "COMPORD" , &handleCOMPORD  },
        { "CSKIN",    &handleCSKIN    },
    };
}

} // namespace Opm
