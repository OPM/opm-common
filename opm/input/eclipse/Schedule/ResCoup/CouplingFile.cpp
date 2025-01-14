/*
  Copyright 2024 Equinor ASA.

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


#include <opm/input/eclipse/Schedule/ResCoup/CouplingFile.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include "../HandlerContext.hpp"


namespace Opm {

ReservoirCoupling::CouplingInfo::CouplingFileFlag couplingFileFlagFromString(
        const std::string& flag_str, const DeckKeyword& keyword
)
{
    if (flag_str == "F") {
        return ReservoirCoupling::CouplingInfo::CouplingFileFlag::FORMATTED;
    } else if (flag_str == "U") {
        return ReservoirCoupling::CouplingInfo::CouplingFileFlag::UNFORMATTED;
    } else {
        throw OpmInputError("Invalid DUMPCUPL value: " + flag_str, keyword.location());
    }
}

void handleDUMPCUPL(HandlerContext& handlerContext)
{
    auto& schedule_state = handlerContext.state();
    auto rescoup = schedule_state.rescoup();
    const auto& keyword = handlerContext.keyword;
    // Opm::Parser::parseFile() (see readDeck.cpp in opm-simulators) will throw an exception if there
    //   is more than one record for this keyword, so we can assume that there is exactly one record here.
    const auto& record = keyword[0];
    const auto& deck_item = record.getItem<ParserKeywords::DUMPCUPL::VALUE>();
    if (deck_item.defaultApplied(0)) {
        throw OpmInputError("DUMPCUPL keyword cannot be defaulted.", keyword.location());
    }
    auto flag_str = deck_item.getTrimmedString(0);
    auto coupling_file_flag = couplingFileFlagFromString(flag_str, keyword);
    rescoup.setCouplingFileFlag(coupling_file_flag);
    schedule_state.rescoup.update( std::move( rescoup ));
}

} // namespace Opm
