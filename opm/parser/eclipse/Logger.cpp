/*
  Copyright 2013 Statoil ASA.

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


#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
#include "Logger.hpp"

namespace Opm {
  
  /// The log levels, used to avoid clutter in the log
  const int Logger::DEBUG = 0;
  const int Logger::INFO = 1;
  const int Logger::ERROR = 2;

  std::string Logger::m_logFile = "log.log";
  int Logger::m_logLevel = INFO;
  std::ofstream Logger::m_logStream;

  void Logger::setLogLevel(int logLevel) {
    m_logLevel = logLevel;
  }

  /// Changes the default path from log.log to path
  void Logger::setPath(const std::string& path) {
    m_logFile = path;
  }

  /// Logs a debug message
  void Logger::debug(const std::string& message) {
    if (m_logLevel <= DEBUG) {
      log(message, "DEBUG");
    }
  }

  /// Logs an info message
  void Logger::info(const std::string& message) {
    if (m_logLevel <= INFO) {
      log(message, "INFO");
    }
  }

  /// Logs an error message
  void Logger::error(const std::string& message) {
    if (m_logLevel <= ERROR) {
      log(message, "ERROR");
    }
  }

  void Logger::log(const std::string& message, std::string logLevel) {
    ptime now = second_clock::universal_time();
    m_logStream << to_simple_string(now) << " " << logLevel << " " << message << "\n";
  }

  void Logger::initLogger() {
    m_logStream.open(m_logFile.c_str(), std::ios::app);
  }

  void Logger::closeLogger() {
    m_logStream.close();
  }

  Logger::~Logger() {

  }
} // namespace Opm
