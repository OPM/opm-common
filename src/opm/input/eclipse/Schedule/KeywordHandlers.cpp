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

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/numeric/cmp.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/SimulatorUpdate.hpp>
#include <opm/input/eclipse/Schedule/Events.hpp>
#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>
#include <opm/input/eclipse/Schedule/Group/GConSale.hpp>
#include <opm/input/eclipse/Schedule/Group/GConSump.hpp>
#include <opm/input/eclipse/Schedule/Group/GroupEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/MSW/SICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>
#include <opm/input/eclipse/Schedule/MSW/Valve.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Network/Balance.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/RFTConfig.hpp>
#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Tuning.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQActive.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/WList.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPDP.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellBrineProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFoamProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMICPProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTracerProperties.hpp>

#include <opm/input/eclipse/Schedule/Well/WVFPDP.hpp>

#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/U.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include "Well/injection.hpp"

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>

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
    std::string trim_wgname(const DeckKeyword& keyword, const std::string& wgname_arg,  const ParseContext& parseContext, ErrorGuard& errors) {
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
    void Schedule::handleAQUCT(HandlerContext& handlerContext) {
        throw OpmInputError("AQUCT is not supported as SCHEDULE keyword", handlerContext.keyword.location());
    }

    void Schedule::handleAQUFETP(HandlerContext& handlerContext) {
        throw OpmInputError("AQUFETP is not supported as SCHEDULE keyword", handlerContext.keyword.location());
    }

    void Schedule::handleAQUFLUX(Schedule::HandlerContext& handlerContext) {
        // auto& aqufluxs = this->snapshots.back().aqufluxs;
        auto& aqufluxs = this->snapshots.back().aqufluxs;
        for (const auto& record : handlerContext.keyword) {
            const auto aquifer = SingleAquiferFlux { record };
            aqufluxs.insert_or_assign(aquifer.id, aquifer);
        }
    }

    void Schedule::handleBCProp(Schedule::HandlerContext& handlerContext) {
        auto& bcprop = this->snapshots.back().bcprop;
        for (const auto& record : handlerContext.keyword) {
            bcprop.updateBCProp(record);
        }
    }

    void Schedule::handleBRANPROP(HandlerContext& handlerContext) {
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
                    ext_network.add_or_replace_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_value));
                } else {
                    ext_network.add_or_replace_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_eq));
                }
            }
        }

        this->snapshots.back().network.update( std::move( ext_network ));
    }

    void Schedule::handleCOMPDAT(HandlerContext& handlerContext)  {
        std::unordered_set<std::string> wells;
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, handlerContext,
                                             isWList(handlerContext.currentStep,
                                                     wellNamePattern));

            for (const auto& name : wellnames) {
                auto well2 = this->snapshots.back().wells.get(name);
                auto connections = std::shared_ptr<WellConnections>( new WellConnections( well2.getConnections()));
                connections->loadCOMPDAT(record, handlerContext.grid, name, handlerContext.keyword.location());

                if (well2.updateConnections(connections, handlerContext.grid)) {
                    auto wdfac = std::make_shared<WDFAC>(well2.getWDFAC());
                    wdfac->updateWDFACType(*connections);
                    well2.updateWDFAC(std::move(wdfac));
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

        if (! wells.empty()) {
            handlerContext.record_well_structure_change();
        }
    }

    void Schedule::handleWELTRAJ(HandlerContext& handlerContext)  {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, handlerContext);

            for (const auto& name : wellnames) {
                auto well2 = this->snapshots.back().wells.get(name);
                auto connections = std::make_shared<WellConnections>(WellConnections(well2.getConnections()));
                connections->loadWELTRAJ(record, handlerContext.grid, name, handlerContext.keyword.location());
                if (well2.updateConnections(connections, handlerContext.grid)) {
                    this->snapshots.back().wells.update( well2 );
                    handlerContext.record_well_structure_change();
                }
                this->snapshots.back().wellgroup_events().addEvent( name, ScheduleEvents::COMPLETION_CHANGE);
                const auto& md = connections->getMD();
                if (!std::is_sorted(std::begin(md), std::end(md))) {
                    auto msg = fmt::format("Well {} measured depth column is not strictly increasing", name);
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }
            }
        }
        this->snapshots.back().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
    }

    void Schedule::handleCOMPTRAJ(HandlerContext& handlerContext)  {
        // Keyword WELTRAJ must be read first
        std::unordered_set<std::string> wells;
        external::cvf::ref<external::cvf::BoundingBoxTree> cellSearchTree = nullptr; 
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, handlerContext );

            for (const auto& name : wellnames) {
                auto well2 = this->snapshots.back().wells.get(name);
                auto connections = std::make_shared<WellConnections>(WellConnections(well2.getConnections()));
                // cellsearchTree is calculated only once and is used to calculated cell intersections of the perforations specified in COMPTRAJ 
                connections->loadCOMPTRAJ(record, handlerContext.grid, name, handlerContext.keyword.location(), cellSearchTree);
                // In the case that defaults are used in WELSPECS for headI/J the headI/J are calculated based on the well trajectory data
                well2.updateHead(connections->getHeadI(), connections->getHeadJ());
                if (well2.updateConnections(connections, handlerContext.grid)) {
                    this->snapshots.back().wells.update( well2 );
                    wells.insert( name );
                }

                if (connections->empty() && well2.getConnections().empty()) {
                    const auto& location = handlerContext.keyword.location();
                    auto msg = fmt::format("Problem with COMPTRAJ/{}\n"
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
        // reference depth exactly when the WELCOML keyword has been completely
        // processed.
        for (const auto& wname : wells) {
            auto& well = this->snapshots.back().wells.get( wname );
            well.updateRefDepth();
            this->snapshots.back().wells.update( std::move(well));
        }

        if (! wells.empty()) {
            handlerContext.record_well_structure_change();
        }
    }

    void Schedule::handleCOMPLUMP(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));

            for (const auto& wname : well_names) {
                auto well = this->snapshots.back().wells.get(wname);
                if (well.handleCOMPLUMP(record)) {
                    this->snapshots.back().wells.update( std::move(well) );

                    handlerContext.record_well_structure_change();
                }
            }
        }
    }

    /*
      The COMPORD keyword is handled together with the WELSPECS keyword in the
      handleWELSPECS function.
    */
    void Schedule::handleCOMPORD(HandlerContext& )
    {
    }

    void Schedule::handleCOMPSEGS(HandlerContext& handlerContext) {
        const auto& record1 = handlerContext.keyword.getRecord(0);
        const std::string& wname = record1.getItem("WELL").getTrimmedString(0);

        if (!this->hasWell(wname, handlerContext.currentStep)) {
            const auto& location = handlerContext.keyword.location();
            if (this->action_wgnames.has_well(wname)) {
                std::string msg = fmt::format(R"(Well: {} not yet defined for keyword {}.
Expecting well to be defined with WELSPECS in ACTIONX before actual use.
File {} line {}.)", wname, location.keyword, location.filename, location.lineno);
                OpmLog::warning(msg);
            } else
                throw OpmInputError(fmt::format("No such well: ", wname), location);
            return;
        }

        auto well = this->snapshots.back().wells.get( wname );

        if (well.getConnections().empty()) {
            const auto& location = handlerContext.keyword.location();
            auto msg = fmt::format("Problem with COMPSEGS/{0}\n"
                                   "In {1} line {2}\n"
                                   "Well {0} is not connected to grid - COMPSEGS will be ignored", wname, location.filename, location.lineno);
            OpmLog::warning(msg);
            return;
        }

        if (well.handleCOMPSEGS(handlerContext.keyword, handlerContext.grid, handlerContext.parseContext, handlerContext.errors))
        {
            this->snapshots.back().wells.update( std::move(well) );
            handlerContext.record_well_structure_change();
        }

        handlerContext.compsegs_handled(wname);
    }

    void Schedule::handleCSKIN(HandlerContext& handlerContext) {
        // Get CSKIN keyword info and current step
        const auto& keyword = handlerContext.keyword;
        const auto& currentStep = handlerContext.currentStep;

        // Loop over records in CSKIN
        for (const auto& record: keyword) {
            // Get well names
            const auto& wellNamePattern = record.getItem( "WELL" ).getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);

            // Loop over well(s) in record
            for (const auto& wname : well_names) {
                // Get well information, modify connection skin factor, and update well
                auto well = this->snapshots[currentStep].wells.get(wname);
                well.handleCSKINConnections(record);
                this->snapshots[currentStep].wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleDRSDT(HandlerContext& handlerContext) {
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

    void Schedule::handleDRSDTCON(HandlerContext& handlerContext) {
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

    void Schedule::handleDRSDTR(HandlerContext& handlerContext) {
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

    void Schedule::handleDRVDT(HandlerContext& handlerContext) {
        std::size_t numPvtRegions = this->m_static.m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRVDTR::DRVDT_MAX>().getSIDouble(0);
            std::fill(maximums.begin(), maximums.end(), max);
            auto& ovp = this->snapshots.back().oilvap();
            OilVaporizationProperties::updateDRVDT(ovp, maximums);
        }
    }

    void Schedule::handleDRVDTR(HandlerContext& handlerContext) {
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

    void Schedule::handleEXIT(HandlerContext& handlerContext) {
        if (handlerContext.actionx_mode)
            this->applyEXIT(handlerContext.keyword, handlerContext.currentStep);
    }

    void Schedule::handleGCONINJE(HandlerContext& handlerContext) {
        using GI = ParserKeywords::GCONINJE;
        auto current_step = handlerContext.currentStep;
        const auto& keyword = handlerContext.keyword;
        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem<GI::GROUP>().getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                this->invalidNamePattern(groupNamePattern, handlerContext);

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
                    Group::GroupInjectionProperties injection{group_name};
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

                        auto udq_active = this->snapshots.back().udq_active.get();
                        if (injection.updateUDQActive(this->getUDQConfig(current_step), udq_active))
                            this->snapshots.back().udq_active.update( std::move(udq_active));
                    }
                }
            }
        }
    }

    void Schedule::handleGCONPROD(HandlerContext& handlerContext) {
        auto current_step = handlerContext.currentStep;
        const auto& keyword = handlerContext.keyword;
        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                this->invalidNamePattern(groupNamePattern, handlerContext);

            const Group::ProductionCMode controlMode = Group::ProductionCModeFromString(record.getItem("CONTROL_MODE").getTrimmedString(0));
            Group::GroupLimitAction groupLimitAction;
            groupLimitAction.allRates = Group::ExceedActionFromString(record.getItem("EXCEED_PROC").getTrimmedString(0));
            groupLimitAction.water = Group::ExceedActionFromString(record.getItem("WATER_EXCEED_PROCEDURE").getTrimmedString(0));
            groupLimitAction.gas = Group::ExceedActionFromString(record.getItem("GAS_EXCEED_PROCEDURE").getTrimmedString(0));
            groupLimitAction.liquid = Group::ExceedActionFromString(record.getItem("LIQUID_EXCEED_PROCEDURE").getTrimmedString(0));

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
                            const auto& parseContext = handlerContext.parseContext;
                            auto& errors = handlerContext.errors;
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
                    production.group_limit_action = groupLimitAction;

                    production.production_controls = 0;
                    // GCONPROD
                    // 'G1' 'ORAT' 1000 100 200 300 NONE =>  constraints 100,200,300 should be ignored
                    //
                    // GCONPROD
                    // 'G1' 'ORAT' 1000 100 200 300 RATE =>  constraints 100,200,300 should be honored
                    if (production.cmode == Group::ProductionCMode::ORAT ||
                        (groupLimitAction.allRates == Group::ExceedAction::RATE &&
                        !apply_default_oil_target)) {
                        production.production_controls |= static_cast<int>(Group::ProductionCMode::ORAT);
                    }
                    if (production.cmode == Group::ProductionCMode::WRAT ||
                        ((groupLimitAction.allRates == Group::ExceedAction::RATE ||
                        groupLimitAction.water == Group::ExceedAction::RATE) &&
                        !apply_default_water_target)) {
                        production.production_controls |= static_cast<int>(Group::ProductionCMode::WRAT);
                    }
                    if (production.cmode == Group::ProductionCMode::GRAT ||
                        ((groupLimitAction.allRates  == Group::ExceedAction::RATE ||
                        groupLimitAction.gas == Group::ExceedAction::RATE) &&
                        !apply_default_gas_target)) {
                        production.production_controls |= static_cast<int>(Group::ProductionCMode::GRAT);
                    }
                    if (production.cmode == Group::ProductionCMode::LRAT ||
                        ((groupLimitAction.allRates == Group::ExceedAction::RATE ||
                        groupLimitAction.liquid == Group::ExceedAction::RATE) &&
                        !apply_default_liquid_target)) {
                        production.production_controls |= static_cast<int>(Group::ProductionCMode::LRAT);
                    }

                    if (!apply_default_resv_target)
                        production.production_controls |= static_cast<int>(Group::ProductionCMode::RESV);

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

    void Schedule::handleGCONSALE(HandlerContext& handlerContext) {
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
            Group::GroupInjectionProperties injection{groupName};
            injection.phase = Phase::GAS;
            if (new_group.updateInjection(injection))
                this->snapshots.back().groups.update(new_group);
        }
        this->snapshots.back().gconsale.update( std::move(new_gconsale) );
    }

    void Schedule::handleGCONSUMP(HandlerContext& handlerContext) {
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

    void
    Schedule::handleGECON(HandlerContext& handlerContext)
    {
        auto gecon = this->snapshots.back().gecon();
        const auto& keyword = handlerContext.keyword;
        auto report_step = handlerContext.currentStep;
        for (const auto& record : keyword) {
            const std::string& groupNamePattern
                = record.getItem<ParserKeywords::GECON::GROUP>().getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                this->invalidNamePattern(groupNamePattern, handlerContext);
            for (const auto& gname : group_names) {
                gecon.add_group(report_step, gname, record);
            }
        }
        this->snapshots.back().gecon.update(std::move(gecon));
    }

    void Schedule::handleGEFAC(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                this->invalidNamePattern(groupNamePattern, handlerContext);

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

    void Schedule::handleGLIFTOPT(HandlerContext& handlerContext) {
        auto glo = this->snapshots.back().glo();
        const auto& keyword = handlerContext.keyword;

        for (const auto& record : keyword) {
            const std::string& groupNamePattern = record.getItem<ParserKeywords::GLIFTOPT::GROUP_NAME>().getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                this->invalidNamePattern(groupNamePattern, handlerContext);

            const auto& max_gas_item = record.getItem<ParserKeywords::GLIFTOPT::MAX_LIFT_GAS_SUPPLY>();
            const double max_lift_gas_value = max_gas_item.hasValue(0)
                ? max_gas_item.getSIDouble(0)
                : -1;

            const auto& max_total_item = record.getItem<ParserKeywords::GLIFTOPT::MAX_TOTAL_GAS_RATE>();
            const double max_total_gas_value = max_total_item.hasValue(0)
                ? max_total_item.getSIDouble(0)
                : -1;

            for (const auto& gname : group_names) {
                auto group = GasLiftGroup(gname);
                group.max_lift_gas(max_lift_gas_value);
                group.max_total_gas(max_total_gas_value);

                glo.add_group(group);
            }
        }

        this->snapshots.back().glo.update( std::move(glo) );
    }

    void Schedule::handleGPMAINT(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);
            if (group_names.empty())
                this->invalidNamePattern(groupNamePattern, handlerContext);

            const auto& target_string = record.getItem<ParserKeywords::GPMAINT::FLOW_TARGET>().get<std::string>(0);

            for (const auto& group_name : group_names) {
                auto new_group = this->snapshots.back().groups.get(group_name);
                if (target_string == "NONE") {
                    new_group.set_gpmaint();
                } else {
                    GPMaint gpmaint(handlerContext.currentStep, record);
                    new_group.set_gpmaint(std::move(gpmaint));
                }
                this->snapshots.back().groups.update( std::move(new_group) );
            }
        }
    }

    void Schedule::handleGRUPNET(HandlerContext& handlerContext) {
        auto network = this->snapshots.back().network.get();
        std::vector<Network::Node> nodes;
        for (const auto& record : handlerContext.keyword) {
             const std::string& groupNamePattern = record.getItem<ParserKeywords::GRUPNET::NAME>().getTrimmedString(0);
             const auto group_names = this->groupNames(groupNamePattern);
             if (group_names.empty())
                 this->invalidNamePattern(groupNamePattern, handlerContext);
             const auto& pressure_item = record.getItem<ParserKeywords::GRUPNET::TERMINAL_PRESSURE>();
             const int vfp_table = record.getItem<ParserKeywords::GRUPNET::VFP_TABLE>().get<int>(0);
             // It is assumed here that item 6 (ADD_GAS_LIFT_GAS) has the two options NO and FLO. THe option ALQ is not supported.
             // For standard networks the summation of ALQ values are weighted with efficiency factors. 
             // Note that, currently, extended networks uses always efficiency factors (this is the default set by WEFAC item 3 (YES), the value NO is not supported.) 
             const std::string& add_gas_lift_gas_string = record.getItem<ParserKeywords::GRUPNET::ADD_GAS_LIFT_GAS>().get<std::string>(0);
             bool add_gas_lift_gas = false;
             if (add_gas_lift_gas_string == "FLO") 
                 add_gas_lift_gas = true;

             for (const auto& group_name : group_names) {
                  const auto& group = this->snapshots.back().groups.get(group_name);
                  const std::string& downtree_node = group_name;
                  const std::string& uptree_node = group.parent();
                  Network::Node node { group_name };
                  node.add_gas_lift_gas(add_gas_lift_gas);
                  // A terminal node is a node with a fixed pressure
                  const bool is_terminal_node = pressure_item.hasValue(0) && (pressure_item.get<double>(0) >= 0);
                  if (is_terminal_node) {
                      if (vfp_table > 0) {
                          std::string msg = fmt::format("The group {} is a terminal node of the network and should not have a vfp table assigned to it.", group_name);
                          throw OpmInputError(msg, handlerContext.keyword.location());
                      }
                      node.terminal_pressure(pressure_item.getSIDouble(0));
                      nodes.push_back(node);
                      // Remove the  branch upstream if it was part of the network
                      if (network.has_node(downtree_node) && network.has_node(uptree_node))
                          network.drop_branch(uptree_node, downtree_node);
                  } else {
                       if (vfp_table <= 0) {
                           // If vfp table is defaulted (or set to <=0) then the group is not part of the network.
                           // If the branch was part of the network then drop it
                           if (network.has_node(downtree_node) && network.has_node(uptree_node))
                               network.drop_branch(uptree_node, downtree_node);
                       } else {
                            if (!uptree_node.empty()) {
                                const auto alq_eq = Network::Branch::AlqEqfromString(record.getItem<ParserKeywords::GRUPNET::ALQ_SURFACE_DENSITY>().get<std::string>(0));
                                if (alq_eq == Network::Branch::AlqEQ::ALQ_INPUT) {
                                    const double alq_value = record.getItem<ParserKeywords::GRUPNET::ALQ>().get<double>(0);
                                    network.add_or_replace_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_value));
                                } else {
                                     network.add_or_replace_branch(Network::Branch(downtree_node, uptree_node, vfp_table, alq_eq));
                                }
                            }
                            nodes.push_back(node);
                       }
                  }
             }
        }
        // To use update_node the node should be associated to a branch via add_branch()
        // so the update of nodes is postponed after creation of branches
        for(const auto& node: nodes)
              network.update_node(node);
        this->snapshots.back().network.update( std::move(network));
    }

    void Schedule::handleGRUPTREE(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& childName = trim_wgname(handlerContext.keyword, record.getItem("CHILD_GROUP").get<std::string>(0), handlerContext.parseContext, handlerContext.errors);
            const std::string& parentName = trim_wgname(handlerContext.keyword, record.getItem("PARENT_GROUP").get<std::string>(0), handlerContext.parseContext, handlerContext.errors);

            if (!this->snapshots.back().groups.has(childName))
                addGroup(childName, handlerContext.currentStep);

            if (!this->snapshots.back().groups.has(parentName))
                addGroup(parentName, handlerContext.currentStep);

            this->addGroupToGroup(parentName, childName);
        }
    }

    void Schedule::handleGUIDERAT(HandlerContext& handlerContext) {
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

    void Schedule::handleLIFTOPT(HandlerContext& handlerContext) {
        auto glo = this->snapshots.back().glo();

        const auto& record = handlerContext.keyword.getRecord(0);

        const double gaslift_increment = record.getItem<ParserKeywords::LIFTOPT::INCREMENT_SIZE>().getSIDouble(0);
        const double min_eco_gradient = record.getItem<ParserKeywords::LIFTOPT::MIN_ECONOMIC_GRADIENT>().getSIDouble(0);
        const double min_wait = record.getItem<ParserKeywords::LIFTOPT::MIN_INTERVAL_BETWEEN_GAS_LIFT_OPTIMIZATIONS>().getSIDouble(0);
        const bool all_newton = DeckItem::to_bool( record.getItem<ParserKeywords::LIFTOPT::OPTIMISE_ALL_ITERATIONS>().get<std::string>(0) );

        glo.gaslift_increment(gaslift_increment);
        glo.min_eco_gradient(min_eco_gradient);
        glo.min_wait(min_wait);
        glo.all_newton(all_newton);

        this->snapshots.back().glo.update( std::move(glo) );
    }

    void Schedule::handleLINCOM(HandlerContext& handlerContext) {
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

    void Schedule::handleMESSAGES(HandlerContext& handlerContext) {
        this->snapshots.back().message_limits().update( handlerContext.keyword );
    }


    void Schedule::handleGEOKeyword(HandlerContext& handlerContext) {
        this->snapshots.back().geo_keywords().push_back(handlerContext.keyword);
        this->snapshots.back().events().addEvent( ScheduleEvents::GEO_MODIFIER );
        if (handlerContext.sim_update)
            handlerContext.sim_update->tran_update = true;
    }

    void Schedule::handleMXUNSUPP(HandlerContext& handlerContext) {
        std::string msg_fmt = fmt::format("Problem with keyword {{keyword}} at report step {}\n"
                                          "In {{file}} line {{line}}\n"
                                          "OPM does not support grid property modifier {} in the Schedule section", handlerContext.currentStep, handlerContext.keyword.name());
        OpmLog::warning(OpmInputError::format(msg_fmt, handlerContext.keyword.location()));
    }

    void Schedule::handleNETBALAN(HandlerContext& handlerContext) {
        this->snapshots.back().network_balance
            .update(Network::Balance{ handlerContext.keyword });
    }

    void Schedule::handleNEXTSTEP(HandlerContext& handlerContext) {
        const auto& record = handlerContext.keyword[0];
        auto next_tstep = record.getItem<ParserKeywords::NEXTSTEP::MAX_STEP>().getSIDouble(0);
        auto apply_to_all = DeckItem::to_bool( record.getItem<ParserKeywords::NEXTSTEP::APPLY_TO_ALL>().get<std::string>(0) );

        this->snapshots.back().next_tstep = NextStep{next_tstep, apply_to_all};
        this->snapshots.back().events().addEvent(ScheduleEvents::TUNING_CHANGE);
    }


    void Schedule::handleNODEPROP(HandlerContext& handlerContext) {
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
            ext_network.update_node(node);
        }

        this->snapshots.back().network.update( ext_network );
    }

    void Schedule::handleNUPCOL(HandlerContext& handlerContext) {
        const int nupcol = handlerContext.keyword.getRecord(0).getItem("NUM_ITER").get<int>(0);

        if (handlerContext.keyword.getRecord(0).getItem("NUM_ITER").defaultApplied(0)) {
            std::string msg = "OPM Flow uses 12 as default NUPCOL value";
            OpmLog::note(msg);
        }

        this->snapshots.back().update_nupcol(nupcol);
    }

    void Schedule::handleRPTONLY(HandlerContext&) {
        this->snapshots.back().rptonly(true);
    }

    void Schedule::handleRPTONLYO(HandlerContext&) {
        this->snapshots.back().rptonly(false);
    }

    void Schedule::handleRPTSCHED(HandlerContext& handlerContext) {
        this->snapshots.back().rpt_config.update( RPTConfig(handlerContext.keyword ));
        auto rst_config = this->snapshots.back().rst_config();
        rst_config.update(handlerContext.keyword, handlerContext.parseContext, handlerContext.errors);
        this->snapshots.back().rst_config.update(std::move(rst_config));
    }

    void Schedule::handleRPTRST(HandlerContext& handlerContext) {
        auto rst_config = this->snapshots.back().rst_config();
        rst_config.update(handlerContext.keyword, handlerContext.parseContext, handlerContext.errors);
        this->snapshots.back().rst_config.update(std::move(rst_config));
    }

    /*
      We do not really handle the SAVE keyword, we just interpret it as: Write a
      normal restart file at this report step.
    */
    void Schedule::handleSAVE(HandlerContext& handlerContext) {
        this->snapshots[handlerContext.currentStep].updateSAVE(true);
    }


    void Schedule::handleSUMTHIN(HandlerContext& handlerContext) {
        auto value = handlerContext.keyword.getRecord(0).getItem(0).getSIDouble(0);
        this->snapshots.back().update_sumthin( value );
    }


    void Schedule::handleTUNING(HandlerContext& handlerContext) {
        const auto numrecords = handlerContext.keyword.size();
        auto tuning = this->snapshots.back().tuning();

        // local helpers
        auto nondefault_or_previous_double = [](const Opm::DeckRecord& rec, const std::string& item_name, double previous_value) {
            const auto& deck_item = rec.getItem(item_name);
            return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).get< double >(0);
        };

        auto nondefault_or_previous_int = [](const Opm::DeckRecord& rec, const std::string& item_name, int previous_value) {
            const auto& deck_item = rec.getItem(item_name);
            return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).get< int >(0);
        };
        
        auto nondefault_or_previous_sidouble = [](const Opm::DeckRecord& rec, const std::string& item_name, double previous_value) {
            const auto& deck_item = rec.getItem(item_name);
            return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).getSIDouble(0);
        };

        // \Note No TSTINIT value should not be used unless explicitly non-defaulted, hence removing value by default
        // \Note (exception is the first time step, which is handled by the Tuning constructor)
        tuning.TSINIT = std::nullopt;
        
        if (numrecords > 0) {
            const auto& record1 = handlerContext.keyword.getRecord(0);

            // \Note A value indicates TSINIT was set in this record
            if (const auto& deck_item = record1.getItem("TSINIT"); !deck_item.defaultApplied(0)) 
                tuning.TSINIT = std::optional<double>{ record1.getItem("TSINIT").getSIDouble(0) };

            tuning.TSMAXZ = nondefault_or_previous_sidouble(record1, "TSMAXZ", tuning.TSMAXZ);
            tuning.TSMINZ = nondefault_or_previous_sidouble(record1, "TSMINZ", tuning.TSMINZ);
            tuning.TSMCHP = nondefault_or_previous_sidouble(record1, "TSMCHP", tuning.TSMCHP);
            tuning.TSFMAX = nondefault_or_previous_double(record1, "TSFMAX", tuning.TSFMAX);
            tuning.TSFMIN = nondefault_or_previous_double(record1, "TSFMIN", tuning.TSFMIN);
            tuning.TSFCNV = nondefault_or_previous_double(record1, "TSFCNV", tuning.TSFCNV);
            tuning.TFDIFF = nondefault_or_previous_double(record1, "TFDIFF", tuning.TFDIFF);
            tuning.THRUPT = nondefault_or_previous_double(record1, "THRUPT", tuning.THRUPT);
            
            const auto& TMAXWCdeckItem = record1.getItem("TMAXWC");
            if (TMAXWCdeckItem.hasValue(0)) {
                tuning.TMAXWC_has_value = true;
                tuning.TMAXWC = nondefault_or_previous_sidouble(record1, "TMAXWC", tuning.TMAXWC);
            }
        }

        if (numrecords > 1) {
            const auto& record2 = handlerContext.keyword.getRecord(1);

            tuning.TRGTTE = nondefault_or_previous_double(record2, "TRGTTE", tuning.TRGTTE);
            tuning.TRGCNV = nondefault_or_previous_double(record2, "TRGCNV", tuning.TRGCNV);
            tuning.TRGMBE = nondefault_or_previous_double(record2, "TRGMBE", tuning.TRGMBE);
            tuning.TRGLCV = nondefault_or_previous_double(record2, "TRGLCV", tuning.TRGLCV);
            tuning.XXXTTE = nondefault_or_previous_double(record2, "XXXTTE", tuning.XXXTTE);
            tuning.XXXCNV = nondefault_or_previous_double(record2, "XXXCNV", tuning.XXXCNV);

            tuning.XXXMBE = nondefault_or_previous_double(record2, "XXXMBE", tuning.XXXMBE);
            
            tuning.XXXLCV = nondefault_or_previous_double(record2, "XXXLCV", tuning.XXXLCV);
            tuning.XXXWFL = nondefault_or_previous_double(record2, "XXXWFL", tuning.XXXWFL);
            tuning.TRGFIP = nondefault_or_previous_double(record2, "TRGFIP", tuning.TRGFIP);

            const auto& TRGSFTdeckItem = record2.getItem("TRGSFT");
            if (TRGSFTdeckItem.hasValue(0)) {
                tuning.TRGSFT_has_value = true;
                tuning.TRGSFT = nondefault_or_previous_double(record2, "TRGSFT", tuning.TRGSFT);
            }

            tuning.THIONX = nondefault_or_previous_double(record2, "THIONX", tuning.THIONX);
            tuning.TRWGHT = nondefault_or_previous_int(record2, "TRWGHT", tuning.TRWGHT);
        }

        if (numrecords > 2) {
            const auto& record3 = handlerContext.keyword.getRecord(2);

            tuning.NEWTMX = nondefault_or_previous_int(record3, "NEWTMX", tuning.NEWTMX);
            tuning.NEWTMN = nondefault_or_previous_int(record3, "NEWTMN", tuning.NEWTMN);
            tuning.LITMAX = nondefault_or_previous_int(record3, "LITMAX", tuning.LITMAX);
            tuning.LITMIN = nondefault_or_previous_int(record3, "LITMIN", tuning.LITMIN);
            tuning.MXWSIT = nondefault_or_previous_int(record3, "MXWSIT", tuning.MXWSIT);
            tuning.MXWPIT = nondefault_or_previous_int(record3, "MXWPIT", tuning.MXWPIT);
            tuning.DDPLIM = nondefault_or_previous_sidouble(record3, "DDPLIM", tuning.DDPLIM);
            tuning.DDSLIM = nondefault_or_previous_double(record3, "DDSLIM", tuning.DDSLIM);
            tuning.TRGDPR = nondefault_or_previous_sidouble(record3, "TRGDPR", tuning.TRGDPR);

            const auto& XXXDPRdeckItem = record3.getItem("XXXDPR");
            if (XXXDPRdeckItem.hasValue(0)) {
                tuning.XXXDPR_has_value = true;
                tuning.XXXDPR = nondefault_or_previous_sidouble(record3, "XXXDPR", tuning.XXXDPR);
            }
        } 

        this->snapshots.back().update_tuning( std::move( tuning ));
        this->snapshots.back().events().addEvent(ScheduleEvents::TUNING_CHANGE);
    }

    void Schedule::handleUDQ(HandlerContext& handlerContext)
    {
        auto new_udq = this->snapshots.back().udq();

        auto segment_matcher_factory = [this]()
        {
            return std::make_unique<SegmentMatcher>(this->snapshots.back());
        };

        for (const auto& record : handlerContext.keyword) {
            new_udq.add_record(segment_matcher_factory, record,
                               handlerContext.keyword.location(),
                               handlerContext.currentStep);
        }

        this->snapshots.back().udq.update(std::move(new_udq));
    }

    void Schedule::handleUDT(HandlerContext& handlerContext)
    {
        auto new_udq = this->snapshots.back().udq();

        using PUDT = ParserKeywords::UDT;

        const auto& header = handlerContext.keyword.getRecord(0);
        const std::string name = header.getItem<PUDT::TABLE_NAME>().get<std::string>(0);

        const int dim = header.getItem<PUDT::DIMENSIONS>().get<int>(0);
        if (dim != 1) {
            throw OpmInputError("Only 1D UDTs are supported",
                                handlerContext.keyword.location());

        }

        const auto& points = handlerContext.keyword.getRecord(1);
        const std::string interp_type = points.getItem<PUDT::INTERPOLATION_TYPE>().get<std::string>(0);
        UDT::InterpolationType type;
        if (interp_type == "NV") {
            type = UDT::InterpolationType::NearestNeighbour;
        } else if (interp_type == "LC") {
            type = UDT::InterpolationType::LinearClamp;
        } else if (interp_type == "LL") {
            type = UDT::InterpolationType::LinearExtrapolate;
        } else {
            throw OpmInputError(fmt::format("Unknown UDT interpolation type {}", interp_type),
                                handlerContext.keyword.location());
        }
        const auto x_vals = points.getItem<ParserKeywords::UDT::INTERPOLATION_POINTS>().getData<double>();

        if (!std::is_sorted(x_vals.begin(), x_vals.end())) {
            throw OpmInputError("UDT: Interpolation points need to be given in ascending order",
                                handlerContext.keyword.location());
        }

        if (auto it = std::adjacent_find(x_vals.begin(), x_vals.end()); it != x_vals.end()) {
            throw OpmInputError(fmt::format("UDT: Interpolation points need to be unique: "
                                            "found duplicate for {}", *it),
                                handlerContext.keyword.location());
        }

        const auto& data = handlerContext.keyword.getRecord(2);
        const auto y_vals = data.getItem<ParserKeywords::UDT::TABLE_VALUES>().getData<double>();

        if (x_vals.size() != y_vals.size()) {
            throw OpmInputError(fmt::format("UDT data size mismatch, number of x-values {}",
                                            ", number of y-values {}",
                                            x_vals.size(), y_vals.size()),
                                handlerContext.keyword.location());
        }

        new_udq.add_table(name, UDT(x_vals, y_vals, type));

        this->snapshots.back().udq.update(std::move(new_udq));
    }

    void Schedule::handleVAPPARS(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            double vap1 = record.getItem("OIL_VAP_PROPENSITY").get< double >(0);
            double vap2 = record.getItem("OIL_DENSITY_PROPENSITY").get< double >(0);
            auto& ovp = this->snapshots.back().oilvap();
            OilVaporizationProperties::updateVAPPARS(ovp, vap1, vap2);
        }
    }

    void Schedule::handleVFPINJ(HandlerContext& handlerContext) {
        auto table = VFPInjTable(handlerContext.keyword, this->m_static.m_unit_system);
        this->snapshots.back().events().addEvent( ScheduleEvents::VFPINJ_UPDATE );
        this->snapshots.back().vfpinj.update( std::move(table) );
    }

    void Schedule::handleVFPPROD(HandlerContext& handlerContext) {
        auto table = VFPProdTable(handlerContext.keyword, this->m_static.gaslift_opt_active, this->m_static.m_unit_system);
        this->snapshots.back().events().addEvent( ScheduleEvents::VFPPROD_UPDATE );
        this->snapshots.back().vfpprod.update( std::move(table) );
    }

    void Schedule::handleWCONHIST(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);

            const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

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

                if (table_nr != 0) {
                    const auto& vfpprod = this->snapshots.back().vfpprod;
                    if (vfpprod.has(table_nr))
                        alq_type = this->snapshots.back().vfpprod(table_nr).getALQType();
                    else {
                        std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                        throw OpmInputError(reason, handlerContext.keyword.location());
                    }
                }
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

    void Schedule::handleWCONPROD(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);

            const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

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

                if (table_nr != 0) {
                    const auto& vfpprod = this->snapshots.back().vfpprod;
                    if (vfpprod.has(table_nr))
                        alq_type = this->snapshots.back().vfpprod(table_nr).getALQType();
                    else {
                        std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                        throw OpmInputError(reason, handlerContext.keyword.location());
                    }
                }
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

                if (well2.getStatus() == WellStatus::OPEN) {
                    this->snapshots.back().wellgroup_events().addEvent(well2.name(), ScheduleEvents::REQUEST_OPEN_WELL);
                }

                if (update_well) {
                    this->snapshots.back().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    this->snapshots.back().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                    this->snapshots.back().wells.update( std::move(well2) );
                }

                auto udq_active = this->snapshots.back().udq_active.get();
                if (properties->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), udq_active))
                    this->snapshots.back().udq_active.update( std::move(udq_active));

                handlerContext.affected_well(well_name);
            }
        }
    }

    void Schedule::handleWCONINJE(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext,
                                              isWList(handlerContext.currentStep,
                                                      wellNamePattern));

            const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

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

                if (this->snapshots.back().wells.get( well_name ).getStatus() == Well::Status::OPEN) {
                    this->snapshots.back().wellgroup_events().addEvent(well_name, ScheduleEvents::REQUEST_OPEN_WELL);
                }

                auto udq_active = this->snapshots.back().udq_active.get();
                if (injection->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), udq_active))
                    this->snapshots.back().udq_active.update( std::move(udq_active) );

                handlerContext.affected_well(well_name);
            }
        }
    }

    void Schedule::handleWCONINJH(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);
            const Well::Status status = WellStatusFromString( record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                this->updateWellStatus(well_name, handlerContext.currentStep, status, handlerContext.keyword.location());
                bool update_well = false;
                auto well2 = this->snapshots.back().wells.get( well_name );
                auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                auto previousInjectorType = injection->injectorType;
                injection->handleWCONINJH(record, well2.isProducer(), well_name, handlerContext.keyword.location());
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

    void Schedule::handleWECON(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well2 = this->snapshots.back().wells.get( well_name );
                auto econ_limits = std::make_shared<WellEconProductionLimits>( record );
                if (well2.updateEconLimits(econ_limits))
                    this->snapshots.back().wells.update( std::move(well2) );
            }
        }
    }

    void Schedule::handleWDFAC(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells.get(well_name);
                auto wdfac = std::make_shared<WDFAC>(well.getWDFAC());
                wdfac->updateWDFAC( record );
                if (well.updateWDFAC(std::move(wdfac)))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWDFACCOR(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELLNAME").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells.get(well_name);
                auto wdfac = std::make_shared<WDFAC>(well.getWDFAC());
                wdfac->updateWDFACCOR( record );
                if (well.updateWDFAC(std::move(wdfac)))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWEFAC(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELLNAME").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));

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

    void Schedule::handleWELOPEN(HandlerContext& handlerContext) {
        const auto& keyword = handlerContext.keyword;
        const auto& currentStep = handlerContext.currentStep;

        auto conn_defaulted = []( const DeckRecord& rec ) {
            auto defaulted = []( const DeckItem& item ) {
                return item.defaultApplied( 0 );
            };

            return std::all_of( rec.begin() + 2, rec.end(), defaulted );
        };

        constexpr auto open = Well::Status::OPEN;

        for (const auto& record : keyword) {
            const auto& wellNamePattern = record.getItem( "WELL" ).getTrimmedString(0);
            const auto& status_str = record.getItem( "STATUS" ).getTrimmedString( 0 );
            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));

            /* if all records are defaulted or just the status is set, only
             * well status is updated
             */
            if (conn_defaulted(record)) {
                const auto new_well_status = WellStatusFromString(status_str);
                for (const auto& wname : well_names) {
                    const auto did_update_well_status =
                        this->updateWellStatus(wname, currentStep, new_well_status);

                    handlerContext.affected_well(wname);

                    if (did_update_well_status) {
                        handlerContext.record_well_structure_change();
                    }

                    if (did_update_well_status && (new_well_status == open)) {
                        // Record possible well injection/production status change
                        auto well2 = this->snapshots[currentStep].wells.get(wname);

                        const auto did_flow_update =
                            (well2.isProducer() && well2.updateHasProduced())
                            ||
                            (well2.isInjector() && well2.updateHasInjected());

                        if (did_flow_update) {
                            this->snapshots[currentStep].wells.update(std::move(well2));
                        }
                    }

                    if (new_well_status == open) {
                        this->snapshots.back().wellgroup_events().addEvent( wname, ScheduleEvents::REQUEST_OPEN_WELL);
                    }
                }
                continue;
            }

            /*
              Some of the connection information has been entered, in this case
              we *only* update the status of the connections, and not the well
              itself. Unless all connections are shut - then the well is also
              shut.
             */
            for (const auto& wname : well_names) {
                {
                    auto well = this->snapshots[currentStep].wells.get(wname);
                    this->snapshots[currentStep].wells.update( std::move(well) );
                }

                const auto connection_status = Connection::StateFromString( status_str );
                {
                    auto well = this->snapshots[currentStep].wells.get(wname);
                    well.handleWELOPENConnections(record, connection_status);
                    this->snapshots[currentStep].wells.update( std::move(well) );
                }

                handlerContext.affected_well(wname);
                handlerContext.record_well_structure_change();

                this->snapshots.back().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
            }
        }
    }

    void Schedule::handleWELPIRuntime(HandlerContext& handlerContext) {
        using WELL_NAME = ParserKeywords::WELPI::WELL_NAME;
        using PI        = ParserKeywords::WELPI::STEADY_STATE_PRODUCTIVITY_OR_INJECTIVITY_INDEX_VALUE;

        auto report_step = handlerContext.currentStep;
        for (const auto& record : handlerContext.keyword) {
            const auto well_names = this->wellNames(record.getItem<WELL_NAME>().getTrimmedString(0),
                                                    handlerContext);
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

                handlerContext.affected_well(well_name);
            }
        }
    }

    void Schedule::handleWELPI(HandlerContext& handlerContext) {
        if (handlerContext.actionx_mode)
            this->handleWELPIRuntime(handlerContext);
        else {
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
            using PI = ParserKeywords::WELPI::STEADY_STATE_PRODUCTIVITY_OR_INJECTIVITY_INDEX_VALUE;
            const auto& keyword = handlerContext.keyword;

            for (const auto& record : keyword) {
                const auto& wellNamePattern = record.getItem<WELL_NAME>().getTrimmedString(0);
                const auto well_names
                    = this->wellNames(wellNamePattern, handlerContext,
                                      isWList(handlerContext.currentStep,
                                              wellNamePattern));

                const auto rawProdIndex = record.getItem<PI>().get<double>(0);
                for (const auto& well_name : well_names) {
                    auto well2 = this->snapshots.back().wells.get(well_name);

                    // Note: Need to ensure we have an independent copy of
                    // well's connections because
                    // Well::updateWellProductivityIndex() implicitly mutates
                    // internal state in the WellConnections class.
                    auto connections = std::make_shared<WellConnections>(well2.getConnections());
                    well2.updateConnections(std::move(connections), true);
                    if (well2.updateWellProductivityIndex())
                        this->snapshots.back().wells.update(std::move(well2));

                    this->snapshots.back().wellgroup_events().addEvent(well_name,
                                                                       ScheduleEvents::WELL_PRODUCTIVITY_INDEX);
                    this->snapshots.back().target_wellpi[well_name] = rawProdIndex;
                }
            }
            this->snapshots.back().events().addEvent(ScheduleEvents::WELL_PRODUCTIVITY_INDEX);
        }
    }

    void Schedule::handleWELSEGS(HandlerContext& handlerContext) {
        const auto& record1 = handlerContext.keyword.getRecord(0);
        const auto& wname = record1.getItem("WELL").getTrimmedString(0);
        if (this->hasWell(wname, handlerContext.currentStep)) {
            auto well = this->snapshots.back().wells.get(wname);
            if (well.handleWELSEGS(handlerContext.keyword)) {
                this->snapshots.back().wells.update( std::move(well) );
                handlerContext.record_well_structure_change();
            }
            handlerContext.welsegs_handled(wname);
        } else {
            const auto& location = handlerContext.keyword.location();
            if (this->action_wgnames.has_well(wname)) {
                std::string msg = fmt::format(R"(Well: {} not yet defined for keyword {}.
Expecting well to be defined with WELSPECS in ACTIONX before actual use.
File {} line {}.)", wname, location.keyword, location.filename, location.lineno);
                OpmLog::warning(msg);
            } else
                throw OpmInputError(fmt::format("No such well: ", wname), location);
        }
    }

    void Schedule::handleWELSPECS(HandlerContext& handlerContext)
    {
        using Kw = ParserKeywords::WELSPECS;

        auto getTrimmedName = [&handlerContext](const auto& item)
        {
            return trim_wgname(handlerContext.keyword,
                               item.template get<std::string>(0),
                               handlerContext.parseContext,
                               handlerContext.errors);
        };

        auto fieldWells = std::vector<std::string>{};
        for (const auto& record : handlerContext.keyword) {
            if (const auto fip_region_number = record.getItem<Kw::FIP_REGION>().get<int>(0);
                fip_region_number != Kw::FIP_REGION::defaultValue)
            {
                const auto& location = handlerContext.keyword.location();
                const auto msg = fmt::format("Non-defaulted FIP region {} in WELSPECS keyword "
                                             "in file {} line {} is not supported. "
                                             "Reset to default value {}.",
                                             fip_region_number,
                                             location.filename,
                                             location.lineno,
                                             Kw::FIP_REGION::defaultValue);
                OpmLog::warning(msg);
            }

            if (const auto& density_calc_type = record.getItem<Kw::DENSITY_CALC>().get<std::string>(0);
                density_calc_type != Kw::DENSITY_CALC::defaultValue)
            {
                const auto& location = handlerContext.keyword.location();
                const auto msg = fmt::format("Non-defaulted density calculation method '{}' "
                                             "in WELSPECS keyword in file {} line {} is "
                                             "not supported. Reset to default value {}.",
                                             density_calc_type,
                                             location.filename,
                                             location.lineno,
                                             Kw::DENSITY_CALC::defaultValue);
                OpmLog::warning(msg);
            }

            const auto wellName = getTrimmedName(record.getItem<Kw::WELL>());
            const auto groupName = getTrimmedName(record.getItem<Kw::GROUP>());

            // We might get here from an ACTIONX context, or we might get
            // called on a well (list) template, to reassign certain well
            // properties--e.g, the well's controlling group--so check if
            // 'wellName' matches any existing well names through pattern
            // matching before treating the wellName as a simple well name.
            //
            // An empty list of well names is okay since that means we're
            // creating a new well in this case.
            const auto allowEmptyWellList = true;
            const auto existingWells =
                this->wellNames(wellName, handlerContext, allowEmptyWellList);

            if (groupName == "FIELD") {
                if (existingWells.empty()) {
                    fieldWells.push_back(wellName);
                }
                else {
                    for (const auto& existingWell : existingWells) {
                        fieldWells.push_back(existingWell);
                    }
                }
            }

            if (! this->snapshots.back().groups.has(groupName)) {
                this->addGroup(groupName, handlerContext.currentStep);
            }

            if (existingWells.empty()) {
                // 'wellName' does not match any existing wells.  Create a
                // new Well object for this well.
                this->welspecsCreateNewWell(record,
                                            wellName,
                                            groupName,
                                            handlerContext);
            }
            else {
                // 'wellName' matches one or more existing wells.  Assign
                // new properties for those wells.
                this->welspecsUpdateExistingWells(record,
                                                  existingWells,
                                                  groupName,
                                                  handlerContext);
            }
        }

        if (! fieldWells.empty()) {
            std::sort(fieldWells.begin(), fieldWells.end());
            fieldWells.erase(std::unique(fieldWells.begin(), fieldWells.end()),
                             fieldWells.end());

            const auto* plural = (fieldWells.size() == 1) ? "" : "s";

            const auto msg_fmt = fmt::format(R"(Well{0} parented directly to 'FIELD'; this is allowed but discouraged.
Well{0} entered with 'FIELD' parent group:
 * {1})", plural, fmt::join(fieldWells, "\n * "));

            handlerContext.parseContext.handleError(ParseContext::SCHEDULE_WELL_IN_FIELD_GROUP,
                                                    msg_fmt,
                                                    handlerContext.keyword.location(),
                                                    handlerContext.errors);
        }

        if (! handlerContext.keyword.empty()) {
            handlerContext.record_well_structure_change();
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

      Update: See the discussion following the definitions of the SI factors, due
      to a bad design we currently need the well to be specified with
      WCONPROD / WCONHIST before WELTARG is applied, if not the units for the
      rates will be wrong.
    */
    void Schedule::handleWELTARG(HandlerContext& handlerContext) {
        const double SiFactorP = this->m_static.m_unit_system.parse("Pressure").getSIScaling();
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));
            if (well_names.empty())
                this->invalidNamePattern( wellNamePattern, handlerContext);

            const auto cmode = WellWELTARGCModeFromString(record.getItem("CMODE").getTrimmedString(0));
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
                    if (prop->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), cmode, udq_active))
                        this->snapshots.back().udq_active.update( std::move(udq_active));
                }
                else {
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                    inj->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2.updateInjection(inj);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2.updateWellGuideRate(new_arg.get<double>());

                    auto udq_active = this->snapshots.back().udq_active.get();
                    if (inj->updateUDQActive(this->getUDQConfig(handlerContext.currentStep), cmode, udq_active))
                        this->snapshots.back().udq_active.update(std::move(udq_active));
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

                handlerContext.affected_well(well_name);
            }
        }
    }

    void Schedule::handleWFOAM(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well2 = this->snapshots.back().wells.get(well_name);
                auto foam_properties = std::make_shared<WellFoamProperties>(well2.getFoamProperties());
                foam_properties->handleWFOAM(record);
                if (well2.updateFoamProperties(foam_properties))
                    this->snapshots.back().wells.update( std::move(well2) );
            }
        }
    }

    void Schedule::handleWGRUPCON(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));

            const bool availableForGroupControl = DeckItem::to_bool(record.getItem("GROUP_CONTROLLED").getTrimmedString(0));
            const double guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
            const double scaling_factor = record.getItem("SCALING_FACTOR").get<double>(0);

            for (const auto& well_name : well_names) {
                auto phase = Well::GuideRateTarget::UNDEFINED;
                if (!record.getItem("PHASE").defaultApplied(0)) {
                    std::string guideRatePhase = record.getItem("PHASE").getTrimmedString(0);
                    phase = WellGuideRateTargetFromString(guideRatePhase);
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

    void Schedule::handleWHISTCTL(HandlerContext& handlerContext) {
        const auto& record = handlerContext.keyword.getRecord(0);
        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const auto controlMode = WellProducerCModeFromString(cmodeString);

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
            handlerContext.parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg_fmt, handlerContext.keyword.location(), handlerContext.errors );
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

    void Schedule::handleWINJMULT(Opm::Schedule::HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL_NAME").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );
                if (well.isProducer()) {
                    const std::string reason = fmt::format("Keyword WINJMULT can only apply to injectors,"
                                                           " but Well {} is a producer", well_name);
                    throw OpmInputError(reason, handlerContext.keyword.location());
                }
                if (well.handleWINJMULT(record, handlerContext.keyword.location())) {
                    this->snapshots.back().wells.update(std::move(well));
                }
            }
        }
    }

    void Schedule::handleWINJTEMP(HandlerContext& handlerContext) {
        // we do not support the "enthalpy" field yet. how to do this is a more difficult
        // question.
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto well_names = wellNames(wellNamePattern, handlerContext);

            const double temp = record.getItem("TEMPERATURE").getSIDouble(0);

            for (const auto& well_name : well_names) {
                const auto& well = this->getWell(well_name, handlerContext.currentStep);
                const double current_temp = !well.isProducer()? well.temperature(): 0.0;
                if (current_temp != temp) {
                    auto well2 = this->snapshots.back().wells( well_name );
                    well2.setWellTemperature(temp);
                    this->snapshots.back().wells.update( std::move(well2) );
                }
            }
        }
    }

    void Schedule::handleWLIFTOPT(HandlerContext& handlerContext) {
        auto glo = this->snapshots.back().glo();

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WLIFTOPT::WELL>().getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern);
            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

            const bool use_glo = DeckItem::to_bool(record.getItem<ParserKeywords::WLIFTOPT::USE_OPTIMIZER>().get<std::string>(0));
            const bool alloc_extra_gas = DeckItem::to_bool( record.getItem<ParserKeywords::WLIFTOPT::ALLOCATE_EXTRA_LIFT_GAS>().get<std::string>(0));
            const double weight_factor = record.getItem<ParserKeywords::WLIFTOPT::WEIGHT_FACTOR>().get<double>(0);
            const double inc_weight_factor = record.getItem<ParserKeywords::WLIFTOPT::DELTA_GAS_RATE_WEIGHT_FACTOR>().get<double>(0);
            const double min_rate = record.getItem<ParserKeywords::WLIFTOPT::MIN_LIFT_GAS_RATE>().getSIDouble(0);
            const auto& max_rate_item = record.getItem<ParserKeywords::WLIFTOPT::MAX_LIFT_GAS_RATE>();

            for (const auto& wname : well_names) {
                auto well = GasLiftWell(wname, use_glo);

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

    void Schedule::handleWLIST(HandlerContext& handlerContext) {
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
                // does not use overload for context to avoid throw
                const auto& names = this->wellNames(well_arg, handlerContext.currentStep,
                                                    handlerContext.matching_wells);
                if (names.empty() && well_arg.find("*") == std::string::npos)
                    throw std::invalid_argument("The well: " + well_arg + " has not been defined in the WELSPECS");

                std::move(names.begin(), names.end(), std::back_inserter(wells));
            }

            if (name[0] != '*')
                throw std::invalid_argument("The list name in WLIST must start with a '*'");

            if (action == "NEW") {
                new_wlm.newList(name, wells);
            }

            if (!new_wlm.hasList(name))
                throw std::invalid_argument("Invalid well list: " + name);

            if (action == "MOV") {
                for (const auto& well : wells) {
                    new_wlm.delWell(well);
                }
            }

            if (action == "DEL") {
                for (const auto& well : wells) {
                    new_wlm.delWListWell(well, name);
                }
            } else if (action != "NEW"){
                for (const auto& well : wells) {
                    new_wlm.addWListWell(well, name);
                }
            }
            this->snapshots.back().wlist_manager.update( std::move(new_wlm) );
        }
    }

    void Schedule::handleWMICP(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );
                auto micp_properties = std::make_shared<WellMICPProperties>( well.getMICPProperties() );
                micp_properties->handleWMICP(record);
                if (well.updateMICPProperties(micp_properties))
                    this->snapshots.back().wells.update( std::move(well));
            }
        }
    }

    void Schedule::handleWPIMULT(HandlerContext& handlerContext) {
        // from the third item to the seventh item in the WPIMULT record, they are numbers indicate
        // the I, J, K location and completion number range.
        // When defaulted, it assumes it is negative
        // When inputting a negative value, it assumes it is defaulted.
        auto defaultConCompRec = [](const DeckRecord& wpimult)
        {
            return std::all_of(wpimult.begin() + 2, wpimult.end(),
                [](const DeckItem& item)
                {
                    return item.defaultApplied(0) || (item.get<int>(0) < 0);
                });
        };

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto& well_names = this->wellNames(wellNamePattern, handlerContext,
                                                     isWList(handlerContext.currentStep,
                                                             wellNamePattern));

            // for the record has defaulted connection and completion information, we do not apply it immediately
            // because we only need to apply the last record with defaulted connection and completion information
            // as a result, we here only record the information of the record with defaulted connection and completion
            // information without applying, because there might be multiple WPIMULT keywords here, and we do not know
            // whether it is the last one.
            const bool default_con_comp = defaultConCompRec(record);
            if (default_con_comp) {
                auto wpimult_global_factor = handlerContext.wpimult_global_factor;
                if (!wpimult_global_factor) {
                    throw std::runtime_error(" wpimult_global_factor is nullptr in function handleWPIMULT ");
                }
                const auto scaling_factor = record.getItem("WELLPI").get<double>(0);
                for (const auto& wname : well_names) {
                    (*wpimult_global_factor)[wname] = scaling_factor;
                }
                continue;
            }

            // the record with non-defaulted connection and completion information will be applied immediately
            for (const auto& wname : well_names) {
                auto well = this->snapshots.back().wells( wname );
                if (well.handleWPIMULT(record))
                    this->snapshots.back().wells.update( std::move(well));
            }
        }
    }

    void Schedule::handleWINJCLN(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WINJCLN::WELL_NAME>().getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);
            for (const auto& well_name: well_names) {
                auto well = this->snapshots.back().wells(well_name);
                well.handleWINJCLN(record, handlerContext.keyword.location());
                this->snapshots.back().wells.update(std::move(well));
            }
        }
    }

    void Schedule::handleWINJDAM(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WINJDAM::WELL_NAME>().getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells(well_name);
                if (well.handleWINJDAM(record, handlerContext.keyword.location())) {
                    this->snapshots.back().wells.update( std::move(well) );
                }
            }
        }
    }

    void Schedule::handleWINJFCNC(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WINJFCNC::WELL>().getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);
            for (const auto& well_name: well_names) {
                auto well = this->snapshots.back().wells(well_name);
                const auto filter_conc = record.getItem<ParserKeywords::WINJFCNC::VOL_CONCENTRATION>().get<UDAValue>(0);
                well.setFilterConc(filter_conc );
                this->snapshots.back().wells.update(std::move(well));
            }
        }
    }

    void Schedule::handleWPMITAB(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );
                auto polymer_properties = std::make_shared<WellPolymerProperties>( well.getPolymerProperties() );
                polymer_properties->handleWPMITAB(record);
                if (well.updatePolymerProperties(polymer_properties))
                    this->snapshots.back().wells.update( std::move(well));
            }
        }
    }

    void Schedule::handleWPOLYMER(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );
                auto polymer_properties = std::make_shared<WellPolymerProperties>( well.getPolymerProperties() );
                polymer_properties->handleWPOLYMER(record);
                if (well.updatePolymerProperties(polymer_properties))
                    this->snapshots.back().wells.update( std::move(well));
            }
        }
    }

    void Schedule::handleWSALT(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern);

            for (const auto& well_name : well_names) {
                auto well2 = this->snapshots.back().wells( well_name );
                auto brine_properties = std::make_shared<WellBrineProperties>(well2.getBrineProperties());
                brine_properties->handleWSALT(record);
                if (well2.updateBrineProperties(brine_properties))
                    this->snapshots.back().wells.update( std::move(well2) );
            }
        }
    }

    void Schedule::handleWSEGITER(HandlerContext& handlerContext) {
        const auto& record = handlerContext.keyword.getRecord(0);
        auto& tuning = this->snapshots.back().tuning();

        tuning.MXWSIT = record.getItem<ParserKeywords::WSEGITER::MAX_WELL_ITERATIONS>().get<int>(0);
        tuning.WSEG_MAX_RESTART = record.getItem<ParserKeywords::WSEGITER::MAX_TIMES_REDUCED>().get<int>(0);
        tuning.WSEG_REDUCTION_FACTOR = record.getItem<ParserKeywords::WSEGITER::REDUCTION_FACTOR>().get<double>(0);
        tuning.WSEG_INCREASE_FACTOR = record.getItem<ParserKeywords::WSEGITER::INCREASING_FACTOR>().get<double>(0);

        this->snapshots.back().events().addEvent(ScheduleEvents::TUNING_CHANGE);
    }

    void Schedule::handleWSEGSICD(HandlerContext& handlerContext) {
        std::map<std::string, std::vector<std::pair<int, SICD> > > spiral_icds = SICD::fromWSEGSICD(handlerContext.keyword);

        for (auto& map_elem : spiral_icds) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, handlerContext);

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

    void Schedule::handleWSEGAICD(HandlerContext& handlerContext) {
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

    void Schedule::handleWSEGVALV(HandlerContext& handlerContext) {
        const double udq_default = this->getUDQConfig(handlerContext.currentStep).params().undefinedValue();
        const std::map<std::string, std::vector<std::pair<int, Valve> > > valves = Valve::fromWSEGVALV(handlerContext.keyword, udq_default);

        for (const auto& map_elem : valves) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            well_name_pattern));

            const std::vector<std::pair<int, Valve> >& valve_pairs = map_elem.second;

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );
                if (well.updateWSEGVALV(valve_pairs))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWSKPTAB(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells( well_name );

                auto polymer_properties = std::make_shared<WellPolymerProperties>(well.getPolymerProperties());
                polymer_properties->handleWSKPTAB(record);
                if (well.updatePolymerProperties(polymer_properties))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWSOLVENT(HandlerContext& handlerContext) {

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , handlerContext);

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

    void Schedule::handleWTEMP(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames( wellNamePattern, handlerContext);
            double temp = record.getItem("TEMP").getSIDouble(0);

            for (const auto& well_name : well_names) {
                const auto& well = this->getWell(well_name, handlerContext.currentStep);
                const double current_temp = !well.isProducer()? well.temperature(): 0.0;
                if (current_temp != temp) {
                    auto well2 = this->snapshots.back().wells( well_name );
                    well2.setWellTemperature(temp);
                    this->snapshots.back().wells.update( std::move(well2) );
                }
            }
        }
    }

    void Schedule::handleWTEST(HandlerContext& handlerContext) {
        auto new_config = this->snapshots.back().wtest_config.get();
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));
            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

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

    void Schedule::handleWTRACER(HandlerContext& handlerContext) {

        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);

            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

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

    void Schedule::handleWPAVE(HandlerContext& handlerContext) {
        auto wpave = PAvg( handlerContext.keyword.getRecord(0) );

        if (wpave.inner_weight() > 1.0) {
            const auto reason =
                fmt::format("Inner block weighting F1 "
                            "must not exceed 1.0. Got {}",
                            wpave.inner_weight());

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        if ((wpave.conn_weight() < 0.0) ||
            (wpave.conn_weight() > 1.0))
        {
            const auto reason =
                fmt::format("Connection weighting factor F2 "
                            "must be between zero and one "
                            "inclusive. Got {} instead.",
                            wpave.conn_weight());

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        for (const auto& wname : this->wellNames(handlerContext.currentStep))
            this->updateWPAVE(wname, handlerContext.currentStep, wpave );

        auto& sched_state = this->snapshots.back();
        sched_state.pavg.update(std::move(wpave));
    }

    void Schedule::handleWVFPDP(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells.get(well_name);
                auto wvfpdp = std::make_shared<WVFPDP>(well.getWVFPDP());
                wvfpdp->update( record );
                if (well.updateWVFPDP(std::move(wvfpdp)))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWVFPEXP(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);
            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells.get(well_name);
                auto wvfpexp = std::make_shared<WVFPEXP>(well.getWVFPEXP());
                wvfpexp->update( record );
                if (well.updateWVFPEXP(std::move(wvfpexp)))
                    this->snapshots.back().wells.update( std::move(well) );
            }
        }
    }

    void Schedule::handleWWPAVE(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext.currentStep);

            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

            auto wpave = PAvg(record);

            if (wpave.inner_weight() > 1.0) {
                const auto reason =
                    fmt::format("Inner block weighting F1 "
                                "must not exceed one. Got {}",
                                wpave.inner_weight());

                throw OpmInputError {
                    reason, handlerContext.keyword.location()
                };
            }

            if ((wpave.conn_weight() < 0.0) ||
                (wpave.conn_weight() > 1.0))
            {
                const auto reason =
                    fmt::format("Connection weighting factor F2 "
                                "must be between zero and one, "
                                "inclusive. Got {} instead.",
                                wpave.conn_weight());

                throw OpmInputError {
                    reason, handlerContext.keyword.location()
                };
            }

            for (const auto& well_name : well_names)
                this->updateWPAVE(well_name, handlerContext.currentStep, wpave);
        }
    }

    void Schedule::handleWPAVEDEP(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WPAVEDEP::WELL>().getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);

            if (well_names.empty())
                this->invalidNamePattern(wellNamePattern, handlerContext);

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

    void Schedule::handleWRFT(HandlerContext& handlerContext)
    {
        auto new_rft = this->snapshots.back().rft_config();

        for (const auto& record : handlerContext.keyword) {
            const auto& item = record.getItem<ParserKeywords::WRFT::WELL>();
            if (! item.hasValue(0)) {
                continue;
            }

            const auto wellNamePattern = record.getItem<ParserKeywords::WRFT::WELL>().getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);

            if (well_names.empty()) {
                this->invalidNamePattern(wellNamePattern, handlerContext);
            }

            for (const auto& well_name : well_names) {
                new_rft.update(well_name, RFTConfig::RFT::YES);
            }
        }

        new_rft.first_open(true);

        this->snapshots.back().rft_config.update(std::move(new_rft));
    }

    void Schedule::handleWRFTPLT(HandlerContext& handlerContext)
    {
        auto new_rft = this->snapshots.back().rft_config();

        const auto rftKey = [](const DeckItem& key)
        {
            return RFTConfig::RFTFromString(key.getTrimmedString(0));
        };

        const auto pltKey = [](const DeckItem& key)
        {
            return RFTConfig::PLTFromString(key.getTrimmedString(0));
        };

        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem<ParserKeywords::WRFTPLT::WELL>().getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, handlerContext);

            if (well_names.empty()) {
                this->invalidNamePattern(wellNamePattern, handlerContext);
                continue;
            }

            const auto RFTKey = rftKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_RFT>());
            const auto PLTKey = pltKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_PLT>());
            const auto SEGKey = pltKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_SEGMENT>());

            for (const auto& well_name : well_names) {
                new_rft.update(well_name, RFTKey);
                new_rft.update(well_name, PLTKey);
                new_rft.update_segment(well_name, SEGKey);
            }
        }

        this->snapshots.back().rft_config.update(std::move(new_rft));
    }


/*
  The WTMULT keyword can optionally use UDA values in three different ways:

    1. The target can be UDA - instead of the standard strings "ORAT", "GRAT",
       "WRAT", ..., the keyword can be configured with a UDA which is evaluated to
       an integer and then mapped to one of the common controls.

    2. The scaling factor itself can be a UDA.

    3. The target we aim to scale might already be specified as a UDA.

  The current implementation does not support UDA usage in any part of WTMULT
  codepath.
*/

    void Schedule::handleWTMULT(HandlerContext& handlerContext) {
        for (const auto& record : handlerContext.keyword) {
            const auto& wellNamePattern = record.getItem<ParserKeywords::WTMULT::WELL>().getTrimmedString(0);
            const auto& control = record.getItem<ParserKeywords::WTMULT::CONTROL>().get<std::string>(0);
            const auto& factor = record.getItem<ParserKeywords::WTMULT::FACTOR>().get<UDAValue>(0);
            const auto& num = record.getItem<ParserKeywords::WTMULT::NUM>().get<int>(0);

            if (factor.is<std::string>()) {
                std::string reason = fmt::format("Use of UDA value: {} is not supported as multiplier", factor.get<std::string>());
                throw OpmInputError(reason, handlerContext.keyword.location());
            }

            if (this->snapshots.back().udq().has_keyword(control)) {
                std::string reason = fmt::format("Use of UDA value: {} is not supported for control target", control);
                throw OpmInputError(reason, handlerContext.keyword.location());
            }

            if (num != 1) {
                std::string reason = fmt::format("Only NUM=1 is supported in WTMULT keyword");
                throw OpmInputError(reason, handlerContext.keyword.location());
            }

            const auto cmode = WellWELTARGCModeFromString(control);
            if (cmode == Well::WELTARGCMode::GUID)
                throw std::logic_error("Multiplying guide rate is not implemented");

            const auto well_names = this->wellNames(wellNamePattern, handlerContext,
                                                    isWList(handlerContext.currentStep,
                                                            wellNamePattern));
            for (const auto& well_name : well_names) {
                auto well = this->snapshots.back().wells.get(well_name);
                if (well.isInjector()) {
                    bool update_well = true;
                    auto properties = std::make_shared<Well::WellInjectionProperties>(well.getInjectionProperties());
                    properties->handleWTMULT( cmode, factor.get<double>());

                    well.updateInjection(properties);
                    if (update_well) {
                        this->snapshots.back().events().addEvent(ScheduleEvents::INJECTION_UPDATE);
                        this->snapshots.back().wellgroup_events().addEvent(well_name, ScheduleEvents::INJECTION_UPDATE);
                        this->snapshots.back().wells.update(std::move(well));
                    }
                } else {
                    bool update_well = true;
                    auto properties = std::make_shared<Well::WellProductionProperties>(well.getProductionProperties());
                    properties->handleWTMULT( cmode, factor.get<double>());

                    well.updateProduction(properties);
                    if (update_well) {
                        this->snapshots.back().events().addEvent(ScheduleEvents::PRODUCTION_UPDATE);
                        this->snapshots.back().wellgroup_events().addEvent(well_name,
                                                                           ScheduleEvents::PRODUCTION_UPDATE);
                        this->snapshots.back().wells.update(std::move(well));
                    }
                }
            }
        }
    }


    bool Schedule::handleNormalKeyword(HandlerContext& handlerContext) {
        using handler_function = void (Schedule::*) (HandlerContext&);
        static const std::unordered_map<std::string,handler_function> handler_functions = {
            { "AQUCT",    &Schedule::handleAQUCT     },
            { "AQUFETP",  &Schedule::handleAQUFETP   },
            { "AQUFLUX",  &Schedule::handleAQUFLUX   },
            { "BCPROP",   &Schedule::handleBCProp    },
            { "BOX",      &Schedule::handleGEOKeyword},
            { "BRANPROP", &Schedule::handleBRANPROP  },
            { "COMPDAT" , &Schedule::handleCOMPDAT   },
            { "COMPLUMP", &Schedule::handleCOMPLUMP  },
            { "COMPORD" , &Schedule::handleCOMPORD   },
            { "COMPSEGS", &Schedule::handleCOMPSEGS  },
            { "COMPTRAJ", &Schedule::handleCOMPTRAJ  },
            { "CSKIN",    &Schedule::handleCSKIN     },
            { "DRSDT"   , &Schedule::handleDRSDT     },
            { "DRSDTCON", &Schedule::handleDRSDTCON  },
            { "DRSDTR"  , &Schedule::handleDRSDTR    },
            { "DRVDT"   , &Schedule::handleDRVDT     },
            { "DRVDTR"  , &Schedule::handleDRVDTR    },
            { "ENDBOX"  , &Schedule::handleGEOKeyword},
            { "EXIT",     &Schedule::handleEXIT      },
            { "GCONINJE", &Schedule::handleGCONINJE  },
            { "GCONPROD", &Schedule::handleGCONPROD  },
            { "GCONSALE", &Schedule::handleGCONSALE  },
            { "GCONSUMP", &Schedule::handleGCONSUMP  },
            { "GECON",    &Schedule::handleGECON     },
            { "GEFAC"   , &Schedule::handleGEFAC     },
            { "GLIFTOPT", &Schedule::handleGLIFTOPT  },
            { "GPMAINT" , &Schedule::handleGPMAINT   },
            { "GRUPNET" , &Schedule::handleGRUPNET   },
            { "GRUPTREE", &Schedule::handleGRUPTREE  },
            { "GUIDERAT", &Schedule::handleGUIDERAT  },
            { "LIFTOPT" , &Schedule::handleLIFTOPT   },
            { "LINCOM"  , &Schedule::handleLINCOM    },
            { "MESSAGES", &Schedule::handleMESSAGES  },
            { "MULTFLT" , &Schedule::handleGEOKeyword},
            { "MULTPV"  , &Schedule::handleMXUNSUPP  },
            { "MULTR"   , &Schedule::handleMXUNSUPP  },
            { "MULTR-"  , &Schedule::handleMXUNSUPP  },
            { "MULTREGT", &Schedule::handleMXUNSUPP  },
            { "MULTSIG" , &Schedule::handleMXUNSUPP  },
            { "MULTSIGV", &Schedule::handleMXUNSUPP  },
            { "MULTTHT" , &Schedule::handleMXUNSUPP  },
            { "MULTTHT-", &Schedule::handleMXUNSUPP  },
            { "MULTX"   , &Schedule::handleGEOKeyword},
            { "MULTX-"  , &Schedule::handleGEOKeyword},
            { "MULTY"   , &Schedule::handleGEOKeyword},
            { "MULTY-"  , &Schedule::handleGEOKeyword},
            { "MULTZ"   , &Schedule::handleGEOKeyword},
            { "MULTZ-"  , &Schedule::handleGEOKeyword},
            { "NETBALAN", &Schedule::handleNETBALAN  },
            { "NEXT",     &Schedule::handleNEXTSTEP  },
            { "NEXTSTEP", &Schedule::handleNEXTSTEP  },
            { "NODEPROP", &Schedule::handleNODEPROP  },
            { "NUPCOL"  , &Schedule::handleNUPCOL    },
            { "RPTONLY" , &Schedule::handleRPTONLY   },
            { "RPTONLYO", &Schedule::handleRPTONLYO  },
            { "RPTRST"  , &Schedule::handleRPTRST    },
            { "RPTSCHED", &Schedule::handleRPTSCHED  },
            { "SAVE"    , &Schedule::handleSAVE      },
            { "SUMTHIN" , &Schedule::handleSUMTHIN   },
            { "TUNING"  , &Schedule::handleTUNING    },
            { "UDQ"     , &Schedule::handleUDQ       },
            { "UDT"     , &Schedule::handleUDT       },
            { "VAPPARS" , &Schedule::handleVAPPARS   },
            { "VFPINJ"  , &Schedule::handleVFPINJ    },
            { "VFPPROD" , &Schedule::handleVFPPROD   },
            { "WCONHIST", &Schedule::handleWCONHIST  },
            { "WCONINJE", &Schedule::handleWCONINJE  },
            { "WCONINJH", &Schedule::handleWCONINJH  },
            { "WCONPROD", &Schedule::handleWCONPROD  },
            { "WECON"   , &Schedule::handleWECON     },
            { "WDFAC"  ,  &Schedule::handleWDFAC     },
            { "WDFACCOR", &Schedule::handleWDFACCOR  },
            { "WEFAC"   , &Schedule::handleWEFAC     },
            { "WELOPEN" , &Schedule::handleWELOPEN   },
            { "WELPI"   , &Schedule::handleWELPI     },
            { "WELSEGS" , &Schedule::handleWELSEGS   },
            { "WELSPECS", &Schedule::handleWELSPECS  },
            { "WELTARG" , &Schedule::handleWELTARG   },
            { "WELTRAJ" , &Schedule::handleWELTRAJ   },
            { "WFOAM"   , &Schedule::handleWFOAM     },
            { "WGRUPCON", &Schedule::handleWGRUPCON  },
            { "WHISTCTL", &Schedule::handleWHISTCTL  },
            { "WINJMULT", &Schedule::handleWINJMULT  },
            { "WINJTEMP", &Schedule::handleWINJTEMP  },
            { "WLIFTOPT", &Schedule::handleWLIFTOPT  },
            { "WLIST"   , &Schedule::handleWLIST     },
            { "WMICP"   , &Schedule::handleWMICP     },
            { "WPAVE"   , &Schedule::handleWPAVE     },
            { "WPAVEDEP", &Schedule::handleWPAVEDEP  },
            { "WVFPDP",   &Schedule::handleWVFPDP    },
            { "WVFPEXP" , &Schedule::handleWVFPEXP   },
            { "WWPAVE"  , &Schedule::handleWWPAVE    },
            { "WPIMULT" , &Schedule::handleWPIMULT   },
            { "WINJDAM" , &Schedule::handleWINJDAM   },
            { "WINJFCNC", &Schedule::handleWINJFCNC  },
            { "WINJCLN",  &Schedule::handleWINJCLN   },
            { "WPMITAB" , &Schedule::handleWPMITAB   },
            { "WPOLYMER", &Schedule::handleWPOLYMER  },
            { "WRFT"    , &Schedule::handleWRFT      },
            { "WRFTPLT" , &Schedule::handleWRFTPLT   },
            { "WSALT"   , &Schedule::handleWSALT     },
            { "WSEGITER", &Schedule::handleWSEGITER  },
            { "WSEGSICD", &Schedule::handleWSEGSICD  },
            { "WSEGAICD", &Schedule::handleWSEGAICD  },
            { "WSEGVALV", &Schedule::handleWSEGVALV  },
            { "WSKPTAB" , &Schedule::handleWSKPTAB   },
            { "WSOLVENT", &Schedule::handleWSOLVENT  },
            { "WTEMP"   , &Schedule::handleWTEMP     },
            { "WTEST"   , &Schedule::handleWTEST     },
            { "WTMULT"  , &Schedule::handleWTMULT    },
            { "WTRACER" , &Schedule::handleWTRACER   },
        };

        auto function_iterator = handler_functions.find(handlerContext.keyword.name());
        if (function_iterator == handler_functions.end()) {
            return false;
        }

        try {
            std::invoke(function_iterator->second, this, handlerContext);
        } catch (const OpmInputError&) {
            throw;
        } catch (const std::logic_error& e) {
            // Rethrow as OpmInputError to provide more context,
            // but add "Internal error: " to the string, as that
            // is what logic_error signifies.
            const OpmInputError opm_error { std::string("Internal error: ") + e.what(), handlerContext.keyword.location() } ;
            OpmLog::error(opm_error.what());
            std::throw_with_nested(opm_error);
        } catch (const std::exception& e) {
            // Rethrow as OpmInputError to provide more context.
            const OpmInputError opm_error { e, handlerContext.keyword.location() } ;
            OpmLog::error(opm_error.what());
            std::throw_with_nested(opm_error);
        }

        return true;
    }

}
