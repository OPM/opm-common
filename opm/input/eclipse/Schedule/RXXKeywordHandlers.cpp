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

#include "RXXKeywordHandlers.hpp"

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/input/eclipse/Schedule/RFTConfig.hpp>
#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>

#include "HandlerContext.hpp"

namespace Opm {

namespace {

void handleRPTONLY(HandlerContext& handlerContext)
{
    handlerContext.state().rptonly(true);
}

void handleRPTONLYO(HandlerContext& handlerContext)
{
    handlerContext.state().rptonly(false);
}

void handleRPTSCHED(HandlerContext& handlerContext)
{
    auto rpt_config = RPTConfig {
        handlerContext.keyword,
        &handlerContext.state().rpt_config(),
        handlerContext.parseContext,
        handlerContext.errors
    };
    handlerContext.state().rpt_config.update(std::move(rpt_config));

    auto rst_config = handlerContext.state().rst_config();
    rst_config.update(handlerContext.keyword,
                      handlerContext.parseContext,
                      handlerContext.errors);
    handlerContext.state().rst_config.update(std::move(rst_config));
}

void handleRPTRST(HandlerContext& handlerContext)
{
    auto rst_config = handlerContext.state().rst_config();
    rst_config.update(handlerContext.keyword,
                      handlerContext.parseContext,
                      handlerContext.errors);
    handlerContext.state().rst_config.update(std::move(rst_config));
}

// We do not really handle the SAVE keyword, we just interpret it as: Write a
// normal restart file at this report step.
void handleSAVE(HandlerContext& handlerContext)
{
    handlerContext.state().updateSAVE(true);
}

void handleWRFT(HandlerContext& handlerContext)
{
    auto new_rft = handlerContext.state().rft_config();

    for (const auto& record : handlerContext.keyword) {
        const auto& item = record.getItem<ParserKeywords::WRFT::WELL>();
        if (! item.hasValue(0)) {
            continue;
        }

        const auto wellNamePattern = record.getItem<ParserKeywords::WRFT::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        for (const auto& well_name : well_names) {
            new_rft.update(well_name, RFTConfig::RFT::YES);
        }
    }

    new_rft.first_open(true);

    handlerContext.state().rft_config.update(std::move(new_rft));
}

void handleWRFTPLT(HandlerContext& handlerContext)
{
    auto new_rft = handlerContext.state().rft_config();

    const auto rftKey = [](const DeckItem& key)
    {
        return RFTConfig::RFTFromString(key.getTrimmedString(0));
    };

    const auto pltKey = [](const DeckItem& key)
    {
        return RFTConfig::PLTFromString(key.getTrimmedString(0));
    };

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<ParserKeywords::WRFTPLT::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
            continue;
        }

        const auto RFTKey = rftKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_RFT>());
        const auto PLTKey = pltKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_PLT>());
        const auto SEGKey = pltKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_SEGMENT>());

        for (const auto& well_name : well_names) {
            new_rft.update(well_name, RFTKey);
            new_rft.update(well_name, PLTKey);
            new_rft.update_segment(well_name, SEGKey);
        }
    }

    handlerContext.state().rft_config.update(std::move(new_rft));
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getRXXHandlers()
{
    return {
        { "RPTONLY" , &handleRPTONLY  },
        { "RPTONLYO", &handleRPTONLYO },
        { "RPTRST"  , &handleRPTRST   },
        { "RPTSCHED", &handleRPTSCHED },
        { "SAVE"    , &handleSAVE     },
        { "WRFT"    , &handleWRFT     },
        { "WRFTPLT" , &handleWRFTPLT  },
    };
}

}
