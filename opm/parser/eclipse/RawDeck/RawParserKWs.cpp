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
#include <stdexcept>
#include "RawParserKWs.hpp"

namespace Opm {

  RawParserKWs::RawParserKWs() {
    initializeFixedKeywordLenghts();
  }

  bool RawParserKWs::keywordExists(const std::string& keyword) const {
    return m_keywordRecordLengths.find(keyword) != m_keywordRecordLengths.end();
  }

  unsigned int RawParserKWs::getFixedNumberOfRecords(const std::string& keyword) const {
    if (keywordExists(keyword)) {
      return m_keywordRecordLengths.find(keyword) -> second;
    } else
      throw std::invalid_argument("Given keyword is not found, offending keyword: " + keyword);
  }

  void RawParserKWs::add(std::pair<std::string, unsigned int> keywordAndNumRecords) {
    m_keywordRecordLengths.insert(keywordAndNumRecords);
  }

  void RawParserKWs::initializeFixedKeywordLenghts() {
    add(std::pair<std::string, unsigned int>("GRIDUNIT", 1));
    add(std::pair<std::string, unsigned int>("INCLUDE", 1));
    add(std::pair<std::string, unsigned int>("RADFIN4", 1));
    add(std::pair<std::string, unsigned int>("DIMENS", 1));
    add(std::pair<std::string, unsigned int>("START", 1));
    add(std::pair<std::string, unsigned int>("GRIDOPTS", 1));
    add(std::pair<std::string, unsigned int>("ENDSCALE", 1));
    add(std::pair<std::string, unsigned int>("EQLOPTS", 1));
    add(std::pair<std::string, unsigned int>("TABDIMS", 1));
    add(std::pair<std::string, unsigned int>("EQLDIMS", 1));
    add(std::pair<std::string, unsigned int>("REGDIMS", 1));
    add(std::pair<std::string, unsigned int>("FAULTDIM", 1));
    add(std::pair<std::string, unsigned int>("WELLDIMS", 1));
    add(std::pair<std::string, unsigned int>("VFPPDIMS", 1));
    add(std::pair<std::string, unsigned int>("RPTSCHED", 1));
    add(std::pair<std::string, unsigned int>("WHISTCTL", 1));


    add(std::pair<std::string, unsigned int>("TITLE", 0));
    add(std::pair<std::string, unsigned int>("RUNSPEC", 0));
    add(std::pair<std::string, unsigned int>("METRIC", 0));
    add(std::pair<std::string, unsigned int>("SCHEDULE", 0));
    add(std::pair<std::string, unsigned int>("SKIPREST", 0));
    add(std::pair<std::string, unsigned int>("NOECHO", 0));
    add(std::pair<std::string, unsigned int>("END", 0));
    add(std::pair<std::string, unsigned int>("OIL", 0));
    add(std::pair<std::string, unsigned int>("GAS", 0));
    add(std::pair<std::string, unsigned int>("WATER", 0));
    add(std::pair<std::string, unsigned int>("DISGAS", 0));
    add(std::pair<std::string, unsigned int>("VAPOIL", 0));
  }

  RawParserKWs::~RawParserKWs() {
  }
}

