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
#ifndef PARSER_ITEM_H
#define PARSER_ITEM_H

#include <string>
#include <sstream>
#include <iostream>
#include <deque>

#include <memory>
#include <boost/lexical_cast.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>

namespace Opm {
    
    class ParserItem {
    public:
        ParserItem(const std::string& itemName);
        ParserItem(const std::string& itemName, ParserItemSizeEnum sizeType);
        explicit ParserItem(const Json::JsonObject& jsonConfig);

        virtual void push_backDimension(const std::string& dimension);
        virtual const std::string& getDimension(size_t index) const;
        virtual DeckItemPtr scan(RawRecordPtr rawRecord) const = 0;
        virtual bool hasDimension() const;
        virtual size_t numDimensions() const;
        const std::string& name() const;
        ParserItemSizeEnum sizeType() const;
        std::string getDescription() const;
        bool defaultSet() const;
        bool scalar() const;
        void setDescription(std::string helpText);

        virtual bool equal(const ParserItem& other) const;
        virtual void inlineNew(std::ostream& /* os */) const {}
      
        virtual ~ParserItem() {
        }
    
    protected:
        bool m_defaultSet;

    private:
        std::string m_name;
        ParserItemSizeEnum m_sizeType;
        std::string m_description;
    };

    typedef std::shared_ptr<const ParserItem> ParserItemConstPtr;
    typedef std::shared_ptr<ParserItem> ParserItemPtr;



    template<typename T>
    bool ParserItemEqual(const T * self , const ParserItem& other) {
        const T * rhs = dynamic_cast<const T*>(&other);
        if (rhs && self->ParserItem::equal(other)) {
            if (self->defaultSet())
                return self->getDefault() == rhs->getDefault();
            return true;
        } else
              return false;
    }

        
    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckItem object.
    /// NOTE: data are popped from the rawRecords deque!
    template<typename ParserItemType , typename DeckItemType , typename ValueType>
    DeckItemPtr ParserItemScan(const ParserItemType * self , RawRecordPtr rawRecord) {
        std::shared_ptr<DeckItemType> deckItem = std::make_shared<DeckItemType>( self->name() , self->scalar() );
        
        if (self->sizeType() == ALL) {
            while (rawRecord->size() > 0) {
                std::string token = rawRecord->pop_front();
                if (tokenContainsStar( token )) {
                    StarToken<ValueType> st(token);
                    ValueType value;

                    if (st.hasValue()) {
                        value = st.value();
                        deckItem->push_backMultiple( value , st.multiplier());
                    } else {
                        value = self->getDefault();
                        for (size_t i=0; i < st.multiplier(); i++)
                            deckItem->push_backDefault( value );
                    }
                } else {
                    ValueType value = readValueToken<ValueType>(token);
                    deckItem->push_back(value);
                }
            }
        } else {
            if (rawRecord->size() == 0)
                // if the record was ended prematurely, use the default value for the
                // item...
                deckItem->push_backDefault( self->getDefault() );
            else {
                // The '*' should be interpreted as a repetition indicator, but it must
                // be preceeded by an integer...
                std::string token = rawRecord->pop_front();
                if (tokenContainsStar( token )) {
                    StarToken<ValueType> st(token);

                    if (!st.hasValue())
                        deckItem->push_backDefault( self->getDefault() );
                    else
                        deckItem->push_back(st.value());

                    // replace the first occurence of "N*FOO" by a sequence of N-1 times
                    // "1*FOO". this is slightly hacky, but it makes it work if the
                    // number of defaults pass item boundaries...
                    std::string singleRepetition;
                    if (st.hasValue())
                        singleRepetition = boost::lexical_cast<std::string>(st.value());
                    else
                        singleRepetition = "1*";

                    for (size_t i=0; i < st.multiplier() - 1; i++)
                        rawRecord->push_front(singleRepetition);
                } else {
                    ValueType value = readValueToken<ValueType>(token);
                    deckItem->push_back(value);
                }
            }
        }
        return deckItem;
    }

    

}

#endif

