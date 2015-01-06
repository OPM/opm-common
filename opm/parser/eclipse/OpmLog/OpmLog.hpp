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

#include <opm/parser/eclipse/OpmLog/Logger.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>
namespace Opm {


/*
  The OpmLog class is a fully static class which manages a proper
  Logger instance.
*/


class OpmLog {
public:
    static void addMessage(int64_t messageFlag , const std::string& message);
private:
    static std::shared_ptr<Logger> getLogger();
    static std::shared_ptr<Logger> m_logger;
};


}




#endif
