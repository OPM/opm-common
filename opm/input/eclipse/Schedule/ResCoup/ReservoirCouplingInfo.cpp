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
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>

namespace Opm {
namespace ReservoirCoupling {
// Class Slave
// ----------------
Slave Slave::serializationTestObject()
{
    return Slave{"RES-1", "RC-01_MOD1_PRED", "../mod1", 1};
}

bool Slave::operator==(const Slave& rhs) const {
    return this->m_name == rhs.m_name;
}


// Class MasterGroup
// -----------------
MasterGroup MasterGroup::serializationTestObject()
{
    return MasterGroup{"D1-M", "RES-1", "MANI-D", 1e+20};
}

bool MasterGroup::operator==(const MasterGroup& rhs) const {
    return this->m_name == rhs.m_name;
}

// Class CouplingInfo
// -------------------

bool CouplingInfo::operator==(const CouplingInfo& rhs) const {
    return this->m_slaves == rhs.m_slaves;
}

CouplingInfo CouplingInfo::serializationTestObject()
{
    CouplingInfo info;
    info.m_slaves = {{"SLAVE1", Slave::serializationTestObject()}};
    return info;
}

} // namespace ReservoirCoupling
} // namespace Opm
