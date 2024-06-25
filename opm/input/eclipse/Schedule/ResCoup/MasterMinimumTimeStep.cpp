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


#include <opm/input/eclipse/Schedule/ResCoup/MasterMinimumTimeStep.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include "../HandlerContext.hpp"


namespace Opm {

void handleRCMASTS(HandlerContext& handlerContext)
{
    auto& schedule_state = handlerContext.state();
    auto rescoup = schedule_state.rescoup();
    auto tuning = schedule_state.tuning();
    const auto& keyword = handlerContext.keyword;
    if (keyword.size() != 1) {
        throw OpmInputError("RCMASTS keyword requires exactly one record.", keyword.location());
    }
    auto record = keyword[0];
    auto deck_item = record.getItem<ParserKeywords::RCMASTS::MIN_TSTEP>();
    if (deck_item.defaultApplied(0)) {
        // The default value is the current value TSMINZ
        rescoup.masterMinTimeStep(tuning.TSMINZ);
    }
    else {
        auto tstep = deck_item.getSIDouble(0);
        if (tstep < 0.0) {
            throw OpmInputError("Negative value for RCMASTS is not allowed.", keyword.location());
        }
        rescoup.masterMinTimeStep(tstep);
    }
    schedule_state.rescoup.update( std::move( rescoup ));
}

} // namespace Opm

