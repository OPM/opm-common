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

#include <utility>
#include "KeywordDataToken.hpp"
namespace Opm {

    KeywordDataToken::KeywordDataToken() {
    }
    
    KeywordDataToken::KeywordDataToken(const std::string& keyword, const std::list<std::string>& dataElements) {
        setKeyword(keyword);
        m_data = dataElements;
    }
    
    KeywordDataToken::KeywordDataToken(const std::string& keyword) {
        m_keyword = keyword;
    }

    void KeywordDataToken::setKeyword(const std::string& keyword) {
        m_keyword = keyword;
    }
    
    void KeywordDataToken::addDataElement(const std::string& element) {
        m_data.push_back(element);
    }

    std::string KeywordDataToken::getKeyword() {
        return m_keyword;
    }
    
    KeywordDataToken::~KeywordDataToken() {
    }
}

