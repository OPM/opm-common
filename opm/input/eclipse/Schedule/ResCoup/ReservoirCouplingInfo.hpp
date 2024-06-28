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

class MasterGroup
{
public:
    MasterGroup() = default;

    explicit MasterGroup(const std::string& name, const std::string& slave_name, const std::string& slave_group_name, double flow_limit_fraction) :
        m_name{name},
        m_slave_name{slave_name},
        m_slave_group_name{slave_group_name},
        m_flow_limit_fraction{flow_limit_fraction}
    {}
    static MasterGroup serializationTestObject();

    const std::string name() const {
        return this->m_name;
    }
    const std::string slaveName() const {
        return this->m_slave_name;
    }
    const std::string slaveGroupName() const {
        return this->m_slave_group_name;
    }
    double flowLimitFraction() const {
        return this->m_flow_limit_fraction;
    }
    void name(const std::string& value) {
        this->m_name = value;
    }
    void slaveName(const std::string& value) {
        this->m_slave_name = value;
    }
    void slaveGroupName(const std::string& value) {
        this->m_slave_group_name = value;
    }
    void flowLimitFraction(double value) {
        this->m_flow_limit_fraction = value;
    }
    bool operator==(const MasterGroup& other) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_slave_name);
        serializer(m_slave_group_name);
        serializer(m_flow_limit_fraction);
    }

private:
    std::string m_name{};
    std::string m_slave_name{};
    std::string m_slave_group_name{};
    double m_flow_limit_fraction{};
};

class Slave {
public:
    Slave() = default;

    explicit Slave(const std::string& name, const std::string& data_filename, const std::string& directory_path, unsigned int numprocs) :
        m_name{name},
        m_data_filename{data_filename},
        m_directory_path{directory_path},
        m_numprocs{numprocs}
    {}
    static Slave serializationTestObject();

    const std::string& name() const {
        return this->m_name;
    }
    const std::string& dataFilename() const {
        return this->m_data_filename;
    }
    const std::string& directoryPath() const {
        return this->m_directory_path;
    }
    unsigned int numprocs() const {
        return this->m_numprocs;
    }

    void name(const std::string& value) {
        this->m_name = value;
    }
    void dataFilename(const std::string& value) {
        this->m_data_filename = value;
    }
    void directoryPath(const std::string& value) {
        this->m_directory_path = value;
    }
    void numprocs(unsigned int value) {
        this->m_numprocs = value;
    }
    bool operator==(const Slave& other) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_data_filename);
        serializer(m_directory_path);
        serializer(m_numprocs);
    }
private:
    std::string m_name{};
    std::string m_data_filename{};
    std::string m_directory_path{};
    unsigned int m_numprocs{};
};

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
