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
#ifndef RESERVOIR_COUPLING_MASTER_GROUP_HPP
#define RESERVOIR_COUPLING_MASTER_GROUP_HPP

#include <map>
#include <stdexcept>
#include <string>

namespace Opm {

class HandlerContext;

namespace ReservoirCoupling {

class MasterGroup {
public:
    MasterGroup() = default;

    explicit MasterGroup(
        const std::string& name,
        const std::string& slave_name,
        const std::string& slave_group_name,
        double flow_limit_fraction
    ) :
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
    std::string m_name;
    std::string m_slave_name;
    std::string m_slave_group_name;
    double m_flow_limit_fraction;
};


} // namespace ReservoirCoupling

extern void handleGRUPMAST(HandlerContext& handlerContext);

} // namespace Opm

#endif // RESERVOIR_COUPLING_MASTER_GROUP_HPP
