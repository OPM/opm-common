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
#ifndef RESERVOIR_COUPLING_GRUPSLAV_HPP
#define RESERVOIR_COUPLING_GRUPSLAV_HPP

#include <string>
#include <map>
#include "../HandlerContext.hpp"

#include <iostream>
#include <stdexcept>

namespace Opm {
namespace ReservoirCoupling {
class GrupSlav {
public:
    enum class FilterFlag {
        MAST,
        SLAV,
        BOTH
    };

    GrupSlav() = default;

    GrupSlav(
        const std::string& name,
        const std::string& master_group_name,
        FilterFlag oil_prod_flag,
        FilterFlag liquid_prod_flag,
        FilterFlag gas_prod_flag,
        FilterFlag fluid_volume_prod_flag,
        FilterFlag oil_inj_flag,
        FilterFlag water_inj_flag,
        FilterFlag gas_inj_flag
    ) :
        m_name{name},
        m_master_group_name{master_group_name},
        m_oil_prod_flag{oil_prod_flag},
        m_liquid_prod_flag{liquid_prod_flag},
        m_gas_prod_flag{gas_prod_flag},
        m_fluid_volume_prod_flag{fluid_volume_prod_flag},
        m_oil_inj_flag{oil_inj_flag},
        m_water_inj_flag{water_inj_flag},
        m_gas_inj_flag{gas_inj_flag}
    {}

    static GrupSlav serializationTestObject();

    const std::string& name() const {
        return this->m_name;
    }

    const std::string& masterGroupName() const {
        return this->m_master_group_name;
    }

    FilterFlag oilProdFlag() const {
        return this->m_oil_prod_flag;
    }

    FilterFlag liquidProdFlag() const {
        return this->m_liquid_prod_flag;
    }

    FilterFlag gasProdFlag() const {
        return this->m_gas_prod_flag;
    }

    FilterFlag fluidVolumeProdFlag() const {
        return this->m_fluid_volume_prod_flag;
    }

    FilterFlag oilInjFlag() const {
        return this->m_oil_inj_flag;
    }

    FilterFlag waterInjFlag() const {
        return this->m_water_inj_flag;
    }

    FilterFlag gasInjFlag() const {
        return this->m_gas_inj_flag;
    }

    void name(const std::string& value) {
        this->m_name = value;
    }

    void masterGroupName(const std::string& value) {
        this->m_master_group_name = value;
    }

    bool operator==(const GrupSlav& other) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_name);
        serializer(m_master_group_name);
        serializer(m_oil_prod_flag);
        serializer(m_liquid_prod_flag);
        serializer(m_gas_prod_flag);
        serializer(m_fluid_volume_prod_flag);
        serializer(m_oil_inj_flag);
        serializer(m_water_inj_flag);
        serializer(m_gas_inj_flag);
    }

    static FilterFlag filterFlagFromString(const std::string& flag) {
        if (flag == "MAST") {
            return FilterFlag::MAST;
        } else if (flag == "SLAV") {
            return FilterFlag::SLAV;
        } else if (flag == "BOTH") {
            return FilterFlag::BOTH;
        } else {
            throw std::invalid_argument("Invalid filter flag: " + flag);
        }
    }
private:
    std::string m_name;
    std::string m_master_group_name;
    FilterFlag m_oil_prod_flag;
    FilterFlag m_liquid_prod_flag;
    FilterFlag m_gas_prod_flag;
    FilterFlag m_fluid_volume_prod_flag;
    FilterFlag m_oil_inj_flag;
    FilterFlag m_water_inj_flag;
    FilterFlag m_gas_inj_flag;
};

// NOTE: This operator is needed by Boost.Test, See tests/parser/ReservoirCouplingTests.cpp
std::ostream& operator<<(std::ostream& os, const GrupSlav::FilterFlag& flag);

} // namespace ReservoirCoupling

extern void handleGRUPSLAV(HandlerContext& handlerContext);

} // namespace Opm

#endif // RESERVOIR_COUPLING_GRUPSLAV_HPP
