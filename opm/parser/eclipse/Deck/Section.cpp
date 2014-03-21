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

#include <iostream>
#include <exception>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

namespace Opm {
    Section::Section(Deck& deck, const std::string& keyword ) {
        if (deck.size() > 0) {
            size_t i;
            for (i=0; i<deck.size()-1; i++)
                m_keywords.addKeyword(deck.getKeyword(i));
        }
    }

    bool Section::hasKeyword( const std::string& keyword ) const {
        return m_keywords.hasKeyword(keyword);
    }
}
