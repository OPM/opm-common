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
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>


namespace Opm {




    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckIntItem object.
    /// NOTE: data are popped from the rawRecords deque!

    DeckIntItemPtr ParserIntItem::scan(size_t expectedItems , RawRecordPtr rawRecord) {
        if (sizeType() == SCALAR && expectedItems > 1)
            throw std::invalid_argument("Can only ask for one item when sizeType == SCALAR");

        {
            DeckIntItemPtr deckItem(new DeckIntItem());

            if (expectedItems) {
                std::vector<int> intsPreparedForDeckItem;
                
                do {
                    std::string token = rawRecord->pop_front();
                    fillIntVectorFromStringToken(token, intsPreparedForDeckItem);
                } while ((intsPreparedForDeckItem.size() < expectedItems) && (rawRecord->getItems().size() > 0U));
                
                if (intsPreparedForDeckItem.size() != expectedItems) {
                    std::string preparedInts = boost::lexical_cast<std::string>(intsPreparedForDeckItem.size());
                    std::string parserSizeValue = boost::lexical_cast<std::string>(expectedItems);
                    throw std::invalid_argument("The number of parsed ints (" + preparedInts + ") did not correspond to the expected number of items:(" + parserSizeValue + ")");
                }
                deckItem->push_back(intsPreparedForDeckItem);
            }
            return deckItem;
        }
    }
    
    
    DeckIntItemPtr ParserIntItem::scan(RawRecordPtr rawRecord) {
        if (sizeType() == SCALAR) 
            return scan(1U , rawRecord);
        else
            throw std::invalid_argument("Unsupported size type, only support SCALAR. Use scan( numTokens , rawRecord) instead ");
    }


    void ParserIntItem::fillIntVectorFromStringToken(std::string token, std::vector<int>& dataVector) {
        try {
            dataVector.push_back(boost::lexical_cast<int>(token));
        }
        catch (std::bad_cast& exception) {
            throw std::invalid_argument("std::bad_cast exception thrown, unable to cast string token (" + token +") to int. Check data.");
        }
    }

}
