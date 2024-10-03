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
#ifndef RESERVOIR_COUPLING_INFO_HPP
#define RESERVOIR_COUPLING_INFO_HPP

#include <opm/input/eclipse/Schedule/ResCoup/Slaves.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/GrupSlav.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/MasterGroup.hpp>
#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/well.hpp>

#include <map>
#include <string>

namespace Opm::ReservoirCoupling {

class CouplingInfo {
public:
    CouplingInfo() = default;

    static CouplingInfo serializationTestObject();
    const std::map<std::string, Slave>& slaves() const {
        return this->m_slaves;
    }
    std::map<std::string, Slave>& slaves() {
        return this->m_slaves;
    }
    const std::map<std::string, MasterGroup>& masterGroups() const {
        return this->m_master_groups;
    }
    std::map<std::string, MasterGroup>& masterGroups() {
        return this->m_master_groups;
    }
    const std::map<std::string, GrupSlav>& grupSlavs() const {
        return this->m_grup_slavs;
    }
    std::map<std::string, GrupSlav>& grupSlavs() {
        return this->m_grup_slavs;
    }
    bool operator==(const CouplingInfo& other) const;
    bool hasSlave(const std::string& name) const {
        return m_slaves.find(name) != m_slaves.end();
    }
    const Slave& slave(const std::string& name) const {
        return m_slaves.at(name);
    }
    int slaveCount() const {
        return m_slaves.size();
    }
    bool hasGrupSlav(const std::string& name) const {
        return m_grup_slavs.find(name) != m_grup_slavs.end();
    }
    const GrupSlav& grupSlav(const std::string& name) const {
        return m_grup_slavs.at(name);
    }
    bool hasMasterGroup(const std::string& name) const {
        return m_master_groups.find(name) != m_master_groups.end();
    }
    const MasterGroup& masterGroup(const std::string& name) const {
        return m_master_groups.at(name);
    }
    int masterGroupCount() const {
        return m_master_groups.size();
    }
    bool masterMode() const {
        return m_master_mode;
    }
    void masterMode(bool master_mode) {
        m_master_mode = master_mode;
    }
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_slaves);
        serializer(m_master_groups);
        serializer(m_grup_slavs);
        serializer(m_master_mode);
    }
private:
    std::map<std::string, Slave> m_slaves;
    std::map<std::string, MasterGroup> m_master_groups;
    std::map<std::string, GrupSlav> m_grup_slavs;
    bool m_master_mode{false};
};

} // namespace Opm::ReservoirCoupling

#endif
