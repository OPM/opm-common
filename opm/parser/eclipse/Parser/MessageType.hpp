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

#ifndef MESSAGETYPE_H
#define MESSAGETYPE_H

#include <cstdint>
#include <string>

namespace Opm {

    enum MessageType {
        Debug     = 1,
        Info      = 2,
        Warning   = 4,
        Error     = 8,
        Problem   = 16,
        Bug       = 32
    };
    const int64_t DefaultMessageTypes = MessageType::Debug + MessageType::Info + MessageType::Warning + MessageType::Error + MessageType::Problem + MessageType::Bug;

}


#endif // OPM_MESSAGETYPE_HEADER_INCLUDED
