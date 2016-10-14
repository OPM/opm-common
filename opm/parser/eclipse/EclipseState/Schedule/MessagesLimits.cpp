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

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MessagesLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>

namespace Opm {


    MessagesLimits::MessagesLimits(TimeMapConstPtr timemap):
        m_message_print_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::MESSAGE_PRINT_LIMIT::defaultValue)),
        m_comment_print_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::COMMENT_PRINT_LIMIT::defaultValue)),
        m_warning_print_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::WARNING_PRINT_LIMIT::defaultValue)),
        m_problem_print_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::PROBLEM_PRINT_LIMIT::defaultValue)),
        m_error_print_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::ERROR_PRINT_LIMIT::defaultValue)),
        m_bug_print_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::BUG_PRINT_LIMIT::defaultValue)),
        m_message_stop_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::MESSAGE_STOP_LIMIT::defaultValue)),
        m_comment_stop_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::COMMENT_STOP_LIMIT::defaultValue)),
        m_warning_stop_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::WARNING_STOP_LIMIT::defaultValue)),
        m_problem_stop_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::PROBLEM_STOP_LIMIT::defaultValue)),
        m_error_stop_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::ERROR_STOP_LIMIT::defaultValue)),
        m_bug_stop_limit(new DynamicState<int>(timemap, ParserKeywords::MESSAGES::BUG_STOP_LIMIT::defaultValue))
        {
        }




    int MessagesLimits::getMessagePrintLimit(size_t timestep) const
    {
        return m_message_print_limit->get(timestep);
    }

    int MessagesLimits::getCommentPrintLimit(size_t timestep) const
    {
        return m_comment_print_limit->get(timestep);
    }

    int MessagesLimits::getWarningPrintLimit(size_t timestep) const
    {
        return m_warning_print_limit->get(timestep);
    }

    int MessagesLimits::getProblemPrintLimit(size_t timestep) const
    {
        return m_problem_print_limit->get(timestep);
    }

    int MessagesLimits::getErrorPrintLimit(size_t timestep) const
    {
        return m_error_print_limit->get(timestep);
    }

    int MessagesLimits::getBugPrintLimit(size_t timestep) const
    {
        return m_bug_print_limit->get(timestep);
    }



    void MessagesLimits::setMessagePrintLimit(size_t timestep, int value)
    {
        m_message_print_limit->update(timestep, value);
    }

    void MessagesLimits::setCommentPrintLimit(size_t timestep, int value)
    {
        m_comment_print_limit->update(timestep, value);
    }

    void MessagesLimits::setWarningPrintLimit(size_t timestep, int value)
    {
        m_warning_print_limit->update(timestep, value);
    }

    void MessagesLimits::setProblemPrintLimit(size_t timestep, int value)
    {
        m_problem_print_limit->update(timestep, value);
    }

    void MessagesLimits::setErrorPrintLimit(size_t timestep, int value)
    {
        m_error_print_limit->update(timestep, value);
    }

    void MessagesLimits::setBugPrintLimit(size_t timestep, int value)
    {
        m_bug_print_limit->update(timestep, value);
    }


    int MessagesLimits::getMessageStopLimit(size_t timestep) const
    {
        return m_message_stop_limit->get(timestep);
    }

    int MessagesLimits::getCommentStopLimit(size_t timestep) const
    {
        return m_comment_stop_limit->get(timestep);
    }

    int MessagesLimits::getWarningStopLimit(size_t timestep) const
    {
        return m_warning_stop_limit->get(timestep);
    }

    int MessagesLimits::getProblemStopLimit(size_t timestep) const
    {
        return m_problem_stop_limit->get(timestep);
    }

    int MessagesLimits::getErrorStopLimit(size_t timestep) const
    {
        return m_error_stop_limit->get(timestep);
    }

    int MessagesLimits::getBugStopLimit(size_t timestep) const
    {
        return m_bug_stop_limit->get(timestep);
    }


    void MessagesLimits::setMessageStopLimit(size_t timestep, int value)
    {
        m_message_stop_limit->update(timestep, value);
    }

    void MessagesLimits::setCommentStopLimit(size_t timestep, int value)
    {
        m_comment_stop_limit->update(timestep, value);
    }

    void MessagesLimits::setWarningStopLimit(size_t timestep, int value)
    {
        m_warning_stop_limit->update(timestep, value);
    }

    void MessagesLimits::setProblemStopLimit(size_t timestep, int value)
    {
        m_problem_stop_limit->update(timestep, value);
    }

    void MessagesLimits::setErrorStopLimit(size_t timestep, int value)
    {
        m_error_stop_limit->update(timestep, value);
    }

    void MessagesLimits::setBugStopLimit(size_t timestep, int value)
    {
        m_bug_stop_limit->update(timestep, value);
    }

}


