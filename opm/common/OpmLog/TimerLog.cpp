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

#include <config.h>

#include <opm/common/OpmLog/TimerLog.hpp>

#include <opm/common/OpmLog/StreamLog.hpp>

#include <chrono>
#include <cstdint>
#include <ostream>
#include <string>

#include <fmt/format.h>

namespace Opm {

TimerLog::TimerLog(const std::string& logFile)
    : StreamLog { logFile, StopTimer | StartTimer }
{}

TimerLog::TimerLog(std::ostream& os)
    : StreamLog { os, StopTimer | StartTimer }
{}

void TimerLog::clear()
{
    m_start = std::chrono::steady_clock::now();
    m_started = false;
}

void TimerLog::addMessageUnconditionally(const std::int64_t messageType,
                                         const std::string& msg)
{
    if (messageType == StopTimer) {
        if (!m_started) {
            StreamLog::addMessageUnconditionally
                (messageType, fmt::format("Timer not started when receiving user "
                                          "message '{}'. Elapsed time is zero.", msg));
            return;
        }

        const auto stop = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(stop - m_start);

        const auto logMessage = fmt::format("{}: {:.8f} seconds", msg, elapsed.count());
        StreamLog::addMessageUnconditionally(messageType, logMessage);
        m_started = false;
    }
    else if (messageType == StartTimer) {
        m_start = std::chrono::steady_clock::now();
        m_started = true;
    }
}

} // namespace Opm
