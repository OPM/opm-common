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
#include <optional>
#include <unordered_set>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GasLiftOpt.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GTNode.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GuideRateConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSale.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSump.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MessageLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RFTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPInjTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/ExtNetwork.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/PAvg.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/PAvgCalculatorCollection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellMatcher.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actions.hpp>

#include <opm/common/utility/ActiveGridCells.hpp>
#include <opm/io/eclipse/rst/state.hpp>


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

  For many of the non-trival objects like eg Well and Group the DynamicState<>
  contains a shared pointer to an underlying object, that way the fill operation
  when the vector is resized is quite fast. The following pattern is quite
  common for the Schedule implementation:


       // Create a new well object.
       std::shared_ptr<Well> new_well = this->getWell( well_name, time_step );

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
    class EclipseState;
    class FieldPropsManager;
    class Python;
    class Runspec;
    class RPTConfig;
    class SCHEDULESection;
    class SummaryState;
    class TimeMap;
    class ErrorGuard;
    class WListManager;
    class UDQConfig;
    class UDQActive;

    class Schedule {
    public:
        using WellMap = OrderedMap<std::string, DynamicState<std::shared_ptr<Well>>>;
        using GroupMap = OrderedMap<std::string, DynamicState<std::shared_ptr<Group>>>;
        using VFPProdMap = std::map<int, DynamicState<std::shared_ptr<VFPProdTable>>>;
        using VFPInjMap = std::map<int, DynamicState<std::shared_ptr<VFPInjTable>>>;

        Schedule() = default;
        explicit Schedule(std::shared_ptr<const Python>) {}
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const FieldPropsManager& fp,
                 const Runspec &runspec,
                 const ParseContext& parseContext,
                 ErrorGuard& errors,
                 std::shared_ptr<const Python> python,
                 const RestartIO::RstState* rst = nullptr);

        template<typename T>
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const FieldPropsManager& fp,
                 const Runspec &runspec,
                 const ParseContext& parseContext,
                 T&& errors,
                 std::shared_ptr<const Python> python,
                 const RestartIO::RstState* rst = nullptr);

        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const FieldPropsManager& fp,
                 const Runspec &runspec,
                 std::shared_ptr<const Python> python,
                 const RestartIO::RstState* rst = nullptr);

        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext,
                 ErrorGuard& errors,
                 std::shared_ptr<const Python> python,
                 const RestartIO::RstState* rst = nullptr);

        template <typename T>
        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext,
                 T&& errors,
                 std::shared_ptr<const Python> python,
                 const RestartIO::RstState* rst = nullptr);

        Schedule(const Deck& deck,
                 const EclipseState& es,
                 std::shared_ptr<const Python> python,
                 const RestartIO::RstState* rst = nullptr);

        // The constructor *without* the Python arg should really only be used from Python itself
        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const RestartIO::RstState* rst = nullptr);

        static Schedule serializeObject();

        /*
         * If the input deck does not specify a start time, Eclipse's 1. Jan
         * 1983 is defaulted
         */
        time_t getStartTime() const;
        time_t posixStartTime() const;
        time_t posixEndTime() const;
        time_t simTime(std::size_t timeStep) const;
        double seconds(std::size_t timeStep) const;
        double stepLength(std::size_t timeStep) const;
        std::optional<int> exitStatus() const;
        const UnitSystem& getUnits() const { return this->unit_system; }

        const TimeMap& getTimeMap() const;

        std::size_t numWells() const;
        std::size_t numWells(std::size_t timestep) const;
        bool hasWell(const std::string& wellName) const;
        bool hasWell(const std::string& wellName, std::size_t timeStep) const;

        WellMatcher wellMatcher(std::size_t report_step) const;
        std::vector<std::string> wellNames(const std::string& pattern, std::size_t timeStep, const std::vector<std::string>& matching_wells = {}) const;
        std::vector<std::string> wellNames(const std::string& pattern) const;
        std::vector<std::string> wellNames(std::size_t timeStep) const;
        std::vector<std::string> wellNames() const;

        std::vector<std::string> groupNames(const std::string& pattern, std::size_t timeStep) const;
        std::vector<std::string> groupNames(std::size_t timeStep) const;
        std::vector<std::string> groupNames(const std::string& pattern) const;
        std::vector<std::string> groupNames() const;
        /*
          The restart_groups function returns a vector of groups pointers which
          is organized as follows:

            1. The number of elements is WELLDIMS::MAXGROUPS + 1
            2. The elements are sorted according to group.insert_index().
            3. If there are less than WELLDIMS::MAXGROUPS nullptr is used.
            4. The very last element corresponds to the FIELD group.
        */
        std::vector<const Group*> restart_groups(std::size_t timeStep) const;

        void updateWell(std::shared_ptr<Well> well, std::size_t reportStep);
        std::vector<std::string> changed_wells(std::size_t reportStep) const;
        const Well& getWell(const std::string& wellName, std::size_t timeStep) const;
        const Well& getWellatEnd(const std::string& well_name) const;
        std::vector<Well> getWells(std::size_t timeStep) const;
        std::vector<Well> getWellsatEnd() const;
        void shut_well(const std::string& well_name, std::size_t report_step);
        void stop_well(const std::string& well_name, std::size_t report_step);
        void open_well(const std::string& well_name, std::size_t report_step);

        std::vector<const Group*> getChildGroups2(const std::string& group_name, std::size_t timeStep) const;
        std::vector<Well> getChildWells2(const std::string& group_name, std::size_t timeStep) const;
        const OilVaporizationProperties& getOilVaporizationProperties(std::size_t timestep) const;
        const Well::ProducerCMode& getGlobalWhistctlMmode(std::size_t timestep) const;

        const UDQActive& udqActive(std::size_t timeStep) const;
        const WellTestConfig& wtestConfig(std::size_t timestep) const;
        const GConSale& gConSale(std::size_t timestep) const;
        const GConSump& gConSump(std::size_t timestep) const;
        const WListManager& getWListManager(std::size_t timeStep) const;
        const UDQConfig& getUDQConfig(std::size_t timeStep) const;
        std::vector<const UDQConfig*> udqConfigList() const;
        const Action::Actions& actions(std::size_t timeStep) const;
        void evalAction(const SummaryState& summary_state, std::size_t timeStep);

        const RPTConfig& report_config(std::size_t timeStep) const;

        GTNode groupTree(std::size_t report_step) const;
        GTNode groupTree(const std::string& root_node, std::size_t report_step) const;
        std::size_t numGroups() const;
        std::size_t numGroups(std::size_t timeStep) const;
        bool hasGroup(const std::string& groupName) const;
        bool hasGroup(const std::string& groupName, std::size_t timeStep) const;
        const Group& getGroup(const std::string& groupName, std::size_t timeStep) const;

        const Tuning& getTuning(std::size_t timeStep) const;
        const MessageLimits& getMessageLimits() const;
        void invalidNamePattern (const std::string& namePattern, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& keyword) const;
        const GuideRateConfig& guideRateConfig(std::size_t timeStep) const;

        const RFTConfig& rftConfig() const;
        const Events& getEvents() const;
        const Events& getWellGroupEvents(const std::string& wellGroup) const;
        bool hasWellGroupEvent(const std::string& wellGroup, uint64_t event_mask, std::size_t reportStep) const;
        const Deck& getModifierDeck(std::size_t timeStep) const;
        bool hasOilVaporizationProperties() const;
        const VFPProdTable& getVFPProdTable(int table_id, std::size_t timeStep) const;
        const VFPInjTable& getVFPInjTable(int table_id, std::size_t timeStep) const;
        std::map<int, std::shared_ptr<const VFPProdTable> > getVFPProdTables(std::size_t timeStep) const;
        std::map<int, std::shared_ptr<const VFPInjTable> > getVFPInjTables(std::size_t timeStep) const;
        /*
          Will remove all completions which are connected to cell which is not
          active. Will scan through all wells and all timesteps.
        */
        void filterConnections(const ActiveGridCells& grid);
        std::size_t size() const;
        const RestartConfig& restart() const;
        RestartConfig& restart();

        void applyAction(std::size_t reportStep, const Action::ActionX& action, const Action::Result& result);
        void applyWellProdIndexScaling(const std::string& well_name, const std::size_t reportStep, const double scalingFactor);
        int getNupcol(std::size_t reportStep) const;


        const Network::ExtNetwork& network(std::size_t report_step) const;
        const GasLiftOpt& glo(std::size_t report_step) const;

        bool operator==(const Schedule& data) const;
        std::shared_ptr<const Python> python() const;

        /*
          The cmp() function compares two schedule instances in a context aware
          manner. Floating point numbers are compared with a tolerance. The
          purpose of this comparison function is to implement regression tests
          for the schedule instances created by loading a restart file.
        */
        static bool cmp(const Schedule& sched1, const Schedule& sched2, std::size_t report_step);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            m_timeMap.serializeOp(serializer);
            auto splitWells = splitDynMap(wells_static);
            serializer.vector(splitWells.first);
            serializer(splitWells.second);
            auto splitGroups = splitDynMap(groups);
            serializer.vector(splitGroups.first);
            serializer(splitGroups.second);
            m_oilvaporizationproperties.serializeOp(serializer);
            m_events.serializeOp(serializer);
            m_modifierDeck.serializeOp(serializer);
            m_tuning.serializeOp(serializer);
            m_messageLimits.serializeOp(serializer);
            m_runspec.serializeOp(serializer);
            auto splitvfpprod = splitDynMap<Map2>(vfpprod_tables);
            serializer.vector(splitvfpprod.first);
            serializer(splitvfpprod.second);
            auto splitvfpinj = splitDynMap<Map2>(vfpinj_tables);
            serializer.vector(splitvfpinj.first);
            serializer(splitvfpinj.second);
            wtest_config.serializeOp(serializer);
            wlist_manager.serializeOp(serializer);
            udq_config.serializeOp(serializer);
            udq_active.serializeOp(serializer);
            guide_rate_config.serializeOp(serializer);
            gconsale.serializeOp(serializer);
            gconsump.serializeOp(serializer);
            global_whistctl_mode.template serializeOp<Serializer, false>(serializer);
            m_actions.serializeOp(serializer);
            m_network.serializeOp(serializer);
            m_glo.serializeOp(serializer);
            m_pavg.serializeOp(serializer);
            rft_config.serializeOp(serializer);
            m_nupcol.template serializeOp<Serializer, false>(serializer);
            restart_config.serializeOp(serializer);
            serializer.map(wellgroup_events);
            if (!serializer.isSerializing()) {
                reconstructDynMap(splitWells.first, splitWells.second, wells_static);
                reconstructDynMap(splitGroups.first, splitGroups.second, groups);
                reconstructDynMap<Map2>(splitvfpprod.first, splitvfpprod.second, vfpprod_tables);
                reconstructDynMap<Map2>(splitvfpinj.first, splitvfpinj.second, vfpinj_tables);
            }
            unit_system.serializeOp(serializer);
        }

    private:
        template<class Key, class Value> using Map2 = std::map<Key,Value>;
        std::shared_ptr<const Python> python_handle;
        TimeMap m_timeMap;
        WellMap wells_static;
        GroupMap groups;
        DynamicState< OilVaporizationProperties > m_oilvaporizationproperties;
        Events m_events;
        DynamicVector< Deck > m_modifierDeck;
        DynamicState<Tuning> m_tuning;
        MessageLimits m_messageLimits;
        Runspec m_runspec;
        VFPProdMap vfpprod_tables;
        VFPInjMap vfpinj_tables;
        DynamicState<std::shared_ptr<WellTestConfig>> wtest_config;
        DynamicState<std::shared_ptr<WListManager>> wlist_manager;
        DynamicState<std::shared_ptr<UDQConfig>> udq_config;
        DynamicState<std::shared_ptr<UDQActive>> udq_active;
        DynamicState<std::shared_ptr<GuideRateConfig>> guide_rate_config;
        DynamicState<std::shared_ptr<GConSale>> gconsale;
        DynamicState<std::shared_ptr<GConSump>> gconsump;
        DynamicState<Well::ProducerCMode> global_whistctl_mode;
        DynamicState<std::shared_ptr<Action::Actions>> m_actions;
        DynamicState<std::shared_ptr<Network::ExtNetwork>> m_network;
        DynamicState<std::shared_ptr<GasLiftOpt>> m_glo;
        DynamicState<std::shared_ptr<PAvg>> m_pavg;
        RFTConfig rft_config;
        DynamicState<int> m_nupcol;
        RestartConfig restart_config;
        UnitSystem unit_system;
        std::optional<int> exit_status;

        std::map<std::string,Events> wellgroup_events;
        void load_rst(const RestartIO::RstState& rst,
                      const EclipseGrid& grid,
                      const FieldPropsManager& fp);
        void addWell(Well well, std::size_t report_step);
        void addWell(const std::string& wellName,
                     const std::string& group,
                     int headI,
                     int headJ,
                     Phase preferredPhase,
                     const std::optional<double>& refDepth,
                     double drainageRadius,
                     bool allowCrossFlow,
                     bool automaticShutIn,
                     int pvt_table,
                     Well::GasInflowEquation gas_inflow,
                     std::size_t timeStep,
                     Connection::Order wellConnectionOrder);
        bool updateWPAVE(const std::string& wname, std::size_t report_step, const PAvg& pavg);

        DynamicState<std::shared_ptr<RPTConfig>> rpt_config;
        void updateNetwork(std::shared_ptr<Network::ExtNetwork> network, std::size_t report_step);
        void updateGuideRateModel(const GuideRateModel& new_model, std::size_t report_step);

        GTNode groupTree(const std::string& root_node, std::size_t report_step, std::size_t level, const std::optional<std::string>& parent_name) const;
        void updateGroup(std::shared_ptr<Group> group, std::size_t reportStep);
        bool checkGroups(const ParseContext& parseContext, ErrorGuard& errors);
        void updateUDQActive( std::size_t timeStep, std::shared_ptr<UDQActive> udq );
        bool updateWellStatus( const std::string& well, std::size_t reportStep, bool runtime, Well::Status status, std::optional<KeywordLocation> = {});
        void addWellToGroup( const std::string& group_name, const std::string& well_name , std::size_t timeStep);
        void iterateScheduleSection(std::shared_ptr<const Python> python, const std::string& input_path, const ParseContext& parseContext ,  ErrorGuard& errors, const SCHEDULESection& , const EclipseGrid& grid,
                                    const FieldPropsManager& fp);
        void addACTIONX(const Action::ActionX& action, std::size_t currentStep);
        void addGroupToGroup( const std::string& parent_group, const std::string& child_group, std::size_t timeStep);
        void addGroupToGroup( const std::string& parent_group, const Group& child_group, std::size_t timeStep);
        void addGroup(const std::string& groupName , std::size_t timeStep);
        void addGroup(const Group& group, std::size_t timeStep);
        void addWell(const std::string& wellName, const DeckRecord& record, std::size_t timeStep, Connection::Order connection_order);
        void checkUnhandledKeywords( const SCHEDULESection& ) const;
        void checkIfAllConnectionsIsShut(std::size_t currentStep);
        void updateUDQ(const DeckKeyword& keyword, std::size_t current_step);
        void handleKeyword(std::shared_ptr<const Python> python,
                           const std::string& input_path,
                           std::size_t currentStep,
                           const SCHEDULESection& section,
                           std::size_t keywordIdx,
                           const DeckKeyword& keyword,
                           const ParseContext& parseContext, ErrorGuard& errors,
                           const EclipseGrid& grid,
                           const FieldPropsManager& fp,
                           std::vector<std::pair<const DeckKeyword*, std::size_t > >& rftProperties);
        void addWellGroupEvent(const std::string& wellGroup, ScheduleEvents::Events event, std::size_t reportStep);

        template<template<class, class> class Map, class Type, class Key>
        std::pair<std::vector<Type>, std::vector<std::pair<Key, std::vector<std::size_t>>>>
        splitDynMap(const Map<Key, Opm::DynamicState<Type>>& map)
        {
            // we have to pack the unique ptrs separately, and use an index map
            // to allow reconstructing the appropriate structures.
            std::vector<std::pair<Key, std::vector<std::size_t>>> asMap;
            std::vector<Type> unique;
            for (const auto& it : map) {
                auto indices = it.second.split(unique);
                asMap.push_back(std::make_pair(it.first, indices));
            }

            return std::make_pair(unique, asMap);
        }

        template<template<class, class> class Map, class Type, class Key>
        void reconstructDynMap(const std::vector<Type>& unique,
                               const std::vector<std::pair<Key, std::vector<std::size_t>>>& asMap,
                               Map<Key, Opm::DynamicState<Type>>& result)
        {
            for (const auto& it : asMap) {
                result[it.first].reconstruct(unique, it.second);
            }
        }

        static std::string formatDate(std::time_t t);
        std::string simulationDays(std::size_t currentStep) const;

        void applyEXIT(const DeckKeyword&, std::size_t currentStep);
        void applyMESSAGES(const DeckKeyword&, std::size_t currentStep);
        void applyWELOPEN(const DeckKeyword&, std::size_t currentStep, bool runtime, const ParseContext&, ErrorGuard&, const std::vector<std::string>& matching_wells = {});
        void applyWRFT(const DeckKeyword&, std::size_t currentStep);
        void applyWRFTPLT(const DeckKeyword&, std::size_t currentStep);

        struct HandlerContext {
            const DeckKeyword& keyword;
            const std::size_t currentStep;
            const EclipseGrid& grid;
            const FieldPropsManager& fieldPropsManager;
            const SCHEDULESection * section = nullptr;
            std::optional<std::size_t> keywordIndex;

            HandlerContext(const DeckKeyword& keyword_,
                           const std::size_t currentStep_,
                           const EclipseGrid& grid_,
                           const FieldPropsManager& fieldPropsManager_) :
                keyword(keyword_),
                currentStep(currentStep_),
                grid(grid_),
                fieldPropsManager(fieldPropsManager_)
            {}


        };

        /**
         * Handles a "normal" keyword. A normal keyword is one that can be handled by a function with the standard set of arguments (the ones that are passed to this function).
         *
         * Normal keywords are found in the file KeywordHandlers.cpp; to add a new keyword handler to the file, add its signature in the list below,
         * add the implementation to KeywordHandlers.cpp, and add a pointer to the handler in the dispatch registry in the implementation of this method, found at the bottom of
         * KeywordHandlers.cpp.
         *
         * For the benefit of automatic cross-checking of the lists, all of these are in alphabetical order.
         *
         * @param handlerContext context object containing the environment in which the handler was invoked
         * @param parseContext context object containing the parsing environment
         * @param errors the error handling object for the current parsing process
         *
         * @return `true` if the keyword was handled
         */
        bool handleNormalKeyword(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors);

        // Keyword Handlers
        void handlePYACTION (std::shared_ptr<const Python> python, const std::string& input_path, const DeckKeyword&, std::size_t currentStep);
        void handleGCONPROD(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGCONINJE(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGLIFTOPT(const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWELPI   (const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors, bool actionx_mode = false, const std::vector<std::string>& matching_wells = {});

        // Normal keyword handlers -- in KeywordHandlers.cpp
        void handleBRANPROP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPDAT  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPLUMP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPORD  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPSEGS (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRSDT    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRSDTR   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRVDT    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRVDTR   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONINJE (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONPROD (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONSALE (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONSUMP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGEFAC    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGLIFTOPT (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGPMAINT  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGRUPNET  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGRUPTREE (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGUIDERAT (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleLIFTOPT  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleLINCOM   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleMESSAGES (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleMULTFLT  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleMXUNSUPP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleNODEPROP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleNUPCOL   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleRPTSCHED (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleTUNING   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleUDQ      (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleVAPPARS  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleVFPINJ   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleVFPPROD  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONHIST (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONINJE (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONINJH (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONPROD (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWECON    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWEFAC    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELOPEN  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELPI    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELSEGS  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELSPECS (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELTARG  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWFOAM    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWGRUPCON (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWHISTCTL (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWINJTEMP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWLIFTOPT (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWLIST    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPAVE    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPAVEDEP (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWWPAVE   (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPIMULT  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPMITAB  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPOLYMER (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSALT    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGITER (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGSICD (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGAICD (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGVALV (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSKPTAB  (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSOLVENT (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTEMP    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTEST    (const HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTRACER  (const HandlerContext&, const ParseContext&, ErrorGuard&);
    };
}

#endif
