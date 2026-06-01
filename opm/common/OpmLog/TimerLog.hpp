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
#ifndef OPM_TIMERLOG_HPP
#define OPM_TIMERLOG_HPP

#include <opm/common/OpmLog/StreamLog.hpp>

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string>

/*
  This class is a simple demonstration of how the logging framework
  can be used to create a simple very special case logging facility.
*/

namespace Opm {

class TimerLog : public StreamLog
{
public:
    static const std::int64_t StartTimer = INT64_C(1) << 12;
    static const std::int64_t StopTimer  = INT64_C(1) << 13;

    explicit TimerLog(const std::string& logFile);
    explicit TimerLog(std::ostream& os);

    void clear();

protected:
    void addMessageUnconditionally(std::int64_t messageFlag,
                                   const std::string& message) override;

private:
    std::chrono::steady_clock::time_point m_start{};
    bool m_started{false};
};

} // namespace Opm

#endif
