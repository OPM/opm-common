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
    namespace MessageType {
        const int64_t Note = 0x01;
        const int64_t Warning = 0x02;
        const int64_t Error = 0x04;
    }

    const int64_t DefaultMessageTypes = MessageType::Note + MessageType::Warning + MessageType::Error;

    bool isPower2(int64_t x);
    std::string fileMessage(const std::string& path, int line , const std::string& msg);
    std::string fileMessage(int64_t messageType , const std::string& path, int line , const std::string& msg);
    std::string prefixMessage(int64_t messageType , const std::string& msg);

}
}

#endif
