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

#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MessageLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPInjTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellTestConfig.hpp>

namespace Opm
{

    class Deck;
    class DeckKeyword;
    class DeckRecord;
    class EclipseGrid;
    class Eclipse3DProperties;
    class SCHEDULESection;
    class TimeMap;
    class UnitSystem;
    class EclipseState;

    class Schedule {
    public:
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const Eclipse3DProperties& eclipseProperties,
                 const Phases &phases,
                 const ParseContext& parseContext = ParseContext());

        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext = ParseContext());

        /*
         * If the input deck does not specify a start time, Eclipse's 1. Jan
         * 1983 is defaulted
         */
        time_t getStartTime() const;
        time_t posixStartTime() const;
        time_t posixEndTime() const;


        const TimeMap& getTimeMap() const;

        size_t numWells() const;
        size_t numWells(size_t timestep) const;
        size_t getMaxNumConnectionsForWells(size_t timestep) const;
        bool hasWell(const std::string& wellName) const;
        const Well* getWell(const std::string& wellName) const;
        std::vector< const Well* > getOpenWells(size_t timeStep) const;
        std::vector< const Well* > getWells() const;
        std::vector< const Well* > getWells(size_t timeStep) const;

        /*
          The overload with a group name argument will return all
          wells beneath that particular group; i.e.

             getWells("FIELD",t);

          is an inefficient way to get all the wells defined at time
          't'.
        */ 
	//std::vector< const Group& > getChildGroups(const std::string& group_name, size_t timeStep) const;
        std::vector< const Group* > getChildGroups(const std::string& group_name, size_t timeStep) const;
        std::vector< const Well* > getWells(const std::string& group, size_t timeStep) const;
	std::vector< const Well* > getChildWells(const std::string& group_name, size_t timeStep) const;
        std::vector< const Well* > getWellsMatching( const std::string& ) const;
        const OilVaporizationProperties& getOilVaporizationProperties(size_t timestep) const;

        const WellTestConfig& wtestConfig(size_t timestep) const;

        const GroupTree& getGroupTree(size_t t) const;
        size_t numGroups() const;
	size_t numGroups(size_t timeStep) const;
        bool hasGroup(const std::string& groupName) const;
        const Group& getGroup(const std::string& groupName) const;
        std::vector< const Group* > getGroups() const;
	std::vector< const Group* > getGroups(size_t timeStep) const;
        const Tuning& getTuning() const;
        const MessageLimits& getMessageLimits() const;
        void invalidNamePattern (const std::string& namePattern, const ParseContext& parseContext, const DeckKeyword& keyword) const;

        const Events& getEvents() const;
        const Deck& getModifierDeck(size_t timeStep) const;
        bool hasOilVaporizationProperties() const;
        const VFPProdTable& getVFPProdTable(int table_id, size_t timeStep) const;
        const VFPInjTable& getVFPInjTable(int table_id, size_t timeStep) const;
        std::map<int, std::shared_ptr<const VFPProdTable> > getVFPProdTables(size_t timeStep) const;
        std::map<int, std::shared_ptr<const VFPInjTable> > getVFPInjTables(size_t timeStep) const;
        /*
          Will remove all completions which are connected to cell which is not
          active. Will scan through all wells and all timesteps.
        */
        void filterConnections(const EclipseGrid& grid);

    private:
        TimeMap m_timeMap;
        OrderedMap< Well > m_wells;
        OrderedMap< Group > m_groups;
        DynamicState< GroupTree > m_rootGroupTree;
        DynamicState< OilVaporizationProperties > m_oilvaporizationproperties;
        Events m_events;
        DynamicVector< Deck > m_modifierDeck;
        Tuning m_tuning;
        MessageLimits m_messageLimits;
        Phases m_phases;
        std::map<int, DynamicState<std::shared_ptr<VFPProdTable>>> vfpprod_tables;
        std::map<int, DynamicState<std::shared_ptr<VFPInjTable>>> vfpinj_tables;
        DynamicState<std::shared_ptr<WellTestConfig>> wtest_config;

        WellProducer::ControlModeEnum m_controlModeWHISTCTL;

        std::vector< Well* > getWells(const std::string& wellNamePattern);
        std::vector< Group* > getGroups(const std::string& groupNamePattern);

        void updateWellStatus( Well& well, size_t reportStep , WellCommon::StatusEnum status);
        void addWellToGroup( Group& newGroup , Well& well , size_t timeStep);
        void iterateScheduleSection(const ParseContext& parseContext ,  const SCHEDULESection& , const EclipseGrid& grid,
                                    const Eclipse3DProperties& eclipseProperties);
        bool handleGroupFromWELSPECS(const std::string& groupName, GroupTree& newTree) const;
        void addGroup(const std::string& groupName , size_t timeStep);
        void addWell(const std::string& wellName, const DeckRecord& record, size_t timeStep, WellCompletion::CompletionOrderEnum wellCompletionOrder);
        void handleCOMPORD(const ParseContext& parseContext, const DeckKeyword& compordKeyword, size_t currentStep);
        void handleWELSPECS( const SCHEDULESection&, size_t, size_t  );
        void handleWCONProducer( const DeckKeyword& keyword, size_t currentStep, bool isPredictionMode,  const ParseContext& parseContext);
        void handleWCONHIST( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWGRUPCON( const DeckKeyword& keyword, size_t currentStep);
        void handleCOMPDAT( const DeckKeyword& keyword,  size_t currentStep, const EclipseGrid& grid, const Eclipse3DProperties& eclipseProperties, const ParseContext& parseContext);
        void handleCOMPLUMP( const DeckKeyword& keyword,  size_t currentStep );
        void handleWELSEGS( const DeckKeyword& keyword, size_t currentStep);
        void handleCOMPSEGS( const DeckKeyword& keyword, size_t currentStep);
        void handleWCONINJE( const SCHEDULESection&,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWPOLYMER( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWSOLVENT( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWINJTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWCONINJH( const SCHEDULESection&,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWELOPEN( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext );
        void handleWELTARG( const SCHEDULESection&,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleGCONINJE( const SCHEDULESection&,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleGCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleGEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleTUNING( const DeckKeyword& keyword, size_t currentStep);
        void handleGRUPTREE( const DeckKeyword& keyword, size_t currentStep);
        void handleGRUPNET( const DeckKeyword& keyword, size_t currentStep);
        void handleWRFT( const DeckKeyword& keyword, size_t currentStep);
        void handleWTEST( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWRFTPLT( const DeckKeyword& keyword, size_t currentStep);
        void handleWPIMULT( const DeckKeyword& keyword, size_t currentStep);
        void handleDRSDT( const DeckKeyword& keyword, size_t currentStep);
        void handleDRVDT( const DeckKeyword& keyword, size_t currentStep);
        void handleVAPPARS( const DeckKeyword& keyword, size_t currentStep);
        void handleWECON( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext);
        void handleWHISTCTL(const ParseContext& parseContext, const DeckKeyword& keyword);
        void handleMESSAGES(const DeckKeyword& keyword, size_t currentStep);
        void handleVFPPROD(const DeckKeyword& vfpprodKeyword, const UnitSystem& unit_system, size_t currentStep);
        void handleVFPINJ(const DeckKeyword& vfpprodKeyword, const UnitSystem& unit_system, size_t currentStep);
        void checkUnhandledKeywords( const SCHEDULESection& ) const;
        void checkIfAllConnectionsIsShut(size_t currentStep);
        void handleKeyword(size_t& currentStep,
                           const SCHEDULESection& section,
                           size_t keywordIdx,
                           const DeckKeyword& keyword,
                           const ParseContext& parseContext,
                           const EclipseGrid& grid,
                           const Eclipse3DProperties& eclipseProperties,
                           const UnitSystem& unit_system,
                           std::vector<std::pair<const DeckKeyword*, size_t > >& rftProperties);

        static double convertInjectionRateToSI(double rawRate, WellInjector::TypeEnum wellType, const Opm::UnitSystem &unitSystem);
        static double convertInjectionRateToSI(double rawRate, Phase wellPhase, const Opm::UnitSystem &unitSystem);
        static bool convertEclipseStringToBool(const std::string& eclipseString);

    };
}

#endif
