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

#ifndef OPM_MESSAGES_HPP
#define OPM_MESSAGES_HPP

#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>

namespace Opm {

    class TimeMap;    

    class MessagesLimits {
    public:

        /*
           This constructor will create a new Messages object which is
           a copy of the input argument, and then all items explicitly
           set in the record are modified.
        */

        MessagesLimits(std::shared_ptr<const TimeMap> timemap);

        ///Get all the value from MESSAGES keyword.
        int getMessagePrintLimit(size_t timestep) const;
        int getCommentPrintLimit(size_t timestep) const;
        int getWarningPrintLimit(size_t timestep) const;
        int getProblemPrintLimit(size_t timestep) const;
        int getErrorPrintLimit(size_t timestep) const;
        int getBugPrintLimit(size_t timestep) const;
        void setMessagePrintLimit(size_t timestep, int value);
        void setCommentPrintLimit(size_t timestep, int value);
        void setWarningPrintLimit(size_t timestep, int value);
        void setProblemPrintLimit(size_t timestep, int value);
        void setErrorPrintLimit(size_t timestep, int value);
        void setBugPrintLimit(size_t timestep, int value);

        int getMessageStopLimit(size_t timestep) const;
        int getCommentStopLimit(size_t timestep) const;
        int getWarningStopLimit(size_t timestep) const;
        int getProblemStopLimit(size_t timestep) const;
        int getErrorStopLimit(size_t timestep) const;
        int getBugStopLimit(size_t timestep) const;
        void setMessageStopLimit(size_t timestep, int value);
        void setCommentStopLimit(size_t timestep, int value);
        void setWarningStopLimit(size_t timestep, int value);
        void setProblemStopLimit(size_t timestep, int value);
        void setErrorStopLimit(size_t timestep, int value);
        void setBugStopLimit(size_t timestep, int value);
    private:
        DynamicState<int> m_message_print_limit;
        DynamicState<int> m_comment_print_limit;
        DynamicState<int> m_warning_print_limit;
        DynamicState<int> m_problem_print_limit;
        DynamicState<int> m_error_print_limit;
        DynamicState<int> m_bug_print_limit;

        DynamicState<int> m_message_stop_limit;
        DynamicState<int> m_comment_stop_limit;
        DynamicState<int> m_warning_stop_limit;
        DynamicState<int> m_problem_stop_limit;
        DynamicState<int> m_error_stop_limit;
        DynamicState<int> m_bug_stop_limit;
    };
}

#endif
