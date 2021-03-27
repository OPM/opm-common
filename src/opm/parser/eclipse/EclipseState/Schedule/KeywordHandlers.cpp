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
#include <opm/parser/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionResult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/SICD.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/Valve.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSump.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GConSale.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RPTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
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
        auto ext_network = this->snapshots.back().network.get();

        for (const auto& record : handlerContext.keyword) {
            const auto& downtree_node = record.getItem<ParserKeywords::BRANPROP::DOWNTREE_NODE>().get<std::string>(0);
            const auto& uptree_node = record.getItem<ParserKeywords::BRANPROP::UPTREE_NODE>().get<std::string>(0);
            const int vfp_table = record.getItem<ParserKeywords::BRANPROP::VFP_TABLE>().get<int>(0);

            if (vfp_table == 0) {
                ext_network.drop_branch(uptree_node, downtree_node);
            } else {
                const auto alq_eq = Network::Branch::AlqEqfromString(record.getItem<ParserKeywords::BRANPROP::ALQ_SURFACE_DENSITY>().get<std::string>(0));

                if (alq_eq == Network::Branch::AlqEQ::ALQ_INPUT) {
                    double alq_value = record.getItem<ParserKeywords::BRANPROP::ALQ>().get<double>(0);
                    ext_network.add_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_value));
                } else {
                    ext_network.add_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_eq));
                }
            }
        }

        this->snapshots.back().network.update( std::move( ext_network ));
    }

    void Schedule::handleCOMPDAT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        if (!handlerContext.grid_ptr)
            throw std::logic_error("BUG: Schedule::handleCOMPDAT() has been called with an invalid grid pointer");

        std::unordered_set<std::string> wells;
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, handlerContext.currentStep);
            if (wellnames.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& name : wellnames) {
                auto well2 = this->snapshots.back().wells.get(name);
                auto connections = std::shared_ptr<WellConnections>( new WellConnections( well2.getConnections()));
                connections->loadCOMPDAT(record, *handlerContext.grid_ptr, *handlerContext.fp_ptr, name, handlerContext.keyword.location());
                if (well2.updateConnections(connections, *handlerContext.grid_ptr, handlerContext.fp_ptr->get_int("PVTNUM"))) {
                    this->snapshots.back().wells.update( well2 );
                    wells.insert( name );
                }

                if (connections->empty() && well2.getConnections().empty()) {
                    const auto& location = handlerContext.keyword.location();
                    auto msg = fmt::format("Problem with COMPDAT/{}\n"
                                           "In {} line {}\n"
                                           "Well {} is not connected to grid - will remain SHUT", name, location.filename, location.lineno, name);
                    OpmLog::warning(msg);
                }
                this->snapshots.back().wellgroup_events().addEvent( name, ScheduleEvents::COMPLETION_CHANGE);
            }
        }
        this->snapshots.back().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);

        // In the case the wells reference depth has been defaulted in the
        // WELSPECS keyword we need to force a calculation of the wells
        // reference depth exactly when the COMPDAT keyword has been completely
        // processed.
        for (const auto& wname : wells) {
            auto& well = this->snapshots.back().wells.get( wname );
            well.updateRefDepth();
            this->snapshots.back().wells.update( std::move(well));
        }
    }

    void Schedule::handleCOMPLUMP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);

            for (const auto& wname : well_names) {
                auto well = this->snapshots.back().wells.get(wname);
                if (well.handleCOMPLUMP(record))
                    this->snapshots.back().wells.update( std::move(well) );
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
        if (!handlerContext.grid_ptr)
            throw std::logic_error("BUG: Schedule::handleCOMPDAT() has been called with an invalid grid pointer");

        const auto& record1 = handlerContext.keyword.getRecord(0);
        const std::string& well_name = record1.getItem("WELL").getTrimmedString(0);

        auto well = this->snapshots.back().wells.get( well_name );

        if (well.getConnections().empty()) {
            const auto& location = handlerContext.keyword.location();
            auto msg = fmt::format("Problem with COMPSEGS/{0}\n"
                                   "In {1} line {2}\n"
                                   "Well {0} is not connected to grid - COMPSEGS will be ignored", well_name, location.filename, location.lineno);
            OpmLog::warning(msg);
            return;
        }

        if (well.handleCOMPSEGS(handlerContext.keyword, *handlerContext.grid_ptr, parseContext, errors))
            this->snapshots.back().wells.update( std::move(well) );
    }

    void Schedule::handleDRSDT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = this->m_static.m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRSDT::DRSDT_MAX>().getSIDouble(0);
            const auto& option = record.getItem<ParserKeywords::DRSDT::OPTION>().get< std::string >(0);
            std::fill(maximums.begin(), maximums.end(), max);
            std::fill(options.begin(), options.end(), option);
            auto& ovp = this->snapshots.back().oilvap();
            OilVaporizationProperties::updateDRSDT(ovp, maximums, options);
        }
    }

    void Schedule::handleDRSDTCON(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = this->m_static.m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRSDTCON::DRSDT_MAX>().getSIDouble(0);
            const auto& option = record.getItem<ParserKeywords::DRSDTCON::OPTION>().get< std::string >(0);
            std::fill(maximums.begin(), maximums.end(), max);
            std::fill(options.begin(), options.end(), option);
            auto& ovp = this->snapshots.back().oilvap();
            OilVaporizationProperties::updateDRSDTCON(ovp, maximums, options);
        }
    }

    void Schedule::handleDRSDTR(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = this->m_static.m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        std::size_t pvtRegionIdx = 0;
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRSDTR::DRSDT_MAX>().getSIDouble(0);
            const auto& option = record.getItem<ParserKeywords::DRSDTR::OPTION>().get< std::string >(0);
            maximums[pvtRegionIdx] = max;
            options[pvtRegionIdx] = option;
            pvtRegionIdx++;
        }
        auto& ovp = this->snapshots.back().oilvap();
        OilVaporizationProperties::updateDRSDT(ovp, maximums, options);
    }

    void Schedule::handleDRVDT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = this->m_static.m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRVDTR::DRVDT_MAX>().getSIDouble(0);
            std::fill(maximums.begin(), maximums.end(), max);
            auto& ovp = this->snapshots.back().oilvap();
            OilVaporizationProperties::updateDRVDT(ovp, maximums);
        }
    }

    void Schedule::handleDRVDTR(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::size_t numPvtRegions = this->m_static.m_runspec.tabdims().getNumPVTTables();
        std::size_t pvtRegionIdx = 0;
        std::vector<double> maximums(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRVDTR::DRVDT_MAX>().getSIDouble(0);
            maximums[pvtRegionIdx] = max;
            pvtRegionIdx++;
        }
        auto& ovp = this->snapshots.back().oilvap();
        OilVaporizationProperties::updateDRVDT(ovp, maximums);
    }

    void Schedule::handleEXIT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        if (handlerContext.runtime)
            this->applyEXIT(handlerContext.keyword, handlerContext.currentStep);
    }

    void Schedule::handleGCONINJE(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto current_step = handlerContext.currentStep;
        const auto& keyword = handlerContext.keyword;
        this->handleGCONINJE(keyword, current_step, parseContext, errors);
    }

    void Schedule::handleGCONINJE(const DeckKeyword& keyword, std::size_t current_step, const ParseContext& parseContext, ErrorGuard& errors) {
        using GI = ParserKeywords::GCONINJE;
        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem<GI::GROUP>().getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, current_step, parseContext, errors, keyword);

            const Group::InjectionCMode controlMode = Group::InjectionCModeFromString(record.getItem<GI::CONTROL_MODE>().getTrimmedString(0));
            const Phase phase = get_phase( record.getItem<GI::PHASE>().getTrimmedString(0));
            const auto surfaceInjectionRate = record.getItem<GI::SURFACE_TARGET>().get<UDAValue>(0);
            const auto reservoirInjectionRate = record.getItem<GI::RESV_TARGET>().get<UDAValue>(0);
            const auto reinj_target = record.getItem<GI::REINJ_TARGET>().get<UDAValue>(0);
            const auto voidage_target = record.getItem<GI::VOIDAGE_TARGET>().get<UDAValue>(0);
            const bool is_free = DeckItem::to_bool(record.getItem<GI::RESPOND_TO_PARENT>().getTrimmedString(0));

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

                auto guide_rate_def = Group::GuideRateInjTarget::NO_GUIDE_RATE;
                double guide_rate = 0;
                if (!is_field) {
                    if (guide_rate_str) {
                        guide_rate_def = Group::GuideRateInjTargetFromString(guide_rate_str.value());
                        guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
                    }
                }

                {
                    // FLD overrides item 8 (is_free i.e if FLD the group is available for higher up groups)
                    const bool availableForGroupControl = (is_free || controlMode == Group::InjectionCMode::FLD)&& !is_field;
                    auto new_group = this->snapshots.back().groups.get(group_name);
                    Group::GroupInjectionProperties injection;
                    injection.phase = phase;
                    injection.cmode = controlMode;
                    injection.surface_max_rate = surfaceInjectionRate;
                    injection.resv_max_rate = reservoirInjectionRate;
                    injection.target_reinj_fraction = reinj_target;
                    injection.target_void_fraction = voidage_target;
                    injection.injection_controls = 0;
                    injection.guide_rate = guide_rate;
                    injection.guide_rate_def = guide_rate_def;
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

                    if (new_group.updateInjection(injection)) {
                        auto new_config = this->snapshots.back().guide_rate();
                        new_config.update_injection_group(group_name, injection);
                        this->snapshots.back().guide_rate.update( std::move(new_config));

                        this->snapshots.back().groups.update( std::move(new_group));
                        this->snapshots.back().events().addEvent( ScheduleEvents::GROUP_INJECTION_UPDATE );
                        this->snapshots.back().wellgroup_events().addEvent( group_name, ScheduleEvents::GROUP_INJECTION_UPDATE);
                    }
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

                auto guide_rate_def = Group::GuideRateProdTarget::NO_GUIDE_RATE;
                double guide_rate = 0;
                if (!is_field) {
                    if (guide_rate_str) {
                        guide_rate_def = Group::GuideRateProdTargetFromString(guide_rate_str.value());

                        if ((guide_rate_def == Group::GuideRateProdTarget::INJV ||
                             guide_rate_def == Group::GuideRateProdTarget::POTN ||
                             guide_rate_def == Group::GuideRateProdTarget::FORM)) {
                            std::string msg_fmt = "Problem with {keyword}\n"
                                "In {file} line {line}\n"
                                "The supplied guide rate will be ignored";
                            parseContext.handleError(ParseContext::SCHEDULE_IGNORED_GUIDE_RATE, msg_fmt, keyword.location(), errors);
                        } else {
                            guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
                            if (guide_rate == 0)
                                guide_rate_def = Group::GuideRateProdTarget::POTN;
                        }
                    }
                }

                {
                    // FLD overrides item 8 (respond_to_parent i.e if FLD the group is available for higher up groups)
                    const bool availableForGroupControl { (respond_to_parent || controlMode == Group::ProductionCMode::FLD) && !is_field } ;
                    auto new_group = this->snapshots.back().groups.get(group_name);
                    Group::GroupProductionProperties production(this->m_static.m_unit_system, group_name);
                    production.cmode = controlMode;
                    production.oil_target = oil_target;
                    production.gas_target = gas_target;
                    production.water_target = water_target;
                    production.liquid_target = liquid_target;
                    production.guide_rate = guide_rate;
                    production.guide_rate_def = guide_rate_def;
                    production.resv_target = resv_target;
                    production.available_group_control = availableForGroupControl;

                    if ((production.cmode == Group::ProductionCMode::ORAT) ||
                        (production.cmode == Group::ProductionCMode::WRAT) ||
                        (production.cmode == Group::ProductionCMode::GRAT) ||
                        (production.cmode == Group::ProductionCMode::LRAT))
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

                    if (new_group.updateProduction(production)) {
                        auto new_config = this->snapshots.back().guide_rate();
                        new_config.update_production_group(new_group);
                        this->snapshots.back().guide_rate.update( std::move(new_config));

                        this->snapshots.back().groups.update( std::move(new_group));
                        this->snapshots.back().events().addEvent(ScheduleEvents::GROUP_PRODUCTION_UPDATE);
                        this->snapshots.back().wellgroup_events().addEvent( group_name, ScheduleEvents::GROUP_PRODUCTION_UPDATE);

                        auto udq_active = this->snapshots.back().udq_active.get();
                        if (production.updateUDQActive(this->getUDQConfig(current_step), udq_active))
                            this->snapshots.back().udq_active.update( std::move(udq_active));
                    }
                }
            }
        }
    }

    void Schedule::handleGCONSALE(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto new_gconsale = this->snapshots.back().gconsale.get();
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);
            auto sales_target = record.getItem("SALES_TARGET").get<UDAValue>(0);
            auto max_rate = record.getItem("MAX_SALES_RATE").get<UDAValue>(0);
            auto min_rate = record.getItem("MIN_SALES_RATE").get<UDAValue>(0);
            std::string procedure = record.getItem("MAX_PROC").getTrimmedString(0);
            auto udqconfig = this->getUDQConfig(handlerContext.currentStep).params().undefinedValue();

            new_gconsale.add(groupName, sales_target, max_rate, min_rate, procedure, udqconfig, this->m_static.m_unit_system);

            auto new_group = this->snapshots.back().groups.get( groupName );
            Group::GroupInjectionProperties injection;
            injection.phase = Phase::GAS;
            if (new_group.updateInjection(injection))
                this->snapshots.back().groups.update(new_group);
        }
        this->snapshots.back().gconsale.update( std::move(new_gconsale) );
    }

    void Schedule::handleGCONSUMP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto new_gconsump = this->snapshots.back().gconsump.get();
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);
            auto consumption_rate = record.getItem("GAS_CONSUMP_RATE").get<UDAValue>(0);
            auto import_rate = record.getItem("GAS_IMPORT_RATE").get<UDAValue>(0);

            std::string network_node_name;
            auto network_node = record.getItem("NETWORK_NODE");
            if (!network_node.defaultApplied(0))
                network_node_name = network_node.getTrimmedString(0);

            auto udqconfig = this->getUDQConfig(handlerContext.currentStep).params().undefinedValue();

            new_gconsump.add(groupName, consumption_rate, import_rate, network_node_name, udqconfig, this->m_static.m_unit_system);
        }
        this->snapshots.back().gconsump.update( std::move(new_gconsump) );
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
                auto new_group = this->snapshots.back().groups.get(group_name);
                if (new_group.update_gefac(gefac, transfer)) {
                    this->snapshots.back().wellgroup_events().addEvent( group_name, ScheduleEvents::WELLGROUP_EFFICIENCY_UPDATE);
                    this->snapshots.back().events().addEvent( ScheduleEvents::WELLGROUP_EFFICIENCY_UPDATE );
                    this->snapshots.back().groups.update(std::move(new_group));
                }
            }
        }
    }

    void Schedule::handleGLIFTOPT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        this->handleGLIFTOPT(handlerContext.keyword, handlerContext.currentStep, parseContext, errors);
    }

    void Schedule::handleGLIFTOPT(const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard&errors) {
        auto glo = this->snapshots.back().glo();

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

                glo.add_group(group);
            }
        }

        this->snapshots.back().glo.update( std::move(glo) );
    }

    void Schedule::handleGPMAINT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                invalidNamePattern(groupNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const auto& target_string = record.getItem<ParserKeywords::GPMAINT::FLOW_TARGET>().get<std::string>(0);

            for (const auto& group_name : group_names) {
                auto new_group = this->snapshots.back().groups.get(group_name);
                if (target_string == "NONE") {
                    new_group.set_gpmaint();
                } else {
                    GPMaint gpmaint(record);
                    new_group.set_gpmaint(std::move(gpmaint));
                }
                this->snapshots.back().groups.update( std::move(new_group) );
            }
        }
    }

    void Schedule::handleGRUPNET(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const auto& groupName = record.getItem("NAME").getTrimmedString(0);

            if (!this->snapshots.back().groups.has(groupName))
                addGroup(groupName , handlerContext.currentStep);

            int table = record.getItem("VFP_TABLE").get< int >(0);

            auto new_group = this->snapshots.back().groups.get( groupName );
            if (new_group.updateNetVFPTable(table))
                this->snapshots.back().groups.update( std::move(new_group) );
        }
    }

    void Schedule::handleGRUPTREE(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& childName = trim_wgname(handlerContext.keyword, record.getItem("CHILD_GROUP").get<std::string>(0), parseContext, errors);
            const std::string& parentName = trim_wgname(handlerContext.keyword, record.getItem("PARENT_GROUP").get<std::string>(0), parseContext, errors);

            if (!this->snapshots.back().groups.has(childName))
                addGroup(childName, handlerContext.currentStep);

            if (!this->snapshots.back().groups.has(parentName))
                addGroup(parentName, handlerContext.currentStep);

            this->addGroupToGroup(parentName, childName);
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
        auto glo = this->snapshots.back().glo();

        const auto& record = handlerContext.keyword.getRecord(0);

        const double gaslift_increment = record.getItem<ParserKeywords::LIFTOPT::INCREMENT_SIZE>().getSIDouble(0);
        const double min_eco_gradient = record.getItem<ParserKeywords::LIFTOPT::MIN_ECONOMIC_GRADIENT>().getSIDouble(0);
        const double min_wait = record.getItem<ParserKeywords::LIFTOPT::MIN_INTERVAL_BETWEEN_GAS_LIFT_OPTIMIZATIONS>().getSIDouble(0);
        const bool all_newton = DeckItem::to_bool( record.getItem<ParserKeywords::LIFTOPT::OPTIMISE_GAS_LIFT>().get<std::string>(0) );

        glo.gaslift_increment(gaslift_increment);
        glo.min_eco_gradient(min_eco_gradient);
        glo.min_wait(min_wait);
        glo.all_newton(all_newton);

        this->snapshots.back().glo.update( std::move(glo) );
    }

    void Schedule::handleLINCOM(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& record = handlerContext.keyword.getRecord(0);
        const auto alpha = record.getItem<ParserKeywords::LINCOM::ALPHA>().get<UDAValue>(0);
        const auto beta  = record.getItem<ParserKeywords::LINCOM::BETA>().get<UDAValue>(0);
        const auto gamma = record.getItem<ParserKeywords::LINCOM::GAMMA>().get<UDAValue>(0);

        auto new_config = this->snapshots.back().guide_rate();
        auto new_model = new_config.model();

        if (new_model.updateLINCOM(alpha, beta, gamma)) {
            new_config.update_model(new_model);
            this->snapshots.back().guide_rate.update( std::move( new_config) );
        }
    }

    void Schedule::handleMESSAGES(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        this->snapshots.back().message_limits().update( handlerContext.keyword );
    }

    void Schedule::handleMULTFLT (const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        this->snapshots.back().geo_keywords().push_back(handlerContext.keyword);
        this->snapshots.back().events().addEvent( ScheduleEvents::GEO_MODIFIER );
    }

    void Schedule::handleMXUNSUPP(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        std::string msg_fmt = fmt::format("Problem with keyword {{keyword}} at report step {}\n"
                                          "In {{file}} line {{line}}\n"
                                          "OPM does not support grid property modifier {} in the Schedule section", handlerContext.currentStep, handlerContext.keyword.name());
        parseContext.handleError( ParseContext::UNSUPPORTED_SCHEDULE_GEO_MODIFIER , msg_fmt, handlerContext.keyword.location(), errors );
    }

    void Schedule::handleNODEPROP(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto ext_network = this->snapshots.back().network.get();

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
                    if (this->snapshots.back().groups.has(name)) {
                        const auto& group = this->getGroup(name, handlerContext.currentStep);
                        if (group.numWells() > 0)
                            throw std::invalid_argument("A manifold group must respond to its own target");
                    }
                }

                node.as_choke(target_group);
            }

            node.add_gas_lift_gas(add_gas_lift_gas);
            ext_network.add_node(node);
        }

        this->snapshots.back().network.update( ext_network );
    }

    void Schedule::handleNUPCOL(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const int nupcol = handlerContext.keyword.getRecord(0).getItem("NUM_ITER").get<int>(0);

        if (handlerContext.keyword.getRecord(0).getItem("NUM_ITER").defaultApplied(0)) {
            std::string msg = "OPM Flow uses 12 as default NUPCOL value";
            OpmLog::note(msg);
        }

        this->snapshots.back().update_nupcol(nupcol);
    }

    void Schedule::handleRPTSCHED(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        this->snapshots.back().rpt_config.update( RPTConfig(handlerContext.keyword ));
        auto rst_config = this->snapshots.back().rst_config();
        rst_config.update(handlerContext.keyword, parseContext, errors);
        this->snapshots.back().rst_config.update(std::move(rst_config));
    }

    void Schedule::handleRPTRST(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto rst_config = this->snapshots.back().rst_config();
        rst_config.update(handlerContext.keyword, parseContext, errors);
        this->snapshots.back().rst_config.update(std::move(rst_config));
    }

    /*
      We do not really handle the SAVE keyword, we just interpret it as: Write a
      normal restart file at this report step.
    */
    void Schedule::handleSAVE(const HandlerContext&, const ParseContext&, ErrorGuard&) {
        auto rst_config = this->snapshots.back().rst_config();
        rst_config.save = true;
        this->snapshots.back().rst_config.update(std::move(rst_config));
    }

    void Schedule::handleTUNING(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto numrecords = handlerContext.keyword.size();
        auto tuning = this->snapshots.back().tuning();

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

        this->snapshots.back().update_tuning( std::move( tuning ));
        this->snapshots.back().events().addEvent(ScheduleEvents::TUNING_CHANGE);
    }

    void Schedule::handleUDQ(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto new_udq = this->snapshots.back().udq();
        for (const auto& record : handlerContext.keyword)
            new_udq.add_record(record, handlerContext.keyword.location(), handlerContext.currentStep);

        this->snapshots.back().udq.update( std::move(new_udq) );
    }

    void Schedule::handleVAPPARS(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            double vap1 = record.getItem("OIL_VAP_PROPENSITY").get< double >(0);
            double vap2 = record.getItem("OIL_DENSITY_PROPENSITY").get< double >(0);
            auto& ovp = this->snapshots.back().oilvap();
            OilVaporizationProperties::updateVAPPARS(ovp, vap1, vap2);
        }
    }

    void Schedule::handleVFPINJ(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto table = VFPInjTable(handlerContext.keyword, this->m_static.m_unit_system);
        this->snapshots.back().events().addEvent( ScheduleEvents::VFPINJ_UPDATE );
        this->snapshots.back().vfpinj.update( std::move(table) );
    }

    void Schedule::handleVFPPROD(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto table = VFPProdTable(handlerContext.keyword, this->m_static.m_unit_system);
        this->snapshots.back().events().addEvent( ScheduleEvents::VFPPROD_UPDATE );
        this->snapshots.back().vfpprod.update( std::move(table) );
    }

    void Schedule::handleWCONHIST(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const Well::Status status = Well::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                this->updateWellStatus( well_name , handlerContext.currentStep , status, handlerContext.keyword.location() );

                std::optional<VFPProdTable::ALQ_TYPE> alq_type;
                auto well2 = this->snapshots.back().wells.get( well_name );
                const bool switching_from_injector = !well2.isProducer();
                auto properties = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                bool update_well = false;

                auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
                if(record.getItem("VFP_TABLE").defaultApplied(0))
                    table_nr = properties->VFPTableNumber;

                if (table_nr != 0)
                    alq_type = this->snapshots.back().vfpprod(table_nr).getALQType();
                properties->handleWCONHIST(alq_type, this->m_static.m_unit_system, record);

                if (switching_from_injector) {
                    properties->resetDefaultBHPLimit();

                    auto inj_props = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                    inj_props->resetBHPLimit();
                    well2.updateInjection(inj_props);
                    update_well = true;
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
                }

                if (well2.updateProduction(properties))
                    update_well = true;

                if (well2.updatePrediction(false))
                    update_well = true;

                if (well2.updateHasProduced())
                    update_well = true;

                if (update_well) {
                    this->snapshots.back().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                    this->snapshots.back().wells.update( well2 );
                }

                if (!well2.getAllowCrossFlow()) {
                    // The numerical content of the rate UDAValues is accessed unconditionally;
                    // since this is in history mode use of UDA values is not allowed anyway.
                    const auto& oil_rate = properties->OilRate;
                    const auto& water_rate = properties->WaterRate;
                    const auto& gas_rate = properties->GasRate;
                    if (oil_rate.zero() && water_rate.zero() && gas_rate.zero()) {
                        std::string msg =
                            "Well " + well2.name() + " is a history matched well with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string(this->seconds(handlerContext.currentStep) / (60*60*24)) + " days";
                        OpmLog::note(msg);
                        this->updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT);
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
                bool update_well = this->updateWellStatus(well_name, handlerContext.currentStep, status, handlerContext.keyword.location());
                std::optional<VFPProdTable::ALQ_TYPE> alq_type;
                auto well2 = this->snapshots.back().wells.get( well_name );
                const bool switching_from_injector = !well2.isProducer();
                auto properties = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                properties->clearControls();
                if (well2.isAvailableForGroupControl())
                    properties->addProductionControl(Well::ProducerCMode::GRUP);

                auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
                if(record.getItem("VFP_TABLE").defaultApplied(0))
                    table_nr = properties->VFPTableNumber;

                if (table_nr != 0)
                    alq_type = this->snapshots.back().vfpprod(table_nr).getALQType();
                properties->handleWCONPROD(alq_type, this->m_static.m_unit_system, well_name, record);

                if (switching_from_injector) {
                    properties->resetDefaultBHPLimit();
                    update_well = true;
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
                }

                if (well2.updateProduction(properties))
                    update_well = true;

                if (well2.updatePrediction(true))
                    update_well = true;

                if (well2.updateHasProduced())
                    update_well = true;

                if (update_well) {
                    this->snapshots.back().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                    this->snapshots.back().wells.update( std::move(well2) );
                }

                auto udq_active = this->snapshots.back().udq_active.get();
                if (properties->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), udq_active))
                    this->snapshots.back().udq_active.update( std::move(udq_active));
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
                this->updateWellStatus(well_name, handlerContext.currentStep, status, handlerContext.keyword.location());

                bool update_well = false;
                auto well2 = this->snapshots.back().wells.get( well_name );

                auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                auto previousInjectorType = injection->injectorType;
                injection->handleWCONINJE(record, well2.isAvailableForGroupControl(), well_name);
                const bool switching_from_producer = well2.isProducer();
                if (well2.updateInjection(injection))
                    update_well = true;

                if (switching_from_producer)
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);

                if (well2.updatePrediction(true))
                    update_well = true;

                if (well2.updateHasInjected())
                    update_well = true;

                if (update_well) {
                    this->snapshots.back().events().addEvent(ScheduleEvents::INJECTION_UPDATE);
                    this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                    if(previousInjectorType != injection->injectorType)
                        this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_TYPE_CHANGED);
                    this->snapshots.back().wells.update( std::move(well2) );
                }

                // if the well has zero surface rate limit or reservior rate limit, while does not allow crossflow,
                // it should be turned off.
                if ( ! well2.getAllowCrossFlow() ) {
                    std::string msg =
                        "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                        "This well will be closed at " + std::to_string ( this->seconds(handlerContext.currentStep) / (60*60*24) ) + " days";

                    if (injection->surfaceInjectionRate.is<double>()) {
                        if (injection->hasInjectionControl(Well::InjectorCMode::RATE) && injection->surfaceInjectionRate.zero()) {
                            OpmLog::note(msg);
                            this->updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT);
                        }
                    }

                    if (injection->reservoirInjectionRate.is<double>()) {
                        if (injection->hasInjectionControl(Well::InjectorCMode::RESV) && injection->reservoirInjectionRate.zero()) {
                            OpmLog::note(msg);
                            this->updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT);
                        }
                    }
                }

                auto udq_active = this->snapshots.back().udq_active.get();
                if (injection->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), udq_active))
                    this->snapshots.back().udq_active.update( std::move(udq_active) );
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
                this->updateWellStatus(well_name, handlerContext.currentStep, status, handlerContext.keyword.location());
                bool update_well = false;
                auto well2 = this->snapshots.back().wells.get( well_name );
                auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                auto previousInjectorType = injection->injectorType;
                injection->handleWCONINJH(record, well2.isProducer(), well_name);
                const bool switching_from_producer = well2.isProducer();

                if (well2.updateInjection(injection))
                    update_well = true;

                if (switching_from_producer)
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);

                if (well2.updatePrediction(false))
                    update_well = true;

                if (well2.updateHasInjected())
                    update_well = true;

                if (update_well) {
                    this->snapshots.back().events().addEvent( ScheduleEvents::INJECTION_UPDATE );
                    this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                    if(previousInjectorType != injection->injectorType)
                        this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_TYPE_CHANGED);
                    this->snapshots.back().wells.update( std::move(well2) );
                }

                if ( ! well2.getAllowCrossFlow() && (injection->surfaceInjectionRate.zero())) {
                    std::string msg =
                        "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                        "This well will be closed at " + std::to_string ( this->seconds(handlerContext.currentStep) / (60*60*24) ) + " days";
                    OpmLog::note(msg);
                    this->updateWellStatus( well_name, handlerContext.currentStep, Well::Status::SHUT);
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
                auto well2 = this->snapshots.back().wells.get( well_name );
                auto econ_limits = std::make_shared<WellEconProductionLimits>( record );
                if (well2.updateEconLimits(econ_limits))
                    this->snapshots.back().wells.update( std::move(well2) );
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
                auto well2 = this->snapshots.back().wells.get( well_name );
                if (well2.updateEfficiencyFactor(efficiencyFactor)){
                    this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::WELLGROUP_EFFICIENCY_UPDATE);
                    this->snapshots.back().events().addEvent(ScheduleEvents::WELLGROUP_EFFICIENCY_UPDATE);
                    this->snapshots.back().wells.update( std::move(well2) );
                }
            }
        }
    }

    void Schedule::handleWELOPEN (const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        this->applyWELOPEN(handlerContext.keyword, handlerContext.currentStep, handlerContext.runtime, parseContext, errors, handlerContext.matching_wells, handlerContext.affected_wells);
    }

    void Schedule::handleWELPI(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        if (handlerContext.runtime)
            this->handleWELPIRuntime(handlerContext);
        else
            this->handleWELPI(handlerContext.keyword, handlerContext.currentStep, parseContext, errors);
    }

    void Schedule::handleWELPIRuntime(const HandlerContext& handlerContext) {
        using WELL_NAME = ParserKeywords::WELPI::WELL_NAME;
        using PI        = ParserKeywords::WELPI::STEADY_STATE_PRODUCTIVITY_OR_INJECTIVITY_INDEX_VALUE;

        auto report_step = handlerContext.currentStep;
        for (const auto& record : handlerContext.keyword) {
            const auto well_names = this->wellNames(record.getItem<WELL_NAME>().getTrimmedString(0),
                                                    report_step,
                                                    handlerContext.matching_wells);
            const auto targetPI = record.getItem<PI>().get<double>(0);

            std::vector<bool> scalingApplicable;
            const auto& current_wellpi = *handlerContext.target_wellpi;
            for (const auto& well_name : well_names) {
                auto wellpi_iter = current_wellpi.find(well_name);
                if (wellpi_iter == current_wellpi.end())
                    throw std::logic_error(fmt::format("Missing current PI for well {}", well_name));

                auto new_well = this->getWell(well_name, report_step);
                auto scalingFactor = new_well.convertDeckPI(targetPI) / wellpi_iter->second;
                new_well.updateWellProductivityIndex();
                new_well.applyWellProdIndexScaling(scalingFactor, scalingApplicable);
                this->snapshots.back().wells.update( std::move(new_well) );
                this->snapshots.back().target_wellpi[well_name] = targetPI;

                if (handlerContext.affected_wells)
                    handlerContext.affected_wells->insert(well_name);
            }
        }
    }

    void Schedule::handleWELPI(const DeckKeyword& keyword, std::size_t report_step, const ParseContext& parseContext, ErrorGuard& errors, const std::vector<std::string>& matching_wells) {
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
                                                    report_step,
                                                    matching_wells);

            if (well_names.empty())
                this->invalidNamePattern(record.getItem<WELL_NAME>().getTrimmedString(0),
                                         report_step, parseContext,
                                         errors, keyword);

            const auto rawProdIndex = record.getItem<PI>().get<double>(0);
            for (const auto& well_name : well_names) {
                auto well2 = this->snapshots.back().wells.get(well_name);

                // Note: Need to ensure we have an independent copy of
                // well's connections because
                // Well::updateWellProductivityIndex() implicitly mutates
                // internal state in the WellConnections class.
                auto connections = std::make_shared<WellConnections>(well2.getConnections());
                well2.updateConnections(std::move(connections), false, true);
                if (well2.updateWellProductivityIndex())
                    this->snapshots.back().wells.update( std::move(well2) );

                this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::WELL_PRODUCTIVITY_INDEX);
                this->snapshots.back().target_wellpi[well_name] = rawProdIndex;
            }
        }

        this->snapshots.back().events().addEvent(ScheduleEvents::WELL_PRODUCTIVITY_INDEX);
    }

    void Schedule::handleWELSEGS(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& record1 = handlerContext.keyword.getRecord(0);
        const auto& wname = record1.getItem("WELL").getTrimmedString(0);

        auto well = this->snapshots.back().wells.get(wname);
        if (well.handleWELSEGS(handlerContext.keyword))
            this->snapshots.back().wells.update( std::move(well) );
    }

    void Schedule::handleWELSPECS(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
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

            if (!this->snapshots.back().groups.has(groupName))
                addGroup(groupName, handlerContext.currentStep);

            if (!hasWell(wellName)) {
                auto wellConnectionOrder = Connection::Order::TRACK;

                const auto& compord = handlerContext.block.get("COMPORD");
                if (compord.has_value()) {
                    for (std::size_t compordRecordNr = 0; compordRecordNr < compord->size(); compordRecordNr++) {
                        const auto& compordRecord = compord->getRecord(compordRecordNr);

                        const std::string& wellNamePattern = compordRecord.getItem(0).getTrimmedString(0);
                        if (Well::wellNameInWellNamePattern(wellName, wellNamePattern)) {
                            const std::string& compordString = compordRecord.getItem(1).getTrimmedString(0);
                            wellConnectionOrder = Connection::OrderFromString(compordString);
                        }
                    }
                }
                this->addWell(wellName, record, handlerContext.currentStep, wellConnectionOrder);
                this->addWellToGroup(groupName, wellName, handlerContext.currentStep);
                if (handlerContext.affected_wells)
                    handlerContext.affected_wells->insert(wellName);
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
                    auto well2 = this->snapshots.back().wells.get( wellName );
                    update  = well2.updateHead(headI, headJ);
                    update |= well2.updateRefDepth(ref_depth);
                    update |= well2.updateDrainageRadius(drainageRadius);
                    update |= well2.updatePVTTable(pvt_table);

                    if (update) {
                        well2.updateRefDepth();
                        this->snapshots.back().wellgroup_events().addEvent( wellName, ScheduleEvents::WELL_WELSPECS_UPDATE);
                        this->snapshots.back().wells.update( std::move(well2) );
                        if (handlerContext.affected_wells)
                            handlerContext.affected_wells->insert(wellName);
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
        const double SiFactorP = this->m_static.m_unit_system.parse("Pressure").getSIScaling();
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const auto cmode = Well::WELTARGCModeFromString(record.getItem("CMODE").getTrimmedString(0));
            const auto new_arg = record.getItem("NEW_VALUE").get<UDAValue>(0);

            for (const auto& well_name : well_names) {
                auto well2 = this->snapshots.back().wells.get(well_name);
                bool update = false;
                if (well2.isProducer()) {
                    auto prop = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                    prop->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2.updateProduction(prop);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2.updateWellGuideRate(new_arg.get<double>());

                    auto udq_active = this->snapshots.back().udq_active.get();
                    if (prop->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), udq_active))
                        this->snapshots.back().udq_active.update( std::move(udq_active));
                } else {
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                    inj->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2.updateInjection(inj);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2.updateWellGuideRate(new_arg.get<double>());
                }
                if (update)
                {
                    if (well2.isProducer()) {
                        this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::PRODUCTION_UPDATE);
                        this->snapshots.back().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    } else {
                        this->snapshots.back().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                        this->snapshots.back().events().addEvent( ScheduleEvents::INJECTION_UPDATE );
                    }
                    this->snapshots.back().wells.update( std::move(well2) );
                }
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
                auto well2 = this->snapshots.back().wells.get(well_name);
                auto foam_properties = std::make_shared<WellFoamProperties>(well2.getFoamProperties());
                foam_properties->handleWFOAM(record);
                if (well2.updateFoamProperties(foam_properties))
                    this->snapshots.back().wells.update( std::move(well2) );
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

                auto well = this->snapshots.back().wells.get(well_name);
                if (well.updateWellGuideRate(availableForGroupControl, guide_rate, phase, scaling_factor)) {
                    auto new_config = this->snapshots.back().guide_rate();
                    new_config.update_well(well);
                    this->snapshots.back().guide_rate.update( std::move(new_config) );
                    this->snapshots.back().wells.update( std::move(well) );
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
            } else
                this->snapshots.back().update_whistctl( controlMode );
        }

        const std::string bhp_terminate = record.getItem("BPH_TERMINATE").getTrimmedString(0);
        if (bhp_terminate == "YES") {
            std::string msg_fmt = "Problem with {keyword}\n"
                                  "In {file} line {line}\n"
                                  "Setting item 2 in {keyword} to 'YES' to stop the run is not supported";
            parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg_fmt, handlerContext.keyword.location(), errors );
        }

        for (const auto& well_ref : this->snapshots.back().wells()) {
            auto well2 = well_ref.get();
            auto prop = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2.updateProduction(prop);
                this->snapshots.back().wells.update( std::move(well2) );
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
                    auto well2 = this->snapshots.back().wells( well_name );
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                    inj->temperature = temp;
                    well2.updateInjection(inj);
                    this->snapshots.back().wells.update( std::move(well2) );
                }
            }
        }
    }

    void Schedule::handleWLIFTOPT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto glo = this->snapshots.back().glo();

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

                glo.add_well(well);
            }
        }

        this->snapshots.back().glo.update( std::move(glo) );
    }

    void Schedule::handleWLIST(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const std::string legal_actions = "NEW:ADD:DEL:MOV";
        for (const auto& record : handlerContext.keyword) {
            const std::string& name = record.getItem("NAME").getTrimmedString(0);
            const std::string& action = record.getItem("ACTION").getTrimmedString(0);
            const std::vector<std::string>& well_args = record.getItem("WELLS").getData<std::string>();
            std::vector<std::string> wells;
            auto new_wlm = this->snapshots.back().wlist_manager.get();

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

            if (action == "NEW") {
                new_wlm.newList(name);
                new_wlm.getList(name).setName(name);
            }

            if (!new_wlm.hasList(name))
                throw std::invalid_argument("Invalid well list: " + name);

            auto& wlist = new_wlm.getList(name);
            if (action == "MOV") {
                for (const auto& well : wells) {
                    auto wel = this->snapshots.back().wells.get( well );
                    wel.clearWlist(name);
                    this->snapshots.back().wells.update( std::move(wel) );
                    new_wlm.delWell(well);
                }
            }

            if (action == "DEL") {
                for (const auto& well : wells)
                    wlist.del(well);
            } else {
                for (const auto& well : wells) {
                    wlist.add(well);
                    auto wel = this->snapshots.back().wells.get( well );
                    std::cout << "kwrd_handl - wlist - name: " << name << std::endl;
                    std::cout << "kwrd_handl - add well: " << well << "  well name wel.name(): " << wel.name() << std::endl;
                    wel.addWlist(name);
                    this->snapshots.back().wells.update( std::move(wel) );
                }
            }
            this->snapshots.back().wlist_manager.update( std::move(new_wlm) );
        }
    }

    void Schedule::handleWPIMULT(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto& well_names = this->wellNames(wellNamePattern, handlerContext.currentStep);

            for (const auto& wname : well_names) {
                auto well = this->snapshots.back().wells( wname );
                if (well.handleWPIMULT(record))
                    this->snapshots.back().wells.update( std::move(well));
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
                auto well = this->snapshots.back().wells( well_name );
                auto polymer_properties = std::make_shared<WellPolymerProperties>( well.getPolymerProperties() );
                polymer_properties->handleWPMITAB(record);
                if (well.updatePolymerProperties(polymer_properties))
                    this->snapshots.back().wells.update( std::move(well));
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
                auto well = this->snapshots.back().wells( well_name );
                auto polymer_properties = std::make_shared<WellPolymerProperties>( well.getPolymerProperties() );
                polymer_properties->handleWPOLYMER(record);
                if (well.updatePolymerProperties(polymer_properties))
                    this->snapshots.back().wells.update( std::move(well));
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
                auto well2 = this->snapshots.back().wells( well_name );
                auto brine_properties = std::make_shared<WellBrineProperties>(well2.getBrineProperties());
                brine_properties->handleWSALT(record);
                if (well2.updateBrineProperties(brine_properties))
                    this->snapshots.back().wells.update( std::move(well2) );
            }
        }
    }

    void Schedule::handleWSEGITER(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        const auto& record = handlerContext.keyword.getRecord(0);
        auto& tuning = this->snapshots.back().tuning();

        tuning.MXWSIT = record.getItem<ParserKeywords::WSEGITER::MAX_WELL_ITERATIONS>().get<int>(0);
        tuning.WSEG_MAX_RESTART = record.getItem<ParserKeywords::WSEGITER::MAX_TIMES_REDUCED>().get<int>(0);
        tuning.WSEG_REDUCTION_FACTOR = record.getItem<ParserKeywords::WSEGITER::REDUCTION_FACTOR>().get<double>(0);
        tuning.WSEG_INCREASE_FACTOR = record.getItem<ParserKeywords::WSEGITER::INCREASING_FACTOR>().get<double>(0);

        this->snapshots.back().events().addEvent(ScheduleEvents::TUNING_CHANGE);
    }

    void Schedule::handleWSEGSICD(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::map<std::string, std::vector<std::pair<int, SICD> > > spiral_icds = SICD::fromWSEGSICD(handlerContext.keyword);

        for (auto& map_elem : spiral_icds) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, handlerContext.currentStep);

            std::vector<std::pair<int, SICD> >& sicd_pairs = map_elem.second;

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );

                const auto& connections = well.getConnections();
                const auto& segments = well.getSegments();
                for (auto& [segment_nr, sicd] : sicd_pairs) {
                    const auto& outlet_segment_length = segments.segmentLength( segments.getFromSegmentNumber(segment_nr).outletSegment() );
                    sicd.updateScalingFactor(outlet_segment_length, connections.segment_perf_length(segment_nr));
                }

                if (well.updateWSEGSICD(sicd_pairs) )
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWSEGAICD(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        std::map<std::string, std::vector<std::pair<int, AutoICD> > > auto_icds = AutoICD::fromWSEGAICD(handlerContext.keyword);

        for (auto& [well_name_pattern, aicd_pairs] : auto_icds) {
            const auto well_names = this->wellNames(well_name_pattern, handlerContext.currentStep);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );

                const auto& connections = well.getConnections();
                const auto& segments = well.getSegments();
                for (auto& [segment_nr, aicd] : aicd_pairs) {
                    const auto& outlet_segment_length = segments.segmentLength( segments.getFromSegmentNumber(segment_nr).outletSegment() );
                    aicd.updateScalingFactor(outlet_segment_length, connections.segment_perf_length(segment_nr));
                }

                if (well.updateWSEGAICD(aicd_pairs, handlerContext.keyword.location()) )
                    this->snapshots.back().wells.update( std::move(well) );
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
                auto well = this->snapshots.back().wells( well_name );
                if (well.updateWSEGVALV(valve_pairs))
                    this->snapshots.back().wells.update( std::move(well) );
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
                auto well = this->snapshots.back().wells( well_name );

                auto polymer_properties = std::make_shared<WellPolymerProperties>(well.getPolymerProperties());
                polymer_properties->handleWSKPTAB(record);
                if (well.updatePolymerProperties(polymer_properties))
                    this->snapshots.back().wells.update( std::move(well) );
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
                        auto well2 = this->snapshots.back().wells( well_name );
                        well2.updateSolventFraction(fraction);
                        this->snapshots.back().wells.update( std::move(well2) );
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
                    auto well2 = this->snapshots.back().wells( well_name );
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well.getInjectionProperties());
                    inj->temperature = temp;
                    well2.updateInjection(inj);
                    this->snapshots.back().wells.update( std::move(well2) );
                }
            }
        }
    }

    void Schedule::handleWTEST(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto new_config = this->snapshots.back().wtest_config.get();
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
                    new_config.drop_well(well_name);
                else
                    new_config.add_well(well_name, reasons, test_interval, num_test, startup_time, handlerContext.currentStep);
            }
        }
        this->snapshots.back().wtest_config.update( std::move(new_config) );
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
                auto well = this->snapshots.back().wells.get( well_name );
                auto wellTracerProperties = std::make_shared<WellTracerProperties>(well.getTracerProperties());
                wellTracerProperties->setConcentration(tracerName, tracerConcentration);
                if (well.updateTracer(wellTracerProperties))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWPAVE(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        auto wpave = PAvg( handlerContext.keyword.getRecord(0) );
        for (const auto& wname : this->wellNames(handlerContext.currentStep))
            this->updateWPAVE(wname, handlerContext.currentStep, wpave );

        auto& sched_state = this->snapshots.back();
        sched_state.pavg.update(std::move(wpave));
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

    void Schedule::handleWPAVEDEP(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WPAVEDEP::WELL>().getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            const auto& item = record.getItem<ParserKeywords::WPAVEDEP::REFDEPTH>();
            if (item.hasValue(0)) {
                auto ref_depth = item.getSIDouble(0);
                for (const auto& well_name : well_names) {
                    auto well = this->snapshots.back().wells.get(well_name);
                    well.updateWPaveRefDepth( ref_depth );
                    this->snapshots.back().wells.update( std::move(well) );
                }
            }
        }
    }

    void Schedule::handleWRFT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto new_rft = this->snapshots.back().rft_config();
        for (const auto& record : handlerContext.keyword) {
            const auto& item = record.getItem<ParserKeywords::WRFT::WELL>();
            if (item.hasValue(0)) {
                const std::string& wellNamePattern = record.getItem<ParserKeywords::WRFT::WELL>().getTrimmedString(0);
                const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);

                if (well_names.empty())
                    invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

                for (const auto& well_name : well_names)
                    new_rft.update(well_name, RFTConfig::RFT::YES);
            }
        }
        new_rft.first_open(true);
        this->snapshots.back().rft_config.update( std::move(new_rft) );
    }


    void Schedule::handleWRFTPLT(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        auto new_rft = this->snapshots.back().rft_config();

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WRFTPLT::WELL>().getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            auto RFTKey = RFTConfig::RFTFromString(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_RFT>().getTrimmedString(0));
            auto PLTKey = RFTConfig::PLTFromString(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_PLT>().getTrimmedString(0));

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, handlerContext.currentStep, parseContext, errors, handlerContext.keyword);

            for (const auto& well_name : well_names) {
                new_rft.update(well_name, RFTKey);
                new_rft.update(well_name, PLTKey);
            }
        }

        this->snapshots.back().rft_config.update( std::move(new_rft) );
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
            { "DRSDTCON", &Schedule::handleDRSDTCON },
            { "DRSDTR"  , &Schedule::handleDRSDTR   },
            { "DRVDT"   , &Schedule::handleDRVDT    },
            { "DRVDTR"  , &Schedule::handleDRVDTR   },
            { "EXIT",     &Schedule::handleEXIT     },
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
            { "RPTRST"  , &Schedule::handleRPTRST   },
            { "RPTSCHED", &Schedule::handleRPTSCHED },
            { "SAVE"    , &Schedule::handleSAVE     },
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
            { "WPAVEDEP", &Schedule::handleWPAVEDEP },
            { "WWPAVE"  , &Schedule::handleWWPAVE   },
            { "WPIMULT" , &Schedule::handleWPIMULT  },
            { "WPMITAB" , &Schedule::handleWPMITAB  },
            { "WPOLYMER", &Schedule::handleWPOLYMER },
            { "WRFT"    , &Schedule::handleWRFT     },
            { "WRFTPLT" , &Schedule::handleWRFTPLT  },
            { "WSALT"   , &Schedule::handleWSALT    },
            { "WSEGITER", &Schedule::handleWSEGITER },
            { "WSEGSICD", &Schedule::handleWSEGSICD },
            { "WSEGAICD", &Schedule::handleWSEGAICD },
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
