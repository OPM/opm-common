/*
  Copyright 2020 Statoil ASA.

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

#include <exception>
#include <fnmatch.h>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/numeric/cmp.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckSection.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionResult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
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

namespace Opm {

namespace {

    /*
      The function trim_wgname() is used to trim the leading and trailing spaces
      away from the group and well arguments given in the WELSPECS and GRUPTREE
      keywords. If the deck argument contains a leading or trailing space that is
      treated as an input error, and the action taken is regulated by the setting
      ParseContext::PARSE_WGNAME_SPACE.

      Observe that the spaces are trimmed *unconditionally* - i.e. if the
      ParseContext::PARSE_WGNAME_SPACE setting is set to InputError::IGNORE that
      means that we do not inform the user about "our fix", but it is *not* possible
      to configure the parser to leave the spaces intact.
    */
    std::string trim_wgname(const DeckKeyword& keyword, const std::string& wgname_arg, const ParseContext& parseContext, ErrorGuard errors) {
        std::string wgname = trim_copy(wgname_arg);
        if (wgname != wgname_arg)  {
            const auto& location = keyword.location();
            std::string msg_fmt = fmt::format("Problem with keyword {{keyword}}\n"
                                              "In {{file}} line {{line}}\n"
                                              "Illegal space in {} when defining WELL/GROUP.", wgname_arg);
            parseContext.handleError(ParseContext::PARSE_WGNAME_SPACE, msg_fmt, location, errors);
        }
        return wgname;
    }

}


    void Schedule::handleBRANPROP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto ext_network = std::make_shared<Network::ExtNetwork>(this->network(handlerContext.currentStep));

        for (const auto& record : handlerContext.keyword) {
            const auto& downtree_node = record.getItem<ParserKeywords::BRANPROP::DOWNTREE_NODE>().get<std::string>(0);
            const auto& uptree_node = record.getItem<ParserKeywords::BRANPROP::UPTREE_NODE>().get<std::string>(0);

            const int vfp_table = record.getItem<ParserKeywords::BRANPROP::VFP_TABLE>().get<int>(0);

            if (vfp_table == 0) {
                ext_network->drop_branch(uptree_node, downtree_node);
            } else {
                const auto alq_eq = Network::Branch::AlqEqfromString(record.getItem<ParserKeywords::BRANPROP::ALQ_SURFACE_DENSITY>().get<std::string>(0));

                if (alq_eq == Network::Branch::AlqEQ::ALQ_INPUT) {
                    double alq_value = record.getItem<ParserKeywords::BRANPROP::ALQ>().get<double>(0);
                    ext_network->add_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_value));
                } else {
                    ext_network->add_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_eq));
                }
            }
        }

        this->updateNetwork(ext_network, handlerContext.currentStep);
    }

    void Schedule::handleCOMPDAT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        std::unordered_set<std::string> wells;
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, handlerContext.currentStep);
            if (wellnames.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& name : wellnames) {
                auto well2 = std::shared_ptr<Well>(new Well( this->getWell(name, handlerContext.currentStep)));
                auto connections = std::shared_ptr<WellConnections>( new WellConnections( well2->getConnections()));
                connections->loadCOMPDAT(record, handlerContext.grid, handlerContext.fieldPropsManager);
                if (well2->updateConnections(connections, handlerContext.grid, handlerContext.fieldPropsManager.get_int("PVTNUM"))) {
                    this->updateWell(std::move(well2), handlerContext.currentStep);
                    wells.insert( name );
                }
                this->addWellGroupEvent(name, ScheduleEvents::COMPLETION_CHANGE, handlerContext.currentStep);
            }
        }
        m_events.addEvent(ScheduleEvents::COMPLETION_CHANGE, handlerContext.currentStep);

        // In the case the wells reference depth has been defaulted in the
        // WELSPECS keyword we need to force a calculation of the wells
        // reference depth exactly when the COMPDAT keyword has been completely
        // processed.
        for (const auto& wname : wells) {
            const auto& dynamic_state = this->wells_static.at(wname);
            auto& well_ptr = dynamic_state.get(handlerContext.currentStep);
            well_ptr->updateRefDepth();
        }
    }

    void Schedule::handleCOMPLUMP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);

            for (const auto& wname : well_names) {
                auto& dynamic_state = this->wells_static.at(wname);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
                if (well_ptr->handleCOMPLUMP(record))
                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
            }
        }
    }

    /*
      The COMPORD keyword is handled together with the WELSPECS keyword in the
      handleWELSPECS function.
    */
    void Schedule::handleCOMPORD(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const auto& methodItem = record.getItem<ParserKeywords::COMPORD::ORDER_TYPE>();
            if ((methodItem.get< std::string >(0) != "TRACK")  && (methodItem.get< std::string >(0) != "INPUT")) {
                std::string msg_fmt = "Problem with {keyword}\n"
                                      "In {file} line {line}\n"
                                      "Only 'TRACK' and 'INPUT' order are supported";
                parseContext.handleError( ParseContext::UNSUPPORTED_COMPORD_TYPE ,msg_fmt , handlerContext.keyword.location(), errors );
            }
        }
    }

    void Schedule::handleCOMPSEGS(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& record1 = handlerContext.keyword.getRecord(0);
        const std::string& well_name = record1.getItem("WELL").getTrimmedString(0);

        auto& dynamic_state = this->wells_static.at(well_name);
        auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
        if (well_ptr->handleCOMPSEGS(handlerContext.keyword, handlerContext.grid, parseContext, errors))
            this->updateWell(std::move(well_ptr), handlerContext.currentStep);
    }

    void Schedule::handleDRSDT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> max(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            std::fill(max.begin(), max.end(), record.getItem("DRSDT_MAX").getSIDouble(0));
            std::fill(options.begin(), options.end(), record.getItem("Option").get< std::string >(0));
            OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(handlerContext.currentStep);
            OilVaporizationProperties::updateDRSDT(ovp, max, options);
            this->m_oilvaporizationproperties.update( handlerContext.currentStep, ovp );
        }
    }

    void Schedule::handleDRSDTR(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> max(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        std::size_t pvtRegionIdx = 0;
        for (const auto& record : handlerContext.keyword) {
            max[pvtRegionIdx] = record.getItem("DRSDT_MAX").getSIDouble(0);
            options[pvtRegionIdx] = record.getItem("Option").get< std::string >(0);
            pvtRegionIdx++;
        }
        OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(handlerContext.currentStep);
        OilVaporizationProperties::updateDRSDT(ovp, max, options);
        this->m_oilvaporizationproperties.update( handlerContext.currentStep, ovp );
    }

    void Schedule::handleDRVDT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> max(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            std::fill(max.begin(), max.end(), record.getItem("DRVDT_MAX").getSIDouble(0));
            OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(handlerContext.currentStep);
            OilVaporizationProperties::updateDRVDT(ovp, max);
            this->m_oilvaporizationproperties.update( handlerContext.currentStep, ovp );
        }
    }

    void Schedule::handleDRVDTR(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::size_t pvtRegionIdx = 0;
        std::vector<double> max(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            max[pvtRegionIdx] = record.getItem("DRVDT_MAX").getSIDouble(0);
            pvtRegionIdx++;
        }
        OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(handlerContext.currentStep);
        OilVaporizationProperties::updateDRVDT(ovp, max);
        this->m_oilvaporizationproperties.update( handlerContext.currentStep, ovp );
    }

    void Schedule::handleGCONINJE(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto current_step = handlerContext.currentStep;
        const auto& keyword = handlerContext.keyword;
        this->handleGCONINJE(keyword, current_step, parseContext, errors);
    }

    void Schedule::handleGCONINJE(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, current_step, parseContext, errors, keyword);

            const Group::InjectionCMode controlMode = Group::InjectionCModeFromString(record.getItem("CONTROL_MODE").getTrimmedString(0));
            const Phase phase = get_phase( record.getItem("PHASE").getTrimmedString(0));
            const auto surfaceInjectionRate = record.getItem("SURFACE_TARGET").get<UDAValue>(0);
            const auto reservoirInjectionRate = record.getItem("RESV_TARGET").get<UDAValue>(0);
            const auto reinj_target = record.getItem("REINJ_TARGET").get<UDAValue>(0);
            const auto voidage_target = record.getItem("VOIDAGE_TARGET").get<UDAValue>(0);
            const bool is_free = DeckItem::to_bool(record.getItem("FREE").getTrimmedString(0));

            for (const auto& group_name : group_names) {
                const bool availableForGroupControl = is_free && (group_name != "FIELD");
                auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, current_step));
                Group::GroupInjectionProperties injection;
                injection.phase = phase;
                injection.cmode = controlMode;
                injection.surface_max_rate = surfaceInjectionRate;
                injection.resv_max_rate = reservoirInjectionRate;
                injection.target_reinj_fraction = reinj_target;
                injection.target_void_fraction = voidage_target;
                injection.injection_controls = 0;
                injection.available_group_control = availableForGroupControl;

                if (!record.getItem("SURFACE_TARGET").defaultApplied(0))
                    injection.injection_controls += static_cast<int>(Group::InjectionCMode::RATE);

                if (!record.getItem("RESV_TARGET").defaultApplied(0))
                    injection.injection_controls += static_cast<int>(Group::InjectionCMode::RESV);

                if (!record.getItem("REINJ_TARGET").defaultApplied(0))
                    injection.injection_controls += static_cast<int>(Group::InjectionCMode::REIN);

                if (!record.getItem("VOIDAGE_TARGET").defaultApplied(0))
                    injection.injection_controls += static_cast<int>(Group::InjectionCMode::VREP);

                if (record.getItem("REINJECT_GROUP").hasValue(0))
                    injection.reinj_group = record.getItem("REINJECT_GROUP").getTrimmedString(0);

                if (record.getItem("VOIDAGE_GROUP").hasValue(0))
                    injection.voidage_group = record.getItem("VOIDAGE_GROUP").getTrimmedString(0);

                if (group_ptr->updateInjection(injection)) {
                    this->updateGroup(std::move(group_ptr), current_step);
                    m_events.addEvent( ScheduleEvents::GROUP_INJECTION_UPDATE , current_step);
                    this->addWellGroupEvent(group_name, ScheduleEvents::GROUP_INJECTION_UPDATE, current_step);
                }
            }
        }
    }

    void Schedule::handleGCONPROD(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto current_step = handlerContext.currentStep;
        const auto& keyword = handlerContext.keyword;
        this->handleGCONPROD(keyword, current_step, parseContext, errors);
    }

    void Schedule::handleGCONPROD(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, current_step, parseContext, errors, keyword);

            const Group::ProductionCMode controlMode = Group::ProductionCModeFromString(record.getItem("CONTROL_MODE").getTrimmedString(0));
            const Group::ExceedAction exceedAction = Group::ExceedActionFromString(record.getItem("EXCEED_PROC").getTrimmedString(0));

            const bool respond_to_parent = DeckItem::to_bool(record.getItem("RESPOND_TO_PARENT").getTrimmedString(0));

            const auto oil_target = record.getItem("OIL_TARGET").get<UDAValue>(0);
            const auto gas_target = record.getItem("GAS_TARGET").get<UDAValue>(0);
            const auto water_target = record.getItem("WATER_TARGET").get<UDAValue>(0);
            const auto liquid_target = record.getItem("LIQUID_TARGET").get<UDAValue>(0);
            const auto resv_target = record.getItem("RESERVOIR_FLUID_TARGET").getSIDouble(0);

            const bool apply_default_oil_target = record.getItem("OIL_TARGET").defaultApplied(0);
            const bool apply_default_gas_target = record.getItem("GAS_TARGET").defaultApplied(0);
            const bool apply_default_water_target = record.getItem("WATER_TARGET").defaultApplied(0);
            const bool apply_default_liquid_target = record.getItem("LIQUID_TARGET").defaultApplied(0);
            const bool apply_default_resv_target = record.getItem("RESERVOIR_FLUID_TARGET").defaultApplied(0);

            std::optional<std::string> guide_rate_str;
            {
                const auto& item = record.getItem("GUIDE_RATE_DEF");
                if (item.hasValue(0)) {
                    const auto& string_value = record.getItem("GUIDE_RATE_DEF").getTrimmedString(0);
                    if (string_value.size() > 0)
                        guide_rate_str = string_value;
                }

            }

            for (const auto& group_name : group_names) {
                const bool is_field { group_name == "FIELD" } ;
                const bool availableForGroupControl { respond_to_parent && !is_field } ;

                auto guide_rate_def = Group::GuideRateTarget::NO_GUIDE_RATE;
                double guide_rate = 0;
                if (!is_field) {
                    if (guide_rate_str) {
                        guide_rate_def = Group::GuideRateTargetFromString(guide_rate_str.value());

                        if ((guide_rate_def == Group::GuideRateTarget::INJV ||
                             guide_rate_def == Group::GuideRateTarget::POTN ||
                             guide_rate_def == Group::GuideRateTarget::FORM)) {
                            std::string msg_fmt = "Problem with {keyword}\n"
                                "In {file} line {line}\n"
                                "The supplied guide rate will be ignored";
                            parseContext.handleError(ParseContext::SCHEDULE_IGNORED_GUIDE_RATE, msg_fmt, keyword.location(), errors);
                        } else {
                            guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
                            if (guide_rate == 0)
                                guide_rate_def = Group::GuideRateTarget::POTN;
                        }
                    }
                }

                {
                    auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, current_step));
                    Group::GroupProductionProperties production(this->unit_system, group_name);
                    production.gconprod_cmode = controlMode;
                    production.active_cmode = controlMode;
                    production.oil_target = oil_target;
                    production.gas_target = gas_target;
                    production.water_target = water_target;
                    production.liquid_target = liquid_target;
                    production.guide_rate = guide_rate;
                    production.guide_rate_def = guide_rate_def;
                    production.resv_target = resv_target;
                    production.available_group_control = availableForGroupControl;

                    if ((production.gconprod_cmode == Group::ProductionCMode::ORAT) ||
                        (production.gconprod_cmode == Group::ProductionCMode::WRAT) ||
                        (production.gconprod_cmode == Group::ProductionCMode::GRAT) ||
                        (production.gconprod_cmode == Group::ProductionCMode::LRAT))
                        production.exceed_action = Group::ExceedAction::RATE;
                    else
                        production.exceed_action = exceedAction;

                    production.production_controls = 0;

                    if (!apply_default_oil_target)
                        production.production_controls += static_cast<int>(Group::ProductionCMode::ORAT);

                    if (!apply_default_gas_target)
                        production.production_controls += static_cast<int>(Group::ProductionCMode::GRAT);

                    if (!apply_default_water_target)
                        production.production_controls += static_cast<int>(Group::ProductionCMode::WRAT);

                    if (!apply_default_liquid_target)
                        production.production_controls += static_cast<int>(Group::ProductionCMode::LRAT);

                    if (!apply_default_resv_target)
                        production.production_controls += static_cast<int>(Group::ProductionCMode::RESV);

                    if (group_ptr->updateProduction(production)) {
                        auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(current_step) );
                        new_config->update_group(*group_ptr);
                        this->guide_rate_config.update( current_step, std::move(new_config) );

                        this->updateGroup(std::move(group_ptr), current_step);
                        m_events.addEvent(ScheduleEvents::GROUP_PRODUCTION_UPDATE, current_step);
                        this->addWellGroupEvent(group_name, ScheduleEvents::GROUP_PRODUCTION_UPDATE, current_step);

                        auto udq = std::make_shared<UDQActive>(this->udqActive(current_step));
                        if (production.updateUDQActive(this->getUDQConfig(current_step), *udq))
                            this->updateUDQActive(current_step, udq);
                    }
                }
            }
        }
    }

    void Schedule::handleGCONSALE(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& current = *this->gconsale.get(handlerContext.currentStep);
        std::shared_ptr<GConSale> new_gconsale(new GConSale(current));
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);
            auto sales_target = record.getItem("SALES_TARGET").get<UDAValue>(0);
            auto max_rate = record.getItem("MAX_SALES_RATE").get<UDAValue>(0);
            auto min_rate = record.getItem("MIN_SALES_RATE").get<UDAValue>(0);
            std::string procedure = record.getItem("MAX_PROC").getTrimmedString(0);
            auto udqconfig = this->getUDQConfig(handlerContext.currentStep).params().undefinedValue();

            new_gconsale->add(groupName, sales_target, max_rate, min_rate, procedure, udqconfig, this->unit_system);

            auto group_ptr = std::make_shared<Group>(this->getGroup(groupName, handlerContext.currentStep));
            Group::GroupInjectionProperties injection;
            injection.phase = Phase::GAS;
            if (group_ptr->updateInjection(injection)) {
                this->updateGroup(std::move(group_ptr), handlerContext.currentStep);
            }
        }
        this->gconsale.update(handlerContext.currentStep, new_gconsale);
    }

    void Schedule::handleGCONSUMP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& current = *this->gconsump.get(handlerContext.currentStep);
        std::shared_ptr<GConSump> new_gconsump(new GConSump(current));
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);
            auto consumption_rate = record.getItem("GAS_CONSUMP_RATE").get<UDAValue>(0);
            auto import_rate = record.getItem("GAS_IMPORT_RATE").get<UDAValue>(0);

            std::string network_node_name;
            auto network_node = record.getItem("NETWORK_NODE");
            if (!network_node.defaultApplied(0))
                network_node_name = network_node.getTrimmedString(0);

            auto udqconfig = this->getUDQConfig(handlerContext.currentStep).params().undefinedValue();

            new_gconsump->add(groupName, consumption_rate, import_rate, network_node_name, udqconfig, this->unit_system);
        }
        this->gconsump.update(handlerContext.currentStep, new_gconsump);
    }

    void Schedule::handleGEFAC(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const bool transfer = DeckItem::to_bool(record.getItem("TRANSFER_EXT_NET").getTrimmedString(0));
            const auto gefac = record.getItem("EFFICIENCY_FACTOR").get<double>(0);

            for (const auto& group_name : group_names) {
                auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, handlerContext.currentStep));
                if (group_ptr->update_gefac(gefac, transfer))
                    this->updateGroup(std::move(group_ptr), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleGLIFTOPT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        this->handleGLIFTOPT(handlerContext.keyword, handlerContext.currentStep, parseContext, errors);
    }

    void Schedule::handleGLIFTOPT(const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard&errors) {
        auto glo = std::make_shared<GasLiftOpt>( this->glo(report_step) );

        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem<ParserKeywords::GLIFTOPT::GROUP_NAME>().getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, report_step, parseContext, errors, keyword);

            const auto& max_gas_item = record.getItem<ParserKeywords::GLIFTOPT::MAX_LIFT_GAS_SUPPLY>();
            const double max_lift_gas_value = max_gas_item.hasValue(0)
                ? max_gas_item.getSIDouble(0)
                : -1;

            const auto& max_total_item = record.getItem<ParserKeywords::GLIFTOPT::MAX_TOTAL_GAS_RATE>();
            const double max_total_gas_value = max_total_item.hasValue(0)
                ? max_total_item.getSIDouble(0)
                : -1;

            for (const auto& gname : group_names) {
                auto group = GasLiftOpt::Group(gname);
                group.max_lift_gas(max_lift_gas_value);
                group.max_total_gas(max_total_gas_value);

                glo->add_group(group);
            }
        }

        this->m_glo.update(report_step, std::move(glo));
    }

    void Schedule::handleGPMAINT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const auto& target_string = record.getItem<ParserKeywords::GPMAINT::FLOW_TARGET>().get<std::string>(0);

            for (const auto& group_name : group_names) {
                auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, handlerContext.currentStep));
                if (target_string == "NONE") {
                    group_ptr->set_gpmaint();
                } else {
                    GPMaint gpmaint(record);
                    group_ptr->set_gpmaint(std::move(gpmaint));
                }
                this->updateGroup(std::move(group_ptr), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleGRUPNET(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const auto& groupName = record.getItem("NAME").getTrimmedString(0);

            if (!hasGroup(groupName))
                addGroup(groupName , handlerContext.currentStep);

            int table = record.getItem("VFP_TABLE").get< int >(0);

            auto group_ptr = std::make_shared<Group>( this->getGroup(groupName, handlerContext.currentStep) );
            if (group_ptr->updateNetVFPTable(table))
                this->updateGroup(std::move(group_ptr), handlerContext.currentStep);
        }
    }

    void Schedule::handleGRUPTREE(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& childName = trim_wgname(handlerContext.keyword, record.getItem("CHILD_GROUP").get<std::string>(0), parseContext, errors);
            const std::string& parentName = trim_wgname(handlerContext.keyword, record.getItem("PARENT_GROUP").get<std::string>(0), parseContext, errors);

            if (!hasGroup(childName))
                addGroup(childName, handlerContext.currentStep);

            if (!hasGroup(parentName))
                addGroup(parentName, handlerContext.currentStep);

            this->addGroupToGroup(parentName, childName, handlerContext.currentStep);
        }
    }

    void Schedule::handleGUIDERAT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& record = handlerContext.keyword.getRecord(0);

        const double min_calc_delay = record.getItem<ParserKeywords::GUIDERAT::MIN_CALC_TIME>().getSIDouble(0);
        const auto phase = GuideRateModel::TargetFromString(record.getItem<ParserKeywords::GUIDERAT::NOMINATED_PHASE>().getTrimmedString(0));
        const double A = record.getItem<ParserKeywords::GUIDERAT::A>().get<double>(0);
        const double B = record.getItem<ParserKeywords::GUIDERAT::B>().get<double>(0);
        const double C = record.getItem<ParserKeywords::GUIDERAT::C>().get<double>(0);
        const double D = record.getItem<ParserKeywords::GUIDERAT::D>().get<double>(0);
        const double E = record.getItem<ParserKeywords::GUIDERAT::E>().get<double>(0);
        const double F = record.getItem<ParserKeywords::GUIDERAT::F>().get<double>(0);
        const bool allow_increase = DeckItem::to_bool( record.getItem<ParserKeywords::GUIDERAT::ALLOW_INCREASE>().getTrimmedString(0));
        const double damping_factor = record.getItem<ParserKeywords::GUIDERAT::DAMPING_FACTOR>().get<double>(0);
        const bool use_free_gas = DeckItem::to_bool( record.getItem<ParserKeywords::GUIDERAT::USE_FREE_GAS>().getTrimmedString(0));

        const auto new_model = GuideRateModel(min_calc_delay, phase, A, B, C, D, E, F, allow_increase, damping_factor, use_free_gas);
        this->updateGuideRateModel(new_model, handlerContext.currentStep);
    }

    void Schedule::handleLIFTOPT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto glo = std::make_shared<GasLiftOpt>(this->glo(handlerContext.currentStep));

        const auto& record = handlerContext.keyword.getRecord(0);

        const double gaslift_increment = record.getItem<ParserKeywords::LIFTOPT::INCREMENT_SIZE>().getSIDouble(0);
        const double min_eco_gradient = record.getItem<ParserKeywords::LIFTOPT::MIN_ECONOMIC_GRADIENT>().getSIDouble(0);
        const double min_wait = record.getItem<ParserKeywords::LIFTOPT::MIN_INTERVAL_BETWEEN_GAS_LIFT_OPTIMIZATIONS>().getSIDouble(0);
        const bool all_newton = DeckItem::to_bool( record.getItem<ParserKeywords::LIFTOPT::OPTIMISE_GAS_LIFT>().get<std::string>(0) );

        glo->gaslift_increment(gaslift_increment);
        glo->min_eco_gradient(min_eco_gradient);
        glo->min_wait(min_wait);
        glo->all_newton(all_newton);

        this->m_glo.update(handlerContext.currentStep, std::move(glo));
    }

    void Schedule::handleLINCOM(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& record = handlerContext.keyword.getRecord(0);
        const auto alpha = record.getItem<ParserKeywords::LINCOM::ALPHA>().get<UDAValue>(0);
        const auto beta  = record.getItem<ParserKeywords::LINCOM::BETA>().get<UDAValue>(0);
        const auto gamma = record.getItem<ParserKeywords::LINCOM::GAMMA>().get<UDAValue>(0);

        auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(handlerContext.currentStep) );
        auto new_model = new_config->model();

        if (new_model.updateLINCOM(alpha, beta, gamma)) {
            new_config->update_model(new_model);
            this->guide_rate_config.update( handlerContext.currentStep, new_config );
        }
    }

    void Schedule::handleMESSAGES(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        return applyMESSAGES(handlerContext.keyword, handlerContext.currentStep);
    }

    void Schedule::handleMULTFLT (const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        this->m_modifierDeck[handlerContext.currentStep].addKeyword(handlerContext.keyword);
        m_events.addEvent(ScheduleEvents::GEO_MODIFIER, handlerContext.currentStep);
    }

    void Schedule::handleMXUNSUPP(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        std::string msg_fmt = fmt::format("Problem with keyword {{keyword}} at report step {}\n"
                                          "In {{file}} line {{line}}\n"
                                          "OPM does not support grid property modifier {} in the Schedule section", handlerContext.currentStep, handlerContext.keyword.name());
        parseContext.handleError( ParseContext::UNSUPPORTED_SCHEDULE_GEO_MODIFIER , msg_fmt, handlerContext.keyword.location(), errors );
    }

    void Schedule::handleNODEPROP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto ext_network = std::make_shared<Network::ExtNetwork>(this->network(handlerContext.currentStep));

        for (const auto& record : handlerContext.keyword) {
            const auto& name = record.getItem<ParserKeywords::NODEPROP::NAME>().get<std::string>(0);
            const auto& pressure_item = record.getItem<ParserKeywords::NODEPROP::PRESSURE>();

            const bool as_choke = DeckItem::to_bool(record.getItem<ParserKeywords::NODEPROP::AS_CHOKE>().get<std::string>(0));
            const bool add_gas_lift_gas = DeckItem::to_bool(record.getItem<ParserKeywords::NODEPROP::ADD_GAS_LIFT_GAS>().get<std::string>(0));

            Network::Node node { name };

            if (pressure_item.hasValue(0) && (pressure_item.get<double>(0) > 0))
                node.terminal_pressure(pressure_item.getSIDouble(0));

            if (as_choke) {
                std::string target_group = name;
                const auto& target_item = record.getItem<ParserKeywords::NODEPROP::CHOKE_GROUP>();

                if (target_item.hasValue(0))
                    target_group = target_item.get<std::string>(0);

                if (target_group != name) {
                    if (this->hasGroup(name, handlerContext.currentStep)) {
                        const auto& group = this->getGroup(name, handlerContext.currentStep);
                        if (group.numWells() > 0)
                            throw std::invalid_argument("A manifold group must respond to its own target");
                    }
                }

                node.as_choke(target_group);
            }

            node.add_gas_lift_gas(add_gas_lift_gas);
            ext_network->add_node(node);
        }

        this->updateNetwork(ext_network, handlerContext.currentStep);
    }

    void Schedule::handleNUPCOL(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const int nupcol = handlerContext.keyword.getRecord(0).getItem("NUM_ITER").get<int>(0);

        if (handlerContext.keyword.getRecord(0).getItem("NUM_ITER").defaultApplied(0)) {
            std::string msg = "OPM Flow uses 12 as default NUPCOL value";
            OpmLog::note(msg);
        }

        this->m_nupcol.update(handlerContext.currentStep, nupcol);
    }

    void Schedule::handleRPTSCHED(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        this->rpt_config.update(handlerContext.currentStep, std::make_shared<RPTConfig>(handlerContext.keyword));
    }

    void Schedule::handleTUNING(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto numrecords = handlerContext.keyword.size();
        Tuning tuning(m_tuning.get(handlerContext.currentStep));

        if (numrecords > 0) {
            const auto& record1 = handlerContext.keyword.getRecord(0);

            tuning.TSINIT = record1.getItem("TSINIT").getSIDouble(0);
            tuning.TSMAXZ = record1.getItem("TSMAXZ").getSIDouble(0);
            tuning.TSMINZ = record1.getItem("TSMINZ").getSIDouble(0);
            tuning.TSMCHP = record1.getItem("TSMCHP").getSIDouble(0);
            tuning.TSFMAX = record1.getItem("TSFMAX").get< double >(0);
            tuning.TSFMIN = record1.getItem("TSFMIN").get< double >(0);
            tuning.TSFCNV = record1.getItem("TSFCNV").get< double >(0);
            tuning.TFDIFF = record1.getItem("TFDIFF").get< double >(0);
            tuning.THRUPT = record1.getItem("THRUPT").get< double >(0);

            const auto& TMAXWCdeckItem = record1.getItem("TMAXWC");
            if (TMAXWCdeckItem.hasValue(0)) {
                tuning.TMAXWC_has_value = true;
                tuning.TMAXWC = TMAXWCdeckItem.getSIDouble(0);
            }
        }

        if (numrecords > 1) {
            const auto& record2 = handlerContext.keyword.getRecord(1);

            tuning.TRGTTE = record2.getItem("TRGTTE").get< double >(0);
            tuning.TRGCNV = record2.getItem("TRGCNV").get< double >(0);
            tuning.TRGMBE = record2.getItem("TRGMBE").get< double >(0);
            tuning.TRGLCV = record2.getItem("TRGLCV").get< double >(0);
            tuning.XXXTTE = record2.getItem("XXXTTE").get< double >(0);
            tuning.XXXCNV = record2.getItem("XXXCNV").get< double >(0);
            tuning.XXXMBE = record2.getItem("XXXMBE").get< double >(0);
            tuning.XXXLCV = record2.getItem("XXXLCV").get< double >(0);
            tuning.XXXWFL = record2.getItem("XXXWFL").get< double >(0);
            tuning.TRGFIP = record2.getItem("TRGFIP").get< double >(0);

            const auto& TRGSFTdeckItem = record2.getItem("TRGSFT");
            if (TRGSFTdeckItem.hasValue(0)) {
                tuning.TRGSFT_has_value = true;
                tuning.TRGSFT = TRGSFTdeckItem.get< double >(0);
            }

            tuning.THIONX = record2.getItem("THIONX").get< double >(0);
            tuning.TRWGHT = record2.getItem("TRWGHT").get< int >(0);
        }

        if (numrecords > 2) {
            const auto& record3 = handlerContext.keyword.getRecord(2);

            tuning.NEWTMX = record3.getItem("NEWTMX").get< int >(0);
            tuning.NEWTMN = record3.getItem("NEWTMN").get< int >(0);
            tuning.LITMAX = record3.getItem("LITMAX").get< int >(0);
            tuning.LITMIN = record3.getItem("LITMIN").get< int >(0);
            tuning.MXWSIT = record3.getItem("MXWSIT").get< int >(0);
            tuning.MXWPIT = record3.getItem("MXWPIT").get< int >(0);
            tuning.DDPLIM = record3.getItem("DDPLIM").getSIDouble(0);
            tuning.DDSLIM = record3.getItem("DDSLIM").get< double >(0);
            tuning.TRGDPR = record3.getItem("TRGDPR").getSIDouble(0);

            const auto& XXXDPRdeckItem = record3.getItem("XXXDPR");
            if (XXXDPRdeckItem.hasValue(0)) {
                tuning.XXXDPR_has_value = true;
                tuning.XXXDPR = XXXDPRdeckItem.getSIDouble(0);
            }
        } else {
            tuning.MXWSIT = ParserKeywords::TUNING::MXWSIT::defaultValue;
        }

        m_tuning.update(handlerContext.currentStep, tuning);
        m_events.addEvent(ScheduleEvents::TUNING_CHANGE, handlerContext.currentStep);
    }

    void Schedule::handleUDQ(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& current = *this->udq_config.get(handlerContext.currentStep);
        std::shared_ptr<UDQConfig> new_udq = std::make_shared<UDQConfig>(current);
        for (const auto& record : handlerContext.keyword)
            new_udq->add_record(record, handlerContext.keyword.location(), handlerContext.currentStep);

        this->udq_config.update(handlerContext.currentStep, new_udq);
    }

    void Schedule::handleVAPPARS(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            double vap1 = record.getItem("OIL_VAP_PROPENSITY").get< double >(0);
            double vap2 = record.getItem("OIL_DENSITY_PROPENSITY").get< double >(0);
            OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(handlerContext.currentStep);
            OilVaporizationProperties::updateVAPPARS(ovp, vap1, vap2);
            this->m_oilvaporizationproperties.update( handlerContext.currentStep, ovp );
        }
    }

    void Schedule::handleVFPINJ(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::shared_ptr<VFPInjTable> table = std::make_shared<VFPInjTable>(handlerContext.keyword, this->unit_system);
        int table_id = table->getTableNum();

        if (vfpinj_tables.find(table_id) == vfpinj_tables.end()) {
            std::pair<int, DynamicState<std::shared_ptr<VFPInjTable> > > pair = std::make_pair(table_id, DynamicState<std::shared_ptr<VFPInjTable> >(this->m_timeMap, nullptr));
            vfpinj_tables.insert( pair );
        }

        auto& table_state = vfpinj_tables.at(table_id);
        table_state.update(handlerContext.currentStep, table);
        this->m_events.addEvent( ScheduleEvents::VFPINJ_UPDATE , handlerContext.currentStep);
    }

    void Schedule::handleVFPPROD(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::shared_ptr<VFPProdTable> table = std::make_shared<VFPProdTable>(handlerContext.keyword, this->unit_system);
        int table_id = table->getTableNum();

        if (vfpprod_tables.find(table_id) == vfpprod_tables.end()) {
            std::pair<int, DynamicState<std::shared_ptr<VFPProdTable> > > pair = std::make_pair(table_id, DynamicState<std::shared_ptr<VFPProdTable> >(this->m_timeMap, nullptr));
            vfpprod_tables.insert( pair );
        }

        auto& table_state = vfpprod_tables.at(table_id);
        table_state.update(handlerContext.currentStep, table);
        this->m_events.addEvent( ScheduleEvents::VFPPROD_UPDATE , handlerContext.currentStep);
    }

    void Schedule::handleWCONHIST(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const Well::Status status = Well::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                updateWellStatus( well_name , handlerContext.currentStep , status, false );

                const auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
                std::optional<VFPProdTable::ALQ_TYPE> alq_type;
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                const bool switching_from_injector = !well2->isProducer();
                auto properties = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());
                bool update_well = false;

                if (table_nr != 0)
                    alq_type = this->getVFPProdTable(table_nr, handlerContext.currentStep).getALQType();
                properties->handleWCONHIST(alq_type, this->unit_system, record);

                if (switching_from_injector) {
                    properties->resetDefaultBHPLimit();

                    auto inj_props = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                    inj_props->resetBHPLimit();
                    well2->updateInjection(inj_props);
                    update_well = true;
                }

                if (well2->updateProduction(properties))
                    update_well = true;

                if (well2->updatePrediction(false))
                    update_well = true;

                if (well2->updateHasProduced())
                    update_well = true;

                if (update_well) {
                    m_events.addEvent( ScheduleEvents::PRODUCTION_UPDATE , handlerContext.currentStep);
                    this->addWellGroupEvent( well2->name(), ScheduleEvents::PRODUCTION_UPDATE, handlerContext.currentStep);
                    this->updateWell(well2, handlerContext.currentStep);
                }

                if (!well2->getAllowCrossFlow()) {
                    // The numerical content of the rate UDAValues is accessed unconditionally;
                    // since this is in history mode use of UDA values is not allowed anyway.
                    const auto& oil_rate = properties->OilRate;
                    const auto& water_rate = properties->WaterRate;
                    const auto& gas_rate = properties->GasRate;
                    if (oil_rate.zero() && water_rate.zero() && gas_rate.zero()) {
                        std::string msg =
                            "Well " + well2->name() + " is a history matched well with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string(m_timeMap.getTimePassedUntil(handlerContext.currentStep) / (60*60*24)) + " days";
                        OpmLog::note(msg);
                        updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT, false );
                    }
                }
            }
        }
    }

    void Schedule::handleWCONPROD(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const Well::Status status = Well::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                updateWellStatus(well_name, handlerContext.currentStep, status, false);
                const auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
                std::optional<VFPProdTable::ALQ_TYPE> alq_type;
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                const bool switching_from_injector = !well2->isProducer();
                auto properties = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());
                bool update_well = switching_from_injector;
                properties->clearControls();
                if (well2->isAvailableForGroupControl())
                    properties->addProductionControl(Well::ProducerCMode::GRUP);

                if (table_nr != 0)
                    alq_type = this->getVFPProdTable(table_nr, handlerContext.currentStep).getALQType();
                properties->handleWCONPROD(alq_type, this->unit_system, well_name, record);

                if (switching_from_injector)
                    properties->resetDefaultBHPLimit();

                if (well2->updateProduction(properties))
                    update_well = true;

                if (well2->updatePrediction(true))
                    update_well = true;

                if (well2->updateHasProduced())
                    update_well = true;

                if (update_well) {
                    m_events.addEvent( ScheduleEvents::PRODUCTION_UPDATE , handlerContext.currentStep);
                    this->addWellGroupEvent( well2->name(), ScheduleEvents::PRODUCTION_UPDATE, handlerContext.currentStep);
                    this->updateWell(std::move(well2), handlerContext.currentStep);
                }

                auto udq = std::make_shared<UDQActive>(this->udqActive(handlerContext.currentStep));
                if (properties->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), *udq))
                    this->updateUDQActive(handlerContext.currentStep, udq);
            }
        }
    }

    void Schedule::handleWCONINJE(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const Well::Status status = Well::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                updateWellStatus(well_name, handlerContext.currentStep, status, false);

                bool update_well = false;
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto injection = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                injection->handleWCONINJE(record, well2->isAvailableForGroupControl(), well_name);

                if (well2->updateInjection(injection))
                    update_well = true;

                if (well2->updatePrediction(true))
                    update_well = true;

                if (well2->updateHasInjected())
                    update_well = true;

                if (update_well) {
                    this->updateWell(well2, handlerContext.currentStep);
                    m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , handlerContext.currentStep );
                    this->addWellGroupEvent( well_name, ScheduleEvents::INJECTION_UPDATE, handlerContext.currentStep);
                }

                // if the well has zero surface rate limit or reservior rate limit, while does not allow crossflow,
                // it should be turned off.
                if ( ! well2->getAllowCrossFlow() ) {
                    std::string msg =
                        "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                        "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(handlerContext.currentStep) / (60*60*24) ) + " days";

                    if (injection->surfaceInjectionRate.is<double>()) {
                        if (injection->hasInjectionControl(Well::InjectorCMode::RATE) && injection->surfaceInjectionRate.zero()) {
                            OpmLog::note(msg);
                            updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT, false );
                        }
                    }

                    if (injection->reservoirInjectionRate.is<double>()) {
                        if (injection->hasInjectionControl(Well::InjectorCMode::RESV) && injection->reservoirInjectionRate.zero()) {
                            OpmLog::note(msg);
                            updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT, false );
                        }
                    }
                }

                auto udq = std::make_shared<UDQActive>(this->udqActive(handlerContext.currentStep));
                if (injection->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), *udq))
                    this->updateUDQActive(handlerContext.currentStep, udq);
            }
        }
    }

    void Schedule::handleWCONINJH(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const Well::Status status = Well::StatusFromString( record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                updateWellStatus(well_name, handlerContext.currentStep, status, false);

                bool update_well = false;
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto injection = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                injection->handleWCONINJH(record, well2->isProducer(), well_name);

                if (well2->updateInjection(injection))
                    update_well = true;

                if (well2->updatePrediction(false))
                    update_well = true;

                if (well2->updateHasInjected())
                    update_well = true;

                if (update_well) {
                    this->updateWell(well2, handlerContext.currentStep);
                    m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , handlerContext.currentStep );
                    this->addWellGroupEvent( well_name, ScheduleEvents::INJECTION_UPDATE, handlerContext.currentStep);
                }

                if ( ! well2->getAllowCrossFlow() && (injection->surfaceInjectionRate.zero())) {
                    std::string msg =
                        "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                        "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(handlerContext.currentStep) / (60*60*24) ) + " days";
                    OpmLog::note(msg);
                    updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT, false );
                }
            }
        }
    }

    void Schedule::handleWECON(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto econ_limits = std::make_shared<WellEconProductionLimits>( record );
                if (well2->updateEconLimits(econ_limits))
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWEFAC(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELLNAME").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const double& efficiencyFactor = record.getItem("EFFICIENCY_FACTOR").get<double>(0);

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                if (well2->updateEfficiencyFactor(efficiencyFactor))
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWELOPEN (const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        this->applyWELOPEN(handlerContext.keyword, handlerContext.currentStep, parseContext, errors);
    }

    void Schedule::handleWELPI(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        this->handleWELPI(handlerContext.keyword, handlerContext.currentStep, parseContext, errors);
    }

    void Schedule::handleWELPI(const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors) {
        // Keyword structure
        //
        //   WELPI
        //     W1   123.45 /
        //     W2*  456.78 /
        //     *P   111.222 /
        //     **X* 333.444 /
        //   /
        //
        // Interpretation of productivity index (item 2) depends on well's preferred phase.
        using WELL_NAME = ParserKeywords::WELPI::WELL_NAME;
        using PI        = ParserKeywords::WELPI::STEADY_STATE_PRODUCTIVITY_OR_INJECTIVITY_INDEX_VALUE;

        for (const auto& record : keyword) {
            const auto well_names = this->wellNames(record.getItem<WELL_NAME>().getTrimmedString(0),
                                                   report_step);

            if (well_names.empty())
                this->invalidNamePattern(record.getItem<WELL_NAME>().getTrimmedString(0),
                                         report_step, parseContext,
                                         errors, keyword);

            const auto rawProdIndex = record.getItem<PI>().get<double>(0);
            for (const auto& well_name : well_names) {
                auto well2 = std::make_shared<Well>(this->getWell(well_name, report_step));

                // Note: Need to ensure we have an independent copy of
                // well's connections because
                // Well::updateWellProductivityIndex() implicitly mutates
                // internal state in the WellConnections class.
                auto connections = std::make_shared<WellConnections>(well2->getConnections());
                well2->forceUpdateConnections(std::move(connections));
                if (well2->updateWellProductivityIndex(rawProdIndex))
                    this->updateWell(std::move(well2), report_step);

                this->addWellGroupEvent(well_name, ScheduleEvents::WELL_PRODUCTIVITY_INDEX, report_step);
            }
        }

        this->m_events.addEvent(ScheduleEvents::WELL_PRODUCTIVITY_INDEX, report_step);
    }

    void Schedule::handleWELSEGS(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& record1 = handlerContext.keyword.getRecord(0);
        const auto& wname = record1.getItem("WELL").getTrimmedString(0);

        auto& dynamic_state = this->wells_static.at(wname);
        auto well_ptr = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
        if (well_ptr->handleWELSEGS(handlerContext.keyword))
            this->updateWell(std::move(well_ptr), handlerContext.currentStep);
    }

    void Schedule::handleWELSPECS(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto COMPORD_in_timestep = [&]() -> const DeckKeyword* {
            const auto& section = *handlerContext.section;
            auto itr = section.begin() + handlerContext.keywordIndex.value();
            for( ; itr != section.end(); ++itr ) {
                if (itr->name() == "DATES") return nullptr;
                if (itr->name() == "TSTEP") return nullptr;
                if (itr->name() == "COMPORD") return std::addressof( *itr );
            }
            return nullptr;
        };


        const auto& keyword = handlerContext.keyword;
        for (std::size_t recordNr = 0; recordNr < keyword.size(); recordNr++) {
            const auto& record = keyword.getRecord(recordNr);
            const std::string& wellName = trim_wgname(keyword, record.getItem<ParserKeywords::WELSPECS::WELL>().get<std::string>(0), parseContext, errors);
            const std::string& groupName = trim_wgname(keyword, record.getItem<ParserKeywords::WELSPECS::GROUP>().get<std::string>(0), parseContext, errors);
            auto density_calc_type = record.getItem<ParserKeywords::WELSPECS::DENSITY_CALC>().get<std::string>(0);
            auto fip_region_number = record.getItem<ParserKeywords::WELSPECS::FIP_REGION>().get<int>(0);

            if (fip_region_number != 0) {
                const auto& location = keyword.location();
                std::string msg = "The FIP_REGION item in the WELSPECS keyword in file: " + location.filename + " line: " + std::to_string(location.lineno) + " using default value: " + std::to_string(ParserKeywords::WELSPECS::FIP_REGION::defaultValue);
                OpmLog::warning(msg);
            }

            if (density_calc_type != "SEG") {
                const auto& location = keyword.location();
                std::string msg = "The DENSITY_CALC item in the WELSPECS keyword in file: " + location.filename + " line: " + std::to_string(location.lineno) + " using default value: " + ParserKeywords::WELSPECS::DENSITY_CALC::defaultValue;
                OpmLog::warning(msg);
            }

            if (!hasGroup(groupName))
                addGroup(groupName, handlerContext.currentStep);

            if (!hasWell(wellName)) {
                auto wellConnectionOrder = Connection::Order::TRACK;

                if( const auto* compordp = COMPORD_in_timestep() ) {
                     const auto& compord = *compordp;

                    for (std::size_t compordRecordNr = 0; compordRecordNr < compord.size(); compordRecordNr++) {
                        const auto& compordRecord = compord.getRecord(compordRecordNr);

                        const std::string& wellNamePattern = compordRecord.getItem(0).getTrimmedString(0);
                        if (Well::wellNameInWellNamePattern(wellName, wellNamePattern)) {
                            const std::string& compordString = compordRecord.getItem(1).getTrimmedString(0);
                            wellConnectionOrder = Connection::OrderFromString(compordString);
                        }
                    }
                }
                this->addWell(wellName, record, handlerContext.currentStep, wellConnectionOrder);
                this->addWellToGroup(groupName, wellName, handlerContext.currentStep);
            } else {
                const auto headI = record.getItem<ParserKeywords::WELSPECS::HEAD_I>().get<int>(0) - 1;
                const auto headJ = record.getItem<ParserKeywords::WELSPECS::HEAD_J>().get<int>(0) - 1;
                const auto& refDepthItem = record.getItem<ParserKeywords::WELSPECS::REF_DEPTH>();
                int pvt_table = record.getItem<ParserKeywords::WELSPECS::P_TABLE>().get<int>(0);
                double drainageRadius = record.getItem<ParserKeywords::WELSPECS::D_RADIUS>().getSIDouble(0);
                std::optional<double> ref_depth;
                if (refDepthItem.hasValue(0))
                    ref_depth = refDepthItem.getSIDouble(0);
                {
                    bool update = false;
                    auto well2 = std::shared_ptr<Well>(new Well( this->getWell(wellName, handlerContext.currentStep)));
                    update  = well2->updateHead(headI, headJ);
                    update |= well2->updateRefDepth(ref_depth);
                    update |= well2->updateDrainageRadius(drainageRadius);
                    update |= well2->updatePVTTable(pvt_table);

                    if (update) {
                        well2->updateRefDepth();
                        this->updateWell(std::move(well2), handlerContext.currentStep);
                        this->addWellGroupEvent(wellName, ScheduleEvents::WELL_WELSPECS_UPDATE, handlerContext.currentStep);
                    }
                }
            }

            this->addWellToGroup(groupName, wellName, handlerContext.currentStep);
        }
    }

    /*
      The documentation for the WELTARG keyword says that the well
      must have been fully specified and initialized using one of the
      WCONxxxx keywords prior to modifying the well using the WELTARG
      keyword.

      The following implementation of handling the WELTARG keyword
      does not check or enforce in any way that this is done (i.e. it
      is not checked or verified that the well is initialized with any
      WCONxxxx keyword).

      Update: See the discussion following the defintions of the SI factors, due
      to a bad design we currently need the well to be specified with
      WCONPROD / WCONHIST before WELTARG is applied, if not the units for the
      rates will be wrong.
    */
    void Schedule::handleWELTARG(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        const double SiFactorP = this->unit_system.parse("Pressure").getSIScaling();
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const auto cmode = Well::WELTARGCModeFromString(record.getItem("CMODE").getTrimmedString(0));
            const auto new_arg = record.getItem("NEW_VALUE").get<UDAValue>(0);

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                bool update = false;
                if (well2->isProducer()) {
                    auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());
                    prop->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2->updateProduction(prop);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2->updateWellGuideRate(new_arg.get<double>());

                    auto udq = std::make_shared<UDQActive>(this->udqActive(handlerContext.currentStep));
                    if (prop->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), *udq))
                        this->updateUDQActive(handlerContext.currentStep, udq);
                } else {
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                    inj->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2->updateInjection(inj);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2->updateWellGuideRate(new_arg.get<double>());
                }
                if (update)
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWFOAM(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                const auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto foam_properties = std::make_shared<WellFoamProperties>(well2->getFoamProperties());
                foam_properties->handleWFOAM(record);
                if (well2->updateFoamProperties(foam_properties))
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWGRUPCON(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);

            const bool availableForGroupControl = DeckItem::to_bool(record.getItem("GROUP_CONTROLLED").getTrimmedString(0));
            const double guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
            const double scaling_factor = record.getItem("SCALING_FACTOR").get<double>(0);

            for (const auto& well_name : well_names) {
                auto phase = Well::GuideRateTarget::UNDEFINED;
                if (!record.getItem("PHASE").defaultApplied(0)) {
                    std::string guideRatePhase = record.getItem("PHASE").getTrimmedString(0);
                    phase = Well::GuideRateTargetFromString(guideRatePhase);
                }

                auto& dynamic_state = this->wells_static.at(well_name);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
                if (well_ptr->updateWellGuideRate(availableForGroupControl, guide_rate, phase, scaling_factor)) {
                    auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(handlerContext.currentStep) );
                    new_config->update_well(*well_ptr);
                    this->guide_rate_config.update(handlerContext.currentStep, std::move(new_config));

                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
                }
            }
        }
    }

    void Schedule::handleWHISTCTL(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& record = handlerContext.keyword.getRecord(0);
        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const auto controlMode = Well::ProducerCModeFromString( cmodeString );

        if (controlMode != Well::ProducerCMode::NONE) {
            if (!Well::WellProductionProperties::effectiveHistoryProductionControl(controlMode) ) {
                std::string msg = "The WHISTCTL keyword specifies an un-supported control mode " + cmodeString
                    + ", which makes WHISTCTL keyword not affect the simulation at all";
                OpmLog::warning(msg);
            } else {
                this->global_whistctl_mode.update(handlerContext.currentStep, controlMode);
            }
        }

        const std::string bhp_terminate = record.getItem("BPH_TERMINATE").getTrimmedString(0);
        if (bhp_terminate == "YES") {
            std::string msg_fmt = "Problem with {keyword}\n"
                                  "In {file} line {line}\n"
                                  "Setting item 2 in {keyword} to 'YES' to stop the run is not supported";
            parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg_fmt, handlerContext.keyword.location(), errors );
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
            auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
            auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
            auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWINJTEMP(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        // we do not support the "enthalpy" field yet. how to do this is a more difficult
        // question.
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const double temp = record.getItem("TEMPERATURE").getSIDouble(0);

            for (const auto& well_name : well_names) {
                // TODO: Is this the right approach? Setting the well temperature only
                // has an effect on injectors, but specifying it for producers won't hurt
                // and wells can also switch their injector/producer status. Note that
                // modifying the injector properties for producer wells currently leads
                // to a very weird segmentation fault downstream. For now, let's take the
                // water route.
                const auto& well = this->getWell(well_name, handlerContext.currentStep);
                const double current_temp = well.getInjectionProperties().temperature;
                if (current_temp != temp && !well.isProducer()) {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well_ptr->getInjectionProperties());
                    inj->temperature = temp;
                    well_ptr->updateInjection(inj);
                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
                }
            }
        }
    }

    void Schedule::handleWLIFTOPT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto glo = std::make_shared<GasLiftOpt>(this->glo(handlerContext.currentStep));

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WLIFTOPT::WELL>().getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const bool use_glo = DeckItem::to_bool(record.getItem<ParserKeywords::WLIFTOPT::USE_OPTIMIZER>().get<std::string>(0));
            const bool alloc_extra_gas = DeckItem::to_bool( record.getItem<ParserKeywords::WLIFTOPT::ALLOCATE_EXTRA_LIFT_GAS>().get<std::string>(0));
            const double weight_factor = record.getItem<ParserKeywords::WLIFTOPT::WEIGHT_FACTOR>().get<double>(0);
            const double inc_weight_factor = record.getItem<ParserKeywords::WLIFTOPT::DELTA_GAS_RATE_WEIGHT_FACTOR>().get<double>(0);
            const double min_rate = record.getItem<ParserKeywords::WLIFTOPT::MIN_LIFT_GAS_RATE>().getSIDouble(0);
            const auto& max_rate_item = record.getItem<ParserKeywords::WLIFTOPT::MAX_LIFT_GAS_RATE>();

            for (const auto& wname : well_names) {
                auto well = GasLiftOpt::Well(wname, use_glo);

                if (max_rate_item.hasValue(0))
                    well.max_rate( max_rate_item.getSIDouble(0) );

                well.weight_factor(weight_factor);
                well.inc_weight_factor(inc_weight_factor);
                well.min_rate(min_rate);
                well.alloc_extra_gas(alloc_extra_gas);

                glo->add_well(well);
            }
        }

        this->m_glo.update(handlerContext.currentStep, std::move(glo));
    }

    void Schedule::handleWLIST(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const std::string legal_actions = "NEW:ADD:DEL:MOV";
        const auto& current = *this->wlist_manager.get(handlerContext.currentStep);
        std::shared_ptr<WListManager> new_wlm(new WListManager(current));
        for (const auto& record : handlerContext.keyword) {
            const std::string& name = record.getItem("NAME").getTrimmedString(0);
            const std::string& action = record.getItem("ACTION").getTrimmedString(0);
            const std::vector<std::string>& well_args = record.getItem("WELLS").getData<std::string>();
            std::vector<std::string> wells;

            if (legal_actions.find(action) == std::string::npos)
                throw std::invalid_argument("The action:" + action + " is not recognized.");

            for (const auto& well_arg : well_args) {
                const auto& names = this->wellNames(well_arg, handlerContext.currentStep);
                if (names.empty() && well_arg.find("*") == std::string::npos)
                    throw std::invalid_argument("The well: " + well_arg + " has not been defined in the WELSPECS");

                std::move(names.begin(), names.end(), std::back_inserter(wells));
            }

            if (name[0] != '*')
                throw std::invalid_argument("The list name in WLIST must start with a '*'");

            if (action == "NEW")
                new_wlm->newList(name);

            if (!new_wlm->hasList(name))
                throw std::invalid_argument("Invalid well list: " + name);

            auto& wlist = new_wlm->getList(name);
            if (action == "MOV") {
                for (const auto& well : wells)
                    new_wlm->delWell(well);
            }

            if (action == "DEL") {
                for (const auto& well : wells)
                    wlist.del(well);
            } else {
                for (const auto& well : wells)
                    wlist.add(well);
            }

            this->wlist_manager.update(handlerContext.currentStep, new_wlm);
        }
    }

    void Schedule::handleWPIMULT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto& well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);

            for (const auto& wname : well_names) {
                auto& dynamic_state = this->wells_static.at(wname);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
                if (well_ptr->handleWPIMULT(record))
                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWPMITAB(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto polymer_properties = std::make_shared<WellPolymerProperties>( well2->getPolymerProperties() );
                polymer_properties->handleWPMITAB(record);
                if (well2->updatePolymerProperties(polymer_properties))
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWPOLYMER(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                const auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto polymer_properties = std::make_shared<WellPolymerProperties>( well2->getPolymerProperties() );
                polymer_properties->handleWPOLYMER(record);
                if (well2->updatePolymerProperties(polymer_properties))
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWSALT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                const auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto brine_properties = std::make_shared<WellBrineProperties>(well2->getBrineProperties());
                brine_properties->handleWSALT(record);
                if (well2->updateBrineProperties(brine_properties))
                    this->updateWell(well2, handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWSEGITER(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        Tuning tuning(m_tuning.get(handlerContext.currentStep));

        const auto& record = handlerContext.keyword.getRecord(0);

        tuning.MXWSIT = record.getItem<ParserKeywords::WSEGITER::MAX_WELL_ITERATIONS>().get<int>(0);
        tuning.WSEG_MAX_RESTART = record.getItem<ParserKeywords::WSEGITER::MAX_TIMES_REDUCED>().get<int>(0);
        tuning.WSEG_REDUCTION_FACTOR = record.getItem<ParserKeywords::WSEGITER::REDUCTION_FACTOR>().get<double>(0);
        tuning.WSEG_INCREASE_FACTOR = record.getItem<ParserKeywords::WSEGITER::INCREASING_FACTOR>().get<double>(0);

        m_tuning.update(handlerContext.currentStep, tuning);
        m_events.addEvent(ScheduleEvents::TUNING_CHANGE, handlerContext.currentStep);
    }

    void Schedule::handleWSEGSICD(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::map<std::string, std::vector<std::pair<int, SICD> > > spiral_icds = SICD::fromWSEGSICD(handlerContext.keyword);

        for (auto& map_elem : spiral_icds) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, handlerContext.currentStep);

            std::vector<std::pair<int, SICD> >& sicd_pairs = map_elem.second;

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );

                const auto& connections = well_ptr->getConnections();
                const auto& segments = well_ptr->getSegments();
                for (auto& [segment_nr, sicd] : sicd_pairs) {
                    const auto& outlet_segment_length = segments.segmentLength( segments.getFromSegmentNumber(segment_nr).outletSegment() );
                    sicd.updateScalingFactor(outlet_segment_length, connections.segment_perf_length(segment_nr));
                }

                if (well_ptr->updateWSEGSICD(sicd_pairs) )
                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWSEGVALV(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const std::map<std::string, std::vector<std::pair<int, Valve> > > valves = Valve::fromWSEGVALV(handlerContext.keyword);

        for (const auto& map_elem : valves) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, handlerContext.currentStep);

            const std::vector<std::pair<int, Valve> >& valve_pairs = map_elem.second;

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
                if (well_ptr->updateWSEGVALV(valve_pairs))
                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWSKPTAB(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[handlerContext.currentStep]);
                auto polymer_properties = std::make_shared<WellPolymerProperties>(well2->getPolymerProperties());
                polymer_properties->handleWSKPTAB(record);
                if (well2->updatePolymerProperties(polymer_properties))
                    this->updateWell(std::move(well2), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWSOLVENT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const double fraction = record.getItem("SOLVENT_FRACTION").get<UDAValue>(0).getSI();

            for (const auto& well_name : well_names) {
                const auto& well = this->getWell(well_name, handlerContext.currentStep);
                const auto& inj = well.getInjectionProperties();
                if (!well.isProducer() && inj.injectorType == InjectorType::GAS) {
                    if (well.getSolventFraction() != fraction) {
                        auto new_well = std::make_shared<Well>(well);
                        new_well->updateSolventFraction(fraction);
                        this->updateWell(std::move(new_well), handlerContext.currentStep);
                    }
                } else {
                    throw std::invalid_argument("The WSOLVENT keyword can only be applied to gas injectors");
                }
            }
        }
    }

    void Schedule::handleWTEMP(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern, handlerContext.currentStep );
            double temp = record.getItem("TEMP").getSIDouble(0);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                // TODO: Is this the right approach? Setting the well temperature only
                // has an effect on injectors, but specifying it for producers won't hurt
                // and wells can also switch their injector/producer status. Note that
                // modifying the injector properties for producer wells currently leads
                // to a very weird segmentation fault downstream. For now, let's take the
                // water route.

                const auto& well = this->getWell(well_name, handlerContext.currentStep);
                const double current_temp = well.getInjectionProperties().temperature;
                if (current_temp != temp && !well.isProducer()) {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[handlerContext.currentStep] );
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well_ptr->getInjectionProperties());
                    inj->temperature = temp;
                    well_ptr->updateInjection(inj);
                    this->updateWell(std::move(well_ptr), handlerContext.currentStep);
                }
            }
        }
    }

    void Schedule::handleWTEST(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& current = *this->wtest_config.get(handlerContext.currentStep);
        std::shared_ptr<WellTestConfig> new_config(new WellTestConfig(current));
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const double test_interval = record.getItem("INTERVAL").getSIDouble(0);
            const std::string& reasons = record.getItem("REASON").get<std::string>(0);
            const int num_test = record.getItem("TEST_NUM").get<int>(0);
            const double startup_time = record.getItem("START_TIME").getSIDouble(0);

            for (const auto& well_name : well_names) {
                if (reasons.empty())
                    new_config->drop_well(well_name);
                else
                    new_config->add_well(well_name, reasons, test_interval, num_test, startup_time, handlerContext.currentStep);
            }
        }
        this->wtest_config.update(handlerContext.currentStep, new_config);
    }

    void Schedule::handleWTRACER(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const double tracerConcentration = record.getItem("CONCENTRATION").get<UDAValue>(0).getSI();
            const std::string& tracerName = record.getItem("TRACER").getTrimmedString(0);

            for (const auto& well_name : well_names) {
                auto well = std::make_shared<Well>(this->getWell(well_name, handlerContext.currentStep));
                auto wellTracerProperties = std::make_shared<WellTracerProperties>(well->getTracerProperties());
                wellTracerProperties->setConcentration(tracerName, tracerConcentration);
                if (well->updateTracer(wellTracerProperties))
                    this->updateWell(std::move(well), handlerContext.currentStep);
            }
        }
    }

    void Schedule::handleWPAVE(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto wpave = std::make_shared<PAvg>( handlerContext.keyword.getRecord(0) );
        for (const auto& wname : this->wellNames(handlerContext.currentStep))
            this->updateWPAVE(wname, handlerContext.currentStep, *wpave );

        this->m_pavg.update( handlerContext.currentStep, std::move(wpave) );
    }

    void Schedule::handleWWPAVE(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            auto wpave = PAvg(record);
            for (const auto& well_name : well_names)
                this->updateWPAVE(well_name, handlerContext.currentStep, wpave);
        }
    }

    bool Schedule::handleNormalKeyword(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        using handler_function = void (Schedule::*)(const HandlerContext&, const ParseContext&, ErrorGuard&);
        static const std::unordered_map<std::string,handler_function> handler_functions = {
            { "BRANPROP", &Schedule::handleBRANPROP },
            { "COMPDAT" , &Schedule::handleCOMPDAT  },
            { "COMPLUMP", &Schedule::handleCOMPLUMP },
            { "COMPORD" , &Schedule::handleCOMPORD  },
            { "COMPSEGS", &Schedule::handleCOMPSEGS },
            { "DRSDT"   , &Schedule::handleDRSDT    },
            { "DRSDTR"  , &Schedule::handleDRSDTR   },
            { "DRVDT"   , &Schedule::handleDRVDT    },
            { "DRVDTR"  , &Schedule::handleDRVDTR   },
            { "GCONINJE", &Schedule::handleGCONINJE },
            { "GCONPROD", &Schedule::handleGCONPROD },
            { "GCONSALE", &Schedule::handleGCONSALE },
            { "GCONSUMP", &Schedule::handleGCONSUMP },
            { "GEFAC"   , &Schedule::handleGEFAC    },
            { "GLIFTOPT", &Schedule::handleGLIFTOPT },
            { "GPMAINT" , &Schedule::handleGPMAINT  },
            { "GRUPNET" , &Schedule::handleGRUPNET  },
            { "GRUPTREE", &Schedule::handleGRUPTREE },
            { "GUIDERAT", &Schedule::handleGUIDERAT },
            { "LIFTOPT" , &Schedule::handleLIFTOPT  },
            { "LINCOM"  , &Schedule::handleLINCOM   },
            { "MESSAGES", &Schedule::handleMESSAGES },
            { "MULTFLT" , &Schedule::handleMULTFLT  },
            { "MULTPV"  , &Schedule::handleMXUNSUPP },
            { "MULTR"   , &Schedule::handleMXUNSUPP },
            { "MULTR-"  , &Schedule::handleMXUNSUPP },
            { "MULTREGT", &Schedule::handleMXUNSUPP },
            { "MULTSIG" , &Schedule::handleMXUNSUPP },
            { "MULTSIGV", &Schedule::handleMXUNSUPP },
            { "MULTTHT" , &Schedule::handleMXUNSUPP },
            { "MULTTHT-", &Schedule::handleMXUNSUPP },
            { "MULTX"   , &Schedule::handleMXUNSUPP },
            { "MULTX-"  , &Schedule::handleMXUNSUPP },
            { "MULTY"   , &Schedule::handleMXUNSUPP },
            { "MULTY-"  , &Schedule::handleMXUNSUPP },
            { "MULTZ"   , &Schedule::handleMXUNSUPP },
            { "MULTZ-"  , &Schedule::handleMXUNSUPP },
            { "NODEPROP", &Schedule::handleNODEPROP },
            { "NUPCOL"  , &Schedule::handleNUPCOL   },
            { "RPTSCHED", &Schedule::handleRPTSCHED },
            { "TUNING"  , &Schedule::handleTUNING   },
            { "UDQ"     , &Schedule::handleUDQ      },
            { "VAPPARS" , &Schedule::handleVAPPARS  },
            { "VFPINJ"  , &Schedule::handleVFPINJ   },
            { "VFPPROD" , &Schedule::handleVFPPROD  },
            { "WCONHIST", &Schedule::handleWCONHIST },
            { "WCONINJE", &Schedule::handleWCONINJE },
            { "WCONINJH", &Schedule::handleWCONINJH },
            { "WCONPROD", &Schedule::handleWCONPROD },
            { "WECON"   , &Schedule::handleWECON    },
            { "WEFAC"   , &Schedule::handleWEFAC    },
            { "WELOPEN" , &Schedule::handleWELOPEN  },
            { "WELPI"   , &Schedule::handleWELPI    },
            { "WELSEGS" , &Schedule::handleWELSEGS  },
            { "WELSPECS", &Schedule::handleWELSPECS },
            { "WELTARG" , &Schedule::handleWELTARG  },
            { "WFOAM"   , &Schedule::handleWFOAM    },
            { "WGRUPCON", &Schedule::handleWGRUPCON },
            { "WHISTCTL", &Schedule::handleWHISTCTL },
            { "WINJTEMP", &Schedule::handleWINJTEMP },
            { "WLIFTOPT", &Schedule::handleWLIFTOPT },
            { "WLIST"   , &Schedule::handleWLIST    },
            { "WPAVE"   , &Schedule::handleWPAVE    },
            { "WWPAVE"  , &Schedule::handleWWPAVE   },
            { "WPIMULT" , &Schedule::handleWPIMULT  },
            { "WPMITAB" , &Schedule::handleWPMITAB  },
            { "WPOLYMER", &Schedule::handleWPOLYMER },
            { "WSALT"   , &Schedule::handleWSALT    },
            { "WSEGITER", &Schedule::handleWSEGITER },
            { "WSEGSICD", &Schedule::handleWSEGSICD },
            { "WSEGVALV", &Schedule::handleWSEGVALV },
            { "WSKPTAB" , &Schedule::handleWSKPTAB  },
            { "WSOLVENT", &Schedule::handleWSOLVENT },
            { "WTEMP"   , &Schedule::handleWTEMP    },
            { "WTEST"   , &Schedule::handleWTEST    },
            { "WTRACER" , &Schedule::handleWTRACER  },
        };

        auto function_iterator = handler_functions.find(handlerContext.keyword.name());
        if (function_iterator == handler_functions.end()) {
            return false;
        }

        try {
            std::invoke(function_iterator->second, this, handlerContext, parseContext, errors);
        } catch (const OpmInputError&) {
            throw;
        } catch (const std::exception& e) {
            const OpmInputError opm_error { e, handlerContext.keyword.location() } ;

            OpmLog::error(opm_error.what());

            std::throw_with_nested(opm_error);
        }

        return true;
    }

}
