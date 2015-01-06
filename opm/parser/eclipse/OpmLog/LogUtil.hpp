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

#ifndef OPM_LOG_UTIL_HPP
#define OPM_LOG_UTIL_HPP

#include <cstdint>
#include <string>

namespace Opm {
namespace Log {

    enum MessageType {
        Note = 0x01,
        Warning = 0x02,
        Error = 0x04
    };

    const int64_t AllMessageTypes = 0xff;

    std::string fileMessage(const std::string& path, size_t line , const std::string& msg);
    std::string fileMessage(Log::MessageType messageType , const std::string& path, size_t line , const std::string& msg);
    std::string prefixMessage(Log::MessageType messageType , const std::string& msg);

}
}

#endif
