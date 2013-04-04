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
#include "Parser.hpp"

namespace Opm {

    Parser::Parser() {
        initializeFixedKeywordLenghts();

    }

    RawDeckPtr Parser::parse(const std::string &path) {
        Logger::initLogger();
        Logger::setLogLevel(Logger::DEBUG);
        Logger::info("Starting parsing of file: " + path);
        RawDeckPtr rawDeck(new RawDeck(m_keywordRecordLengths));
        rawDeck->readDataIntoDeck(path);
        Logger::info("Done parsing of file: " + path);
        return rawDeck;
    }

    void Parser::initializeFixedKeywordLenghts() {
        m_keywordRecordLengths.insert(std::pair<std::string, int>("GRIDUNIT", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("INCLUDE", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("RADFIN4", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("DIMENS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("START", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("GRIDOPTS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("ENDSCALE", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("EQLOPTS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("TABDIMS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("EQLDIMS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("REGDIMS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("FAULTDIM", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("WELLDIMS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("VFPPDIMS", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("RPTSCHED", 1));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("TITLE", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("RUNSPEC", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("METRIC", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("SCHEDULE", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("SKIPREST", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("NOECHO", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("END", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("OIL", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("GAS", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("WATER", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("DISGAS", 0));
        m_keywordRecordLengths.insert(std::pair<std::string, int>("VAPOIL", 0));
    }

    Parser::~Parser() {
    }
} // namespace Opm
