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

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GTNode.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MessageLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RFTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPInjTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actions.hpp>


/*
  The DynamicState<std::shared_ptr<T>> pattern: The quantities in the Schedule
  section like e.g. wellrates and completion properties are typically
  characterized by the following behaviour:

    1. They can be updated repeatedly at arbitrary points in the Schedule
       section.

    2. The value set at one timestep will apply until is explicitly set again at
       a later timestep.

  These properties are typically stored in a DynamicState<T> container; the
  DynamicState<T> class is a container which implements this semantics:

    1. It is legitimate to ask for an out-of-range value, you will then get the
       last value which has been set.

    2. When assigning an out-of-bounds value the container will append the
       currently set value until correct length has been reached, and then the
       new value will be assigned.

    3. The DynamicState<T> has an awareness of the total length of the time
       axis, trying to access values beyound that is illegal.

  For many of the non-trival objects like eg Well2 and Group2 the DynamicState<>
  contains a shared pointer to an underlying object, that way the fill operation
  when the vector is resized is quite fast. The following pattern is quite
  common for the Schedule implementation:


       // Create a new well object.
       std::shared_ptr<Well> new_well = this->getWell2( well_name, time_step );

       // Update the new well object with new settings from the deck, the
       // updateXXXX() method will return true if the well object was actually
       // updated:
       if (new_well->updateRate( new_rate ))
           this->dynamic_state.update( time_step, new_well);

*/

namespace Opm
{

    class Actions;
    class Deck;
    class DeckKeyword;
    class DeckRecord;
    class EclipseGrid;
    class Eclipse3DProperties;
    class EclipseState;
    class Runspec;
    class SCHEDULESection;
    class SummaryState;
    class TimeMap;
    class UnitSystem;
    class ErrorGuard;
    class WListManager;
    class UDQInput;

    class Schedule {
    public:
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const Eclipse3DProperties& eclipseProperties,
                 const Runspec &runspec,
                 const ParseContext& parseContext,
                 ErrorGuard& errors);

        template<typename T>
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const Eclipse3DProperties& eclipseProperties,
                 const Runspec &runspec,
                 const ParseContext& parseContext,
                 T&& errors);

        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const Eclipse3DProperties& eclipseProperties,
                 const Runspec &runspec);

        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext,
                 ErrorGuard& errors);

        template <typename T>
        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext,
                 T&& errors);

        Schedule(const Deck& deck,
                 const EclipseState& es);

        /*
         * If the input deck does not specify a start time, Eclipse's 1. Jan
         * 1983 is defaulted
         */
        time_t getStartTime() const;
        time_t posixStartTime() const;
        time_t posixEndTime() const;
        time_t simTime(size_t timeStep) const;
        double seconds(size_t timeStep) const;
        double stepLength(size_t timeStep) const;

        const TimeMap& getTimeMap() const;

        size_t numWells() const;
        size_t numWells(size_t timestep) const;
        bool hasWell(const std::string& wellName) const;
        bool hasWell(const std::string& wellName, std::size_t timeStep) const;

        std::vector<std::string> wellNames(const std::string& pattern, size_t timeStep, const std::vector<std::string>& matching_wells = {}) const;
        std::vector<std::string> wellNames(const std::string& pattern) const;
        std::vector<std::string> wellNames(size_t timeStep) const;
        std::vector<std::string> wellNames() const;

        std::vector<std::string> groupNames(const std::string& pattern, size_t timeStep) const;
        std::vector<std::string> groupNames(size_t timeStep) const;
        std::vector<std::string> groupNames(const std::string& pattern) const;
        std::vector<std::string> groupNames() const;

        void updateWell(std::shared_ptr<Well2> well, size_t reportStep);
        const Well2& getWell2(const std::string& wellName, size_t timeStep) const;
        const Well2& getWell2atEnd(const std::string& well_name) const;
        std::vector<Well2> getWells2(size_t timeStep) const;
        std::vector<Well2> getWells2atEnd() const;

        std::vector<const Group2*> getChildGroups2(const std::string& group_name, size_t timeStep) const;
        std::vector<Well2> getChildWells2(const std::string& group_name, size_t timeStep, GroupWellQueryMode query_mode) const;
        const OilVaporizationProperties& getOilVaporizationProperties(size_t timestep) const;

        const WellTestConfig& wtestConfig(size_t timestep) const;
        const WListManager& getWListManager(size_t timeStep) const;
        const UDQInput& getUDQConfig(size_t timeStep) const;
        const Actions& actions() const;
        void evalAction(const SummaryState& summary_state, size_t timeStep);

        GTNode groupTree(std::size_t report_step) const;
        GTNode groupTree(const std::string& root_node, std::size_t report_step) const;
        size_t numGroups() const;
        size_t numGroups(size_t timeStep) const;
        bool hasGroup(const std::string& groupName) const;
        const Group2& getGroup2(const std::string& groupName, size_t timeStep) const;

        const Tuning& getTuning() const;
        const MessageLimits& getMessageLimits() const;
        void invalidNamePattern (const std::string& namePattern, const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& keyword) const;

        const RFTConfig& rftConfig() const;
        const Events& getEvents() const;
        const Events& getWellEvents(const std::string& well) const;
        bool hasWellEvent(const std::string& well, uint64_t event_mask, size_t reportStep) const;
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
        size_t size() const;

        void applyAction(size_t reportStep, const ActionX& action, const std::vector<std::string>& matching_wells);
    private:
        TimeMap m_timeMap;
        OrderedMap< std::string, DynamicState<std::shared_ptr<Well2>>> wells_static;
        OrderedMap< std::string, DynamicState<std::shared_ptr<Group2>>> groups;
        DynamicState< OilVaporizationProperties > m_oilvaporizationproperties;
        Events m_events;
        DynamicVector< Deck > m_modifierDeck;
        Tuning m_tuning;
        MessageLimits m_messageLimits;
        Runspec m_runspec;
        std::map<int, DynamicState<std::shared_ptr<VFPProdTable>>> vfpprod_tables;
        std::map<int, DynamicState<std::shared_ptr<VFPInjTable>>> vfpinj_tables;
        DynamicState<std::shared_ptr<WellTestConfig>> wtest_config;
        DynamicState<std::shared_ptr<WListManager>> wlist_manager;
        DynamicState<std::shared_ptr<UDQInput>> udq_config;
        DynamicState<WellProducer::ControlModeEnum> global_whistctl_mode;
        RFTConfig rft_config;

        Actions m_actions;

        std::map<std::string,Events> well_events;

        GTNode groupTree(const std::string& root_node, std::size_t report_step, const GTNode * parent) const;
        void updateGroup(std::shared_ptr<Group2> group, size_t reportStep);
        bool updateWellStatus( const std::string& well, size_t reportStep , WellCommon::StatusEnum status);
        void addWellToGroup( const std::string& group_name, const std::string& well_name , size_t timeStep);
        void iterateScheduleSection(const ParseContext& parseContext ,  ErrorGuard& errors, const SCHEDULESection& , const EclipseGrid& grid,
                                    const Eclipse3DProperties& eclipseProperties);
        void addGroupToGroup( const std::string& parent_group, const std::string& child_group, size_t timeStep);
        void addGroupToGroup( const std::string& parent_group, const Group2& child_group, size_t timeStep);
        void addGroup(const std::string& groupName , size_t timeStep, const UnitSystem& unit_system);
        void addWell(const std::string& wellName, const DeckRecord& record, size_t timeStep, WellCompletion::CompletionOrderEnum wellCompletionOrder, const UnitSystem& unit_system);
        void handleUDQ(const DeckKeyword& keyword, size_t currentStep);
        void handleWLIST(const DeckKeyword& keyword, size_t currentStep);
        void handleCOMPORD(const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& compordKeyword, size_t currentStep);
        void handleWELSPECS( const SCHEDULESection&, size_t, size_t , const UnitSystem& unit_system, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWCONHIST( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWGRUPCON( const DeckKeyword& keyword, size_t currentStep);
        void handleCOMPDAT( const DeckKeyword& keyword,  size_t currentStep, const EclipseGrid& grid, const Eclipse3DProperties& eclipseProperties, const ParseContext& parseContext, ErrorGuard& errors);
        void handleCOMPLUMP( const DeckKeyword& keyword,  size_t currentStep );
        void handleWELSEGS( const DeckKeyword& keyword, size_t currentStep);
        void handleCOMPSEGS( const DeckKeyword& keyword, size_t currentStep, const EclipseGrid& grid, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWCONINJE( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWFOAM( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWPOLYMER( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWSOLVENT( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWTRACER( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWPMITAB( const DeckKeyword& keyword,  const size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWSKPTAB( const DeckKeyword& keyword,  const size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWINJTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWCONINJH(const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWELOPEN( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors, const std::vector<std::string>& matching_wells = {});
        void handleWELTARG( const SCHEDULESection&,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGCONINJE( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleTUNING( const DeckKeyword& keyword, size_t currentStep);
        void handleGRUPTREE( const DeckKeyword& keyword, size_t currentStep, const UnitSystem& unit_system, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGRUPNET( const DeckKeyword& keyword, size_t currentStep, const UnitSystem& unit_system);
        void handleWRFT( const DeckKeyword& keyword, size_t currentStep);
        void handleWTEST( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWRFTPLT( const DeckKeyword& keyword, size_t currentStep);
        void handleWPIMULT( const DeckKeyword& keyword, size_t currentStep);
        void handleDRSDT( const DeckKeyword& keyword, size_t currentStep);
        void handleDRVDT( const DeckKeyword& keyword, size_t currentStep);
        void handleDRSDTR( const DeckKeyword& keyword, size_t currentStep);
        void handleDRVDTR( const DeckKeyword& keyword, size_t currentStep);
        void handleVAPPARS( const DeckKeyword& keyword, size_t currentStep);
        void handleWECON( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWHISTCTL(const DeckKeyword& keyword, std::size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors);
        void handleMESSAGES(const DeckKeyword& keyword, size_t currentStep);
        void handleVFPPROD(const DeckKeyword& vfpprodKeyword, const UnitSystem& unit_system, size_t currentStep);
        void handleVFPINJ(const DeckKeyword& vfpprodKeyword, const UnitSystem& unit_system, size_t currentStep);
        void checkUnhandledKeywords( const SCHEDULESection& ) const;
        void checkIfAllConnectionsIsShut(size_t currentStep);
        void handleKeyword(size_t& currentStep,
                           const SCHEDULESection& section,
                           size_t keywordIdx,
                           const DeckKeyword& keyword,
                           const ParseContext& parseContext, ErrorGuard& errors,
                           const EclipseGrid& grid,
                           const Eclipse3DProperties& eclipseProperties,
                           const UnitSystem& unit_system,
                           std::vector<std::pair<const DeckKeyword*, size_t > >& rftProperties);
        void addWellEvent(const std::string& well, ScheduleEvents::Events event, size_t reportStep);
    };
}

#endif
