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

#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>
#include <opm/parser/eclipse/OpmLog/Logger.hpp>
#include <opm/parser/eclipse/OpmLog/StreamLog.hpp>

namespace Opm {


    std::shared_ptr<Logger> OpmLog::getLogger() {
        if (!m_logger)
            m_logger.reset( new Logger() );

        return m_logger;
    }


    std::shared_ptr<StreamLog> OpmLog::getStreamLog(const std::string& filename) {
        if (!m_streamLog) {
            m_streamLog.reset(new StreamLog(filename, Log::DefaultMessageTypes));
        }
        m_info = 0;
        m_error = 0;
        m_bug = 0;
        m_problem = 0;
        m_warning = 0;
    }


    void OpmLog::addMessage(int64_t messageFlag , const std::string& message) {
        if (m_logger)
            m_logger->addMessage( messageFlag , message );
    }


    void OpmLog::info(const std::string& message)
    {
        const std::string msg = Log::prefixMessage(Log::MessageType::Info, message);
        m_streamLog->addMessage(Log::MessageType::Info, msg);
        m_info += 1;
    }


    void OpmLog::warning(const std::string& message)
    {
        const std::string msg = Log::prefixMessage(Log::MessageType::Warning, message);
        m_streamLog->addMessage(Log::MessageType::Warning, msg);
        m_warning += 1;
    }


    void OpmLog::problem(const std::string& message)
    {
        const std::string msg = Log::prefixMessage(Log::MessageType::Problem, message);
        m_streamLog->addMessage(Log::MessageType::Problem, msg);
        m_problem += 1;
    }


    void OpmLog::error(const std::string& message)
    {
        const std::string msg = Log::prefixMessage(Log::MessageType::Error, message);
        m_streamLog->addMessage(Log::MessageType::Error, msg);
        m_error += 1;
    }


    void OpmLog::bug(const std::string& message)
    {
        const std::string msg = Log::prefixMessage(Log::MessageType::Bug, message);
        m_streamLog->addMessage(Log::MessageType::Bug, msg);
        m_bug += 1;
    }


    void OpmLog::summary()
    {
        const std::string summary_msg = "\n\nError summary:" + 
            std::string("\nWarnings          " + std::to_string(m_warning)) +
            std::string("\nProblems          " + std::to_string(m_problem)) +
            std::string("\nErrors            " + std::to_string(m_error)) + 
            std::string("\nBugs              " + std::to_string(m_bug))+ "\n";
            std::string("\nInfo              " + std::to_string(m_info))+ "\n";

        m_streamLog->addMessage(Log::MessageType::Info, summary_msg);
    }

    
    bool OpmLog::enabledMessageType( int64_t messageType ) {
        if (m_logger)
            return m_logger->enabledMessageType( messageType );
        else
            return Logger::enabledDefaultMessageType( messageType );
    }

    bool OpmLog::hasBackend(const std::string& name) {
        if (m_logger)
            return m_logger->hasBackend( name );
        else
            return false;
    }


    bool OpmLog::removeBackend(const std::string& name) {
        if (m_logger)
            return m_logger->removeBackend( name );
        else
            return false;
    }


    void OpmLog::addMessageType( int64_t messageType , const std::string& prefix) {
        auto logger = OpmLog::getLogger();
        logger->addMessageType( messageType , prefix );
    }


    void OpmLog::addBackend(const std::string& name , std::shared_ptr<LogBackend> backend) {
        auto logger = OpmLog::getLogger();
        return logger->addBackend( name , backend );
    }


/******************************************************************/

    std::shared_ptr<Logger> OpmLog::m_logger;
    std::shared_ptr<StreamLog> OpmLog::m_streamLog;
    int OpmLog::m_info;
    int OpmLog::m_error;
    int OpmLog::m_bug;
    int OpmLog::m_problem;
    int OpmLog::m_warning;
}
