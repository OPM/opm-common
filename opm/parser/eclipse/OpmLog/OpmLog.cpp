/*
  Copyright 2014 Statoil ASA.

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

#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>

namespace Opm {



    std::string OpmLog::fileMessage(const std::string& filename , size_t line , const std::string& message) {
        std::ostringstream oss;

        oss << filename << ":" << line << ": " << message;

        return oss.str();
    }


    std::string OpmLog::prefixMessage(MessageType messageType, const std::string& message) {
        std::string prefix;
        switch (messageType) {
        case Note:
            prefix = "note";
            break;
        case Warning:
            prefix = "warning";
            break;
        case Error:
            prefix = "error";
            break;
        default:
            throw std::invalid_argument("Unhandled messagetype");
        }

        return prefix + ": " + message;
    }

    /*
    std::shared_ptr<MessageCounter> OpmLog::getMessageCounter() {
          if (!m_logger)
          m_logger.reset( new MessageCounter() );

          return m_logger;
    }
    */

    void OpmLog::addMessage(MessageType messageType , const std::string& message) {
        /*
          auto logger = OpmLog::getMessageCounter();
          logger->addMessage( "" , -1 , messageType , message );
        */
    }


/******************************************************************/

    //std::shared_ptr<MessageCounter> OpmLog::m_logger;
}
