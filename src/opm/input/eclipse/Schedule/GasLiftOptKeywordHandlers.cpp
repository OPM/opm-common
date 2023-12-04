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

#include "GasLiftOptKeywordHandlers.hpp"

#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>

#include "HandlerContext.hpp"

namespace Opm {

namespace {

void handleGLIFTOPT(HandlerContext& handlerContext)
{
    auto glo = handlerContext.state().glo();
    const auto& keyword = handlerContext.keyword;

    for (const auto& record : keyword) {
        const std::string& groupNamePattern = record.getItem<ParserKeywords::GLIFTOPT::GROUP_NAME>().getTrimmedString(0);
        const auto group_names = handlerContext.groupNames(groupNamePattern);
        if (group_names.empty()) {
            handlerContext.invalidNamePattern(groupNamePattern);
        }

        const auto& max_gas_item = record.getItem<ParserKeywords::GLIFTOPT::MAX_LIFT_GAS_SUPPLY>();
        const double max_lift_gas_value = max_gas_item.hasValue(0)
            ? max_gas_item.getSIDouble(0)
            : -1;

        const auto& max_total_item = record.getItem<ParserKeywords::GLIFTOPT::MAX_TOTAL_GAS_RATE>();
        const double max_total_gas_value = max_total_item.hasValue(0)
            ? max_total_item.getSIDouble(0)
            : -1;

        for (const auto& gname : group_names) {
            auto group = GasLiftGroup(gname);
            group.max_lift_gas(max_lift_gas_value);
            group.max_total_gas(max_total_gas_value);

            glo.add_group(group);
        }
    }

    handlerContext.state().glo.update( std::move(glo) );
}

void handleLIFTOPT(HandlerContext& handlerContext)
{
    auto glo = handlerContext.state().glo();

    const auto& record = handlerContext.keyword.getRecord(0);

    const double gaslift_increment = record.getItem<ParserKeywords::LIFTOPT::INCREMENT_SIZE>().getSIDouble(0);
    const double min_eco_gradient = record.getItem<ParserKeywords::LIFTOPT::MIN_ECONOMIC_GRADIENT>().getSIDouble(0);
    const double min_wait = record.getItem<ParserKeywords::LIFTOPT::MIN_INTERVAL_BETWEEN_GAS_LIFT_OPTIMIZATIONS>().getSIDouble(0);
    const bool all_newton = DeckItem::to_bool( record.getItem<ParserKeywords::LIFTOPT::OPTIMISE_ALL_ITERATIONS>().get<std::string>(0) );

    glo.gaslift_increment(gaslift_increment);
    glo.min_eco_gradient(min_eco_gradient);
    glo.min_wait(min_wait);
    glo.all_newton(all_newton);

    handlerContext.state().glo.update( std::move(glo) );
}

void handleWLIFTOPT(HandlerContext& handlerContext)
{
    auto glo = handlerContext.state().glo();

    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem<ParserKeywords::WLIFTOPT::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        const bool use_glo = DeckItem::to_bool(record.getItem<ParserKeywords::WLIFTOPT::USE_OPTIMIZER>().get<std::string>(0));
        const bool alloc_extra_gas = DeckItem::to_bool( record.getItem<ParserKeywords::WLIFTOPT::ALLOCATE_EXTRA_LIFT_GAS>().get<std::string>(0));
        const double weight_factor = record.getItem<ParserKeywords::WLIFTOPT::WEIGHT_FACTOR>().get<double>(0);
        const double inc_weight_factor = record.getItem<ParserKeywords::WLIFTOPT::DELTA_GAS_RATE_WEIGHT_FACTOR>().get<double>(0);
        const double min_rate = record.getItem<ParserKeywords::WLIFTOPT::MIN_LIFT_GAS_RATE>().getSIDouble(0);
        const auto& max_rate_item = record.getItem<ParserKeywords::WLIFTOPT::MAX_LIFT_GAS_RATE>();

        for (const auto& wname : well_names) {
            auto well = GasLiftWell(wname, use_glo);

            if (max_rate_item.hasValue(0))
                well.max_rate( max_rate_item.getSIDouble(0) );

            well.weight_factor(weight_factor);
            well.inc_weight_factor(inc_weight_factor);
            well.min_rate(min_rate);
            well.alloc_extra_gas(alloc_extra_gas);

            glo.add_well(well);
        }
    }

    handlerContext.state().glo.update( std::move(glo) );
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getGasLiftOptHandlers()
{
    return {
        { "GLIFTOPT", &handleGLIFTOPT },
        { "LIFTOPT" , &handleLIFTOPT  },
        { "WLIFTOPT", &handleWLIFTOPT },
    };
}

}
