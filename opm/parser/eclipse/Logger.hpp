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


#ifndef LOGGER_HPP
#define	LOGGER_HPP
#include <string>
#include <fstream>

namespace Opm {

    class Logger {
    public:
        static const int DEBUG;
        static const int INFO;
        static const int ERROR;
        
        static void initLogger();
        static void closeLogger();
        static void setLogLevel(int logLevel);
        static void setPath(const std::string& path);
        static void debug(const std::string& message);
        static void info(const std::string& message);
        static void error(const std::string& message);
        virtual ~Logger();
    private:
        static std::string m_logFile;
        static std::ofstream m_logStream;
        static int m_logLevel;
        static void log(const std::string& message, std::string logLevel);
        static void initLoggingConstants();
    };
}// namespace Opm
#endif	/* LOGGER_HPP */

