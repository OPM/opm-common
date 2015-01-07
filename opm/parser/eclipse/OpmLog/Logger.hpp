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

#ifndef OPM_LOGGER_HPP
#define OPM_LOGGER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>
#include <opm/parser/eclipse/OpmLog/LogBackend.hpp>

namespace Opm {

class Logger {
public:
    Logger();
    void addMessage(int64_t messageFlag , const std::string& message) const;
    void addBackend(const std::string& name , std::shared_ptr<LogSink> backend);
    bool hasBackend(const std::string& name);

private:
    void updateGlobalMask( int64_t mask );

    int64_t m_globalMask;
    std::map<std::string , std::shared_ptr<LogBackend> > m_backends;
};

}
#endif
