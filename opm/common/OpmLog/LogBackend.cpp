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

#include <opm/common/OpmLog/LogBackend.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>

namespace Opm {

    LogBackend::LogBackend( int64_t mask ) :
        m_mask(mask)
    {
    }

    LogBackend::~LogBackend()
    {
    }

    void LogBackend::configureDecoration(bool use_prefix, bool use_color_coding)
    {
        use_prefix_ = use_prefix;
        use_color_coding_ = use_color_coding;
    }

    int64_t LogBackend::getMask() const
    {
        return m_mask;
    }

    bool LogBackend::includeMessage(int64_t messageFlag)
    {
        return ((messageFlag & m_mask) == messageFlag) && (messageFlag > 0);
    }

    std::string LogBackend::decorateMessage(int64_t messageFlag, const std::string& message)
    {
        std::string msg = message;
        if (use_prefix_) {
            msg = Log::prefixMessage(messageFlag, msg);
        }
        if (use_color_coding_) {
            msg = Log::colorCodeMessage(messageFlag, msg);
        }
        return msg;
    }


}
