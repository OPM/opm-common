/*
  Copyright 2015 Statoil ASA.

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

#ifndef OPM_LOGGER_HPP
#define OPM_LOGGER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>
#include <opm/parser/eclipse/OpmLog/LogBackend.hpp>

namespace Opm {

class Logger {
public:
    Logger();
    void addMessage(int64_t messageType , const std::string& message) const;

    bool enabledMessageType( int64_t messageType) const;
    void addMessageType( int64_t messageType , const std::string& prefix);

    void addBackend(const std::string& name , std::shared_ptr<LogBackend> backend);
    bool hasBackend(const std::string& name);


    template <class BackendType>
    std::shared_ptr<BackendType> getBackend(const std::string& name) const {
        auto pair = m_backends.find( name );
        if (pair == m_backends.end())
            throw std::invalid_argument("Invalid backend name: " + name);
        else
            return std::static_pointer_cast<BackendType>(m_backends.find(name)->second);
    }


private:
    void updateGlobalMask( int64_t mask );

    int64_t m_enabledTypes;
    int64_t m_globalMask;
    std::map<std::string , std::shared_ptr<LogBackend> > m_backends;
};

}
#endif
