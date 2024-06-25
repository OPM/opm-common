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

#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <opm/input/eclipse/Schedule/ScheduleState.hpp>

#include <fmt/format.h>

namespace Opm {

namespace {
void checkValidGroupName(const std::string& name, HandlerContext& handlerContext)
{
    auto groups = handlerContext.state().groups;
    if (!groups.has(name)) {
        std::string msg = fmt::format("Group '{}': Not defined. Master groups should be defined in advance by using GRUPTREE before referenced in GRUPMAST.", name);
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    auto group = groups(name);
    if (group.wells().size() > 0) {
        std::string msg = fmt::format("Group '{}' has wells: A master group cannot contain any wells or subordinate groups.", name);
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    if (group.groups().size() > 0) {
        std::string msg = fmt::format("Group '{}' has subgroups: A master group cannot contain any wells or subordinate groups.", name);
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
}

void checkValidSlaveName(const std::string& name, HandlerContext& handlerContext)
{
    auto rescoup = handlerContext.state().rescoup();
    if (!rescoup.hasSlave(name)) {
        std::string msg = fmt::format("Slave reservoir '{}': Not defined. Slave reservoirs should be defined in advance by using SLAVES before referenced in GRUPMAST.", name);
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
}

void handleGRUPMAST(HandlerContext& handlerContext)
{
    auto rescoup = handlerContext.state().rescoup();
    const auto& keyword = handlerContext.keyword;
    for (const auto& record : keyword) {
        const std::string& name =
           record.getItem<ParserKeywords::GRUPMAST::MASTER_GROUP>().getTrimmedString(0);
        checkValidGroupName(name, handlerContext);
        const std::string& slave_name =
           record.getItem<ParserKeywords::GRUPMAST::SLAVE_RESERVOIR>().getTrimmedString(0);
        checkValidSlaveName(slave_name, handlerContext);
        const std::string& slave_group_name =
           record.getItem<ParserKeywords::GRUPMAST::SLAVE_GROUP>().getTrimmedString(0);
        double flow_limit_fraction =
              record.getItem<ParserKeywords::GRUPMAST::LIMITING_FRACTION>().get<double>(0);
        ReservoirCoupling::MasterGroup master_group{
             name, slave_name, slave_group_name, flow_limit_fraction
        };
        rescoup.masterGroups().emplace( name, std::move( master_group ));
    }
    handlerContext.state().rescoup.update( std::move( rescoup ));
}

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
            // NOTE: This error should be captured by the keyword validator in readDeck() in
            //       opm-simulators
           std::string msg = fmt::format("{{keyword}}: Number of processors must be positive. Got: {}.", numprocs_int);
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
        { "GRUPMAST", &handleGRUPMAST}
    };
}

} // namespace Opm
