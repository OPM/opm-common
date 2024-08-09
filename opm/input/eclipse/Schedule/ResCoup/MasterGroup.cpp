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

#include <opm/input/eclipse/Schedule/ResCoup/MasterGroup.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include "../HandlerContext.hpp"

#include <fmt/format.h>

#include <limits>
#include <stdexcept>

namespace Opm {
namespace ReservoirCoupling {
MasterGroup MasterGroup::serializationTestObject()
{
    return MasterGroup{"D1-M", "RES-1", "MANI-D", 1e+20};
}

bool MasterGroup::operator==(const MasterGroup& rhs) const {
    return
        this->m_name == rhs.m_name &&
        this->m_slave_name == rhs.m_slave_name &&
        this->m_slave_group_name == rhs.m_slave_group_name &&
        this->m_flow_limit_fraction == rhs.m_flow_limit_fraction;
}

} // namespace ReservoirCoupling

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
    const auto& rescoup = handlerContext.state().rescoup();
    if (!rescoup.hasSlave(name)) {
        std::string msg = fmt::format("Slave reservoir '{}': Not defined. Slave reservoirs should be defined in advance by using SLAVES before referenced in GRUPMAST.", name);
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
}

void handleGRUPMAST(HandlerContext& handlerContext)
{
    const auto& schedule_state = handlerContext.state();
    auto rescoup = schedule_state.rescoup();
    const auto& keyword = handlerContext.keyword;
    bool slave_mode = handlerContext.static_schedule().slave_mode;
    if (slave_mode) {
        std::string msg = fmt::format("GRUPMAST keyword is not allowed in slave mode.");
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    if (schedule_state.sim_step() != 0) {
        // Currently, I cannot see any reason why GRUPMAST should be allowed at
        // any other report step than the first one. So to keep it simple, we throw
        // an error if it is used in any other step. This will also simplify the
        // implementation details of MPI communication between master and slave for now..
        std::string msg = fmt::format(
            "GRUPMAST keyword is only allowed in the first schedule report step.");
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    if (rescoup.slaveCount() > 0) {
        // Since SLAVES has been defined, we are now certain that we are in master mode
        rescoup.masterMode(true);
    }
    for (const auto& record : keyword) {
        const std::string& name =
           record.getItem<ParserKeywords::GRUPMAST::MASTER_GROUP>().getTrimmedString(0);
        if (rescoup.hasMasterGroup(name)) {
            std::string msg = fmt::format("Master group '{}' already defined. Redefining", name);
            OpmLog::warning(OpmInputError::format(msg, handlerContext.keyword.location()));
        }
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


} // namespace Opm
