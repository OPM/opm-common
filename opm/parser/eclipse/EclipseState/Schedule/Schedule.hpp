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

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <time.h>

#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GasLiftOpt.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GTNode.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GuideRateConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MessageLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/ExtNetwork.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RPTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleDeck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/PAvg.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellMatcher.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WriteRestartFileEvents.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletedCells.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleDeck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RPTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/SimulatorUpdate.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm
{
    class ActiveGridCells;
    class Deck;
    class DeckKeyword;
    class DeckRecord;
    class EclipseState;
    class FieldPropsManager;
    class ParseContext;
    class SCHEDULESection;
    class SummaryState;
    class ErrorGuard;
    class UDQConfig;

    namespace RestartIO { struct RstState; }


    struct ScheduleStatic {
        std::shared_ptr<const Python> m_python_handle;
        std::string m_input_path;
        ScheduleRestartInfo rst_info;
        MessageLimits m_deck_message_limits;
        UnitSystem m_unit_system;
        Runspec m_runspec;
        RSTConfig rst_config;
        std::optional<int> output_interval;
        double sumthin{-1.0};
        bool rptonly{false};

        ScheduleStatic() = default;

        explicit ScheduleStatic(std::shared_ptr<const Python> python_handle) :
            m_python_handle(python_handle)
        {}

        ScheduleStatic(std::shared_ptr<const Python> python_handle,
                       const ScheduleRestartInfo& restart_info,
                       const Deck& deck,
                       const Runspec& runspec,
                       const std::optional<int>& output_interval_,
                       const ParseContext& parseContext,
                       ErrorGuard& errors);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            m_deck_message_limits.serializeOp(serializer);
            this->rst_info.serializeOp(serializer);
            m_runspec.serializeOp(serializer);
            m_unit_system.serializeOp(serializer);
            serializer(this->m_input_path);
            rst_info.serializeOp(serializer);
            rst_config.serializeOp(serializer);
            serializer(this->output_interval);
        }


        static ScheduleStatic serializeObject() {
            auto python = std::make_shared<Python>(Python::Enable::OFF);
            ScheduleStatic st(python);
            st.m_deck_message_limits = MessageLimits::serializeObject();
            st.m_runspec = Runspec::serializeObject();
            st.m_unit_system = UnitSystem::newFIELD();
            st.m_input_path = "Some/funny/path";
            st.rst_config = RSTConfig::serializeObject();
            st.rst_info = ScheduleRestartInfo::serializeObject();
            return st;
        }

        bool operator==(const ScheduleStatic& other) const {
            return this->m_input_path == other.m_input_path &&
                   this->m_deck_message_limits == other.m_deck_message_limits &&
                   this->m_unit_system == other.m_unit_system &&
                   this->rst_config == other.rst_config &&
                   this->rst_info == other.rst_info &&
                   this->m_runspec == other.m_runspec;
        }
    };


    class Schedule {
    public:
        Schedule() = default;
        explicit Schedule(std::shared_ptr<const Python> python_handle);
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const FieldPropsManager& fp,
                 const Runspec &runspec,
                 const ParseContext& parseContext,
                 ErrorGuard& errors,
                 std::shared_ptr<const Python> python,
                 const std::optional<int>& output_interval = {},
                 const RestartIO::RstState* rst = nullptr);

        template<typename T>
        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const FieldPropsManager& fp,
                 const Runspec &runspec,
                 const ParseContext& parseContext,
                 T&& errors,
                 std::shared_ptr<const Python> python,
                 const std::optional<int>& output_interval = {},
                 const RestartIO::RstState* rst = nullptr);

        Schedule(const Deck& deck,
                 const EclipseGrid& grid,
                 const FieldPropsManager& fp,
                 const Runspec &runspec,
                 std::shared_ptr<const Python> python,
                 const std::optional<int>& output_interval = {},
                 const RestartIO::RstState* rst = nullptr);

        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext,
                 ErrorGuard& errors,
                 std::shared_ptr<const Python> python,
                 const std::optional<int>& output_interval = {},
                 const RestartIO::RstState* rst = nullptr);

        template <typename T>
        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const ParseContext& parseContext,
                 T&& errors,
                 std::shared_ptr<const Python> python,
                 const std::optional<int>& output_interval = {},
                 const RestartIO::RstState* rst = nullptr);

        Schedule(const Deck& deck,
                 const EclipseState& es,
                 std::shared_ptr<const Python> python,
                 const std::optional<int>& output_interval = {},
                 const RestartIO::RstState* rst = nullptr);

        // The constructor *without* the Python arg should really only be used from Python itself
        Schedule(const Deck& deck,
                 const EclipseState& es,
                 const std::optional<int>& output_interval = {},
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
        const UnitSystem& getUnits() const { return this->m_static.m_unit_system; }
        const Runspec& runspec() const { return this->m_static.m_runspec; }

        std::size_t numWells() const;
        std::size_t numWells(std::size_t timestep) const;
        bool hasWell(const std::string& wellName) const;
        bool hasWell(const std::string& wellName, std::size_t timeStep) const;

        WellMatcher wellMatcher(std::size_t report_step) const;
        std::vector<std::string> wellNames(const std::string& pattern, std::size_t timeStep, const std::vector<std::string>& matching_wells = {}) const;
        std::vector<std::string> wellNames(const std::string& pattern) const;
        std::vector<std::string> wellNames(std::size_t timeStep) const;
        std::vector<std::string> wellNames() const;

        bool hasGroup(const std::string& groupName, std::size_t timeStep) const;
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

        std::vector<std::string> changed_wells(std::size_t reportStep) const;
        const Well& getWell(std::size_t well_index, std::size_t timeStep) const;
        const Well& getWell(const std::string& wellName, std::size_t timeStep) const;
        const Well& getWellatEnd(const std::string& well_name) const;
        std::vector<Well> getWells(std::size_t timeStep) const;
        std::vector<Well> getWellsatEnd() const;
        void shut_well(const std::string& well_name, std::size_t report_step);
        void stop_well(const std::string& well_name, std::size_t report_step);
        void open_well(const std::string& well_name, std::size_t report_step);

        std::vector<const Group*> getChildGroups2(const std::string& group_name, std::size_t timeStep) const;
        std::vector<Well> getChildWells2(const std::string& group_name, std::size_t timeStep) const;
        Well::ProducerCMode getGlobalWhistctlMmode(std::size_t timestep) const;

        const UDQConfig& getUDQConfig(std::size_t timeStep) const;
        void evalAction(const SummaryState& summary_state, std::size_t timeStep);

        GTNode groupTree(std::size_t report_step) const;
        GTNode groupTree(const std::string& root_node, std::size_t report_step) const;
        const Group& getGroup(const std::string& groupName, std::size_t timeStep) const;

        void invalidNamePattern (const std::string& namePattern, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& keyword) const;

        std::optional<std::size_t> first_RFT() const;
        /*
          Will remove all completions which are connected to cell which is not
          active. Will scan through all wells and all timesteps.
        */
        void filterConnections(const ActiveGridCells& grid);
        std::size_t size() const;

        bool write_rst_file(std::size_t report_step) const;
        const std::map< std::string, int >& rst_keywords( size_t timestep ) const;

        SimulatorUpdate applyAction(std::size_t reportStep, const time_point& sim_time, const Action::ActionX& action, const Action::Result& result, const std::unordered_map<std::string, double>& wellpi);
        void applyWellProdIndexScaling(const std::string& well_name, const std::size_t reportStep, const double scalingFactor);


        const GasLiftOpt& glo(std::size_t report_step) const;

        bool operator==(const Schedule& data) const;
        std::shared_ptr<const Python> python() const;


        const ScheduleState& back() const;
        const ScheduleState& operator[](std::size_t index) const;
        std::vector<ScheduleState>::const_iterator begin() const;
        std::vector<ScheduleState>::const_iterator end() const;
        void create_next(const time_point& start_time, const std::optional<time_point>& end_time);
        void create_next(const ScheduleBlock& block);
        void create_first(const time_point& start_time, const std::optional<time_point>& end_time);


        /*
          The cmp() function compares two schedule instances in a context aware
          manner. Floating point numbers are compared with a tolerance. The
          purpose of this comparison function is to implement regression tests
          for the schedule instances created by loading a restart file.
        */
        static bool cmp(const Schedule& sched1, const Schedule& sched2, std::size_t report_step);
        void applyKeywords(std::vector<DeckKeyword*>& keywords, std::size_t timeStep);

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            m_sched_deck.serializeOp(serializer);
            serializer.vector(snapshots);
            m_static.serializeOp(serializer);
            restart_output.serializeOp(serializer);
            this->completed_cells.serializeOp(serializer);

            pack_unpack<PAvg, Serializer>(serializer);
            pack_unpack<WellTestConfig, Serializer>(serializer);
            pack_unpack<GConSale, Serializer>(serializer);
            pack_unpack<GConSump, Serializer>(serializer);
            pack_unpack<WListManager, Serializer>(serializer);
            pack_unpack<Network::ExtNetwork, Serializer>(serializer);
            pack_unpack<Network::Balance, Serializer>(serializer);
            pack_unpack<RPTConfig, Serializer>(serializer);
            pack_unpack<Action::Actions, Serializer>(serializer);
            pack_unpack<UDQActive, Serializer>(serializer);
            pack_unpack<UDQConfig, Serializer>(serializer);
            pack_unpack<NameOrder, Serializer>(serializer);
            pack_unpack<GroupOrder, Serializer>(serializer);
            pack_unpack<GuideRateConfig, Serializer>(serializer);
            pack_unpack<GasLiftOpt, Serializer>(serializer);
            pack_unpack<RFTConfig, Serializer>(serializer);
            pack_unpack<RSTConfig, Serializer>(serializer);

            pack_unpack_map<int, VFPProdTable, Serializer>(serializer);
            pack_unpack_map<int, VFPInjTable, Serializer>(serializer);
            pack_unpack_map<std::string, Group, Serializer>(serializer);
            pack_unpack_map<std::string, Well, Serializer>(serializer);
        }

        template <typename T, class Serializer>
        void pack_unpack(Serializer& serializer) {
            std::vector<T> value_list;
            std::vector<std::size_t> index_list;

            if (serializer.isSerializing())
                pack_state<T>(value_list, index_list);

            serializer.vector(value_list);
            serializer.template vector<std::size_t, false>(index_list);

            if (!serializer.isSerializing())
                unpack_state<T>(value_list, index_list);
        }

        template <typename T>
        std::vector<std::pair<std::size_t,  T>> unique() const {
            std::vector<std::pair<std::size_t, T>> values;
            for (std::size_t index = 0; index < this->snapshots.size(); index++) {
                const auto& member = this->snapshots[index].get<T>();
                const auto& value = member.get();
                if (values.empty() || !(value == values.back().second))
                    values.push_back( std::make_pair(index, value));
            }
            return values;
        }


        template <typename T>
        void pack_state(std::vector<T>& value_list, std::vector<std::size_t>& index_list) const {
            auto unique_values = this->unique<T>();
            for (auto& [index, value] : unique_values) {
                value_list.push_back( std::move(value) );
                index_list.push_back( index );
            }
        }


        template <typename T>
        void unpack_state(const std::vector<T>& value_list, const std::vector<std::size_t>& index_list) {
            std::size_t unique_index = 0;
            while (unique_index < value_list.size()) {
                const auto& value = value_list[unique_index];
                const auto& first_index = index_list[unique_index];
                auto last_index = this->snapshots.size();
                if (unique_index < (value_list.size() - 1))
                    last_index = index_list[unique_index + 1];

                auto& target_state = this->snapshots[first_index];
                target_state.get<T>().update( std::move(value) );
                for (std::size_t index=first_index + 1; index < last_index; index++)
                    this->snapshots[index].get<T>().update( target_state.get<T>() );

                unique_index++;
            }
        }


        template <typename K, typename T, class Serializer>
        void pack_unpack_map(Serializer& serializer) {
            std::vector<T> value_list;
            std::vector<std::size_t> index_list;

            if (serializer.isSerializing())
                pack_map<K,T>(value_list, index_list);

            serializer.vector(value_list);
            serializer(index_list);

            if (!serializer.isSerializing())
                unpack_map<K,T>(value_list, index_list);
        }


        template <typename K, typename T>
        void pack_map(std::vector<T>& value_list,
                      std::vector<std::size_t>& index_list) {

            const auto& last_map = this->snapshots.back().get_map<K,T>();
            std::vector<K> key_list{ last_map.keys() };
            std::unordered_map<K,T> current_value;

            for (std::size_t index = 0; index < this->snapshots.size(); index++) {
                auto& state = this->snapshots[index];
                const auto& current_map = state.template get_map<K,T>();
                for (const auto& key : key_list) {
                    auto& value = current_map.get_ptr(key);
                    if (value) {
                        auto it = current_value.find(key);
                        if (it == current_value.end() || !(*value == it->second)) {
                            value_list.push_back( *value );
                            index_list.push_back( index );

                            current_value[key] = *value;
                        }
                    }
                }
            }
        }


        template <typename K, typename T>
        void unpack_map(const std::vector<T>& value_list,
                        const std::vector<std::size_t>& index_list) {

            std::unordered_map<K, std::vector<std::pair<std::size_t, T>>> storage;
            for (std::size_t storage_index = 0; storage_index < value_list.size(); storage_index++) {
                const auto& value = value_list[storage_index];
                const auto& time_index = index_list[storage_index];

                storage[ value.name() ].emplace_back( time_index, value );
            }

            for (const auto& [key, values] : storage) {
                for (std::size_t unique_index = 0; unique_index < values.size(); unique_index++) {
                    const auto& [time_index, value] = values[unique_index];
                    auto last_index = this->snapshots.size();
                    if (unique_index < (values.size() - 1))
                        last_index = values[unique_index + 1].first;

                    auto& map_value = this->snapshots[time_index].template get_map<K,T>();
                    map_value.update(std::move(value));

                    for (std::size_t index=time_index + 1; index < last_index; index++) {
                        auto& forward_map = this->snapshots[index].template get_map<K,T>();
                        forward_map.update( key, map_value );
                    }
                }
            }
        }



    private:
        ScheduleStatic m_static;
        ScheduleDeck m_sched_deck;
        std::optional<int> exit_status;
        std::vector<ScheduleState> snapshots;
        WriteRestartFileEvents restart_output;
        CompletedCells completed_cells;

        void load_rst(const RestartIO::RstState& rst,
                      const ScheduleGrid& grid,
                      const FieldPropsManager& fp);
        void addWell(Well well);
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

        void updateGuideRateModel(const GuideRateModel& new_model, std::size_t report_step);
        GTNode groupTree(const std::string& root_node, std::size_t report_step, std::size_t level, const std::optional<std::string>& parent_name) const;
        bool checkGroups(const ParseContext& parseContext, ErrorGuard& errors);
        bool updateWellStatus( const std::string& well, std::size_t reportStep, Well::Status status, std::optional<KeywordLocation> = {});
        void addWellToGroup( const std::string& group_name, const std::string& well_name , std::size_t timeStep);
        void iterateScheduleSection(std::size_t load_start,
                                    std::size_t load_end,
                                    const ParseContext& parseContext,
                                    ErrorGuard& errors,
                                    const ScheduleGrid& grid,
                                    const std::unordered_map<std::string, double> * target_wellpi,
                                    const std::string& prefix);
        void addACTIONX(const Action::ActionX& action);
        void addGroupToGroup( const std::string& parent_group, const std::string& child_group);
        void addGroup(const std::string& groupName , std::size_t timeStep);
        void addGroup(Group group);
        void addGroup(const RestartIO::RstGroup& rst_group, std::size_t timeStep);
        void addWell(const std::string& wellName, const DeckRecord& record, std::size_t timeStep, Connection::Order connection_order);
        void checkIfAllConnectionsIsShut(std::size_t currentStep);
        void end_report(std::size_t report_step);
        void handleKeyword(std::size_t currentStep,
                           const ScheduleBlock& block,
                           const DeckKeyword& keyword,
                           const ParseContext& parseContext, ErrorGuard& errors,
                           const ScheduleGrid& grid,
                           const std::vector<std::string>& matching_wells,
                           bool runtime,
                           SimulatorUpdate * sim_update,
                           const std::unordered_map<std::string, double> * target_wellpi);

        void prefetch_cell_properties(const ScheduleGrid& grid, const DeckKeyword& keyword);
        static std::string formatDate(std::time_t t);
        std::string simulationDays(std::size_t currentStep) const;

        bool must_write_rst_file(std::size_t report_step) const;

        void applyEXIT(const DeckKeyword&, std::size_t currentStep);
        void applyWELOPEN(const DeckKeyword&, std::size_t currentStep, const ParseContext&, ErrorGuard&, const std::vector<std::string>& matching_wells = {}, SimulatorUpdate * sim_update = nullptr);

        struct HandlerContext {
            const ScheduleBlock& block;
            const DeckKeyword& keyword;
            const std::size_t currentStep;
            const std::vector<std::string>& matching_wells;
            const bool actionx_mode;
            SimulatorUpdate * sim_update;
            const std::unordered_map<std::string, double> * target_wellpi;
            const ScheduleGrid& grid;

            HandlerContext(const ScheduleBlock& block_,
                           const DeckKeyword& keyword_,
                           const ScheduleGrid& grid_,
                           const std::size_t currentStep_,
                           const std::vector<std::string>& matching_wells_,
                           bool actionx_mode_,
                           SimulatorUpdate * sim_update_,
                           const std::unordered_map<std::string, double> * target_wellpi_)
            : block(block_)
            , keyword(keyword_)
            , currentStep(currentStep_)
            , matching_wells(matching_wells_)
            , actionx_mode(actionx_mode_)
            , sim_update(sim_update_)
            , target_wellpi(target_wellpi_)
            , grid(grid_)
            {}

            void affected_well(const std::string& well_name) {
                if (this->sim_update)
                    this->sim_update->affected_wells.insert(well_name);
            }

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
        bool handleNormalKeyword(HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors);

        // Keyword Handlers
        void handlePYACTION(const DeckKeyword&);
        void handleGCONPROD(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGCONINJE(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors);
        void handleGLIFTOPT(const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors);
        void handleWELPI   (const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors, const std::vector<std::string>& matching_wells = {});
        void handleWELPIRuntime(HandlerContext&);

        // Normal keyword handlers -- in KeywordHandlers.cpp
        void handleBRANPROP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPDAT   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPLUMP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPORD   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleCOMPSEGS  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRSDT     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRSDTCON  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRSDTR    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRVDT     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleDRVDTR    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleEXIT      (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONINJE  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONPROD  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONSALE  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGCONSUMP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGEFAC     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGEOKeyword(HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGLIFTOPT  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGPMAINT   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGRUPNET   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGRUPTREE  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleGUIDERAT  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleLIFTOPT   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleLINCOM    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleMESSAGES  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleMXUNSUPP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleNETBALAN  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleNEXTSTEP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleNODEPROP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleNUPCOL    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleRPTONLY   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleRPTONLYO  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleRPTRST    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleRPTSCHED  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleTUNING    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleSAVE      (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleSUMTHIN   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleUDQ       (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleVAPPARS   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleVFPINJ    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleVFPPROD   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONHIST  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONINJE  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONINJH  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWCONPROD  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWECON     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWEFAC     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELOPEN   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELPI     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELSEGS   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELSPECS  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWELTARG   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWFOAM     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWGRUPCON  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWHISTCTL  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWINJTEMP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWLIFTOPT  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWLIST     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWMICP     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPAVE     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPAVEDEP  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWWPAVE    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPIMULT   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPMITAB   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWPOLYMER  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWRFT      (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWRFTPLT   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSALT     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGITER  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGSICD  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGAICD  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSEGVALV  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSKPTAB   (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWSOLVENT  (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTEMP     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTEST     (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTMULT    (HandlerContext&, const ParseContext&, ErrorGuard&);
        void handleWTRACER   (HandlerContext&, const ParseContext&, ErrorGuard&);
    };
}

#endif
