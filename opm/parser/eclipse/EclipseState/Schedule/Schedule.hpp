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
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <memory>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <map>

namespace Opm
{

    const boost::gregorian::date defaultStartDate( 1983 , boost::gregorian::Jan , 1);

    class Schedule {
    public:
        Schedule(std::shared_ptr<const EclipseGrid> grid , DeckConstPtr deck);
        boost::posix_time::ptime getStartTime() const
        { return m_timeMap->getStartTime(/*timeStepIdx=*/0); }
        TimeMapConstPtr getTimeMap() const;

        size_t numWells() const;
        size_t numWells(size_t timestep) const;
        size_t getMaxNumCompletionsForWells(size_t timestep) const;
        bool hasWell(const std::string& wellName) const;
        WellPtr getWell(const std::string& wellName) const;
        std::vector<WellConstPtr> getWells() const;
        std::vector<WellConstPtr> getWells(size_t timeStep) const;
        std::vector<WellPtr> getWells(const std::string& wellNamePattern) const;

        GroupTreePtr getGroupTree(size_t t) const;
        size_t numGroups() const;
        bool hasGroup(const std::string& groupName) const;
        GroupPtr getGroup(const std::string& groupName) const;
        TuningPtr getTuning() const;



    private:
        TimeMapPtr m_timeMap;
        OrderedMap<WellPtr> m_wells;
        std::shared_ptr<const EclipseGrid> m_grid;
        std::map<std::string , GroupPtr> m_groups;
        std::shared_ptr<DynamicState<GroupTreePtr> > m_rootGroupTree;
        TuningPtr m_tuning;

        void addWellToGroup( GroupPtr newGroup , WellPtr well , size_t timeStep);
        void initFromDeck(DeckConstPtr deck);
        void createTimeMap(DeckConstPtr deck);
        void initRootGroupTreeNode(TimeMapConstPtr timeMap);
        void iterateScheduleSection(DeckConstPtr deck);
        bool handleGroupFromWELSPECS(const std::string& groupName, GroupTreePtr newTree) const;
        void addGroup(const std::string& groupName , size_t timeStep);
        void addWell(const std::string& wellName, DeckRecordConstPtr record, size_t timeStep);
        void checkWELSPECSConsistency(WellConstPtr well, DeckKeywordConstPtr keyword, size_t recordIdx) const;
        void handleWELSPECS(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWCONProducer(DeckKeywordConstPtr keyword, size_t currentStep, bool isPredictionMode);
        void handleWCONHIST(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWCONPROD(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWGRUPCON(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleCOMPDAT(DeckKeywordConstPtr keyword,  size_t currentStep);
        void handleWCONINJE(DeckConstPtr deck, DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWPOLYMER(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWCONINJH(DeckConstPtr deck, DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWELOPEN(DeckKeywordConstPtr keyword, size_t currentStep, bool hascomplump);
        void handleWELTARG(DeckConstPtr deck, DeckKeywordConstPtr keyword, size_t currentStep);
        void handleGCONINJE(DeckConstPtr deck, DeckKeywordConstPtr keyword, size_t currentStep);
        void handleGCONPROD(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleTUNING(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleDATES(DeckKeywordConstPtr keyword);
        void handleTSTEP(DeckKeywordConstPtr keyword);
        void handleGRUPTREE(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWRFT(DeckKeywordConstPtr keyword, size_t currentStep);
        void handleWRFTPLT(DeckKeywordConstPtr keyword, size_t currentStep);

        void checkUnhandledKeywords(DeckConstPtr deck) const;

        static double convertInjectionRateToSI(double rawRate, WellInjector::TypeEnum wellType, const Opm::UnitSystem &unitSystem);
        static double convertInjectionRateToSI(double rawRate, Phase::PhaseEnum wellPhase, const Opm::UnitSystem &unitSystem);
        static bool convertEclipseStringToBool(const std::string& eclipseString);

    };
    typedef std::shared_ptr<Schedule> SchedulePtr;
    typedef std::shared_ptr<const Schedule> ScheduleConstPtr;
}

#endif
