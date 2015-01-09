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
#include <sstream>
#include <stdexcept>
#include <iostream>

#include <opm/parser/eclipse/OpmLog/Logger.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>



static bool isPower2(int64_t x) {
    return ((x != 0) && !(x & (x - 1)));
}

namespace Opm {


    Logger::Logger()
        : m_globalMask(0),
          m_enabledTypes(0)
    {
        addMessageType( Log::MessageType::Error , "error");
        addMessageType( Log::MessageType::Warning , "warning");
        addMessageType( Log::MessageType::Note , "note");
    }

    void Logger::addMessage(int64_t messageType , const std::string& message) const {
        if (m_globalMask & messageType) {
            for (auto iter = m_backends.begin(); iter != m_backends.end(); ++iter) {
                std::shared_ptr<LogBackend> backend = (*iter).second;
                backend->addMessage( messageType , message );
            }
        }
    }


    void Logger::updateGlobalMask( int64_t mask ) {
        m_globalMask |= mask;
    }


    bool Logger::hasBackend(const std::string& name) {
        if (m_backends.find( name ) == m_backends.end())
            return false;
        else
            return true;
    }


    void Logger::addBackend(const std::string& name , std::shared_ptr<LogBackend> backend) {
        updateGlobalMask( backend->getMask() );
        m_backends[ name ] = backend;
    }





    bool Logger::enabledMessageType( int64_t messageType) const {
        if (isPower2( messageType)) {
            if ((messageType & m_enabledTypes) == 0)
                return false;
            else
                return true;
        } else
            throw std::invalid_argument("The message type id must be ~ 2^n");
    }



    void Logger::addMessageType( int64_t messageType , const std::string& prefix) {
        if (isPower2( messageType)) {
            m_enabledTypes |= messageType;
        } else
            throw std::invalid_argument("The message type id must be ~ 2^n");
    }

}
