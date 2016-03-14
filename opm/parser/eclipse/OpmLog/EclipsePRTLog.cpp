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

#include <opm/parser/eclipse/OpmLog/EclipsePRTLog.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>

namespace Opm {


    EclipsePRTLog::EclipsePRTLog(const std::string& logFile, int64_t messageMask) : StreamLog(logFile, messageMask)
    {}

    
    EclipsePRTLog::EclipsePRTLog(std::ostream& os, int64_t messageMask) : StreamLog(os, messageMask)
    {}

    
    void EclipsePRTLog::addMessage(int64_t messageType, const std::string& message) 
    {
        StreamLog::addMessage(messageType, message);
        m_count[messageType]++;
    }


    size_t EclipsePRTLog::numMessages(int64_t messageType) const 
    {
        if (Log::isPower2( messageType )) {
            auto iter = m_count.find( messageType );
            if (iter == m_count.end())
                return 0;
            else
                return (*iter).second;
        } else
            throw std::invalid_argument("The messageType ID must be 2^n");
    }


    void EclipsePRTLog::clear()
    {
        m_count.clear();
    }


    EclipsePRTLog::~EclipsePRTLog()
    {
        const auto warning = numMessages(Log::MessageType::Warning);
        const auto debug = numMessages(Log::MessageType::Debug);
        const auto info = numMessages(Log::MessageType::Info);
        const auto error = numMessages(Log::MessageType::Error);
        const auto problem = numMessages(Log::MessageType::Problem);
        const auto bug = numMessages(Log::MessageType::Bug);

        //output summary.
        const std::string summary_msg = "\n\nError summary:" + 
            std::string("\nWarnings          " + warning) +
            std::string("\nProblems          " + problem) +
            std::string("\nErrors            " + error) + 
            std::string("\nBugs              " + bug) + 
            std::string("\nDebug             " + debug) +
            std::string("\nProblems          " + problem) +"\n";
        addMessage(Log::MessageType::Info, summary_msg);
    }

}
