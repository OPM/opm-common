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
#include <opm/parser/eclipse/EclipseState/Schedule/MessageLimits.hpp>

namespace Opm {


    MessageLimits::MessageLimits( const TimeMap& timemap ) :
        m_message_print_limit(timemap, ParserKeywords::MESSAGES::MESSAGE_PRINT_LIMIT::defaultValue),
        m_comment_print_limit(timemap, ParserKeywords::MESSAGES::COMMENT_PRINT_LIMIT::defaultValue),
        m_warning_print_limit(timemap, ParserKeywords::MESSAGES::WARNING_PRINT_LIMIT::defaultValue),
        m_problem_print_limit(timemap, ParserKeywords::MESSAGES::PROBLEM_PRINT_LIMIT::defaultValue),
        m_error_print_limit(timemap, ParserKeywords::MESSAGES::ERROR_PRINT_LIMIT::defaultValue),
        m_bug_print_limit(timemap, ParserKeywords::MESSAGES::BUG_PRINT_LIMIT::defaultValue),
        m_message_stop_limit(timemap, ParserKeywords::MESSAGES::MESSAGE_STOP_LIMIT::defaultValue),
        m_comment_stop_limit(timemap, ParserKeywords::MESSAGES::COMMENT_STOP_LIMIT::defaultValue),
        m_warning_stop_limit(timemap, ParserKeywords::MESSAGES::WARNING_STOP_LIMIT::defaultValue),
        m_problem_stop_limit(timemap, ParserKeywords::MESSAGES::PROBLEM_STOP_LIMIT::defaultValue),
        m_error_stop_limit(timemap, ParserKeywords::MESSAGES::ERROR_STOP_LIMIT::defaultValue),
        m_bug_stop_limit(timemap, ParserKeywords::MESSAGES::BUG_STOP_LIMIT::defaultValue)
        {
        }




    int MessageLimits::getMessagePrintLimit(size_t timestep) const
    {
        return m_message_print_limit.get(timestep);
    }

    int MessageLimits::getCommentPrintLimit(size_t timestep) const
    {
        return m_comment_print_limit.get(timestep);
    }

    int MessageLimits::getWarningPrintLimit(size_t timestep) const
    {
        return m_warning_print_limit.get(timestep);
    }

    int MessageLimits::getProblemPrintLimit(size_t timestep) const
    {
        return m_problem_print_limit.get(timestep);
    }

    int MessageLimits::getErrorPrintLimit(size_t timestep) const
    {
        return m_error_print_limit.get(timestep);
    }

    int MessageLimits::getBugPrintLimit(size_t timestep) const
    {
        return m_bug_print_limit.get(timestep);
    }



    void MessageLimits::setMessagePrintLimit(size_t timestep, int value)
    {
        m_message_print_limit.update(timestep, value);
    }

    void MessageLimits::setCommentPrintLimit(size_t timestep, int value)
    {
        m_comment_print_limit.update(timestep, value);
    }

    void MessageLimits::setWarningPrintLimit(size_t timestep, int value)
    {
        m_warning_print_limit.update(timestep, value);
    }

    void MessageLimits::setProblemPrintLimit(size_t timestep, int value)
    {
        m_problem_print_limit.update(timestep, value);
    }

    void MessageLimits::setErrorPrintLimit(size_t timestep, int value)
    {
        m_error_print_limit.update(timestep, value);
    }

    void MessageLimits::setBugPrintLimit(size_t timestep, int value)
    {
        m_bug_print_limit.update(timestep, value);
    }


    int MessageLimits::getMessageStopLimit(size_t timestep) const
    {
        return m_message_stop_limit.get(timestep);
    }

    int MessageLimits::getCommentStopLimit(size_t timestep) const
    {
        return m_comment_stop_limit.get(timestep);
    }

    int MessageLimits::getWarningStopLimit(size_t timestep) const
    {
        return m_warning_stop_limit.get(timestep);
    }

    int MessageLimits::getProblemStopLimit(size_t timestep) const
    {
        return m_problem_stop_limit.get(timestep);
    }

    int MessageLimits::getErrorStopLimit(size_t timestep) const
    {
        return m_error_stop_limit.get(timestep);
    }

    int MessageLimits::getBugStopLimit(size_t timestep) const
    {
        return m_bug_stop_limit.get(timestep);
    }


    void MessageLimits::setMessageStopLimit(size_t timestep, int value)
    {
        m_message_stop_limit.update(timestep, value);
    }

    void MessageLimits::setCommentStopLimit(size_t timestep, int value)
    {
        m_comment_stop_limit.update(timestep, value);
    }

    void MessageLimits::setWarningStopLimit(size_t timestep, int value)
    {
        m_warning_stop_limit.update(timestep, value);
    }

    void MessageLimits::setProblemStopLimit(size_t timestep, int value)
    {
        m_problem_stop_limit.update(timestep, value);
    }

    void MessageLimits::setErrorStopLimit(size_t timestep, int value)
    {
        m_error_stop_limit.update(timestep, value);
    }

    void MessageLimits::setBugStopLimit(size_t timestep, int value)
    {
        m_bug_stop_limit.update(timestep, value);
    }

}


