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
#ifndef SCHEDULE_HPP
#define SCHEDULE_HPP


#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>


#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <map>

namespace Opm 
{
    
    const boost::gregorian::date defaultStartDate( 1983 , boost::gregorian::Jan , 1);

    class Schedule {
    public:
        Schedule(DeckConstPtr deck);
        boost::gregorian::date getStartDate() const;
        TimeMapConstPtr getTimeMap() const;

        size_t numWells() const;
        bool hasWell(const std::string& wellName) const;
        WellPtr getWell(const std::string& wellName) const;
        GroupTreePtr getGroupTree(size_t t) const;
  
    private:
        TimeMapPtr m_timeMap;
        std::map<std::string , WellPtr> m_wells;
        boost::shared_ptr<DynamicState<GroupTreePtr> > m_rootGroupTree;

        void initFromDeck(DeckConstPtr deck);
        void createTimeMap(DeckConstPtr deck);
        void initRootGroupTreeNode(TimeMapConstPtr timeMap);
        void iterateScheduleSection(DeckConstPtr deck);
        bool handleGroupFromWELSPECS(const std::string& groupName, GroupTreePtr newTree) const;

        void addWell(const std::string& wellName);
        void handleWELSPECS(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWELSPECS(DeckKeywordConstPtr keyword);
        void handleWCONProducer(DeckKeywordConstPtr keyword, size_t currentStep, bool isPredictionMode);
        void handleWCONHIST(DeckKeywordConstPtr keyword , size_t currentStep);
        void handleWCONPROD(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleCOMPDAT(DeckKeywordConstPtr keyword , size_t currentStep);
        void handleWCONINJE(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWCONINJH(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleDATES(DeckKeywordConstPtr keyword);
        void handleTSTEP(DeckKeywordConstPtr keyword);
        void handleGRUPTREE(DeckKeywordConstPtr keyword, size_t currentStep);

    };
    typedef boost::shared_ptr<Schedule> SchedulePtr;
    typedef boost::shared_ptr<const Schedule> ScheduleConstPtr;
}

#endif
