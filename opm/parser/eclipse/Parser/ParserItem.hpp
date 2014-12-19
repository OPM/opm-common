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
        bool scalar() const;
        void setDescription(std::string helpText);

        virtual void inlineNew(std::ostream& /* os */) const = 0;

        virtual ~ParserItem() {
        }

        virtual bool equal(const ParserItem& other) const = 0;

    protected:
        template <class T>
        bool parserRawItemEqual(const ParserItem &other) const {
            const T * lhs = dynamic_cast<const T*>(this);
            const T * rhs = dynamic_cast<const T*>(&other);
            if (!lhs || !rhs)
                return false;

            if (lhs->name() != rhs->name())
                return false;

            if (lhs->getDescription() != rhs->getDescription())
                return false;

            if (lhs->sizeType() != rhs->sizeType())
                return false;

            if (lhs->m_defaultSet != rhs->m_defaultSet)
                return false;

            // we only care that the default value is equal if it was
            // specified...
            if (lhs->m_defaultSet && lhs->getDefault() != rhs->getDefault())
                return false;

            return true;
        }

        bool m_defaultSet;

    private:
        std::string m_name;
        ParserItemSizeEnum m_sizeType;
        std::string m_description;
    };

    typedef std::shared_ptr<const ParserItem> ParserItemConstPtr;
    typedef std::shared_ptr<ParserItem> ParserItemPtr;

    /// Scans the rawRecords data according to the ParserItems definition.
    /// returns a DeckItem object.
    /// NOTE: data are popped from the rawRecords deque!
    template<typename ParserItemType , typename DeckItemType , typename ValueType>
    DeckItemPtr ParserItemScan(const ParserItemType * self , RawRecordPtr rawRecord) {
        std::shared_ptr<DeckItemType> deckItem = std::make_shared<DeckItemType>( self->name() , self->scalar() );

        if (self->sizeType() == ALL) {
            while (rawRecord->size() > 0) {
                std::string token = rawRecord->pop_front();

                std::string countString;
                std::string valueString;
                if (isStarToken(token, countString, valueString)) {
                    StarToken st(token, countString, valueString);
                    ValueType value;

                    if (st.hasValue()) {
                        value = readValueToken<ValueType>(st.valueString());
                        deckItem->push_backMultiple( value , st.count());
                    } else {
                        value = self->getDefault();
                        for (size_t i=0; i < st.count(); i++)
                            deckItem->push_backDefault( value );
                    }
                } else {
                    ValueType value = readValueToken<ValueType>(token);
                    deckItem->push_back(value);
                }
            }
        } else {
            if (rawRecord->size() == 0)
                // if the record was ended prematurely, use the default value for the item...
                deckItem->push_backDefault( self->getDefault() );
            else {
                // The '*' should be interpreted as a repetition indicator, but it must
                // be preceeded by an integer...
                std::string token = rawRecord->pop_front();
                std::string countString;
                std::string valueString;
                if (isStarToken(token, countString, valueString)) {
                    StarToken st(token, countString, valueString);

                    if (!st.hasValue())
                        deckItem->push_backDefault( self->getDefault() );
                    else
                        deckItem->push_back(readValueToken<ValueType>(st.valueString()));

                    // replace the first occurence of "N*FOO" by a sequence of N-1 times
                    // "1*FOO". this is slightly hacky, but it makes it work if the
                    // number of defaults pass item boundaries...
                    std::string singleRepetition;
                    if (st.hasValue())
                        singleRepetition = st.valueString();
                    else
                        singleRepetition = "1*";

                    for (size_t i=0; i < st.count() - 1; i++)
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

