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

#include <string>
#include <stdexcept>

#include <opm/parser/eclipse/Parser/ParserConst.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywordSize.hpp>

namespace Opm {

    ParserKeywordSize::ParserKeywordSize() {
        keywordSizeType = UNDEFINED;
        m_fixedSize = 0;
    }

    ParserKeywordSize::ParserKeywordSize(size_t fixedSize) {
        keywordSizeType = FIXED;
        m_fixedSize = fixedSize;
    }

    size_t ParserKeywordSize::getFixedSize() const {
        if (keywordSizeType == FIXED) {
            return m_fixedSize;
        } else
            throw std::logic_error("Only the FIXED recordSize is supported.\n");
    }

    bool ParserKeywordSize::hasFixedSize() const {
        return keywordSizeType == FIXED;
    }

    ParserKeywordSize::~ParserKeywordSize() {

    }


}

