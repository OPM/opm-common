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
#include <opm/parser/eclipse/Parser/ItemSize.hpp>



namespace Opm {
  
  ItemSize::ItemSize() {
    m_sizeType  = UNSPECIFIED;
    m_sizeValue = 0;
  }


  ItemSize::ItemSize(int sizeValue) {
    m_sizeType  = ITEM_FIXED;
    m_sizeValue = sizeValue;
  }


  ItemSize::ItemSize(ItemSizeEnum sizeType) {
    m_sizeType  = sizeType;
    m_sizeValue = 0;
  }


  ItemSize::ItemSize(ItemSizeEnum sizeType, size_t sizeValue) {
    m_sizeType  = sizeType;
    m_sizeValue = sizeValue;
  }


  ItemSizeEnum ItemSize::sizeType() const{
    return m_sizeType;
  }

 
  size_t ItemSize::sizeValue() const{
    if (m_sizeType == ITEM_FIXED)
      return m_sizeValue;
    else 
      throw std::invalid_argument("Can not ask for actual size when type != ITEM_FIXED");
  }

  
}
