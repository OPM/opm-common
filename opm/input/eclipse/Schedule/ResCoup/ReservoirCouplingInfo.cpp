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
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>

namespace Opm {
namespace ReservoirCoupling {

CouplingInfo::CouplingFileFlag CouplingInfo::couplingFileFlagFromString(
        const std::string& flag_str, const DeckKeyword& keyword
)
{
    if (flag_str == "F") {
        return CouplingInfo::CouplingFileFlag::FORMATTED;
    } else if (flag_str == "U") {
        return CouplingInfo::CouplingFileFlag::UNFORMATTED;
    } else {
        throw Opm::OpmInputError("Invalid DUMPCUPL value: " + flag_str, keyword.location());
    }
}

bool CouplingInfo::operator==(const CouplingInfo& rhs) const {
    return this->m_slaves == rhs.m_slaves &&
           this->m_master_groups == rhs.m_master_groups &&
           this->m_grup_slavs == rhs.m_grup_slavs &&
           this->m_master_min_time_step == rhs.m_master_min_time_step &&
           this->m_write_coupling_file_flag == rhs.m_write_coupling_file_flag &&
           this->m_read_coupling_file_name == rhs.m_read_coupling_file_name &&
           this->m_read_coupling_file_flag == rhs.m_read_coupling_file_flag;
}

CouplingInfo CouplingInfo::serializationTestObject()
{
    CouplingInfo info;
    info.m_slaves = {{"SLAVE1", Slave::serializationTestObject()}};
    return info;
}

} // namespace ReservoirCoupling
} // namespace Opm
