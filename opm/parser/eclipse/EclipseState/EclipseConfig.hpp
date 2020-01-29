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

#ifndef OPM_ECLIPSE_CONFIG_HPP
#define OPM_ECLIPSE_CONFIG_HPP

#include <memory>

#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>

namespace Opm {

    class Deck;
    class ParseContext;
    class ErrorGuard;

    class EclipseConfig
    {
    public:
        EclipseConfig() = default;
        EclipseConfig(const Deck& deck, const ParseContext& parseContext, ErrorGuard& errors);
        EclipseConfig(const InitConfig& initConfig,
                      const RestartConfig& restartConfig);

        const InitConfig& init() const;
        IOConfig& io();
        const IOConfig& io() const;
        const RestartConfig& restart() const;
        const InitConfig& getInitConfig() const;
        const RestartConfig& getRestartConfig() const;

        bool operator==(const EclipseConfig& data) const;

    private:
        InitConfig m_initConfig;
        RestartConfig m_restartConfig;
    };
}

#endif // OPM_ECLIPSE_CONFIG_HPP
