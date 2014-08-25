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

#include <stdexcept>

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
    bool DeckItem::setInDeck() const {
        if ((m_valueStatus & DeckValue::SET_IN_DECK) == DeckValue::SET_IN_DECK)
            return true;
        else
            return false;
    }


    bool DeckItem::defaultApplied() const {
        if ((m_valueStatus & DeckValue::DEFAULT) == DeckValue::DEFAULT)
            return true;
        else
            return false;
    }

    
    void DeckItem::assertValueSet() const {
        if (m_valueStatus == DeckValue::NOT_SET)
            throw std::invalid_argument("Trying to access property: " + name() + " which has not been properly set - either explicitly or with a default.");
    }


}
