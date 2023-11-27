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

#include "GuideRateKeywordHandlers.hpp"

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/L.hpp>

#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateModel.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include "../HandlerContext.hpp"

namespace Opm {

namespace {

void handleGUIDERAT(HandlerContext& handlerContext)
{
    const auto& record = handlerContext.keyword.getRecord(0);

    const double min_calc_delay = record.getItem<ParserKeywords::GUIDERAT::MIN_CALC_TIME>().getSIDouble(0);
    const auto phase = GuideRateModel::TargetFromString(record.getItem<ParserKeywords::GUIDERAT::NOMINATED_PHASE>().getTrimmedString(0));
    const double A = record.getItem<ParserKeywords::GUIDERAT::A>().get<double>(0);
    const double B = record.getItem<ParserKeywords::GUIDERAT::B>().get<double>(0);
    const double C = record.getItem<ParserKeywords::GUIDERAT::C>().get<double>(0);
    const double D = record.getItem<ParserKeywords::GUIDERAT::D>().get<double>(0);
    const double E = record.getItem<ParserKeywords::GUIDERAT::E>().get<double>(0);
    const double F = record.getItem<ParserKeywords::GUIDERAT::F>().get<double>(0);
    const bool allow_increase = DeckItem::to_bool( record.getItem<ParserKeywords::GUIDERAT::ALLOW_INCREASE>().getTrimmedString(0));
    const double damping_factor = record.getItem<ParserKeywords::GUIDERAT::DAMPING_FACTOR>().get<double>(0);
    const bool use_free_gas = DeckItem::to_bool( record.getItem<ParserKeywords::GUIDERAT::USE_FREE_GAS>().getTrimmedString(0));

    const auto new_model = GuideRateModel(min_calc_delay, phase, A, B, C, D, E, F, allow_increase, damping_factor, use_free_gas);
    auto new_config = handlerContext.state().guide_rate();
    if (new_config.update_model(new_model)) {
        handlerContext.state().guide_rate.update( std::move(new_config) );
    }
}

void handleLINCOM(HandlerContext& handlerContext)
{
    const auto& record = handlerContext.keyword.getRecord(0);
    const auto alpha = record.getItem<ParserKeywords::LINCOM::ALPHA>().get<UDAValue>(0);
    const auto beta  = record.getItem<ParserKeywords::LINCOM::BETA>().get<UDAValue>(0);
    const auto gamma = record.getItem<ParserKeywords::LINCOM::GAMMA>().get<UDAValue>(0);

    auto new_config = handlerContext.state().guide_rate();
    auto new_model = new_config.model();

    if (new_model.updateLINCOM(alpha, beta, gamma)) {
        new_config.update_model(new_model);
        handlerContext.state().guide_rate.update( std::move( new_config) );
    }
}

void handleWGRUPCON(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);

        const bool availableForGroupControl = DeckItem::to_bool(record.getItem("GROUP_CONTROLLED").getTrimmedString(0));
        const double guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
        const double scaling_factor = record.getItem("SCALING_FACTOR").get<double>(0);

        for (const auto& well_name : well_names) {
            auto phase = Well::GuideRateTarget::UNDEFINED;
            if (!record.getItem("PHASE").defaultApplied(0)) {
                std::string guideRatePhase = record.getItem("PHASE").getTrimmedString(0);
                phase = WellGuideRateTargetFromString(guideRatePhase);
            }

            auto well = handlerContext.state().wells.get(well_name);
            if (well.updateWellGuideRate(availableForGroupControl, guide_rate, phase, scaling_factor)) {
                auto new_config = handlerContext.state().guide_rate();
                new_config.update_well(well);
                handlerContext.state().guide_rate.update( std::move(new_config) );
                handlerContext.state().wells.update( std::move(well) );
            }
        }
    }
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getGuideRateHandlers()
{
    return {
        { "GUIDERAT", &handleGUIDERAT },
        { "LINCOM"  , &handleLINCOM   },
        { "WGRUPCON", &handleWGRUPCON },
    };
}

}
