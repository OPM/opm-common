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

#include "MSWKeywordHandlers.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/input/eclipse/Schedule/Action/WGNames.hpp>
#include <opm/input/eclipse/Schedule/MSW/AICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include "../HandlerContext.hpp"

#include <fmt/format.h>

namespace Opm {

namespace {

void handleCOMPSEGS(HandlerContext& handlerContext)
{
    const auto& record1 = handlerContext.keyword.getRecord(0);
    const std::string& wname = record1.getItem("WELL").getTrimmedString(0);

    if (!handlerContext.state().wells.has(wname)) {
        const auto& location = handlerContext.keyword.location();
        if (handlerContext.action_wgnames().has_well(wname)) {
            std::string msg = fmt::format(R"(Well: {} not yet defined for keyword {}.
Expecting well to be defined with WELSPECS in ACTIONX before actual use.
File {} line {}.)", wname, location.keyword, location.filename, location.lineno);
            OpmLog::warning(msg);
        } else
            throw OpmInputError(fmt::format("No such well: ", wname), location);
        return;
    }

    auto well = handlerContext.state().wells.get( wname );

    if (well.getConnections().empty()) {
        const auto& location = handlerContext.keyword.location();
        auto msg = fmt::format("Problem with COMPSEGS/{0}\n"
                               "In {1} line {2}\n"
                               "Well {0} is not connected to grid - "
                               "COMPSEGS will be ignored",
                               wname, location.filename, location.lineno);
        OpmLog::warning(msg);
        return;
    }

    if (well.handleCOMPSEGS(handlerContext.keyword, handlerContext.grid,
                            handlerContext.parseContext, handlerContext.errors))
    {
        handlerContext.state().wells.update( std::move(well) );
        handlerContext.record_well_structure_change();
    }

    handlerContext.compsegs_handled(wname);
}

void handleWELSEGS(HandlerContext& handlerContext)
{
    const auto& record1 = handlerContext.keyword.getRecord(0);
    const auto& wname = record1.getItem("WELL").getTrimmedString(0);
    if (handlerContext.state().wells.has(wname)) {
        auto well = handlerContext.state().wells.get(wname);
        if (well.handleWELSEGS(handlerContext.keyword)) {
            handlerContext.state().wells.update( std::move(well) );
            handlerContext.record_well_structure_change();
        }
        handlerContext.welsegs_handled(wname);
    } else {
        const auto& location = handlerContext.keyword.location();
        if (handlerContext.action_wgnames().has_well(wname)) {
            std::string msg = fmt::format(R"(Well: {} not yet defined for keyword {}.
Expecting well to be defined with WELSPECS in ACTIONX before actual use.
File {} line {}.)", wname, location.keyword, location.filename, location.lineno);
            OpmLog::warning(msg);
        } else
            throw OpmInputError(fmt::format("No such well: ", wname), location);
    }
}

void handleWSEGAICD(HandlerContext& handlerContext)
{
    std::map<std::string, std::vector<std::pair<int, AutoICD> > > auto_icds = AutoICD::fromWSEGAICD(handlerContext.keyword);

    for (const auto& [well_name_pattern, aicd_pairs] : auto_icds) {
        const auto well_names = handlerContext.wellNames(well_name_pattern, true);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );

            if (well.updateWSEGAICD(aicd_pairs, handlerContext.keyword.location()) )
                handlerContext.state().wells.update( std::move(well) );
        }
    }
}

void handleWSEGITER(HandlerContext& handlerContext)
{
    const auto& record = handlerContext.keyword.getRecord(0);
    auto& tuning = handlerContext.state().tuning();

    tuning.MXWSIT = record.getItem<ParserKeywords::WSEGITER::MAX_WELL_ITERATIONS>().get<int>(0);
    tuning.WSEG_MAX_RESTART = record.getItem<ParserKeywords::WSEGITER::MAX_TIMES_REDUCED>().get<int>(0);
    tuning.WSEG_REDUCTION_FACTOR = record.getItem<ParserKeywords::WSEGITER::REDUCTION_FACTOR>().get<double>(0);
    tuning.WSEG_INCREASE_FACTOR = record.getItem<ParserKeywords::WSEGITER::INCREASING_FACTOR>().get<double>(0);

    handlerContext.state().events().addEvent(ScheduleEvents::TUNING_CHANGE);
}

void handleWSEGSICD(HandlerContext& handlerContext)
{
    std::map<std::string, std::vector<std::pair<int, SICD> > > spiral_icds = SICD::fromWSEGSICD(handlerContext.keyword);

    for (const auto& map_elem : spiral_icds) {
        const std::string& well_name_pattern = map_elem.first;
        const auto well_names = handlerContext.wellNames(well_name_pattern, false);

        const std::vector<std::pair<int, SICD> >& sicd_pairs = map_elem.second;

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );

            if (well.updateWSEGSICD(sicd_pairs) )
                handlerContext.state().wells.update( std::move(well) );
        }
    }
}

void handleWSEGVALV(HandlerContext& handlerContext)
{
    const double udq_default = handlerContext.state().udq.get().params().undefinedValue();
    const std::map<std::string, std::vector<std::pair<int, Valve> > > valves = Valve::fromWSEGVALV(handlerContext.keyword, udq_default);

    for (const auto& map_elem : valves) {
        const std::string& well_name_pattern = map_elem.first;
        const auto well_names = handlerContext.wellNames(well_name_pattern);

        const std::vector<std::pair<int, Valve> >& valve_pairs = map_elem.second;

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );
            if (well.updateWSEGVALV(valve_pairs))
                handlerContext.state().wells.update( std::move(well) );

            handlerContext.affected_well(well_name);
        }
    }
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getMSWHandlers()
{
    return {
        { "COMPSEGS", &handleCOMPSEGS },
        { "WELSEGS" , &handleWELSEGS  },
        { "WSEGAICD", &handleWSEGAICD },
        { "WSEGITER", &handleWSEGITER },
        { "WSEGSICD", &handleWSEGSICD },
        { "WSEGVALV", &handleWSEGVALV },
    };
}

}
