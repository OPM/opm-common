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
#include <stdexcept>
#include <string>

namespace Opm {

    class LogBackend;

class Logger {

public:
    Logger();
    void addMessage(std::int64_t messageType , const std::string& message) const;
    void addTaggedMessage(std::int64_t messageType, const std::string& tag, const std::string& message) const;

    static bool enabledDefaultMessageType( std::int64_t messageType);
    bool enabledMessageType( std::int64_t messageType) const;
    void addMessageType( std::int64_t messageType , const std::string& prefix);
    std::int64_t enabledMessageTypes() const;

    void addBackend(const std::string& name , std::shared_ptr<LogBackend> backend);
    bool hasBackend(const std::string& name);
    bool removeBackend(const std::string& name);
    void removeAllBackends();

    template <class BackendType>
    std::shared_ptr<BackendType> getBackend(const std::string& name) const {
        auto pair = m_backends.find( name );
        if (pair == m_backends.end())
            throw std::invalid_argument("Invalid backend name: " + name);
        else
            return std::static_pointer_cast<BackendType>(m_backends.find(name)->second);
    }

    template <class BackendType>
    std::shared_ptr<BackendType> popBackend(const std::string& name)  {
        auto pair = m_backends.find( name );
        if (pair == m_backends.end())
            throw std::invalid_argument("Invalid backend name: " + name);
        else {
            std::shared_ptr<LogBackend> backend = (*pair).second;
            removeBackend( name );
            return std::static_pointer_cast<BackendType>(backend);
        }
    }


private:
    void updateGlobalMask( std::int64_t mask );
    static bool enabledMessageType( std::int64_t enabledTypes , std::int64_t messageType);

    std::int64_t m_globalMask;
    std::int64_t m_enabledTypes;
    std::map<std::string , std::shared_ptr<LogBackend> > m_backends;
};

}
#endif
