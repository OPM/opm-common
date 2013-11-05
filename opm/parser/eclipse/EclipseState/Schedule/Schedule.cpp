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
            initTimeMap( deck );
            initWells( deck );
        } else
            throw std::invalid_argument("Deck does not contain SCHEDULE section.\n");
    }


    void Schedule::initTimeMap(DeckConstPtr deck) {
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
    }

    
    void Schedule::initWells(DeckConstPtr deck) {
        DeckKeywordConstPtr scheduleKeyword = deck->getKeyword( "SCHEDULE" );
        size_t deckIndex = scheduleKeyword->getDeckIndex() + 1;
        size_t currentStep = 0;

        while (deckIndex < deck->size()) {
            DeckKeywordConstPtr keyword = deck->getKeyword( deckIndex );

            if (keyword->name() == "TSTEP") 
                currentStep += keyword->size();

            if (keyword->name() == "DATES") 
                currentStep += keyword->size();

            if (keyword->name() == "WELSPECS") 
                handleWELSPECS( keyword );
                
            if (keyword->name() == "WCONHIST") 
                handleWCONHIST( keyword , currentStep );

            deckIndex++;
        }
    }
    


    void Schedule::addWell(const std::string& wellName) {
        WellPtr well(new Well(wellName , m_timeMap));
        m_wells[ wellName ] = well;
    }


    void Schedule::handleWELSPECS(DeckKeywordConstPtr keyword) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord( recordNr );
            const std::string& wellName = record->getItem(0)->getString(0);
                    
            if (!hasWell(wellName))
                addWell(wellName);
                    
            {
                WellPtr well = getWell(wellName);
                well->addWELSPECS( record );
            }
        }
    }


    void Schedule::handleWCONHIST(DeckKeywordConstPtr keyword , size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord( recordNr );
            const std::string& wellName = record->getItem(0)->getString(0);
            double orat = record->getItem("ORAT")->getDouble(0);
            WellPtr well = getWell(wellName);
            
            well->setOilRate( currentStep , orat );
        }
    }



    boost::gregorian::date Schedule::getStartDate() const {
        return m_timeMap->getStartDate();
    }


    TimeMapConstPtr Schedule::getTimeMap() const {
        return m_timeMap;
    }


    size_t Schedule::numWells() const {
        return m_wells.size();
    }


    bool Schedule::hasWell(const std::string& wellName) const {
        return m_wells.find( wellName ) != m_wells.end();
    }


    WellPtr Schedule::getWell(const std::string& wellName) const {
        if (hasWell(wellName)) {
            return m_wells.at(wellName);
        }
        else
            throw std::invalid_argument("Well: " + wellName + " does not exist");
    }


}
