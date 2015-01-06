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

#include <opm/parser/eclipse/OpmLog/Logger.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>

namespace Opm {

    Logger::Logger()
        : m_globalMask(0)
    {
    }

    void Logger::addMessage(int64_t messageFlag , const std::string& message) const {
        if (m_globalMask & messageFlag) {
            for (auto iter = m_backends.begin(); iter != m_backends.end(); ++iter) {
                std::shared_ptr<LogBackend> backend = (*iter).second;
                backend->addMessage( messageFlag , message );
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



}
