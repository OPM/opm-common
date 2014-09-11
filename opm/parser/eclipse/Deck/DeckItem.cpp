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

#include <opm/parser/eclipse/Deck/DeckItem.hpp>

#include <boost/lexical_cast.hpp>

#include <stdexcept>
#include <cassert>

namespace Opm {

    DeckItem::DeckItem(const std::string& name_, bool scalar) {
        m_name = name_;
        m_scalar = scalar;
        m_valueStatus = DeckValue::NOT_SET;
    }

    const std::string& DeckItem::name() const {
        return m_name;
    }

    /**
       This function will return true if the item has been explicitly
       set in the deck.
    */
    bool DeckItem::wasSetInDeck(size_t /*index*/) const {
        return (m_valueStatus & DeckValue::SET_IN_DECK) == DeckValue::SET_IN_DECK;
    }

    bool DeckItem::defaultApplied(size_t index) const {
        assert(m_dataPointDefaulted.size() == size());
        assertSize(index);

        return m_dataPointDefaulted[index];
    }

    void DeckItem::assertSize(size_t index) const {
        if (index >= size())
            throw std::out_of_range("Index must be smaller than "
                                    + boost::lexical_cast<std::string>(size())
                                    + " but is "
                                    + boost::lexical_cast<std::string>(index));
    }


}
