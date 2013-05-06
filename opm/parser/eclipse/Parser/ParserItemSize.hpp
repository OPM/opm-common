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

#ifndef PARSER_ITEM_SIZE_H
#define PARSER_ITEM_SIZE_H

#include <boost/shared_ptr.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

namespace Opm {

    class ParserItemSize {
    private:
        ParserItemSizeEnum m_sizeType;
        size_t m_sizeValue;

    public:
        ParserItemSize();
        ParserItemSize(int sizeValue);
        ParserItemSize(ParserItemSizeEnum sizeType);
        ParserItemSize(ParserItemSizeEnum sizeType, size_t sizeValue);

        ParserItemSizeEnum sizeType() const;
        size_t sizeValue() const;
    };
    typedef boost::shared_ptr<ParserItemSize> ParserItemSizePtr;
    typedef boost::shared_ptr<const ParserItemSize> ParserItemSizeConstPtr;
}

#endif
