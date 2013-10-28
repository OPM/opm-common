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

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>


namespace Opm 
{
    Schedule::Schedule(DeckConstPtr deck) {
        if (deck->hasKeyword("SCHEDULE")) {
            boost::gregorian::date startDate( defaultStartDate );
            if (deck->hasKeyword("START")) {
                DeckKeywordConstPtr startKeyword = deck->getKeyword("START");
                startDate = TimeMap::dateFromEclipse( startKeyword->getRecord(0));
            }

            m_timeMap = TimeMapPtr(new TimeMap(startDate));
            
            DeckKeywordConstPtr scheduleKeyword = deck->getKeyword( "SCHEDULE" );
            size_t deckIndex = scheduleKeyword->getDeckIndex() + 1;
            while (deckIndex < deck->size()) {
                DeckKeywordConstPtr keyword = deck->getKeyword( deckIndex );

                if (keyword->name() == "DATES")
                    m_timeMap->addFromDATESKeyword( keyword );
                else if (keyword->name() == "TSTEP")
                    m_timeMap->addFromTSTEPKeyword( keyword );
                
                deckIndex++;
            }
        } else
            throw std::invalid_argument("Deck does not contain SCHEDULE section.\n");
    }


    boost::gregorian::date Schedule::getStartDate() const {
        return m_timeMap->getStartDate();
    }


    TimeMapConstPtr Schedule::getTimeMap() const {
        return m_timeMap;
    }

}
