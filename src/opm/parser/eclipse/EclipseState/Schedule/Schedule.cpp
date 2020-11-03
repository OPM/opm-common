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

#include <algorithm>
#include <ctime>
#include <fnmatch.h>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/numeric/cmp.hpp>
#include <opm/common/utility/String.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckSection.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionResult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/SICD.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/Valve.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RPTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/Node.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WList.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellFoamProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellBrineProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include "Well/injection.hpp"
#include "MSW/Compsegs.hpp"

namespace Opm {


namespace {

    bool name_match(const std::string& pattern, const std::string& name) {
        int flags = 0;
        return (fnmatch(pattern.c_str(), name.c_str(), flags) == 0);
    }

    std::pair<std::time_t, std::size_t> restart_info(const RestartIO::RstState * rst) {
        if (!rst)
            return std::make_pair(std::time_t{0}, std::size_t{0});
        else
            return rst->header.restart_info();
    }
}

    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const FieldPropsManager& fp,
                        const Runspec &runspec,
                        const ParseContext& parseContext,
                        ErrorGuard& errors,
                        [[maybe_unused]] std::shared_ptr<const Python> python,
                        const RestartIO::RstState * rst)
    try :
        python_handle(python),
        m_timeMap( deck , restart_info( rst )),
        m_oilvaporizationproperties( this->m_timeMap, OilVaporizationProperties(runspec.tabdims().getNumPVTTables()) ),
        m_events( this->m_timeMap ),
        m_modifierDeck( this->m_timeMap, Deck{} ),
        m_tuning( this->m_timeMap, Tuning() ),
        m_messageLimits( this->m_timeMap ),
        m_runspec( runspec ),
        wtest_config(this->m_timeMap, std::make_shared<WellTestConfig>() ),
        wlist_manager( this->m_timeMap, std::make_shared<WListManager>()),
        udq_config(this->m_timeMap, std::make_shared<UDQConfig>(deck)),
        udq_active(this->m_timeMap, std::make_shared<UDQActive>()),
        guide_rate_config(this->m_timeMap, std::make_shared<GuideRateConfig>()),
        gconsale(this->m_timeMap, std::make_shared<GConSale>() ),
        gconsump(this->m_timeMap, std::make_shared<GConSump>() ),
        global_whistctl_mode(this->m_timeMap, Well::ProducerCMode::CMODE_UNDEFINED),
        m_actions(this->m_timeMap, std::make_shared<Action::Actions>()),
        m_network(this->m_timeMap, std::make_shared<Network::ExtNetwork>()),
        m_glo(this->m_timeMap, std::make_shared<GasLiftOpt>()),
        rft_config(this->m_timeMap),
        m_nupcol(this->m_timeMap, runspec.nupcol()),
        restart_config(m_timeMap, deck, parseContext, errors),
        rpt_config(this->m_timeMap, std::make_shared<RPTConfig>())
    {
        addGroup( "FIELD", 0, deck.getActiveUnitSystem());
        if (rst)
            this->load_rst(*rst, grid, fp, deck.getActiveUnitSystem());

        /*
          We can have the MESSAGES keyword anywhere in the deck, we
          must therefor also scan the part of the deck prior to the
          SCHEDULE section to initialize valid MessageLimits object.
        */
        for (std::size_t keywordIdx = 0; keywordIdx < deck.size(); ++keywordIdx) {
            const auto& keyword = deck.getKeyword(keywordIdx);
            if (keyword.name() == "SCHEDULE")
                break;

            if (keyword.name() == "MESSAGES")
                applyMESSAGES(keyword, 0);
        }

        if (DeckSection::hasSCHEDULE(deck))
            iterateScheduleSection( python, deck.getInputPath(), parseContext, errors, SCHEDULESection( deck ), grid, fp);
    }
    catch (const OpmInputError& opm_error) {
        throw;
    }
    catch (const std::exception& std_error) {
        OpmLog::error(fmt::format("An error occured while creating the reservoir schedule\n"
                                  "Internal error: {}", std_error.what()));
        throw;
    }




    template <typename T>
    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const FieldPropsManager& fp,
                        const Runspec &runspec,
                        const ParseContext& parseContext,
                        T&& errors,
                        std::shared_ptr<const Python> python,
                        const RestartIO::RstState * rst) :
        Schedule(deck, grid, fp, runspec, parseContext, errors, python, rst)
    {}


    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const FieldPropsManager& fp,
                        const Runspec &runspec,
                        std::shared_ptr<const Python> python,
                        const RestartIO::RstState * rst) :
        Schedule(deck, grid, fp, runspec, ParseContext(), ErrorGuard(), python, rst)
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es, const ParseContext& parse_context, ErrorGuard& errors, std::shared_ptr<const Python> python, const RestartIO::RstState * rst) :
        Schedule(deck,
                 es.getInputGrid(),
                 es.fieldProps(),
                 es.runspec(),
                 parse_context,
                 errors,
                 python,
                 rst)
    {}


    template <typename T>
    Schedule::Schedule(const Deck& deck, const EclipseState& es, const ParseContext& parse_context, T&& errors, std::shared_ptr<const Python> python, const RestartIO::RstState * rst) :
        Schedule(deck,
                 es.getInputGrid(),
                 es.fieldProps(),
                 es.runspec(),
                 parse_context,
                 errors,
                 python,
                 rst)
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es, std::shared_ptr<const Python> python, const RestartIO::RstState * rst) :
        Schedule(deck, es, ParseContext(), ErrorGuard(), python, rst)
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es, const RestartIO::RstState * rst) :
        Schedule(deck, es, ParseContext(), ErrorGuard(), std::make_shared<const Python>(), rst)
    {}


    /*
      In general the serializeObject() instances are used as targets for
      deserialization, i.e. the serialized buffer is unpacked into this
      instance. However the Schedule object is a top level object, and the
      simulator will instantiate and manage a Schedule object to unpack into, so
      the instance created here is only for testing.
    */
    Schedule Schedule::serializeObject()
    {
        auto python = std::make_shared<Python>(Python::Enable::OFF);
        Schedule result(python);

        result.m_timeMap = TimeMap::serializeObject();
        result.wells_static.insert({"test1", {{std::make_shared<Opm::Well>(Opm::Well::serializeObject())},1}});
        result.groups.insert({"test2", {{std::make_shared<Opm::Group>(Opm::Group::serializeObject())},1}});
        result.m_oilvaporizationproperties = {{Opm::OilVaporizationProperties::serializeObject()},1};
        result.m_events = Events::serializeObject();
        result.m_modifierDeck = DynamicVector<Deck>({Deck::serializeObject()});
        result.m_tuning = {{Tuning::serializeObject()}, 1};
        result.m_messageLimits = MessageLimits::serializeObject();
        result.m_runspec = Runspec::serializeObject();
        result.vfpprod_tables = {{1, {{std::make_shared<VFPProdTable>(VFPProdTable::serializeObject())}, 1}}};
        result.vfpinj_tables = {{2, {{std::make_shared<VFPInjTable>(VFPInjTable::serializeObject())}, 1}}};
        result.wtest_config = {{std::make_shared<WellTestConfig>(WellTestConfig::serializeObject())}, 1};
        result.wlist_manager = {{std::make_shared<WListManager>(WListManager::serializeObject())}, 1};
        result.udq_config = {{std::make_shared<UDQConfig>(UDQConfig::serializeObject())}, 1};
        result.m_network  = {{std::make_shared<Network::ExtNetwork>(Network::ExtNetwork::serializeObject())}, 1};
        result.m_glo = {{std::make_shared<GasLiftOpt>(GasLiftOpt::serializeObject())}, 1};
        result.udq_active = {{std::make_shared<UDQActive>(UDQActive::serializeObject())}, 1};
        result.guide_rate_config = {{std::make_shared<GuideRateConfig>(GuideRateConfig::serializeObject())}, 1};
        result.gconsale = {{std::make_shared<GConSale>(GConSale::serializeObject())}, 1};
        result.gconsump = {{std::make_shared<GConSump>(GConSump::serializeObject())}, 1};
        result.global_whistctl_mode = {{Well::ProducerCMode::CRAT}, 1};
        result.m_actions = {{std::make_shared<Action::Actions>(Action::Actions::serializeObject())}, 1};
        result.rft_config = RFTConfig::serializeObject();
        result.m_nupcol = {{1}, 1};
        result.restart_config = RestartConfig::serializeObject();
        result.wellgroup_events = {{"test", Events::serializeObject()}};

        return result;
    }

    std::time_t Schedule::getStartTime() const {
        return this->posixStartTime( );
    }

    time_t Schedule::posixStartTime() const {
        return m_timeMap.getStartTime( 0 );
    }

    time_t Schedule::posixEndTime() const {
        return this->m_timeMap.getEndTime();
    }


    void Schedule::handleKeyword(std::shared_ptr<const Python> python,
                                 const std::string& input_path,
                                 std::size_t currentStep,
                                 const SCHEDULESection& section,
                                 std::size_t keywordIdx,
                                 const DeckKeyword& keyword,
                                 const ParseContext& parseContext,
                                 ErrorGuard& errors,
                                 const EclipseGrid& grid,
                                 const FieldPropsManager& fp,
                                 std::vector<std::pair<const DeckKeyword*, std::size_t > >& rftProperties) {

        const HandlerContext handlerContext { section, keyword, keywordIdx, currentStep, grid, fp };

        if (handleNormalKeyword(handlerContext, parseContext, errors))
            return;

        else if (keyword.name() == "WRFT")
            rftProperties.push_back( std::make_pair( &keyword , currentStep ));

        else if (keyword.name() == "WRFTPLT")
            rftProperties.push_back( std::make_pair( &keyword , currentStep ));

        else if (keyword.name() == "PYACTION")
            handlePYACTION(python, input_path, keyword, currentStep);
    }

namespace {

class ScheduleLogger {
public:
    explicit ScheduleLogger(bool restart_skip)
    {
        if (restart_skip)
            this->log_function = &OpmLog::note;
        else
            this->log_function = &OpmLog::info;
    }

    void operator()(const std::string& msg) {
        this->log_function(msg);
    }

    void info(const std::string& msg) {
        OpmLog::info(msg);
    }

    void complete_step(const std::string& msg) {
        this->step_count += 1;
        if (this->step_count == this->max_print) {
            this->log_function(msg);
            OpmLog::info("Report limit reached, see PRT-file for remaining Schedule initialization.\n");
            this->log_function = &OpmLog::note;
        } else
            this->log_function( msg + "\n");
    };

    void restart() {
        this->step_count = 0;
        this->log_function = &OpmLog::info;
    }


private:
    std::size_t step_count = 0;
    std::size_t max_print  = 5;
    void (*log_function)(const std::string&);
};

}

    void Schedule::iterateScheduleSection(std::shared_ptr<const Opm::Python> python, const std::string& input_path, const ParseContext& parseContext , ErrorGuard& errors, const SCHEDULESection& section , const EclipseGrid& grid,
                                          const FieldPropsManager& fp) {

        std::vector<std::pair< const DeckKeyword* , std::size_t> > rftProperties;
        const auto& unit_system = section.unitSystem();
        std::string time_unit = unit_system.name(UnitSystem::measure::time);
        auto convert_time = [&unit_system](double seconds) { return unit_system.from_si(UnitSystem::measure::time, seconds); };
        std::size_t keywordIdx = 0;
        std::string current_file;
        const auto& time_map = this->m_timeMap;
        /*
          The keywords in the skiprest_whitelist set are loaded from the
          SCHEDULE section even though the SKIPREST keyword is in action. The
          full list includes some additional keywords which we do not support at
          all.
        */
        std::unordered_set<std::string> skiprest_whitelist = {"VFPPROD", "VFPINJ", "RPTSCHED", "RPTRST", "TUNING", "MESSAGES"};
        std::size_t currentStep = 0;
        /*
          The behavior of variable restart_skip is more lenient than the
          SKIPREST keyword. If this is a restarted[1] run the loop iterating
          over keywords will skip the all keywords[2] until DATES keyword with
          the restart date is encountered - irrespective of whether the SKIPREST
          keyword is present in the deck or not.

          [1]: opm/flow can restart in a mode where all the keywords from the
               historical part of the Schedule section is internalized, and only
               the solution fields are read from the restart file. In this case
               we will have TimeMap::restart_offset() == 0.

          [2]: With the exception of the keywords in the skiprest_whitelist;
               these keywords will be assigned to report step 0.
        */

        auto restart_skip = currentStep < this->m_timeMap.restart_offset();
        ScheduleLogger logger(restart_skip);
        {
            const auto& schedule_keyword = section.getKeyword<ParserKeywords::SCHEDULE>();
            const auto& location = schedule_keyword.location();
            current_file = location.filename;
            logger.info(fmt::format("\nProcessing dynamic information from\n{} line {}", current_file, location.lineno));
            if (restart_skip)
                logger.info(fmt::format("This is a restarted run - skipping until report step {} at {}", time_map.restart_offset(), Schedule::formatDate(time_map.restart_time())));

            logger(fmt::format("Initializing report step {}/{} at {} {} {} line {}",
                               currentStep + 1,
                               this->size(),
                               Schedule::formatDate(this->getStartTime()),
                               convert_time(time_map.getTimePassedUntil(currentStep)),
                               time_unit,
                               location.lineno));
        }
        while (true) {
            if (keywordIdx == section.size())
                break;

            const auto& keyword = section.getKeyword(keywordIdx);
            const auto& location = keyword.location();
            if (location.filename != current_file) {
                logger(fmt::format("Reading from: {} line {}", location.filename, location.lineno));
                current_file = location.filename;
            }

            if (keyword.name() == "DATES") {
                checkIfAllConnectionsIsShut(currentStep);
                for (const auto& record : keyword) {
                    if (restart_skip) {
                        auto deck_time = TimeMap::timeFromEclipse(record);
                        if (deck_time == time_map.restart_time()) {
                            restart_skip = false;
                            currentStep = time_map.restart_offset();
                            logger.restart();
                            logger(fmt::format("Found restart date {}", Schedule::formatDate(deck_time)));
                        } else
                            logger(fmt::format("Skipping DATES keyword {}", Schedule::formatDate(deck_time)));
                    } else {
                        currentStep += 1;
                        if (currentStep < this->size()) {
                            const auto& start_date = Schedule::formatDate(this->simTime(currentStep));
                            const auto& days = convert_time(this->stepLength(currentStep - 1));
                            const auto& days_total = convert_time(time_map.getTimePassedUntil(currentStep));
                            logger.complete_step(fmt::format("Complete report step {0} ({1} {2}) at {3} ({4} {2})", currentStep,  days, time_unit, start_date, days_total));

                            logger(fmt::format("Initializing report step {}/{} at {} ({} {}) - line {}",
                                               currentStep + 1,
                                               this->size(),
                                               start_date,
                                               convert_time(time_map.getTimePassedUntil(currentStep)),
                                               time_unit,
                                               location.lineno));

                        }
                    }
                }
                keywordIdx++;
                continue;
            }

            if (keyword.name() == "TSTEP") {
                checkIfAllConnectionsIsShut(currentStep);
                if (restart_skip)
                    logger(OpmInputError::format("Skipping TSTEP keyword at {file} line {line}", keyword.location()));
                else {
                    for (const auto& tstep : keyword.getRecord(0).getItem(0).getSIDoubleData()) {
                        currentStep += 1;
                        const auto& end_date = Schedule::formatDate( this->simTime(currentStep) );
                        logger.complete_step(fmt::format("TSTEP {:4} {} {} -> {}", currentStep, convert_time(tstep), time_unit, end_date));
                    }
                }
                keywordIdx++;
                continue;
            }

            if (restart_skip && skiprest_whitelist.count(keyword.name()) == 0) {
                logger(fmt::format("Skipping keyword: {} while loading SCHEDULE section", keyword.name()));
                keywordIdx++;
                continue;
            }

            if (keyword.name() == "ACTIONX") {
                Action::ActionX action(keyword, this->m_timeMap.getStartTime(currentStep));
                while (true) {
                    keywordIdx++;
                    if (keywordIdx == section.size())
                        throw std::invalid_argument("Invalid ACTIONX section - missing ENDACTIO");

                    const auto& action_keyword = section.getKeyword(keywordIdx);
                    if (action_keyword.name() == "ENDACTIO")
                        break;

                    if (Action::ActionX::valid_keyword(action_keyword.name()))
                        action.addKeyword(action_keyword);
                    else {
                        std::string msg_fmt = "The keyword {keyword} is not supported in the ACTIONX block\n"
                                              "In {file} line {line}.";
                        parseContext.handleError( ParseContext::ACTIONX_ILLEGAL_KEYWORD, msg_fmt, action_keyword.location(), errors);
                    }
                }
                this->addACTIONX(action, currentStep);
                keywordIdx++;
                continue;
            }
            logger(fmt::format("Processing keyword {} at line {}", location.keyword, location.lineno));
            this->handleKeyword(python,
                                input_path,
                                currentStep,
                                section,
                                keywordIdx,
                                keyword,
                                parseContext,
                                errors,
                                grid,
                                fp,
                                rftProperties);
            keywordIdx++;
        }
        checkIfAllConnectionsIsShut(currentStep);


        for (auto rftPair = rftProperties.begin(); rftPair != rftProperties.end(); ++rftPair) {
            const DeckKeyword& keyword = *rftPair->first;
            std::size_t timeStep = rftPair->second;
            if (keyword.name() == "WRFT")
                applyWRFT(keyword,  timeStep);

            if (keyword.name() == "WRFTPLT")
                applyWRFTPLT(keyword, timeStep);
        }

        checkUnhandledKeywords(section);
    }

    void Schedule::addACTIONX(const Action::ActionX& action, std::size_t currentStep) {
        auto new_actions = std::make_shared<Action::Actions>( this->actions(currentStep) );
        new_actions->add(action);
        this->m_actions.update(currentStep, new_actions);
    }

    void Schedule::checkUnhandledKeywords(const SCHEDULESection& /*section*/) const {
    }

    void Schedule::handlePYACTION(std::shared_ptr<const Python> python, const std::string& input_path, const DeckKeyword& keyword, std::size_t currentStep) {
        if (!python->enabled()) {
            //Must have a real Python instance here - to ensure that IMPORT works
            const auto& loc = keyword.location();
            OpmLog::warning("This version of flow is built without support for Python. Keyword PYACTION in file: " + loc.filename + " line: " + std::to_string(loc.lineno) + " is ignored.");
            return;
        }

        const auto& name = keyword.getRecord(0).getItem<ParserKeywords::PYACTION::NAME>().get<std::string>(0);
        const auto& run_count = Action::PyAction::from_string( keyword.getRecord(0).getItem<ParserKeywords::PYACTION::RUN_COUNT>().get<std::string>(0) );
        const auto& module_arg = keyword.getRecord(1).getItem<ParserKeywords::PYACTION::FILENAME>().get<std::string>(0);
        std::string module;
        if (input_path.empty())
            module = module_arg;
        else
            module = input_path + "/" + module_arg;

        Action::PyAction pyaction(python, name, run_count, module);
        auto new_actions = std::make_shared<Action::Actions>( this->actions(currentStep) );
        new_actions->add(pyaction);
        this->m_actions.update(currentStep, new_actions);
    }

    void Schedule::applyEXIT(const DeckKeyword& keyword, std::size_t report_step) {
        int status = keyword.getRecord(0).getItem<ParserKeywords::EXIT::STATUS_CODE>().get<int>(0);
        OpmLog::info("Simulation exit with status: " + std::to_string(status) + " requested as part of ACTIONX at report_step: " + std::to_string(report_step));
        this->exit_status = status;
    }

    void Schedule::shut_well(const std::string& well_name, std::size_t report_step) {
        this->updateWellStatus(well_name, report_step, Well::Status::SHUT, true);
    }

    void Schedule::open_well(const std::string& well_name, std::size_t report_step) {
        this->updateWellStatus(well_name, report_step, Well::Status::OPEN, true);
    }

    void Schedule::stop_well(const std::string& well_name, std::size_t report_step) {
        this->updateWellStatus(well_name, report_step, Well::Status::STOP, true);
    }

    void Schedule::updateWell(std::shared_ptr<Well> well, std::size_t reportStep) {
        auto& dynamic_state = this->wells_static.at(well->name());
        dynamic_state.update(reportStep, std::move(well));
    }


    /*
      Function is quite dangerous - because if this is called while holding a
      Well pointer that will go stale and needs to be refreshed.
    */
    bool Schedule::updateWellStatus( const std::string& well_name, std::size_t reportStep , Well::Status status, bool update_connections) {
        bool update = false;
        auto& dynamic_state = this->wells_static.at(well_name);
        auto well2 = std::make_shared<Well>(*dynamic_state[reportStep]);
        if (well2->updateStatus(status, update_connections)) {
            m_events.addEvent( ScheduleEvents::WELL_STATUS_CHANGE, reportStep );
            this->addWellGroupEvent( well2->name(), ScheduleEvents::WELL_STATUS_CHANGE, reportStep);
            this->updateWell(well2, reportStep);
            update = true;
            if (status == Well::Status::OPEN)
                this->rft_config.addWellOpen(well_name, reportStep);
        }
        return update;
    }

    /*
      This routine is called when UDQ keywords is added in an ACTIONX block.
    */
    void Schedule::updateUDQ(const DeckKeyword& keyword, std::size_t current_step) {
        const auto& current = *this->udq_config.get(current_step);
        std::shared_ptr<UDQConfig> new_udq = std::make_shared<UDQConfig>(current);
        for (const auto& record : keyword)
            new_udq->add_record(record, keyword.location(), current_step);

        auto next_index = this->udq_config.update_equal(current_step, new_udq);
        if (next_index) {
            for (const auto& [report_step, udq_ptr] : this->udq_config.unique() ) {
                if (report_step > current_step) {
                    for (const auto& record : keyword)
                        udq_ptr->add_record(record, keyword.location(), current_step);
                }
            }
        }
    }

    void Schedule::applyWELOPEN(const DeckKeyword& keyword, std::size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors, const std::vector<std::string>& matching_wells) {

        auto conn_defaulted = []( const DeckRecord& rec ) {
            auto defaulted = []( const DeckItem& item ) {
                return item.defaultApplied( 0 );
            };

            return std::all_of( rec.begin() + 2, rec.end(), defaulted );
        };

        constexpr auto open = Well::Status::OPEN;
        bool action_mode = !matching_wells.empty();

        for (const auto& record : keyword) {
            const auto& wellNamePattern = record.getItem( "WELL" ).getTrimmedString(0);
            const auto& status_str = record.getItem( "STATUS" ).getTrimmedString( 0 );
            const auto well_names = this->wellNames(wellNamePattern, currentStep, matching_wells);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, currentStep, parseContext, errors, keyword);

            /* if all records are defaulted or just the status is set, only
             * well status is updated
             */
            if (conn_defaulted( record )) {
                const auto well_status = Well::StatusFromString( status_str );
                for (const auto& wname : well_names) {
                    {
                        const auto& well = this->getWell(wname, currentStep);
                        if( well_status == open && !well.canOpen() ) {
                            auto days = m_timeMap.getTimePassedUntil( currentStep ) / (60 * 60 * 24);
                            std::string msg = "Well " + wname
                                + " where crossflow is banned has zero total rate."
                                + " This well is prevented from opening at "
                                + std::to_string( days ) + " days";
                            OpmLog::note(msg);
                        } else {
                            this->updateWellStatus( wname, currentStep, well_status, false );
                            if (well_status == open)
                                this->rft_config.addWellOpen(wname, currentStep);
                        }
                    }
                }

                continue;
            }

            for (const auto& wname : well_names) {
                const auto comp_status = Connection::StateFromString( status_str );
                {
                    auto& dynamic_state = this->wells_static.at(wname);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                    if (well_ptr->handleWELOPEN(record, comp_status, action_mode))
                        // The updateWell call breaks test at line 825 and 831 in ScheduleTests
                        this->updateWell(std::move(well_ptr), currentStep);
                }

                m_events.addEvent( ScheduleEvents::COMPLETION_CHANGE, currentStep );
            }
        }
    }

    void Schedule::applyMESSAGES(const DeckKeyword& keyword, std::size_t currentStep) {
        const auto& record = keyword.getRecord(0);
        using  set_limit_fptr = decltype( std::mem_fn( &MessageLimits::setMessagePrintLimit ) );
        static const std::pair<std::string , set_limit_fptr> setters[] = {
            {"MESSAGE_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setMessagePrintLimit )},
            {"COMMENT_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setCommentPrintLimit )},
            {"WARNING_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setWarningPrintLimit )},
            {"PROBLEM_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setProblemPrintLimit )},
            {"ERROR_PRINT_LIMIT"   , std::mem_fn( &MessageLimits::setErrorPrintLimit   )},
            {"BUG_PRINT_LIMIT"     , std::mem_fn( &MessageLimits::setBugPrintLimit     )},
            {"MESSAGE_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setMessageStopLimit  )},
            {"COMMENT_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setCommentStopLimit  )},
            {"WARNING_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setWarningStopLimit  )},
            {"PROBLEM_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setProblemStopLimit  )},
            {"ERROR_STOP_LIMIT"    , std::mem_fn( &MessageLimits::setErrorStopLimit    )},
            {"BUG_STOP_LIMIT"      , std::mem_fn( &MessageLimits::setBugStopLimit      )},
        };

        for (const auto& pair : setters) {
            const auto& item = record.getItem( pair.first );
            if (!item.defaultApplied(0)) {
                const set_limit_fptr& fptr = pair.second;
                int value = item.get<int>(0);
                fptr( this->m_messageLimits , currentStep , value );
            }
        }
    }

    void Schedule::applyWRFT(const DeckKeyword& keyword, std::size_t currentStep) {
        /* Rule for handling RFT: Request current RFT data output for specified wells, plus output when
         * any well is subsequently opened
         */

        for (const auto& record : keyword) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, currentStep);
            for (const auto& well_name : well_names)
                this->rft_config.updateRFT(well_name, currentStep, RFTConfig::RFT::YES);

        }

        this->rft_config.setWellOpenRFT(currentStep);
    }

    void Schedule::applyWRFTPLT(const DeckKeyword& keyword, std::size_t currentStep) {
        for (const auto& record : keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            RFTConfig::RFT RFTKey = RFTConfig::RFTFromString(record.getItem("OUTPUT_RFT").getTrimmedString(0));
            RFTConfig::PLT PLTKey = RFTConfig::PLTFromString(record.getItem("OUTPUT_PLT").getTrimmedString(0));
            const auto well_names = wellNames(wellNamePattern, currentStep);
            for (const auto& well_name : well_names) {
                this->rft_config.updateRFT(well_name, currentStep, RFTKey);
                this->rft_config.updatePLT(well_name, currentStep, PLTKey);
            }
        }
    }

    const RFTConfig& Schedule::rftConfig() const {
        return this->rft_config;
    }


    void Schedule::invalidNamePattern( const std::string& namePattern,  std::size_t, const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& keyword ) const {
        std::string msg_fmt = fmt::format("No wells/groups match the pattern: \'{}\'", namePattern);
        parseContext.handleError( ParseContext::SCHEDULE_INVALID_NAME, msg_fmt, keyword.location(), errors );
    }

    const TimeMap& Schedule::getTimeMap() const {
        return this->m_timeMap;
    }

    GTNode Schedule::groupTree(const std::string& root_node, std::size_t report_step, std::size_t level, const std::optional<std::string>& parent_name) const {
        auto root_group = this->getGroup(root_node, report_step);
        GTNode tree(root_group, level, parent_name);

        for (const auto& wname : root_group.wells()) {
            const auto& well = this->getWell(wname, report_step);
            tree.add_well(well);
        }

        for (const auto& gname : root_group.groups()) {
            auto child_group = this->groupTree(gname, report_step, level + 1, root_node);
            tree.add_group(child_group);
        }

        return tree;
    }

    GTNode Schedule::groupTree(const std::string& root_node, std::size_t report_step) const {
        return this->groupTree(root_node, report_step, 0, {});
    }

    GTNode Schedule::groupTree(std::size_t report_step) const {
        return this->groupTree("FIELD", report_step);
    }

    void Schedule::addWell(const std::string& wellName,
                           const DeckRecord& record,
                           std::size_t timeStep,
                           Connection::Order wellConnectionOrder,
                           const UnitSystem& unit_system)
    {
        // We change from eclipse's 1 - n, to a 0 - n-1 solution
        int headI = record.getItem("HEAD_I").get< int >(0) - 1;
        int headJ = record.getItem("HEAD_J").get< int >(0) - 1;
        Phase preferredPhase;
        {
            const std::string phaseStr = record.getItem("PHASE").getTrimmedString(0);
            if (phaseStr == "LIQ") {
                // We need a workaround in case the preferred phase is "LIQ",
                // which is not proper phase and will cause the get_phase()
                // function to throw. In that case we choose to treat it as OIL.
                preferredPhase = Phase::OIL;
                OpmLog::warning("LIQ_PREFERRED_PHASE",
                                "LIQ preferred phase not supported for well " + wellName + ", using OIL instead");
            } else {
                preferredPhase = get_phase(phaseStr);
            }
        }
        const auto& refDepthItem = record.getItem("REF_DEPTH");
        std::optional<double> ref_depth;
        if (refDepthItem.hasValue( 0 ))
            ref_depth = refDepthItem.getSIDouble( 0 );

        double drainageRadius = record.getItem( "D_RADIUS" ).getSIDouble(0);

        bool allowCrossFlow = true;
        const std::string& allowCrossFlowStr = record.getItem<ParserKeywords::WELSPECS::CROSSFLOW>().getTrimmedString(0);
        if (allowCrossFlowStr == "NO")
            allowCrossFlow = false;

        bool automaticShutIn = true;
        const std::string& automaticShutInStr = record.getItem<ParserKeywords::WELSPECS::AUTO_SHUTIN>().getTrimmedString(0);
        if (automaticShutInStr == "STOP") {
            automaticShutIn = false;
        }

        const std::string& group = record.getItem<ParserKeywords::WELSPECS::GROUP>().getTrimmedString(0);
        auto pvt_table = record.getItem<ParserKeywords::WELSPECS::P_TABLE>().get<int>(0);
        auto gas_inflow = Well::GasInflowEquationFromString( record.getItem<ParserKeywords::WELSPECS::INFLOW_EQ>().get<std::string>(0) );

        this->addWell(wellName,
                      group,
                      headI,
                      headJ,
                      preferredPhase,
                      ref_depth,
                      drainageRadius,
                      allowCrossFlow,
                      automaticShutIn,
                      pvt_table,
                      gas_inflow,
                      timeStep,
                      wellConnectionOrder,
                      unit_system);
    }

    void Schedule::addWell(Well well, std::size_t report_step) {
        const std::string wname = well.name();

        m_events.addEvent( ScheduleEvents::NEW_WELL , report_step );
        wellgroup_events.insert( std::make_pair(wname, Events(this->m_timeMap)));
        this->addWellGroupEvent(wname, ScheduleEvents::NEW_WELL, report_step);

        well.setInsertIndex(this->wells_static.size());
        this->wells_static.insert( std::make_pair(wname, DynamicState<std::shared_ptr<Well>>(m_timeMap, nullptr)));
        auto& dynamic_well_state = this->wells_static.at(wname);
        dynamic_well_state.update(report_step, std::make_shared<Well>(std::move(well)));
    }

    void Schedule::addWell(const std::string& wellName,
                           const std::string& group,
                           int headI,
                           int headJ,
                           Phase preferredPhase,
                           const std::optional<double>& ref_depth,
                           double drainageRadius,
                           bool allowCrossFlow,
                           bool automaticShutIn,
                           int pvt_table,
                           Well::GasInflowEquation gas_inflow,
                           std::size_t timeStep,
                           Connection::Order wellConnectionOrder,
                           const UnitSystem& unit_system) {

        Well well(wellName,
                  group,
                  timeStep,
                  0,
                  headI, headJ,
                  ref_depth,
                  WellType(preferredPhase),
                  this->global_whistctl_mode[timeStep],
                  wellConnectionOrder,
                  unit_system,
                  this->getUDQConfig(timeStep).params().undefinedValue(),
                  drainageRadius,
                  allowCrossFlow,
                  automaticShutIn,
                  pvt_table,
                  gas_inflow);

        this->addWell( std::move(well), timeStep );
    }


    std::size_t Schedule::numWells() const {
        return wells_static.size();
    }

    std::size_t Schedule::numWells(std::size_t timestep) const {
        auto well_names = this->wellNames(timestep);
        return well_names.size();
    }

    bool Schedule::hasWell(const std::string& wellName) const {
        return wells_static.count( wellName ) > 0;
    }

    bool Schedule::hasWell(const std::string& wellName, std::size_t timeStep) const {
        if (this->wells_static.count(wellName) == 0)
            return false;

        const auto& well = this->getWellatEnd(wellName);
        return well.hasBeenDefined(timeStep);
    }

    std::vector< const Group* > Schedule::getChildGroups2(const std::string& group_name, std::size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: '" + group_name + "'");

        const auto& group = getGroup(group_name, timeStep);
        std::vector<const Group*> child_groups;

        if (group.defined( timeStep )) {
            for (const auto& child_name : group.groups()) {
                child_groups.push_back( std::addressof(this->getGroup(child_name, timeStep)));
            }
        }

        return child_groups;
    }

    std::vector< Well > Schedule::getChildWells2(const std::string& group_name, std::size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: '" + group_name + "'");

        const auto& dynamic_state = this->groups.at(group_name);
        const auto& group_ptr = dynamic_state.get(timeStep);
        if (group_ptr) {
            std::vector<Well> wells;

            if (group_ptr->groups().size()) {
                for (const auto& child_name : group_ptr->groups()) {
                    const auto& child_wells = getChildWells2(child_name, timeStep);
                    wells.insert(wells.end(), child_wells.begin(), child_wells.end());
                }
            } else {
                for (const auto& well_name : group_ptr->wells()) {
                    wells.push_back( this->getWell(well_name, timeStep));
                }
            }

            return wells;
        } else {
            return {};
        }
    }

    /*
      This function will return a list of wells which have changed
      *structurally* in the last report_step; wells where only production
      settings have changed will not be included.
    */
    std::vector<std::string> Schedule::changed_wells(std::size_t report_step) const {
        std::vector<std::string> wells;

        for (const auto& dynamic_pair : this->wells_static) {
            const auto& well_ptr = dynamic_pair.second.get(report_step);
            if (well_ptr) {
                if (report_step > 0) {
                    const auto& prev = dynamic_pair.second.get(report_step - 1);
                    if (prev) {
                        if (!well_ptr->cmp_structure( *prev ))
                            wells.push_back( well_ptr->name() );
                    } else {
                        wells.push_back( well_ptr->name() );
                    }
                } else {
                    wells.push_back( well_ptr->name() );
                }
            }
        }

        return wells;
    }


    std::vector<Well> Schedule::getWells(std::size_t timeStep) const {
        std::vector<Well> wells;
        if (timeStep >= this->m_timeMap.size())
            throw std::invalid_argument("timeStep argument beyond the length of the simulation");

        for (const auto& dynamic_pair : this->wells_static) {
            auto& well_ptr = dynamic_pair.second.get(timeStep);
            if (well_ptr)
                wells.push_back(*well_ptr.get());
        }
        return wells;
    }

    std::vector<Well> Schedule::getWellsatEnd() const {
        return this->getWells(this->m_timeMap.size() - 1);
    }

    const Well& Schedule::getWellatEnd(const std::string& well_name) const {
        return this->getWell(well_name, this->m_timeMap.size() - 1);
    }

    const Well& Schedule::getWell(const std::string& wellName, std::size_t timeStep) const {
        if (this->wells_static.count(wellName) == 0)
            throw std::invalid_argument("No such well: " + wellName);

        const auto& dynamic_state = this->wells_static.at(wellName);
        auto& well_ptr = dynamic_state.get(timeStep);
        if (!well_ptr)
            throw std::invalid_argument("Well: " + wellName + " not yet defined at step: " + std::to_string(timeStep));

        return *well_ptr;
    }

    const Group& Schedule::getGroup(const std::string& groupName, std::size_t timeStep) const {
        if (this->groups.count(groupName) == 0)
            throw std::invalid_argument("No such group: '" + groupName + "'");

        const auto& dynamic_state = this->groups.at(groupName);
        auto& group_ptr = dynamic_state.get(timeStep);
        if (!group_ptr)
            throw std::invalid_argument("Group: " + groupName + " not yet defined at step: " + std::to_string(timeStep));

        return *group_ptr;
    }

    void Schedule::updateGroup(std::shared_ptr<Group> group, std::size_t reportStep) {
        auto& dynamic_state = this->groups.at(group->name());
        dynamic_state.update(reportStep, std::move(group));
    }


    void Schedule::updateGuideRateModel(const GuideRateModel& new_model, std::size_t report_step) {
        auto new_config = std::make_shared<GuideRateConfig>(this->guideRateConfig(report_step));
        if (new_config->update_model(new_model))
            this->guide_rate_config.update( report_step, new_config );
    }

    /*
      There are many SCHEDULE keyword which take a wellname as argument. In
      addition to giving a fully qualified name like 'W1' you can also specify
      shell wildcard patterns like like 'W*', you can get all the wells in the
      well-list '*WL'[1] and the wellname '?' is used to get all the wells which
      already have matched a condition in a ACTIONX keyword. This function
      should be one-stop function to get all well names according to a input
      pattern. The timestep argument is used to check that the wells have
      indeed been defined at the point in time we are considering.

      [1]: The leading '*' in a WLIST name should not be interpreted as a shell
           wildcard!
    */


    std::vector<std::string> Schedule::wellNames(const std::string& pattern, std::size_t timeStep, const std::vector<std::string>& matching_wells) const {
        // ACTIONX handler
        if (pattern == "?")
            return { matching_wells.begin(), matching_wells.end() };

        auto wm = this->wellMatcher(timeStep);
        return wm.wells(pattern);
    }


    WellMatcher Schedule::wellMatcher(std::size_t report_step) const {
        std::vector<std::string> wnames;
        for (const auto& well_pair : this->wells_static) {
            const auto& dynamic_state = well_pair.second;
            if (dynamic_state.get(report_step))
                wnames.push_back(well_pair.first);
        }
        return WellMatcher(wnames, this->getWListManager(report_step));
    }


    std::vector<std::string> Schedule::wellNames(const std::string& pattern) const {
        return this->wellNames(pattern, this->size() - 1);
    }

    std::vector<std::string> Schedule::wellNames(std::size_t timeStep) const {
        std::vector<std::string> names;
        for (const auto& well_pair : this->wells_static) {
            const auto& well_name = well_pair.first;
            const auto& dynamic_state = well_pair.second;
            auto open_step = dynamic_state.find_not(nullptr);
            if (open_step.value() <= timeStep)
                names.push_back(well_name);
        }
        return names;
    }

    std::vector<std::string> Schedule::wellNames() const {
        std::vector<std::string> names;
        for (const auto& well_pair : this->wells_static)
            names.push_back(well_pair.first);

        return names;
    }

    std::vector<std::string> Schedule::groupNames(const std::string& pattern, std::size_t timeStep) const {
        if (pattern.size() == 0)
            return {};

        // Normal pattern matching
        auto star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            std::vector<std::string> names;
            for (const auto& group_pair : this->groups) {
                if (name_match(pattern, group_pair.first)) {
                    const auto& dynamic_state = group_pair.second;
                    const auto& group_ptr = dynamic_state.get(timeStep);
                    if (group_ptr)
                        names.push_back(group_pair.first);
                }
            }
            return names;
        }

        // Normal group name without any special characters
        if (this->hasGroup(pattern)) {
            const auto& dynamic_state = this->groups.at(pattern);
            const auto& group_ptr = dynamic_state.get(timeStep);
            if (group_ptr)
                return { pattern };
        }
        return {};
    }

    std::vector<std::string> Schedule::groupNames(std::size_t timeStep) const {
        std::vector<std::string> names;
        for (const auto& group_pair : this->groups) {
            const auto& dynamic_state = group_pair.second;
            const auto& group_ptr = dynamic_state.get(timeStep);
            if (group_ptr)
                names.push_back(group_pair.first);
        }
        return names;
    }

    std::vector<std::string> Schedule::groupNames(const std::string& pattern) const {
        if (pattern.size() == 0)
            return {};

        // Normal pattern matching
        auto star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            int flags = 0;
            std::vector<std::string> names;
            for (const auto& group_pair : this->groups) {
                if (fnmatch(pattern.c_str(), group_pair.first.c_str(), flags) == 0)
                    names.push_back(group_pair.first);
            }
            return names;
        }

        // Normal group name without any special characters
        if (this->hasGroup(pattern))
            return { pattern };

        return {};
    }

    std::vector<std::string> Schedule::groupNames() const {
        std::vector<std::string> names;
        for (const auto& group_pair : this->groups)
            names.push_back(group_pair.first);

        return names;
    }

    std::vector<const Group*> Schedule::restart_groups(std::size_t timeStep) const {
        std::size_t wdmax = this->m_runspec.wellDimensions().maxGroupsInField();
        std::vector<const Group*> rst_groups(wdmax + 1 , nullptr );
        for (const auto& group_name : this->groupNames(timeStep)) {
            const auto& group = this->getGroup(group_name, timeStep);

            if (group.name() == "FIELD")
                rst_groups.back() = &group;
            else
                rst_groups[group.insert_index() - 1] = &group;
        }
        return rst_groups;
    }


    void Schedule::addGroup(const Group& group, std::size_t timeStep) {
        this->groups.insert( std::make_pair( group.name(), DynamicState<std::shared_ptr<Group>>(this->m_timeMap, nullptr)));
        auto group_ptr = std::make_shared<Group>(group);
        auto& dynamic_state = this->groups.at(group.name());
        dynamic_state.update(timeStep, group_ptr);

        this->m_events.addEvent( ScheduleEvents::NEW_GROUP , timeStep );
        this->wellgroup_events.insert( std::make_pair(group.name(), Events(this->m_timeMap)));
        this->addWellGroupEvent(group.name(), ScheduleEvents::NEW_GROUP, timeStep);

        // All newly created groups are attached to the field group,
        // can then be relocated with the GRUPTREE keyword.
        if (group.name() != "FIELD")
            this->addGroupToGroup("FIELD", *group_ptr, timeStep);
    }


    void Schedule::addGroup(const std::string& groupName, std::size_t timeStep, const UnitSystem& unit_system) {
        const std::size_t insert_index = this->groups.size();
        auto udq_undefined = this->getUDQConfig(timeStep).params().undefinedValue();
        auto group = Group{ groupName, insert_index, timeStep, udq_undefined, unit_system };
        this->addGroup(group, timeStep);
    }

    std::size_t Schedule::numGroups() const {
        return groups.size();
    }

    std::size_t Schedule::numGroups(std::size_t timeStep) const {
        const auto group_names = this->groupNames(timeStep);
        return group_names.size();
    }

    bool Schedule::hasGroup(const std::string& groupName) const {
        return groups.count(groupName) > 0;
    }

    bool Schedule::hasGroup(const std::string& groupName, std::size_t timeStep) const {
        if (timeStep >= this->size())
            return false;

        auto grpMap = this->groups.find(groupName);

        return (grpMap != this->groups.end())
            && grpMap->second.at(timeStep);
    }

    void Schedule::addGroupToGroup( const std::string& parent_group, const Group& child_group, std::size_t timeStep) {
        // Add to new parent
        auto& dynamic_state = this->groups.at(parent_group);
        auto parent_ptr = std::make_shared<Group>( *dynamic_state[timeStep] );
        if (parent_ptr->addGroup(child_group.name()))
            this->updateGroup(std::move(parent_ptr), timeStep);

        // Check and update backreference in child
        if (child_group.parent() != parent_group) {
            auto old_parent = std::make_shared<Group>( this->getGroup(child_group.parent(), timeStep) );
            old_parent->delGroup(child_group.name());
            this->updateGroup(std::move(old_parent), timeStep);

            auto child_ptr = std::make_shared<Group>( child_group );
            child_ptr->updateParent(parent_group);
            this->updateGroup(std::move(child_ptr), timeStep);

        }
    }

    void Schedule::addGroupToGroup( const std::string& parent_group, const std::string& child_group, std::size_t timeStep) {
        this->addGroupToGroup(parent_group, this->getGroup(child_group, timeStep), timeStep);
    }

    void Schedule::addWellToGroup( const std::string& group_name, const std::string& well_name , std::size_t timeStep) {
        const auto& well = this->getWell(well_name, timeStep);
        const auto old_gname = well.groupName();
        if (old_gname != group_name) {
            auto well_ptr = std::make_shared<Well>( well );
            well_ptr->updateGroup(group_name);
            this->updateWell(well_ptr, timeStep);
            this->addWellGroupEvent(well_ptr->name(), ScheduleEvents::WELL_WELSPECS_UPDATE, timeStep);

            // Remove well child reference from previous group
            auto group = std::make_shared<Group>(this->getGroup(old_gname, timeStep));
            group->delWell(well_name);
            this->updateGroup(std::move(group), timeStep);
        }

        // Add well child reference to new group
        auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, timeStep));
        group_ptr->addWell(well_name);
        this->updateGroup(group_ptr, timeStep);
        this->m_events.addEvent( ScheduleEvents::GROUP_CHANGE , timeStep);
    }


    const Tuning& Schedule::getTuning(std::size_t timeStep) const {
        return this->m_tuning.get( timeStep );
    }

    const Deck& Schedule::getModifierDeck(std::size_t timeStep) const {
        return m_modifierDeck.iget( timeStep );
    }

    const MessageLimits& Schedule::getMessageLimits() const {
        return m_messageLimits;
    }


    const Events& Schedule::getWellGroupEvents(const std::string& wellGroup) const {
        if (this->wellgroup_events.count(wellGroup) > 0)
            return this->wellgroup_events.at(wellGroup);
        else
            throw std::invalid_argument("No such well og group " + wellGroup);
    }

    void Schedule::addWellGroupEvent(const std::string& wellGroup, ScheduleEvents::Events event, std::size_t reportStep)  {
        auto& events = this->wellgroup_events.at(wellGroup);
        events.addEvent(event, reportStep);
    }

    bool Schedule::hasWellGroupEvent(const std::string& wellGroup, uint64_t event_mask, std::size_t reportStep) const {
        const auto& events = this->getWellGroupEvents(wellGroup);
        return events.hasEvent(event_mask, reportStep);
    }

    const Events& Schedule::getEvents() const {
        return this->m_events;
    }

    const OilVaporizationProperties& Schedule::getOilVaporizationProperties(std::size_t timestep) const {
        return m_oilvaporizationproperties.get(timestep);
    }

    const Well::ProducerCMode& Schedule::getGlobalWhistctlMmode(std::size_t timestep) const {
        return global_whistctl_mode.get(timestep);
    }


    bool Schedule::hasOilVaporizationProperties() const {
        for (std::size_t i = 0; i < this->m_timeMap.size(); ++i)
            if (m_oilvaporizationproperties.at( i ).defined()) return true;

        return false;
    }

    void Schedule::checkIfAllConnectionsIsShut(std::size_t timeStep) {
        const auto& well_names = this->wellNames(timeStep);
        for (const auto& wname : well_names) {
            const auto& well = this->getWell(wname, timeStep);
            const auto& connections = well.getConnections();
            if (connections.allConnectionsShut() && well.getStatus() != Well::Status::SHUT) {
                std::string msg =
                    "All completions in well " + well.name() + " is shut at " + std::to_string ( m_timeMap.getTimePassedUntil(timeStep) / (60*60*24) ) + " days. \n" +
                    "The well is therefore also shut.";
                OpmLog::note(msg);
                this->updateWellStatus( well.name(), timeStep, Well::Status::SHUT, false);
            }
        }
    }


    void Schedule::filterConnections(const ActiveGridCells& grid) {
        for (auto& dynamic_pair : this->wells_static) {
            auto& dynamic_state = dynamic_pair.second;
            for (auto& well_pair : dynamic_state.unique()) {
                if (well_pair.second)
                    well_pair.second->filterConnections(grid);
            }
        }
    }

    const VFPProdTable& Schedule::getVFPProdTable(int table_id, std::size_t timeStep) const {
        const auto pair = vfpprod_tables.find(table_id);
        if (pair == vfpprod_tables.end())
            throw std::invalid_argument("No such table id: " + std::to_string(table_id));

        auto table_ptr = pair->second.get(timeStep);
        if (!table_ptr)
            throw std::invalid_argument("Table not yet defined at timeStep:" + std::to_string(timeStep));

        return *table_ptr;
    }

    const VFPInjTable& Schedule::getVFPInjTable(int table_id, std::size_t timeStep) const {
        const auto pair = vfpinj_tables.find(table_id);
        if (pair == vfpinj_tables.end())
            throw std::invalid_argument("No such table id: " + std::to_string(table_id));

        auto table_ptr = pair->second.get(timeStep);
        if (!table_ptr)
            throw std::invalid_argument("Table not yet defined at timeStep:" + std::to_string(timeStep));

        return *table_ptr;
    }

    std::map<int, std::shared_ptr<const VFPInjTable> > Schedule::getVFPInjTables(std::size_t timeStep) const {
        std::map<int, std::shared_ptr<const VFPInjTable> > tables;
        for (const auto& pair : this->vfpinj_tables) {
            if (pair.second.get(timeStep)) {
                tables.insert(std::make_pair(pair.first, pair.second.get(timeStep)) );
            }
        }
        return tables;
    }

    std::map<int, std::shared_ptr<const VFPProdTable> > Schedule::getVFPProdTables(std::size_t timeStep) const {
        std::map<int, std::shared_ptr<const VFPProdTable> > tables;
        for (const auto& pair : this->vfpprod_tables) {
            if (pair.second.get(timeStep)) {
                tables.insert(std::make_pair(pair.first, pair.second.get(timeStep)) );
            }
        }
        return tables;
    }

    const UDQActive& Schedule::udqActive(std::size_t timeStep) const {
        return *this->udq_active[timeStep];
    }

    void Schedule::updateUDQActive( std::size_t timeStep, std::shared_ptr<UDQActive> udq ) {
        this->udq_active.update(timeStep, udq);
    }

    const WellTestConfig& Schedule::wtestConfig(std::size_t timeStep) const {
        const auto& ptr = this->wtest_config.get(timeStep);
        return *ptr;
    }

    const GConSale& Schedule::gConSale(std::size_t timeStep) const {
        const auto& ptr = this->gconsale.get(timeStep);
        return *ptr;
    }

    const GConSump& Schedule::gConSump(std::size_t timeStep) const {
        const auto& ptr = this->gconsump.get(timeStep);
        return *ptr;
    }

    const WListManager& Schedule::getWListManager(std::size_t timeStep) const {
        const auto& ptr = this->wlist_manager.get(timeStep);
        return *ptr;
    }

    const UDQConfig& Schedule::getUDQConfig(std::size_t timeStep) const {
        const auto& ptr = this->udq_config.get(timeStep);
        return *ptr;
    }

    std::vector<const UDQConfig*> Schedule::udqConfigList() const {
        std::vector<const UDQConfig*> udq_list;
        for (const auto& udq_pair : this->udq_config.unique())
            udq_list.push_back( udq_pair.second.get() );
        return udq_list;
    }

    const GuideRateConfig& Schedule::guideRateConfig(std::size_t timeStep) const {
        const auto& ptr = this->guide_rate_config.get(timeStep);
        return *ptr;
    }

    const RPTConfig& Schedule::report_config(std::size_t timeStep) const {
        const auto& ptr = this->rpt_config.get(timeStep);
        return *ptr;
    }

    std::optional<int> Schedule::exitStatus() const {
        return this->exit_status;
    }

    std::size_t Schedule::size() const {
        return this->m_timeMap.size();
    }


    double  Schedule::seconds(std::size_t timeStep) const {
        return this->m_timeMap.seconds(timeStep);
    }

    time_t Schedule::simTime(std::size_t timeStep) const {
        return this->m_timeMap[timeStep];
    }

    double Schedule::stepLength(std::size_t timeStep) const {
        return this->m_timeMap.getTimeStepLength(timeStep);
    }


    const Action::Actions& Schedule::actions(std::size_t timeStep) const {
        const auto& ptr = this->m_actions.get(timeStep);
        return *ptr;
    }

    void Schedule::applyAction(std::size_t reportStep, const Action::ActionX& action, const Action::Result& result) {
        ParseContext parseContext;
        ErrorGuard errors;

        for (const auto& keyword : action) {
            if (!Action::ActionX::valid_keyword(keyword.name()))
                throw std::invalid_argument("The keyword: " + keyword.name() + " can not be handled in the ACTION body");

            if (keyword.name() == "WELOPEN")
                this->applyWELOPEN(keyword, reportStep, parseContext, errors, result.wells());

            if (keyword.name() == "EXIT")
                this->applyEXIT(keyword, reportStep);

            if (keyword.name() == "UDQ")
                this->updateUDQ(keyword, reportStep);
        }
    }

    void Schedule::applyWellProdIndexScaling(const std::string& well_name, const std::size_t reportStep, const double scalingFactor) {
        auto wstat = this->wells_static.find(well_name);
        if (wstat == this->wells_static.end())
            return;

        auto unique_well_instances = wstat->second.unique();

        auto end   = unique_well_instances.end();
        auto start = std::lower_bound(unique_well_instances.begin(), end, reportStep,
            [](const auto& time_well_pair, const auto lookup) -> bool
        {
            //     time                 < reportStep
            return time_well_pair.first < lookup;
        });

        if (start == end)
            // Report step after last?
            return;

        // Relies on wells_static being OrderedMap<string, DynamicState<shared_ptr<>>>
        // which means unique_well_instances is a vector<pair<report_step, shared_ptr<>>>
        std::vector<bool> scalingApplicable;
        auto wellPtr = start->second;
        wellPtr->applyWellProdIndexScaling(scalingFactor, scalingApplicable);

        for (; start != end; ++start)
            if (! wellPtr->hasSameConnectionsPointers(*start->second)) {
                wellPtr = start->second;
                wellPtr->applyWellProdIndexScaling(scalingFactor, scalingApplicable);
            }
    }

    RestartConfig& Schedule::restart() {
        return this->restart_config;
    }

    const RestartConfig& Schedule::restart() const {
        return this->restart_config;
    }

    int Schedule::getNupcol(std::size_t reportStep) const {
        return this->m_nupcol.get(reportStep);
    }

    bool Schedule::operator==(const Schedule& data) const {
        auto&& comparePtr = [](const auto& t1, const auto& t2) {
                               if ((t1 && !t2) || (!t1 && t2))
                                   return false;
                               if (!t1)
                                   return true;

                               return *t1 == *t2;
        };

        auto&& compareDynState = [comparePtr](const auto& state1, const auto& state2) {
            if (state1.data().size() != state2.data().size())
                return false;
            return std::equal(state1.data().begin(), state1.data().end(),
                              state2.data().begin(), comparePtr);
        };

        auto&& compareMap = [compareDynState](const auto& map1, const auto& map2) {
            if (map1.size() != map2.size())
                return false;
            auto it2 = map2.begin();
            for (const auto& it : map1) {
                if (it.first != it2->first)
                    return false;
                if (!compareDynState(it.second, it2->second))
                    return false;

                ++it2;
            }
            return true;
        };

        return this->m_timeMap == data.m_timeMap &&
               compareMap(this->wells_static, data.wells_static) &&
               compareMap(this->groups, data.groups) &&
               this->m_oilvaporizationproperties == data.m_oilvaporizationproperties &&
               this->m_events == data.m_events &&
               this->m_modifierDeck == data.m_modifierDeck &&
               this->m_tuning == data.m_tuning &&
               this->m_messageLimits == data.m_messageLimits &&
               this->m_runspec == data.m_runspec &&
               compareMap(this->vfpprod_tables, data.vfpprod_tables) &&
               compareMap(this->vfpinj_tables, data.vfpinj_tables) &&
               compareDynState(this->m_network, data.m_network) &&
               compareDynState(this->m_glo, data.m_glo) &&
               compareDynState(this->wtest_config, data.wtest_config) &&
               compareDynState(this->wlist_manager, data.wlist_manager) &&
               compareDynState(this->udq_config, data.udq_config) &&
               compareDynState(this->udq_active, data.udq_active) &&
               compareDynState(this->guide_rate_config, data.guide_rate_config) &&
               compareDynState(this->gconsale, data.gconsale) &&
               compareDynState(this->gconsump, data.gconsump) &&
               this->global_whistctl_mode == data.global_whistctl_mode &&
               compareDynState(this->m_actions, data.m_actions) &&
               compareDynState(this->rpt_config, data.rpt_config) &&
               rft_config  == data.rft_config &&
               this->m_nupcol == data.m_nupcol &&
               this->restart_config == data.restart_config &&
               this->wellgroup_events == data.wellgroup_events;
     }


    std::string Schedule::formatDate(std::time_t t) {
        const auto ts { TimeStampUTC(t) } ;
        return fmt::format("{:04d}-{:02d}-{:02d}" , ts.year(), ts.month(), ts.day());
    }

    std::string Schedule::simulationDays(const UnitSystem& unit_system, std::size_t currentStep) const {
        const double sim_time { unit_system.from_si(UnitSystem::measure::time, simTime(currentStep)) } ;
        return fmt::format("{} {}", sim_time, unit_system.name(UnitSystem::measure::time));
    }

namespace {

    // Duplicated from Well.cpp
    Connection::Order order_from_int(int int_value) {
        switch(int_value) {
        case 0:
            return Connection::Order::TRACK;
        case 1:
            return Connection::Order::DEPTH;
        case 2:
            return Connection::Order::INPUT;
        default:
            throw std::invalid_argument("Invalid integer value: " + std::to_string(int_value) + " encountered when determining connection ordering");
        }
    }
}

    void Schedule::load_rst(const RestartIO::RstState& rst_state, const EclipseGrid& grid, const FieldPropsManager& fp, const UnitSystem& unit_system)
    {
        double udq_undefined = 0;
        const auto report_step = rst_state.header.report_step - 1;

        for (const auto& rst_group : rst_state.groups) {
            auto group = Group{ rst_group, this->groups.size(), static_cast<std::size_t>(report_step), udq_undefined, unit_system };
            this->addGroup(group, report_step);

            if (group.isProductionGroup()) {
                this->m_events.addEvent(ScheduleEvents::GROUP_PRODUCTION_UPDATE, report_step + 1);
                this->addWellGroupEvent(rst_group.name, ScheduleEvents::GROUP_PRODUCTION_UPDATE, report_step + 1);
            }

            if (group.isInjectionGroup()) {
                this->m_events.addEvent(ScheduleEvents::GROUP_INJECTION_UPDATE, report_step + 1);
                this->addWellGroupEvent(rst_group.name, ScheduleEvents::GROUP_INJECTION_UPDATE, report_step + 1);
            }
        }

        for (std::size_t group_index = 0; group_index < rst_state.groups.size(); group_index++) {
            const auto& rst_group = rst_state.groups[group_index];

            if (rst_group.parent_group == 0)
                continue;

            if (rst_group.parent_group == rst_state.header.max_groups_in_field)
                continue;

            const auto& parent_group = rst_state.groups[rst_group.parent_group - 1];
            this->addGroupToGroup(parent_group.name, rst_group.name, report_step);
        }

        for (const auto& rst_well : rst_state.wells) {
            Opm::Well well(rst_well, report_step, unit_system, udq_undefined);
            std::vector<Opm::Connection> rst_connections;

            for (const auto& rst_conn : rst_well.connections)
                rst_connections.emplace_back(rst_conn, grid, fp);

            if (rst_well.segments.empty()) {
                Opm::WellConnections connections(order_from_int(rst_well.completion_ordering),
                                                 rst_well.ij[0],
                                                 rst_well.ij[1],
                                                 rst_connections);
                well.updateConnections( std::make_shared<WellConnections>( std::move(connections) ), grid, fp.get_int("PVTNUM"));
            } else {
                std::unordered_map<int, Opm::Segment> rst_segments;
                for (const auto& rst_segment : rst_well.segments) {
                    Opm::Segment segment(rst_segment);
                    rst_segments.insert(std::make_pair(rst_segment.segment, std::move(segment)));
                }

                auto [connections, segments] = Compsegs::rstUpdate(rst_well, rst_connections, rst_segments);
                well.updateConnections( std::make_shared<WellConnections>(std::move(connections)), grid, fp.get_int("PVTNUM"));
                well.updateSegments( std::make_shared<WellSegments>(std::move(segments) ));
            }

            this->addWell(well, report_step);
            this->addWellToGroup(well.groupName(), well.name(), report_step);
        }

        m_tuning.update(report_step, rst_state.tuning);
        m_events.addEvent( ScheduleEvents::TUNING_CHANGE , report_step);

        {
            const auto& header = rst_state.header;
            bool time_interval = 0;
            GuideRateModel::Target target = GuideRateModel::Target::OIL;
            bool allow_increase = true;
            bool use_free_gas = false;
            if (GuideRateModel::rst_valid(time_interval,
                                          header.guide_rate_a,
                                          header.guide_rate_b,
                                          header.guide_rate_c,
                                          header.guide_rate_d,
                                          header.guide_rate_e,
                                          header.guide_rate_f,
                                          header.guide_rate_damping)) {
                auto guide_rate_model = GuideRateModel(time_interval,
                                                       target,
                                                       header.guide_rate_a,
                                                       header.guide_rate_b,
                                                       header.guide_rate_c,
                                                       header.guide_rate_d,
                                                       header.guide_rate_e,
                                                       header.guide_rate_f,
                                                       allow_increase,
                                                       header.guide_rate_damping,
                                                       use_free_gas);
                this->updateGuideRateModel(guide_rate_model, report_step);
            }
        }
    }

    std::shared_ptr<const Python> Schedule::python() const
    {
        return this->python_handle;
    }


    void Schedule::updateNetwork(std::shared_ptr<Network::ExtNetwork> network, std::size_t report_step) {
        this->m_network.update(report_step, std::move(network));
    }

    const Network::ExtNetwork& Schedule::network(std::size_t report_step) const {
        return *this->m_network[report_step];
    }

    const GasLiftOpt& Schedule::glo(std::size_t report_step) const {
        return *this->m_glo[report_step];
    }

namespace {
/*
  The insane trickery here (thank you Stackoverflow!) is to be able to provide a
  simple templated comparison function

     template <typename T>
     int not_equal(const T& arg1, const T& arg2, const std::string& msg);

  which will print arg1 and arg2 on stderr *if* T supports operator<<, otherwise
  it will just print the typename of T.
*/


template<typename T, typename = int>
struct cmpx
{
    int neq(const T& arg1, const T& arg2, const std::string& msg) {
        if (arg1 == arg2)
            return 0;

        std::cerr << "Error when comparing <" << typeid(arg1).name() << ">: " << msg << std::endl;
        return 1;
    }
};

template <typename T>
struct cmpx<T, decltype(std::cout << T(), 0)>
{
    int neq(const T& arg1, const T& arg2, const std::string& msg) {
        if (arg1 == arg2)
            return 0;

        std::cerr << "Error when comparing: " << msg << " " << arg1 << " != " << arg2 << std::endl;
        return 1;
    }
};


template <typename T>
int not_equal(const T& arg1, const T& arg2, const std::string& msg) {
    return cmpx<T>().neq(arg1, arg2, msg);
}


template <>
int not_equal(const double& arg1, const double& arg2, const std::string& msg) {
    if (Opm::cmp::scalar_equal(arg1, arg2))
        return 0;

    std::cerr << "Error when comparing: " << msg << " " << arg1 << " != " << arg2 << std::endl;
    return 1;
}

template <>
int not_equal(const UDAValue& arg1, const UDAValue& arg2, const std::string& msg) {
    if (arg1.is<double>())
        return not_equal( arg1.get<double>(), arg2.get<double>(), msg);
    else
        return not_equal( arg1.get<std::string>(), arg2.get<std::string>(), msg);
}


std::string well_msg(const std::string& well, const std::string& msg) {
    return "Well: " + well + " " + msg;
}

std::string well_segment_msg(const std::string& well, int segment_number, const std::string& msg) {
    return "Well: " + well + " Segment: " + std::to_string(segment_number) + " " + msg;
}

std::string well_connection_msg(const std::string& well, const Connection& conn, const std::string& msg) {
    return "Well: " + well + " Connection: " + std::to_string(conn.getI()) + ", " + std::to_string(conn.getJ()) + ", " + std::to_string(conn.getK()) + "  " + msg;
}

}

bool Schedule::cmp(const Schedule& sched1, const Schedule& sched2, std::size_t report_step) {
    int count = not_equal(sched1.wellNames(report_step), sched2.wellNames(report_step), "Wellnames");
    if (count != 0)
        return false;

    {
        const auto& tm1 = sched1.getTimeMap();
        const auto& tm2 = sched2.getTimeMap();
        if (not_equal(tm1.size(), tm2.size(), "TimeMap: size()"))
            count += 1;

        for (auto& step_index = report_step; step_index < std::min(tm1.size(), tm2.size()) - 1; step_index++) {
            if (not_equal(tm1[step_index], tm2[step_index], "TimePoint[" + std::to_string(step_index) + "]"))
                count += 1;
        }

    }

    for (const auto& wname : sched1.wellNames(report_step)) {
        const auto& well1 = sched1.getWell(wname, report_step);
        const auto& well2 = sched2.getWell(wname, report_step);
        int well_count = 0;
        {
            const auto& connections2 = well2.getConnections();
            const auto& connections1 = well1.getConnections();

            well_count += not_equal( connections1.ordering(), connections2.ordering(), well_msg(well1.name(), "Connection: ordering"));
            for (std::size_t icon = 0; icon < connections1.size(); icon++) {
                const auto& conn1 = connections1[icon];
                const auto& conn2 = connections2[icon];
                well_count += not_equal( conn1.getI(), conn2.getI(), well_connection_msg(well1.name(), conn1, "I"));
                well_count += not_equal( conn1.getJ() , conn2.getJ() , well_connection_msg(well1.name(), conn1, "J"));
                well_count += not_equal( conn1.getK() , conn2.getK() , well_connection_msg(well1.name(), conn1, "K"));
                well_count += not_equal( conn1.state() , conn2.state(), well_connection_msg(well1.name(), conn1, "State"));
                well_count += not_equal( conn1.dir() , conn2.dir(), well_connection_msg(well1.name(), conn1, "dir"));
                well_count += not_equal( conn1.complnum() , conn2.complnum(), well_connection_msg(well1.name(), conn1, "complnum"));
                well_count += not_equal( conn1.segment() , conn2.segment(), well_connection_msg(well1.name(), conn1, "segment"));
                well_count += not_equal( conn1.kind() , conn2.kind(), well_connection_msg(well1.name(), conn1, "CFKind"));
                well_count += not_equal( conn1.sort_value(), conn2.sort_value(), well_connection_msg(well1.name(), conn1, "sort_value"));


                well_count += not_equal( conn1.CF(), conn2.CF(), well_connection_msg(well1.name(), conn1, "CF"));
                well_count += not_equal( conn1.Kh(), conn2.Kh(), well_connection_msg(well1.name(), conn1, "Kh"));
                well_count += not_equal( conn1.rw(), conn2.rw(), well_connection_msg(well1.name(), conn1, "rw"));
                well_count += not_equal( conn1.depth(), conn2.depth(), well_connection_msg(well1.name(), conn1, "depth"));

                //well_count += not_equal( conn1.r0(), conn2.r0(), well_connection_msg(well1.name(), conn1, "r0"));
                well_count += not_equal( conn1.skinFactor(), conn2.skinFactor(), well_connection_msg(well1.name(), conn1, "skinFactor"));

            }
        }

        if (not_equal(well1.isMultiSegment(), well2.isMultiSegment(), well_msg(well1.name(), "Is MSW")))
            return false;

        if (well1.isMultiSegment()) {
            const auto& segments1 = well1.getSegments();
            const auto& segments2 = well2.getSegments();
            if (not_equal(segments1.size(), segments2.size(), "Segments: size"))
                return false;

            for (std::size_t iseg=0; iseg < segments1.size(); iseg++) {
                const auto& segment1 = segments1[iseg];
                const auto& segment2 = segments2[iseg];
                //const auto& segment2 = segments2.getFromSegmentNumber(segment1.segmentNumber());
                well_count += not_equal(segment1.segmentNumber(), segment2.segmentNumber(), well_segment_msg(well1.name(), segment1.segmentNumber(), "segmentNumber"));
                well_count += not_equal(segment1.branchNumber(), segment2.branchNumber(), well_segment_msg(well1.name(), segment1.segmentNumber(), "branchNumber"));
                well_count += not_equal(segment1.outletSegment(), segment2.outletSegment(), well_segment_msg(well1.name(), segment1.segmentNumber(), "outletSegment"));
                well_count += not_equal(segment1.totalLength(), segment2.totalLength(), well_segment_msg(well1.name(), segment1.segmentNumber(), "totalLength"));
                well_count += not_equal(segment1.depth(), segment2.depth(), well_segment_msg(well1.name(), segment1.segmentNumber(), "depth"));
                well_count += not_equal(segment1.internalDiameter(), segment2.internalDiameter(), well_segment_msg(well1.name(), segment1.segmentNumber(), "internalDiameter"));
                well_count += not_equal(segment1.roughness(), segment2.roughness(), well_segment_msg(well1.name(), segment1.segmentNumber(), "roughness"));
                well_count += not_equal(segment1.crossArea(), segment2.crossArea(), well_segment_msg(well1.name(), segment1.segmentNumber(), "crossArea"));
                well_count += not_equal(segment1.volume(), segment2.volume(), well_segment_msg(well1.name(), segment1.segmentNumber(), "volume"));
            }
        }

        well_count += not_equal(well1.getStatus(), well2.getStatus(), well_msg(well1.name(), "status"));
        {
            const auto& prod1 = well1.getProductionProperties();
            const auto& prod2 = well2.getProductionProperties();
            well_count += not_equal(prod1.name, prod2.name , well_msg(well1.name(), "Prod: name"));
            well_count += not_equal(prod1.OilRate, prod2.OilRate, well_msg(well1.name(), "Prod: OilRate"));
            well_count += not_equal(prod1.GasRate, prod2.GasRate, well_msg(well1.name(), "Prod: GasRate"));
            well_count += not_equal(prod1.WaterRate, prod2.WaterRate, well_msg(well1.name(), "Prod: WaterRate"));
            well_count += not_equal(prod1.LiquidRate, prod2.LiquidRate, well_msg(well1.name(), "Prod: LiquidRate"));
            well_count += not_equal(prod1.ResVRate, prod2.ResVRate, well_msg(well1.name(), "Prod: ResVRate"));
            well_count += not_equal(prod1.BHPTarget, prod2.BHPTarget, well_msg(well1.name(), "Prod: BHPTarget"));
            well_count += not_equal(prod1.THPTarget, prod2.THPTarget, well_msg(well1.name(), "Prod: THPTarget"));
            well_count += not_equal(prod1.VFPTableNumber, prod2.VFPTableNumber, well_msg(well1.name(), "Prod: VFPTableNumber"));
            well_count += not_equal(prod1.ALQValue, prod2.ALQValue, well_msg(well1.name(), "Prod: ALQValue"));
            well_count += not_equal(prod1.predictionMode, prod2.predictionMode, well_msg(well1.name(), "Prod: predictionMode"));
            if (!prod1.predictionMode) {
                well_count += not_equal(prod1.bhp_hist_limit, prod2.bhp_hist_limit, well_msg(well1.name(), "Prod: bhp_hist_limit"));
                well_count += not_equal(prod1.thp_hist_limit, prod2.thp_hist_limit, well_msg(well1.name(), "Prod: thp_hist_limit"));
                well_count += not_equal(prod1.BHPH, prod2.BHPH, well_msg(well1.name(), "Prod: BHPH"));
                well_count += not_equal(prod1.THPH, prod2.THPH, well_msg(well1.name(), "Prod: THPH"));
            }
            well_count += not_equal(prod1.productionControls(), prod2.productionControls(), well_msg(well1.name(), "Prod: productionControls"));
            if (well1.getStatus() == Well::Status::OPEN)
                well_count += not_equal(prod1.controlMode, prod2.controlMode, well_msg(well1.name(), "Prod: controlMode"));
            well_count += not_equal(prod1.whistctl_cmode, prod2.whistctl_cmode, well_msg(well1.name(), "Prod: whistctl_cmode"));
        }
        {
            const auto& inj1 = well1.getInjectionProperties();
            const auto& inj2 = well2.getInjectionProperties();

            well_count += not_equal(inj1.name, inj2.name, well_msg(well1.name(), "Well::Inj: name"));
            well_count += not_equal(inj1.surfaceInjectionRate, inj2.surfaceInjectionRate, well_msg(well1.name(), "Well::Inj: surfaceInjectionRate"));
            well_count += not_equal(inj1.reservoirInjectionRate, inj2.reservoirInjectionRate, well_msg(well1.name(), "Well::Inj: reservoirInjectionRate"));
            well_count += not_equal(inj1.BHPTarget, inj2.BHPTarget, well_msg(well1.name(), "Well::Inj: BHPTarget"));
            well_count += not_equal(inj1.THPTarget, inj2.THPTarget, well_msg(well1.name(), "Well::Inj: THPTarget"));
            well_count += not_equal(inj1.bhp_hist_limit, inj2.bhp_hist_limit, well_msg(well1.name(), "Well::Inj: bhp_hist_limit"));
            well_count += not_equal(inj1.thp_hist_limit, inj2.thp_hist_limit, well_msg(well1.name(), "Well::Inj: thp_hist_limit"));
            well_count += not_equal(inj1.BHPH, inj2.BHPH, well_msg(well1.name(), "Well::Inj: BHPH"));
            well_count += not_equal(inj1.THPH, inj2.THPH, well_msg(well1.name(), "Well::Inj: THPH"));
            well_count += not_equal(inj1.VFPTableNumber, inj2.VFPTableNumber, well_msg(well1.name(), "Well::Inj: VFPTableNumber"));
            well_count += not_equal(inj1.predictionMode, inj2.predictionMode, well_msg(well1.name(), "Well::Inj: predictionMode"));
            well_count += not_equal(inj1.injectionControls, inj2.injectionControls, well_msg(well1.name(), "Well::Inj: injectionControls"));
            well_count += not_equal(inj1.injectorType, inj2.injectorType, well_msg(well1.name(), "Well::Inj: injectorType"));
            well_count += not_equal(inj1.controlMode, inj2.controlMode, well_msg(well1.name(), "Well::Inj: controlMode"));
        }

        {
            well_count += well2.firstTimeStep() > report_step;
            well_count += not_equal( well1.groupName(), well2.groupName(), well_msg(well1.name(), "Well: groupName"));
            well_count += not_equal( well1.getHeadI(), well2.getHeadI(), well_msg(well1.name(), "Well: getHeadI"));
            well_count += not_equal( well1.getHeadJ(), well2.getHeadJ(), well_msg(well1.name(), "Well: getHeadJ"));
            well_count += not_equal( well1.getRefDepth(), well2.getRefDepth(), well_msg(well1.name(), "Well: getRefDepth"));
            well_count += not_equal( well1.isMultiSegment(), well2.isMultiSegment() , well_msg(well1.name(), "Well: isMultiSegment"));
            well_count += not_equal( well1.isAvailableForGroupControl(), well2.isAvailableForGroupControl() , well_msg(well1.name(), "Well: isAvailableForGroupControl"));
            well_count += not_equal( well1.getGuideRate(), well2.getGuideRate(), well_msg(well1.name(), "Well: getGuideRate"));
            well_count += not_equal( well1.getGuideRatePhase(), well2.getGuideRatePhase(), well_msg(well1.name(), "Well: getGuideRatePhase"));
            well_count += not_equal( well1.getGuideRateScalingFactor(), well2.getGuideRateScalingFactor(), well_msg(well1.name(), "Well: getGuideRateScalingFactor"));
            well_count += not_equal( well1.predictionMode(), well2.predictionMode(), well_msg(well1.name(), "Well: predictionMode"));
            well_count += not_equal( well1.canOpen(), well2.canOpen(), well_msg(well1.name(), "Well: canOpen"));
            well_count += not_equal( well1.isProducer(), well2.isProducer(), well_msg(well1.name(), "Well: isProducer"));
            well_count += not_equal( well1.isInjector(), well2.isInjector(), well_msg(well1.name(), "Well: isInjector"));
            if (well1.isInjector())
                well_count += not_equal( well1.injectorType(), well2.injectorType(), well_msg(well1.name(), "Well1: injectorType"));
            well_count += not_equal( well1.seqIndex(), well2.seqIndex(), well_msg(well1.name(), "Well: seqIndex"));
            well_count += not_equal( well1.getAutomaticShutIn(), well2.getAutomaticShutIn(), well_msg(well1.name(), "Well: getAutomaticShutIn"));
            well_count += not_equal( well1.getAllowCrossFlow(), well2.getAllowCrossFlow(), well_msg(well1.name(), "Well: getAllowCrossFlow"));
            well_count += not_equal( well1.getSolventFraction(), well2.getSolventFraction(), well_msg(well1.name(), "Well: getSolventFraction"));
            well_count += not_equal( well1.getStatus(), well2.getStatus(), well_msg(well1.name(), "Well: getStatus"));
            //well_count += not_equal( well1.getInjectionProperties(), well2.getInjectionProperties(), "Well: getInjectionProperties");


            if (well1.isProducer())
                well_count += not_equal( well1.getPreferredPhase(), well2.getPreferredPhase(), well_msg(well1.name(), "Well: getPreferredPhase"));
            well_count += not_equal( well1.getDrainageRadius(), well2.getDrainageRadius(), well_msg(well1.name(), "Well: getDrainageRadius"));
            well_count += not_equal( well1.getEfficiencyFactor(), well2.getEfficiencyFactor(), well_msg(well1.name(), "Well: getEfficiencyFactor"));
        }
        count += well_count;
        if (well_count > 0)
            std::cerr << std::endl;
    }
    return (count == 0);
}
}
