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

#include <stdexcept>
#include <limits>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/GrupSlav.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include "../HandlerContext.hpp"

#include <fmt/format.h>

namespace Opm {
namespace ReservoirCoupling {
GrupSlav GrupSlav::serializationTestObject()
{
    return GrupSlav{"MANI-D", "D1-M", FilterFlag::MAST, FilterFlag::MAST, FilterFlag::MAST,
                    FilterFlag::MAST, FilterFlag::MAST, FilterFlag::MAST, FilterFlag::MAST};
}

bool GrupSlav::operator==(const GrupSlav& rhs) const {
    return
        this->m_name == rhs.m_name &&
        this->m_master_group_name == rhs.m_master_group_name &&
        this->m_oil_prod_flag == rhs.m_oil_prod_flag &&
        this->m_liquid_prod_flag == rhs.m_liquid_prod_flag &&
        this->m_gas_prod_flag == rhs.m_gas_prod_flag &&
        this->m_fluid_volume_prod_flag == rhs.m_fluid_volume_prod_flag &&
        this->m_oil_inj_flag == rhs.m_oil_inj_flag &&
        this->m_water_inj_flag == rhs.m_water_inj_flag &&
        this->m_gas_inj_flag == rhs.m_gas_inj_flag;
}

// NOTE: This function is needed by Boost.Test, see test/ReservoirCoupling/GrupSlavTest.cpp
std::ostream& operator<<(std::ostream& os, const GrupSlav::FilterFlag& flag) {
    switch (flag) {
        case GrupSlav::FilterFlag::MAST:
            os << "MAST";
            break;
        case GrupSlav::FilterFlag::SLAV:
            os << "SLAV";
            break;
        case GrupSlav::FilterFlag::BOTH:
            os << "BOTH";
            break;
        default:
            throw std::invalid_argument("Invalid filter flag");
    }
    return os;
}

} // namespace ReservoirCoupling

void checkValidSlaveGroupName(const std::string& name, HandlerContext& handlerContext)
{
    auto rescoup = handlerContext.state().rescoup();
    if (rescoup.hasGrupSlav(name)) {
        std::string msg = fmt::format("GRUPSLAV group {} already defined. Redefining", name);
        OpmLog::warning(OpmInputError::format(msg, handlerContext.keyword.location()));
    }
    auto groups = handlerContext.state().groups;
    if (!groups.has(name)) {
        std::string msg = fmt::format("Group '{}': Not defined. Slave groups should be defined in advance by using GRUPTREE or WELSPECS before referenced in GRUPSLAV.", name);
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
}

ReservoirCoupling::GrupSlav::FilterFlag getFilterFlag(const DeckItem& keyword, HandlerContext& handlerContext)
{
    try {
        return ReservoirCoupling::GrupSlav::filterFlagFromString(keyword.getTrimmedString(0));
    } catch (const std::invalid_argument& e) {
        std::string msg = fmt::format("Invalid filter flag: {}", keyword.getTrimmedString(0));
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
}

void handleGRUPSLAV(HandlerContext& handlerContext)
{
    auto rescoup = handlerContext.state().rescoup();
    const auto& keyword = handlerContext.keyword;
    bool slave_mode = handlerContext.static_schedule().slave_mode;
    if (!slave_mode) {
        std::string msg = fmt::format("GRUPSLAV is only allowed in slave mode.");
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    for (const auto& record : keyword) {
        const std::string& group_name =
            record.getItem<ParserKeywords::GRUPSLAV::SLAVE_GROUP>().getTrimmedString(0);
        checkValidSlaveGroupName(group_name, handlerContext);
        auto deck_item = record.getItem<ParserKeywords::GRUPSLAV::MASTER_GROUP>();
        std::string master_group_name;
        if (deck_item.defaultApplied(0)) {
            // If defaulted, the master group name is the same as the slave group name
            master_group_name = group_name;
        } else {
            master_group_name = deck_item.getTrimmedString(0);
        }
        auto oil_prod_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::OIL_PROD_CONSTRAINTS>(), handlerContext);
        auto liquid_prod_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::WAT_PROD_CONSTRAINTS>(), handlerContext);
        auto gas_prod_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::GAS_PROD_CONSTRAINTS>(), handlerContext);
        auto fluid_volume_prod_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::FLUID_VOL_PROD_CONSTRAINTS>(), handlerContext);
        auto oil_inj_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::OIL_INJ_CONSTRAINTS>(), handlerContext);
        auto water_inj_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::WAT_INJ_CONSTRAINTS>(), handlerContext);
        auto gas_inj_flag = getFilterFlag(
            record.getItem<ParserKeywords::GRUPSLAV::GAS_INJ_CONSTRAINTS>(), handlerContext);

        ReservoirCoupling::GrupSlav grupslav{ group_name, master_group_name, oil_prod_flag, liquid_prod_flag,
                           gas_prod_flag, fluid_volume_prod_flag, oil_inj_flag, water_inj_flag,
                           gas_inj_flag};
        rescoup.grupSlavs().emplace( group_name, std::move( grupslav ));
    }
    // TODO: - Validate that a slave group is not subordinate to another slave group
    //       - Validate that a slave group is not subject to production or injection
    //         constraints applied to any superior group within the slave reservoir
    handlerContext.state().rescoup.update( std::move( rescoup ));
}

} // namespace Opm
