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

    // Inline methods (alphabetically)

    const std::map<std::string, GrupSlav>& grupSlavs() const {
        return this->m_grup_slavs;
    }

    const GrupSlav& grupSlav(const std::string& name) const {
        return m_grup_slavs.at(name);
    }

    std::map<std::string, GrupSlav>& grupSlavs() {
        return this->m_grup_slavs;
    }

    bool hasGrupSlav(const std::string& name) const {
        return m_grup_slavs.find(name) != m_grup_slavs.end();
    }

    bool hasMasterGroup(const std::string& name) const {
        return m_master_groups.find(name) != m_master_groups.end();
    }

    bool hasSlave(const std::string& name) const {
        return m_slaves.find(name) != m_slaves.end();
    }

    const MasterGroup& masterGroup(const std::string& name) const {
        return m_master_groups.at(name);
    }

    int masterGroupCount() const {
        return m_master_groups.size();
    }

    const std::map<std::string, MasterGroup>& masterGroups() const {
        return this->m_master_groups;
    }

    bool masterMode() const {
        return m_master_mode;
    }

    void masterMode(bool master_mode) {
        m_master_mode = master_mode;
    }

    std::map<std::string, MasterGroup>& masterGroups() {
        return this->m_master_groups;
    }

    double masterMinTimeStep() const {
        return m_master_min_time_step;
    }

    void masterMinTimeStep(double tstep) {
        m_master_min_time_step = tstep;
    }

    const Slave& slave(const std::string& name) const {
        return m_slaves.at(name);
    }

    const std::map<std::string, Slave>& slaves() const {
        return this->m_slaves;
    }

    int slaveCount() const {
        return m_slaves.size();
    }

    std::map<std::string, Slave>& slaves() {
        return this->m_slaves;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_slaves);
        serializer(m_master_groups);
        serializer(m_grup_slavs);
        serializer(m_master_mode);
        serializer(m_master_min_time_step);
    }

    // Non-inline methods (defined in CouplingInfo.cpp)

    bool operator==(const CouplingInfo& other) const;
    static CouplingInfo serializationTestObject();

private:
    std::map<std::string, Slave> m_slaves;
    std::map<std::string, MasterGroup> m_master_groups;
    std::map<std::string, GrupSlav> m_grup_slavs;
    bool m_master_mode{false};
    // Default value: No limit, however a positive value can be set by using keyword RCMASTS
    double m_master_min_time_step{0.0};
};

} // namespace Opm::ReservoirCoupling

#endif
