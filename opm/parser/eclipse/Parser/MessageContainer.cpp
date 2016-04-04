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


    void MessageContainer::error( const std::string& msg,
                                  const std::string& filename,
                                  const int lineno ) {
        m_messages.emplace_back( Message {
            MessageType::MessageTypeEnum::Error, msg,
            { filename, lineno } }
        );
    }


    void MessageContainer::error( const std::string& msg ) {
        m_messages.emplace_back(
            Message { MessageType::MessageTypeEnum::Error, msg, {} }
        );
    }


    void MessageContainer::bug( const std::string& msg,
                                const std::string& filename,
                                const int lineno ) {
        m_messages.emplace_back( Message {
                MessageType::MessageTypeEnum::Bug, msg,
                { filename, lineno }
            }
        );
    }


    void MessageContainer::bug( const std::string& msg ) {
        m_messages.emplace_back(
            Message { MessageType::MessageTypeEnum::Bug, msg,{} }
        );
    }


    void MessageContainer::warning( const std::string& msg,
                                    const std::string& filename,
                                    const int lineno ) {
        m_messages.emplace_back( Message {
                MessageType::MessageTypeEnum::Warning, msg,
                { filename, lineno }
            }
        );
    }


    void MessageContainer::warning( const std::string& msg ) {
        m_messages.emplace_back(
            Message { MessageType::MessageTypeEnum::Warning, msg,{} }
        );
    }



    void MessageContainer::info( const std::string& msg,
                                 const std::string& filename,
                                 const int lineno ) {
        m_messages.emplace_back( Message {
                MessageType::MessageTypeEnum::Info, msg,
                { filename, lineno }
            }
        );
    }


    void MessageContainer::info( const std::string& msg ) {
        m_messages.emplace_back(
            Message { MessageType::MessageTypeEnum::Info, msg,{} }
        );
    }


    void MessageContainer::debug( const std::string& msg,
                                  const std::string& filename,
                                  const int lineno ) {
        m_messages.emplace_back( Message {
                MessageType::MessageTypeEnum::Debug, msg,
                { filename, lineno }
            }
        );
    }


    void MessageContainer::debug( const std::string& msg ) {
        m_messages.emplace_back(
            Message { MessageType::MessageTypeEnum::Debug, msg,{} }
        );
    }


    void MessageContainer::problem( const std::string& msg,
                                    const std::string& filename,
                                    const int lineno ) {
        m_messages.emplace_back( Message {
                MessageType::MessageTypeEnum::Problem, msg,
                { filename, lineno }
            }
        );
    }


    void MessageContainer::problem( const std::string& msg ) {
        m_messages.emplace_back(
            Message { MessageType::MessageTypeEnum::Problem, msg, {} }
        );
    }

    void MessageContainer::add( const Message& msg ) {
        this->m_messages.push_back( msg );
    }

    void MessageContainer::add( Message&& msg ) {
        this->m_messages.push_back( std::move( msg ) );
    }


    MessageContainer::const_iterator MessageContainer::begin() const {
        return m_messages.begin();
    }

    MessageContainer::const_iterator MessageContainer::end() const {
        return m_messages.end();
    }

} // namespace Opm
