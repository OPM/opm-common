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

#ifndef OPMLOG_HPP
#define OPMLOG_HPP

#include <memory>
#include <cstdint>

namespace Opm {


/*
  The OpmLog class is a fully static class which manages a proper
  MessageCounter instance.
*/


class OpmLog {
public:
    enum MessageType {
        Note = 0x01,
        Warning = 0x02,
        Error = 0x04
    };

    static const int64_t AllMessageTypes = 0xff;

    static void addMessage(MessageType messageType , const std::string& message);
    static std::string fileMessage(const std::string& path, size_t line , const std::string& msg);
    static std::string prefixMessage(MessageType messageType , const std::string& msg);
private:
    /*
      static std::shared_ptr<MessageCounter> getMessageCounter();
      static std::shared_ptr<MessageCounter> m_logger;
    */
};


}




#endif
