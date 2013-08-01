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

#include <opm/parser/eclipse/Deck/Deck.hpp>

namespace Opm {

    Deck::Deck() {
        m_keywords = KeywordContainerPtr(new KeywordContainer());
    }

    bool Deck::hasKeyword(const std::string& keyword) const {
        return m_keywords->hasKeyword(keyword);
    }
    
    void Deck::addKeyword( DeckKeywordConstPtr keyword) {
        m_keywords->addKeyword(keyword);
    }
    
    DeckKeywordConstPtr Deck::getKeyword(const std::string& keyword, size_t index) const {
        return m_keywords->getKeyword(keyword , index);
    }

}

