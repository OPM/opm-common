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
    Section::Section(Deck& deck, const std::string& startKeyword ) {
        initializeStartStopKeywords();
        populateKeywords(deck, startKeyword);
    }

    void Section::populateKeywords(const Deck& deck, const std::string& startKeyword)
    {
        if (!m_startStopKeywords.count(startKeyword))
            throw std::invalid_argument("The specified keyword does correspond to a section");
        std::string stopKeyword = m_startStopKeywords.at(startKeyword);

        size_t i;
        bool isCollecting = false;
        for (i=0; i<deck.size(); i++) {
            if (!isCollecting && startKeyword.compare(deck.getKeyword(i)->name()) == 0) {
                isCollecting = true;
            }
            if (isCollecting) {
                m_keywords.addKeyword(deck.getKeyword(i));
            }
            if (deck.getKeyword(i)->name().compare(stopKeyword) == 0) {
                break;
            }
        }
    }

    void Section::initializeStartStopKeywords() {
        m_startStopKeywords.insert ( std::pair<std::string, std::string>("RUNSPEC", "GRID") );
        m_startStopKeywords.insert ( std::pair<std::string, std::string>("GRID", "EDIT") );
        m_startStopKeywords.insert ( std::pair<std::string, std::string>("EDIT", "PROPS") );
        m_startStopKeywords.insert ( std::pair<std::string, std::string>("PROPS", "REGIONS") );
        m_startStopKeywords.insert ( std::pair<std::string, std::string>("SOLUTION", "SUMMARY") );
        m_startStopKeywords.insert ( std::pair<std::string, std::string>("SUMMARY", "SCHEDULE") );
    }

    bool Section::hasKeyword( const std::string& keyword ) const {
        return m_keywords.hasKeyword(keyword);
    }
}
