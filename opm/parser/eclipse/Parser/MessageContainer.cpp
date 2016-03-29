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

#include <opm/parser/eclipse/Parser/MessageContainer.hpp>

#include <memory>


namespace Opm {


    void MessageContainer::error(const std::string& msg, const std::string& filename, const int lineno)
    {
        std::unique_ptr<Location> loc(new Location{filename, lineno});
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Error, msg, std::move(loc)});
    }


    void MessageContainer::error(const std::string& msg)
    {
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Error, msg, nullptr});
    }


    void MessageContainer::bug(const std::string& msg, const std::string& filename, const int lineno)
    {
        std::unique_ptr<Location> loc(new Location{filename, lineno});
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Bug, msg, std::move(loc)});
    }


    void MessageContainer::bug(const std::string& msg)
    {
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Bug, msg, nullptr});
    }


    void MessageContainer::warning(const std::string& msg, const std::string& filename, const int lineno)
    {
        std::unique_ptr<Location> loc(new Location{filename, lineno});
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Warning, msg, std::move(loc)});
    }


    void MessageContainer::warning(const std::string& msg)
    {
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Warning, msg, nullptr});
    }



    void MessageContainer::info(const std::string& msg, const std::string& filename, const int lineno)
    {
        std::unique_ptr<Location> loc(new Location{filename, lineno});
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Info, msg, std::move(loc)});
    }


    void MessageContainer::info(const std::string& msg)
    {
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Info, msg, nullptr});
    }


    void MessageContainer::debug(const std::string& msg, const std::string& filename, const int lineno)
    {
        std::unique_ptr<Location> loc(new Location{filename, lineno});
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Debug, msg, std::move(loc)});
    }


    void MessageContainer::debug(const std::string& msg)
    {
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Debug, msg, nullptr});
    }


    void MessageContainer::problem(const std::string& msg, const std::string& filename, const int lineno)
    {
        std::unique_ptr<Location> loc(new Location{filename, lineno});
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Problem, msg, std::move(loc)});
    }


    void MessageContainer::problem(const std::string& msg)
    {
        m_messages.emplace_back(Message{MessageType::MessageTypeEnum::Problem, msg, nullptr});
    }



    std::vector<Message>::const_iterator MessageContainer::begin() const
    {
        return m_messages.begin();
    }


    std::vector<Message>::const_iterator MessageContainer::end() const
    {
        return m_messages.end();
    }

} // namespace Opm
