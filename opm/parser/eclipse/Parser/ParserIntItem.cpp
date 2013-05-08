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

#include <boost/lexical_cast.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>


namespace Opm {

    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckIntItem object.
    /// NOTE: data are popped from the rawRecords deque!

    DeckIntItemPtr ParserIntItem::scan(RawRecordPtr rawRecord) {
        DeckIntItemPtr deckItem(new DeckIntItem());

        if (size()->sizeType() == ITEM_FIXED) {
            std::string token = rawRecord->pop_front();
            std::vector<int> intsFromCurrentToken;
            fillIntVector(token, intsFromCurrentToken);
            deckItem->push_back(intsFromCurrentToken);
        }

        return deckItem;
    }
    
    void ParserIntItem::fillIntVector(std::string token, std::vector<int>& intsFromCurrentToken) {
        intsFromCurrentToken.push_back(boost::lexical_cast<int>(token));
    }
}
