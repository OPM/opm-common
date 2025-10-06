/*
 Copyright 2016 Statoil ASA.

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

#include <memory>

#include <opm/input/eclipse/EclipseState/EclipseConfig.hpp>
#include <opm/input/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>


namespace Opm {

    EclipseConfig::EclipseConfig(const Deck& deck, const Phases& phases) :
        m_initConfig(deck, phases),
        fip_config(deck),
        io_config(deck)
    {
    }


    EclipseConfig::EclipseConfig(const InitConfig& initConfig,
                                 const FIPConfig& fip_conf,
                                 const IOConfig& io_conf):
        m_initConfig(initConfig),
        fip_config(fip_conf),
        io_config(io_conf)
    {
    }


    EclipseConfig EclipseConfig::serializationTestObject()
    {
        EclipseConfig result;
        result.m_initConfig = InitConfig::serializationTestObject();
        result.fip_config = FIPConfig::serializationTestObject();
        result.io_config = IOConfig::serializationTestObject();

        return result;
    }

    InitConfig& EclipseConfig::init() {
        return const_cast<InitConfig &>(this->m_initConfig);
    }

    const InitConfig& EclipseConfig::init() const{
        return m_initConfig;
    }

    bool EclipseConfig::operator==(const EclipseConfig& data) const {
        return this->init() == data.init() &&
               this->fip() == data.fip() &&
               this->io() == data.io();
    }

    IOConfig& EclipseConfig::io() {
        return this->io_config;
    }

    const IOConfig& EclipseConfig::io() const {
        return this->io_config;
    }

    const FIPConfig& EclipseConfig::fip() const {
        return this->fip_config;
    }

    bool EclipseConfig::rst_cmp(const EclipseConfig& full_config, const EclipseConfig& rst_config) {
        return IOConfig::rst_cmp(full_config.io(), rst_config.io()) &&
               InitConfig::rst_cmp(full_config.init(), rst_config.init());
    }
}
