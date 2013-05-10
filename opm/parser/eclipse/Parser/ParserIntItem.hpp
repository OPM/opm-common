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


#ifndef PARSERINTITEM_HPP
#define PARSERINTITEM_HPP

#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>


namespace Opm {

    class ParserIntItem : public ParserItem {
    public:
        ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType);
        ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType, int defaultValue);

        DeckIntItemPtr scan(size_t expectedItems , RawRecordPtr rawRecord);
        DeckIntItemPtr scan(RawRecordPtr rawRecord);
        
        int getDefault() const {
            return m_default;
        }

    private:
        void fillIntVectorFromStringToken(std::string token, std::vector<int>& intsFromCurrentToken, bool& defaultActive);
        DeckIntItemPtr scan__(size_t expectedItems , bool scanAll , RawRecordPtr rawRecord);
        int  m_default;
    };
}

#endif  /* PARSERINTITEM_HPP */

