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


#include <opm/input/eclipse/Schedule/ResCoup/ReadCouplingFile.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/U.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include "../HandlerContext.hpp"


namespace Opm {

void handleUSECUPL(HandlerContext& handlerContext)
{
    auto& schedule_state = handlerContext.state();
    auto rescoup = schedule_state.rescoup();
    const auto& keyword = handlerContext.keyword;
    // Opm::Parser::parseFile() (see readDeck.cpp in opm-simulators) will throw an exception if there
    //   is more than one record for this keyword, so we can assume that there is exactly one record here.
    auto record = keyword[0];
    {
        auto deck_item = record.getItem<ParserKeywords::USECUPL::BASE>();
        if (deck_item.defaultApplied(0)) {
            throw OpmInputError("Root name of coupling file (item 1) cannot be defaulted.", keyword.location());
        }
        auto root_name = deck_item.getTrimmedString(0);
        rescoup.setReadCouplingFileName(root_name);
    }
    {
        auto deck_item = record.getItem<ParserKeywords::USECUPL::FMT>();
        if (deck_item.defaultApplied(0)) {
            throw OpmInputError("Format of coupling file (item 2) cannot be defaulted.", keyword.location());
        }
        auto flag_str = deck_item.getTrimmedString(0);
        auto coupling_file_flag = ReservoirCoupling::CouplingInfo::couplingFileFlagFromString(
                flag_str, keyword);
        rescoup.setReadCouplingFileFlag(coupling_file_flag);
    }
    schedule_state.rescoup.update( std::move( rescoup ));
}

} // namespace Opm

