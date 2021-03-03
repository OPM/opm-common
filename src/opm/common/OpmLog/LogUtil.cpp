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
#include <opm/common/OpmLog/LogUtil.hpp>


namespace Opm {

namespace Log {

namespace {

static const std::string debugPrefix   = std::string{"Debug  : "};
static const std::string notePrefix    = std::string{"Note   : "};
static const std::string infoPrefix    = std::string{"Info   : "};
static const std::string warningPrefix = std::string{"\nWarning: "};
static const std::string errorPrefix   = std::string{"\nError  : "};
static const std::string problemPrefix = std::string{"\nProblem: "};
static const std::string bugPrefix     = std::string{"\nBug    : "};
static const std::string blankPrefix   = std::string{"         "};
}

    bool isPower2(int64_t x) {
        return ((x != 0) && !(x & (x - 1)));
    }



    std::string fileMessage(const KeywordLocation& location, const std::string& message) {
        std::ostringstream oss;

        oss << message << "\n" << "In file " << location.filename << ", line " << location.lineno << "\n";

        return oss.str();
    }

    std::string fileMessage(int64_t messageType , const KeywordLocation& location, const std::string& message) {
        return fileMessage( location , prefixMessage( messageType , message ));
    }


    std::string prefixMessage(int64_t messageType, const std::string& message) {
        switch (messageType) {
        case MessageType::Continuation:
            return blankPrefix + message;

        case MessageType::Debug:
            return debugPrefix + message;

        case MessageType::Note:
            return notePrefix + message;

        case MessageType::Info:
            return infoPrefix + message;

        case MessageType::Warning:
            return warningPrefix + message;

        case MessageType::Error:
            return errorPrefix + message;

        case MessageType::Problem:
            return problemPrefix + message;

        case MessageType::Bug:
            return bugPrefix + message;

        default:
            throw std::invalid_argument("Unhandled messagetype");
        }
    }

    std::string colorCodeMessage(int64_t messageType, const std::string& message) {
        switch (messageType) {
        case MessageType::Debug:
        case MessageType::Note:
        case MessageType::Info:
            return message; // No color coding, not even the code for default color.
        case MessageType::Warning:
            return AnsiTerminalColors::blue_strong + message + AnsiTerminalColors::none;
        case MessageType::Problem:
            return AnsiTerminalColors::magenta_strong + message + AnsiTerminalColors::none;
        case MessageType::Error:
        case MessageType::Bug:
            return AnsiTerminalColors::red_strong + message + AnsiTerminalColors::none;
        default:
            throw std::invalid_argument("Unhandled messagetype");
        }
    }

}
}
