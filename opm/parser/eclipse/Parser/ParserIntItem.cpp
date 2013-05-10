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

    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType) : ParserItem(itemName, sizeType) {
        m_default = defaultInt();
    }

    
    ParserIntItem::ParserIntItem(const std::string& itemName, ParserItemSizeEnum sizeType, int defaultValue) : ParserItem(itemName, sizeType) {
        m_default = defaultValue;
    }


    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckIntItem object.
    /// NOTE: data are popped from the rawRecords deque!

    DeckIntItemPtr ParserIntItem::scan__(size_t expectedItems , bool scanAll , RawRecordPtr rawRecord) {
        if (sizeType() == SCALAR && expectedItems > 1)
            throw std::invalid_argument("Can only ask for one item when sizeType == SCALAR");

        {
            DeckIntItemPtr deckItem(new DeckIntItem());

            if (expectedItems) {
                std::vector<int> intsPreparedForDeckItem;
                bool defaultActive;
                
                do {
                    std::string token = rawRecord->pop_front();
                    fillIntVectorFromStringToken(token, intsPreparedForDeckItem , defaultActive);
                } while (((intsPreparedForDeckItem.size() < expectedItems) || scanAll) && (rawRecord->getItems().size() > 0U));
                
                if (intsPreparedForDeckItem.size() >= expectedItems) {
                    if (scanAll)
                        deckItem->push_back(intsPreparedForDeckItem);
                    else {
                        deckItem->push_back(intsPreparedForDeckItem , expectedItems);
                        
                        if (intsPreparedForDeckItem.size() > expectedItems) {
                            size_t offset = intsPreparedForDeckItem.size();
                            size_t extraItems = intsPreparedForDeckItem.size() - expectedItems;
                            
                            for (size_t i=1; i <= extraItems; i++) {
                                if (defaultActive)
                                    rawRecord->push_front("*");
                                else {
                                    int intValue = intsPreparedForDeckItem[ offset - i ];
                                    std::string stringValue = boost::lexical_cast<std::string>( intValue );
                                    
                                    rawRecord->push_front( stringValue );
                                }
                            }
                        }
                    }
                } else {
                    std::string preparedInts = boost::lexical_cast<std::string>(intsPreparedForDeckItem.size());
                    std::string parserSizeValue = boost::lexical_cast<std::string>(expectedItems);
                    throw std::invalid_argument("The number of parsed ints (" + preparedInts + ") did not correspond to the expected number of items:(" + parserSizeValue + ")");
                }

            }
            return deckItem;
        }
    }
    
    DeckIntItemPtr ParserIntItem::scan(size_t expectedItems, RawRecordPtr rawRecord) {
        return scan__( expectedItems , false , rawRecord);
    }

    
    DeckIntItemPtr ParserIntItem::scan(RawRecordPtr rawRecord) {
        if (sizeType() == SCALAR) 
            return scan(1U , rawRecord);
        else if (sizeType() == ALL)
            return scan__(0 , true , rawRecord);
        else
            throw std::invalid_argument("Unsupported size type, only support SCALAR. Use scan( numTokens , rawRecord) instead ");
    }


    void ParserIntItem::fillIntVectorFromStringToken(std::string token, std::vector<int>& dataVector, bool& defaultActive) {
        int multiplier = 1;
        int value = m_default;
        defaultActive = false;
        {
            bool   starStart = false;
            bool   OK = false;
            char * endPtr;
            const char * tokenPtr = token.c_str();
            
            int val1 = (int) strtol(tokenPtr , &endPtr, 10);
            
            if (strlen(endPtr)) {
                // The reading stopped at a non-integer character; now we
                // continue to see if the stopping character was a '*' which
                // might be interpreted either as a default in '*' or '5*' OR as
                // a multiplication symbol in '5*7'.
                
                if (endPtr[0] == '*') {
                    // We stopped at a star.

                    
                    if (endPtr == tokenPtr) {
                        // We have not read anything - the string starts with a '*'; for strings
                        // starting with '*' we do not accept anything following the '*'.
                        multiplier = 1;
                        starStart = true;
                    } else
                        multiplier = val1;
                    

                    endPtr++; 
                    if (strlen(endPtr)) {
                        // There is more data following after the star.
                        
                        if (!starStart) {
                            // Continue the reading after the star; if the token
                            // starts with a star we do not accept anything
                            // following after the star.
                            tokenPtr = endPtr;
                            {
                                int val2 = (int) strtol(tokenPtr , &endPtr, 10);
                                if (strlen(endPtr) == 0) {
                                    value = val2;
                                    OK = true;
                                }
                            }
                        }
                    } else {
                        // Nothing following the star; i.e. it should be
                        // interpreted as a default sign. That is OK.
                        OK = true;
                        defaultActive = true;
                    }
                }
            } else {
                // We just read the whole token in one pass: token = "100"
                value = val1;
                OK = true;
            }


            if (!OK)
                throw std::invalid_argument("std::bad_cast exception thrown, unable to cast string token (" + token +") one of forms: 'int'   '*'   'int*int'  'int*'");
            
            for (int i=0; i < multiplier; i++) 
                dataVector.push_back( value );
        }
        //boost::lexical_cast<int>(token)
    }
}
