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

#ifndef RECORD_ITEM_SIZE_H
#define RECORD_ITEM_SIZE_H

#include <boost/shared_ptr.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

namespace Opm {

  class ItemSize {

  private:
    ItemSizeEnum m_sizeType;
    size_t       m_sizeValue;
    
  public:
    ItemSize();
    ItemSize(int sizeValue);
    ItemSize(ItemSizeEnum sizeType);
    ItemSize(ItemSizeEnum sizeType, size_t sizeValue);
    
    ItemSizeEnum sizeType() const;
    size_t sizeValue() const;
  };
  typedef boost::shared_ptr<ItemSize> ItemSizePtr;
  typedef boost::shared_ptr<const ItemSize> ItemSizeConstPtr;
}

#endif
