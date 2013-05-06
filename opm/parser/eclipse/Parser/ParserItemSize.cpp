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

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserItemSize.hpp>



namespace Opm {

    ParserItemSize::ParserItemSize() {
        m_sizeType = UNSPECIFIED;
        m_sizeValue = 0;
    }

    ParserItemSize::ParserItemSize(int sizeValue) {
        m_sizeType = ITEM_FIXED;
        m_sizeValue = sizeValue;
    }

    ParserItemSize::ParserItemSize(ParserItemSizeEnum sizeType) {
        m_sizeType = sizeType;
        m_sizeValue = 0;
    }

    ParserItemSize::ParserItemSize(ParserItemSizeEnum sizeType, size_t sizeValue) {
        if (sizeType == ITEM_BOX || sizeType == UNSPECIFIED)
            throw std::invalid_argument("Cannot combine ITEM_BOX/UNSPECIFIED with explicit size value");
        else {
            m_sizeType = sizeType;
            m_sizeValue = sizeValue;
        }
    }

    ParserItemSizeEnum ParserItemSize::sizeType() const {
        return m_sizeType;
    }

    size_t ParserItemSize::sizeValue() const {
        if (m_sizeType == ITEM_FIXED)
            return m_sizeValue;
        else
            throw std::invalid_argument("Can not ask for actual size when type != ITEM_FIXED");
    }
}
