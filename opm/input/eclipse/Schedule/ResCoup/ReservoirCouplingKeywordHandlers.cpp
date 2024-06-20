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

#include "ReservoirCouplingKeywordHandlers.hpp"
#include "ReservoirCouplingInfo.hpp"
#include "../HandlerContext.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <opm/input/eclipse/Schedule/ScheduleState.hpp>

#include <fmt/format.h>

namespace Opm {

namespace {

void handleSLAVES(HandlerContext& handlerContext)
{
    auto rescoup = handlerContext.state().rescoup();
    const auto& keyword = handlerContext.keyword;

    for (const auto& record : keyword) {
        const std::string& slave_name = record.getItem<ParserKeywords::SLAVES::SLAVE_RESERVOIR>().getTrimmedString(0);
        const std::string& data_filename = record.getItem<ParserKeywords::SLAVES::SLAVE_ECLBASE>().getTrimmedString(0);
        const std::string& directory_path = record.getItem<ParserKeywords::SLAVES::DIRECTORY>().getTrimmedString(0);
        const int numprocs_int = record.getItem<ParserKeywords::SLAVES::NUM_PE>().get<int>(0);
        if (numprocs_int <= 0) {
           std::string msg = fmt::format("Number of processors must be positive. Got: {}.", numprocs_int);
           throw OpmInputError(msg, handlerContext.keyword.location());
        }
        const unsigned int numprocs = static_cast<unsigned int>(numprocs_int);
        ReservoirCoupling::Slave slave{ slave_name, data_filename, directory_path, numprocs};
        rescoup.slaves().emplace( slave_name, std::move( slave ));
    }

    handlerContext.state().rescoup.update( std::move( rescoup ));
}
} // anonymous namespace

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getReservoirCouplingHandlers()
{
    return {
        { "SLAVES", &handleSLAVES },
    };
}

} // namespace Opm
