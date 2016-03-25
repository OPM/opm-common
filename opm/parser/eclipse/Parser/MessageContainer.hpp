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

#ifndef MESSAGECONTAINER_H
#define MESSAGECONTAINER_H

#endif // OPM_MESSAGECONTAINER_HEADER_INCLUDED

#include <string>
#include <vector>
#include <memory>

namespace Opm {

    namespace MessageType {

        enum MessageTypeEnum {
            DEBUG     = 1,
            INFO      = 2,
            WARNING   = 3,
            ERROR     = 4,
            PROBLEM   = 5,
            BUG       = 6
        };

    } // namespace MessageType


    struct Location {
        std::string filename;
        int lineno;
    };


    struct Message {
        MessageType::MessageTypeEnum mtype;
        std::string message;
        std::unique_ptr<Location> location;
    };


    ///Message container is used to replace OpmLog functionalities.
    class MessageContainer {
    public:
        void error(const std::string& msg, const std::string& filename, const int lineno);        
        void error(const std::string& msg);

        void bug(const std::string& msg, const std::string& filename, const int lineno);
        void bug(const std::string& msg);

        void warning(const std::string& msg, const std::string& filename, const int lineno);
        void warning(const std::string& msg);

        void info(const std::string& msg, const std::string& filename, const int lineno);
        void info(const std::string& msg);

        void debug(const std::string& msg, const std::string& filename, const int lineno);
        void debug(const std::string& msg);

        void problem(const std::string& msg, const std::string& filename, const int lineno);
        void problem(const std::string& msg);

        std::vector<Message>::const_iterator begin() const;
        std::vector<Message>::const_iterator end() const;
    private:
        std::vector<Message> m_messages;
    };

} // namespace Opm
