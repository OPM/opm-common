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
#include <algorithm>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

namespace Opm {
    Section::Section(Deck& deck, const std::string& startKeyword, const std::vector<std::string>& stopKeywords ) {
        populateKeywords(deck, startKeyword, stopKeywords);
    }

    void Section::populateKeywords(const Deck& deck, const std::string& startKeyword,
                                   const std::vector<std::string>& stopKeywords)
    {
        size_t i;
        bool isCollecting = false;
        for (i=0; i<deck.size(); i++) {
            std::cout << deck.getKeyword(i)->name() << std::endl;
            if (!isCollecting && startKeyword.compare(deck.getKeyword(i)->name()) == 0) {
                std::cout << "Found start keyword, starting to add...." << std::endl;
                isCollecting = true;
            }
            if (std::find(stopKeywords.begin(), stopKeywords.end(), deck.getKeyword(i)->name()) != stopKeywords.end()) {
                std::cout << "Found stop keyword, quitting...." << std::endl;
                break;
            }
            if (isCollecting) {
                std::cout << "Adding...." << std::endl;
                m_keywords.addKeyword(deck.getKeyword(i));
            }
        }
    }

    bool Section::hasKeyword( const std::string& keyword ) const {
        return m_keywords.hasKeyword(keyword);
    }
}
