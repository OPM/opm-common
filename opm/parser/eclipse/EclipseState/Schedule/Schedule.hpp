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

#include <map>
#include <memory>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>


namespace Opm
{

    template< typename > class DynamicState;
    template< typename > class DynamicVector;

    class EclipseGrid;
    class Events;
    class Group;
    class GroupTree;
    class IOConfig;
    class OilVaporizationProperties;
    class ParseMode;
    class SCHEDULESection;
    class TimeMap;
    class Tuning;
    class Well;

    const boost::gregorian::date defaultStartDate( 1983 , boost::gregorian::Jan , 1);

    class Schedule {
    public:
        Schedule(const ParseMode& parseMode , std::shared_ptr<const EclipseGrid> grid , std::shared_ptr< const Deck > deck, std::shared_ptr< IOConfig > ioConfig);
        boost::posix_time::ptime getStartTime() const;
        std::shared_ptr< const TimeMap > getTimeMap() const;

        size_t numWells() const;
        size_t numWells(size_t timestep) const;
        size_t getMaxNumCompletionsForWells(size_t timestep) const;
        bool hasWell(const std::string& wellName) const;
        std::shared_ptr< Well > getWell(const std::string& wellName);
        std::vector<std::shared_ptr< Well >> getOpenWells(size_t timeStep);
        std::vector<std::shared_ptr< const Well >> getWells() const;
        std::vector<std::shared_ptr< const Well >> getWells(size_t timeStep) const;
        std::vector<std::shared_ptr< Well >> getWells(const std::string& wellNamePattern);
        std::shared_ptr< const OilVaporizationProperties > getOilVaporizationProperties(size_t timestep);

        std::shared_ptr< GroupTree > getGroupTree(size_t t) const;
        size_t numGroups() const;
        bool hasGroup(const std::string& groupName) const;
        std::shared_ptr< Group > getGroup(const std::string& groupName) const;
        std::shared_ptr< Tuning > getTuning() const;

        bool initOnly() const;
        const Events& getEvents() const;
        bool hasOilVaporizationProperties();
        std::shared_ptr<const Deck> getModifierDeck(size_t timeStep) const;



    private:
        std::shared_ptr< TimeMap > m_timeMap;
        OrderedMap<std::shared_ptr< Well >> m_wells;
        std::shared_ptr<const EclipseGrid> m_grid;
        std::map<std::string , std::shared_ptr< Group >> m_groups;
        std::shared_ptr<DynamicState<std::shared_ptr< GroupTree >> > m_rootGroupTree;
        std::shared_ptr<DynamicState<std::shared_ptr< OilVaporizationProperties > > > m_oilvaporizationproperties;
        std::shared_ptr<Events> m_events;
        std::shared_ptr<DynamicVector<std::shared_ptr<Deck> > > m_modifierDeck;
        std::shared_ptr< Tuning > m_tuning;
        bool nosim;


        void updateWellStatus(std::shared_ptr<Well> well, size_t reportStep , WellCommon::StatusEnum status);
        void addWellToGroup( std::shared_ptr< Group > newGroup , std::shared_ptr< Well > well , size_t timeStep);
        void initFromDeck(const ParseMode& parseMode , std::shared_ptr< const Deck > deck, std::shared_ptr< IOConfig > ioConfig);
        void initializeNOSIM(std::shared_ptr< const Deck > deck);
        void createTimeMap(std::shared_ptr< const Deck > deck);
        void initRootGroupTreeNode(std::shared_ptr< const TimeMap > timeMap);
        void initOilVaporization(std::shared_ptr< const TimeMap > timeMap);
        void iterateScheduleSection(const ParseMode& parseMode , std::shared_ptr<const SCHEDULESection> section, std::shared_ptr< IOConfig > ioConfig);
        bool handleGroupFromWELSPECS(const std::string& groupName, std::shared_ptr< GroupTree > newTree) const;
        void addGroup(const std::string& groupName , size_t timeStep);
        void addWell(const std::string& wellName, std::shared_ptr< const DeckRecord > record, size_t timeStep, WellCompletion::CompletionOrderEnum wellCompletionOrder);
        void handleCOMPORD(const ParseMode& parseMode, std::shared_ptr<const DeckKeyword> compordKeyword, size_t currentStep);
        void checkWELSPECSConsistency(std::shared_ptr< const Well > well, std::shared_ptr< const DeckKeyword > keyword, size_t recordIdx) const;
        void handleWELSPECS(std::shared_ptr<const SCHEDULESection> section, std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWCONProducer(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep, bool isPredictionMode);
        void handleWCONHIST(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWCONPROD(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWGRUPCON(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleCOMPDAT(std::shared_ptr< const DeckKeyword > keyword,  size_t currentStep);
        void handleWELSEGS(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleCOMPSEGS(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWCONINJE(std::shared_ptr<const SCHEDULESection> section, std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWPOLYMER(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWSOLVENT(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWCONINJH(std::shared_ptr<const SCHEDULESection> section, std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWELOPEN(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep, bool hascomplump);
        void handleWELTARG(std::shared_ptr<const SCHEDULESection> section, std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleGCONINJE(std::shared_ptr<const SCHEDULESection> section, std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleGCONPROD(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleGEFAC(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleTUNING(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleNOSIM();
        void handleRPTRST(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep, std::shared_ptr< IOConfig > ioConfig);
        void handleRPTSCHED(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep, std::shared_ptr< IOConfig > ioConfig);
        void handleDATES(std::shared_ptr< const DeckKeyword > keyword);
        void handleTSTEP(std::shared_ptr< const DeckKeyword > keyword);
        void handleGRUPTREE(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWRFT(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWRFTPLT(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleWPIMULT(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleDRSDT(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleDRVDT(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);
        void handleVAPPARS(std::shared_ptr< const DeckKeyword > keyword, size_t currentStep);

        void checkUnhandledKeywords(std::shared_ptr<const SCHEDULESection> section) const;

        static double convertInjectionRateToSI(double rawRate, WellInjector::TypeEnum wellType, const Opm::UnitSystem &unitSystem);
        static double convertInjectionRateToSI(double rawRate, Phase::PhaseEnum wellPhase, const Opm::UnitSystem &unitSystem);
        static bool convertEclipseStringToBool(const std::string& eclipseString);


        void setOilVaporizationProperties(const std::shared_ptr< OilVaporizationProperties > vapor, size_t timestep);

    };
    typedef std::shared_ptr<Schedule> SchedulePtr;
    typedef std::shared_ptr<const Schedule> ScheduleConstPtr;
}

#endif
