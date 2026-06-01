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

#ifndef ECLIPSEPRTLOG_H
#define ECLIPSEPRTLOG_H

#include <opm/common/OpmLog/StreamLog.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace Opm {

class EclipsePRTLog : public StreamLog {

public:
    using StreamLog::StreamLog;

    std::size_t numMessages(std::int64_t messageType) const;

    ~EclipsePRTLog() override;

    /// \brief Construct a logger to the <MODEL>.PRT file
    /// \param logFile The name of the logfile to use.
    /// \param messageMask Mask for logging of messages.
    /// \param append If true then we append messages to the file.
    ///               Otherwise a new file is created.
    /// \param print_summary If true print a summary to the PRT file.
    EclipsePRTLog(const std::string& logFile , std::int64_t messageMask,
                  bool append, bool print_summary);

    /// \brief Construct a logger to the <MODEL>.PRT file
    /// \param os The stream to write the log to.
    /// \param messageMask Mask for logging of messages
    /// \param print_summary If true print a summary to the PRT file.
    EclipsePRTLog(std::ostream& os, std::int64_t messageMask,
                  bool print_summary);

protected:
    void addMessageUnconditionally(std::int64_t messageType, const std::string& message) override;

private:
    std::map<std::int64_t, std::size_t> m_count;
    /// \brief Whether to print a summary to the log file.
    bool print_summary_ = true;
};
}
#endif // ECLIPSEPRTLOG_H
