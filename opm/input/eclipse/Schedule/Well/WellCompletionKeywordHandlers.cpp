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

#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>

#include "../HandlerContext.hpp"

#include <fmt/format.h>

#include <functional>
#include <memory>
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
    auto wells = std::unordered_set<std::string> {};
    auto well_connected = std::unordered_map<std::string, bool> {};

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto wellnames = handlerContext.wellNames(wellNamePattern);

        for (const auto& name : wellnames) {
            auto well2 = handlerContext.state().wells(name);

            const auto& orig_connections = well2.getConnections();
            auto connections = std::make_shared<WellConnections>(orig_connections);
            const auto origWellConnSetIsEmpty = connections->empty();

            // Connections opened respectively shut by this record; used to
            // raise respectively clear REQUEST_OPEN_COMPLETION events below.
            auto requested_open_complnums = std::vector<int>{};
            auto requested_shut_complnums = std::vector<int>{};

            std::invoke(compdatKwHandler, connections,
                        record, name, well2.getWDFAC(),
                        handlerContext.grid,
                        handlerContext.keyword.location(),
                        handlerContext.parseContext,
                        handlerContext.errors,
                        requested_open_complnums,
                        requested_shut_complnums);

            for (const int complnum : requested_open_complnums) {
                handlerContext.state().wellcompletion_events()
                    .addEvent(name, complnum, ScheduleEvents::REQUEST_OPEN_COMPLETION);
            }

            // A connection shut by this record cancels any REQUEST_OPEN_COMPLETION
            // raised for the same connection earlier in this keyword.
            for (const int complnum : requested_shut_complnums) {
                handlerContext.state().wellcompletion_events()
                    .clearEvent(name, complnum, ScheduleEvents::REQUEST_OPEN_COMPLETION);
            }

            const auto isConnected = !origWellConnSetIsEmpty || !connections->empty();

            const auto& [connectedPos, inserted] =
                well_connected.emplace(name, isConnected);

            if ((! inserted) && isConnected) {
                // Note condition here.  If we inserted a new well, then
                // ->second is "isConnected".  Otherwise, we leave ->second
                // unchanged if "isConnected" is false and unconditionally
                // set it to true if "isConnected" is true.
                //
                // This block achieves the same effect as
                //
                //   ->second = ->second || isConnected
                connectedPos->second = true;
            }

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

            const auto msg = fmt::format(R"(Potential problem with {}/{}
In {} line {}
Well {} is not connected to grid - will remain SHUT)",
                                         location.keyword, wname,
                                         location.filename,
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

namespace Opm {

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getWellCompletionHandlers()
{
    return {
        { "COMPDAT" , &handleCOMPDAT  },
        { "COMPDATL", &handleCOMPDATL },
        { "COMPLUMP", &handleCOMPLUMP },
        { "COMPORD" , &handleCOMPORD  },
        { "CSKIN",    &handleCSKIN    },
    };
}

} // namespace Opm
