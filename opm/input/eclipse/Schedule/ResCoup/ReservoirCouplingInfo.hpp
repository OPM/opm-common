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
#ifndef RESERVOIR_COUPLING_SLAVES_HPP
#define RESERVOIR_COUPLING_SLAVES_HPP

#include <optional>
#include <string>
#include <map>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/group.hpp>

namespace Opm {
namespace ReservoirCoupling {

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
    std::string m_name;
    std::string m_data_filename;
    std::string m_directory_path;
    unsigned int m_numprocs;
};

class CouplingInfo {
public:
    CouplingInfo() = default;

    static CouplingInfo serializationTestObject();
    std::map<std::string, Slave>& slaves() {
        return this->m_slaves;
    }
    bool operator==(const CouplingInfo& other) const;
    bool has_slave(const std::string& name) const {
        return m_slaves.find(name) != m_slaves.end();
    }
    const Slave& slave(const std::string& name) const {
        return m_slaves.at(name);
    }
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_slaves);
    }
private:
    std::map<std::string, Slave> m_slaves;
};

} // namespace ReservoirCoupling
} // namespace Opm
#endif