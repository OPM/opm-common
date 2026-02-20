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

#include "WellKeywordHandlers.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQActive.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPDP.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFractureSeeds.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestConfig.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include "../HandlerContext.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm {

namespace {

/*
  The function trim_wgname() is used to trim the leading and trailing spaces
  away from the group and well arguments given in the WELSPECS
  keyword. If the deck argument contains a leading or trailing space that is
  treated as an input error, and the action taken is regulated by the setting
  ParseContext::PARSE_WGNAME_SPACE.

  Observe that the spaces are trimmed *unconditionally* - i.e. if the
  ParseContext::PARSE_WGNAME_SPACE setting is set to InputError::IGNORE that
  means that we do not inform the user about "our fix", but it is *not* possible
  to configure the parser to leave the spaces intact.
*/
std::string trim_wgname(const DeckKeyword& keyword,
                        const std::string& wgname_arg,
                        const ParseContext& parseContext,
                        ErrorGuard& errors)
{
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

void updateOpenShutEvents(HandlerContext& handlerContext, const std::string& well_name){
    if (handlerContext.getWellStatus(well_name) == WellStatus::OPEN) {
        handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::REQUEST_OPEN_WELL);
        handlerContext.state().wellgroup_events().clearEvent( well_name, ScheduleEvents::REQUEST_SHUT_WELL);
    }
    if (handlerContext.getWellStatus(well_name) == WellStatus::SHUT) {
        handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::REQUEST_SHUT_WELL);
        handlerContext.state().wellgroup_events().clearEvent( well_name, ScheduleEvents::REQUEST_OPEN_WELL);
    }
}

void handleWCONHIST(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

        for (const auto& well_name : well_names) {
            handlerContext.updateWellStatus(well_name, status,
                                            handlerContext.keyword.location());

            std::optional<VFPProdTable::ALQ_TYPE> alq_type;
            auto well2 = handlerContext.state().wells.get( well_name );
            const bool switching_from_injector = !well2.isProducer();
            auto properties = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
            bool update_well = false;

            auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
            if (record.getItem("VFP_TABLE").defaultApplied(0)) { // Default 1* use the privious set vfp table
                table_nr = properties->VFPTableNumber;
            }

            if (table_nr != 0) {
                const auto& vfpprod = handlerContext.state().vfpprod;
                if (vfpprod.has(table_nr)) {
                    alq_type = handlerContext.state().vfpprod(table_nr).getALQType();
                } else {
                    std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                    throw OpmInputError(reason, handlerContext.keyword.location());
                }
            }
            double default_bhp;
            if (handlerContext.state().bhp_defaults.get().prod_target) {
                default_bhp = *handlerContext.state().bhp_defaults.get().prod_target;
            } else {
                default_bhp = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                            ParserKeywords::FBHPDEF::TARGET_BHP::defaultValue);
            }

            // Injectors at a restart time will not have any WellProductionProperties with the
            // proper whistctl_cmode, so this needs to be set before the call to handleWCONHIST
            if (switching_from_injector) {
                properties->whistctl_cmode = handlerContext.state().whistctl();
            }

            properties->handleWCONHIST(alq_type,
                                       table_nr,
                                       default_bhp,
                                       handlerContext.static_schedule().m_unit_system, record);

            if (switching_from_injector) {
                if (properties->bhp_hist_limit_defaulted) {
                    properties->setBHPLimit(default_bhp);
                }

                auto inj_props = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                inj_props->resetBHPLimit();
                well2.updateInjection(inj_props);
                update_well = true;
                handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
            }

            if (well2.updateProduction(properties)) {
                update_well = true;
            }

            if (well2.updatePrediction(false)) {
                update_well = true;
            }

            if (well2.updateHasProduced()) {
                update_well = true;
            }

            if (update_well) {
                handlerContext.state().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                handlerContext.state().wells.update( well2 );
                handlerContext.affected_well(well_name);
            }

            // Add Event if well open/shut is requested
            updateOpenShutEvents(handlerContext, well_name);
        }
    }
}

void handleWCONINJE(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);

        const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

        for (const auto& well_name : well_names) {
            handlerContext.updateWellStatus(well_name, status,
                                            handlerContext.keyword.location());

            bool update_well = false;
            auto well2 = handlerContext.state().wells.get( well_name );

            auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
            auto previousInjectorType = injection->injectorType;

            double default_bhp_limit;
            const auto& usys = handlerContext.static_schedule().m_unit_system;
            if (handlerContext.state().bhp_defaults.get().inj_limit) {
                default_bhp_limit = usys.from_si(UnitSystem::measure::pressure,
                                                 *handlerContext.state().bhp_defaults.get().inj_limit);
            } else {
                default_bhp_limit = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                  ParserKeywords::WCONINJE::BHP::defaultValue.get<double>());
                default_bhp_limit = usys.from_si(UnitSystem::measure::pressure,
                                                 default_bhp_limit);
            }

            auto table_nr = record.getItem("VFP_TABLE").get< int >(0);

            if (table_nr != 0) {
                const auto& vfpinj = handlerContext.state().vfpinj;
                if (!vfpinj.has(table_nr)) {
                    std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                    throw OpmInputError(reason, handlerContext.keyword.location());
                }
            }

            injection->handleWCONINJE(record, default_bhp_limit,
                                      well2.isAvailableForGroupControl(), well_name, handlerContext.keyword.location());

            const bool switching_from_producer = well2.isProducer();
            if (well2.updateInjection(injection)) {
                update_well = true;
            }

            if (switching_from_producer) {
                handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
            }

            if (well2.updatePrediction(true)) {
                update_well = true;
            }

            if (well2.updateHasInjected()) {
                update_well = true;
            }

            if (update_well) {
                handlerContext.state().events().addEvent(ScheduleEvents::INJECTION_UPDATE);
                handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                if (previousInjectorType != injection->injectorType) {
                    handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_TYPE_CHANGED);
                }
                handlerContext.state().wells.update( std::move(well2) );
                handlerContext.affected_well(well_name);
            }
             // Add Event if well open/shut is requested
            updateOpenShutEvents(handlerContext, well_name);
            auto udq_active = handlerContext.state().udq_active.get();
            if (injection->updateUDQActive(handlerContext.state().udq.get(), udq_active)) {
                handlerContext.state().udq_active.update( std::move(udq_active) );
            }
        }
    }
}

void handleWCONINJH(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);
        const Well::Status status = WellStatusFromString( record.getItem("STATUS").getTrimmedString(0));

        for (const auto& well_name : well_names) {
            handlerContext.updateWellStatus(well_name, status,
                                            handlerContext.keyword.location());
            bool update_well = false;
            auto well2 = handlerContext.state().wells.get( well_name );
            auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
            auto previousInjectorType = injection->injectorType;

            double default_bhp_limit;
            if (handlerContext.state().bhp_defaults.get().inj_limit) {
                default_bhp_limit = *handlerContext.state().bhp_defaults.get().inj_limit;
            } else {
                default_bhp_limit = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                  6891.2);
            }

            auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
            if (record.getItem("VFP_TABLE").defaultApplied(0)) { // Default 1* use the privious set vfp table
                table_nr = injection->VFPTableNumber;
            }
            if (table_nr != 0) {
                const auto& vfpinj = handlerContext.state().vfpinj;
                if (!vfpinj.has(table_nr)) {
                    std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                    throw OpmInputError(reason, handlerContext.keyword.location());
                }
            }

            injection->handleWCONINJH(record, table_nr, default_bhp_limit,
                                      well2.isProducer(), well_name,
                                      handlerContext.keyword.location());

            const bool switching_from_producer = well2.isProducer();
            if (well2.updateInjection(injection)) {
                update_well = true;
            }

            if (switching_from_producer) {
                handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
            }

            if (well2.updatePrediction(false)) {
                update_well = true;
            }

            if (well2.updateHasInjected()) {
                update_well = true;
            }

            if (update_well) {
                handlerContext.state().events().addEvent( ScheduleEvents::INJECTION_UPDATE );
                handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                if (previousInjectorType != injection->injectorType) {
                    handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_TYPE_CHANGED);
                }
                handlerContext.state().wells.update( std::move(well2) );
                handlerContext.affected_well(well_name);
            }

            // Add Event if well open/shut is requested
            updateOpenShutEvents(handlerContext, well_name);
        }
    }
}

bool belongsToAutoChokeGroup(const Well& well, const ScheduleState& state) {
    const auto& network = state.network.get();
    if (!network.active())
        return false;
    auto group_name = well.groupName();
    while (group_name != "FIELD") {
        if (network.has_node(group_name)) {
            auto node_name = group_name;
            if (network.node(node_name).as_choke())
                return true;
            while (network.uptree_branch(node_name)) {
                node_name = network.uptree_branch(node_name)->uptree_node();
                if (network.node(node_name).as_choke())
                    return true;
            }
        }
        group_name = state.groups.get(group_name).parent();
    }
    return false;
}

void handleWCONPROD(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

        for (const auto& well_name : well_names) {
            bool update_well = handlerContext.updateWellStatus(well_name, status,
                                                               handlerContext.keyword.location());
            std::optional<VFPProdTable::ALQ_TYPE> alq_type;
            auto well2 = handlerContext.state().wells.get( well_name );
            const bool switching_from_injector = !well2.isProducer();
            auto properties = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
            properties->clearControls();
            if (well2.isAvailableForGroupControl() || belongsToAutoChokeGroup(well2, handlerContext.state())) {
                properties->addProductionControl(Well::ProducerCMode::GRUP);
            }

            auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
            if (table_nr != 0) {
                const auto& vfpprod = handlerContext.state().vfpprod;
                if (vfpprod.has(table_nr)) {
                    alq_type = handlerContext.state().vfpprod(table_nr).getALQType();
                } else {
                    std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                    throw OpmInputError(reason, handlerContext.keyword.location());
                }
            }

            double default_bhp_target;
            if (handlerContext.state().bhp_defaults.get().prod_target) {
                default_bhp_target = *handlerContext.state().bhp_defaults.get().prod_target;
            } else {
                default_bhp_target = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                   ParserKeywords::WCONPROD::BHP::defaultValue.get<double>());
            }

            const auto& phases = handlerContext.static_schedule().m_runspec.phases();
            properties->handleWCONPROD(alq_type, table_nr, default_bhp_target,
                                       handlerContext.static_schedule().m_unit_system,
                                       well_name, phases, record, handlerContext.keyword.location());

            if (switching_from_injector) {
                if (properties->bhp_hist_limit_defaulted) {
                    properties->setBHPLimit(default_bhp_target);
                }
                update_well = true;
                handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
            }

            if (well2.updateProduction(properties)) {
                update_well = true;
            }

            if (well2.updatePrediction(true)) {
                update_well = true;
            }

            if (well2.updateHasProduced()) {
                update_well = true;
            }

            if (update_well) {
                handlerContext.state().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                handlerContext.state().wells.update( std::move(well2) );
                handlerContext.affected_well(well_name);
            }

            // Add Event if well open/shut is requested
            updateOpenShutEvents(handlerContext, well_name);

            auto udq_active = handlerContext.state().udq_active.get();
            if (properties->updateUDQActive(handlerContext.state().udq.get(), udq_active)) {
                handlerContext.state().udq_active.update( std::move(udq_active));
            }
        }
    }
}

void handleWCYCLE(HandlerContext& handlerContext)
{
    auto new_config = handlerContext.state().wcycle();
    for (const auto& record : handlerContext.keyword) {
        new_config.addRecord(record);
    }
    handlerContext.state().wcycle.update(std::move(new_config));
}

void handleWELLSTRE(HandlerContext& handlerContext)
{
    auto& inj_streams = handlerContext.state().inj_streams;
    for (const auto& record : handlerContext.keyword) {
        const auto stream_name = record.getItem<ParserKeywords::WELLSTRE::STREAM>().getTrimmedString(0);
        const auto& composition = record.getItem<ParserKeywords::WELLSTRE::COMPOSITIONS>().getSIDoubleData();
        const std::size_t num_comps = handlerContext.static_schedule().m_runspec.numComps();
        if (composition.size() != num_comps) {
            const std::string msg = fmt::format("The number of the composition values for stream '{}' is not the same as the number of components.", stream_name);
            throw OpmInputError(msg, handlerContext.keyword.location());
        }

        const double sum = std::accumulate(composition.begin(), composition.end(), 0.0);
        if (std::abs(sum - 1.0) > std::numeric_limits<double>::epsilon()) {
            const std::string msg = fmt::format("The sum of the composition values for stream '{}' is not 1.0, but {}.", stream_name, sum);
            throw OpmInputError(msg, handlerContext.keyword.location());
        }
        auto composition_ptr = std::make_shared<std::vector<double>>(composition);
        inj_streams.update(stream_name, std::move(composition_ptr));
    }

}

void handleWELOPEN(HandlerContext& handlerContext)
{
    const auto& keyword = handlerContext.keyword;

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
        const auto well_names = handlerContext.wellNames(wellNamePattern);

        /* if all records are defaulted or just the status is set, only
         * well status is updated
         */
        if (conn_defaulted(record)) {
            const auto new_well_status = WellStatusFromString(status_str);
            for (const auto& wname : well_names) {
                const auto did_update_well_status =
                    handlerContext.updateWellStatus(wname, new_well_status);

                handlerContext.affected_well(wname);

                if (did_update_well_status) {
                    handlerContext.record_well_structure_change();
                }

                if (did_update_well_status && (new_well_status == open)) {
                    // Record possible well injection/production status change
                    auto well2 = handlerContext.state().wells.get(wname);

                    const auto did_flow_update =
                        (well2.isProducer() && well2.updateHasProduced())
                        ||
                        (well2.isInjector() && well2.updateHasInjected());

                    if (did_flow_update) {
                        handlerContext.state().wells.update(std::move(well2));
                    }
                }
                // Add Event if well open/shut is requested
                updateOpenShutEvents(handlerContext, wname);
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
            const auto connection_status = Connection::StateFromString( status_str );
            {
                auto well = handlerContext.state().wells.get(wname);
                well.handleWELOPENConnections(record, connection_status);
                handlerContext.state().wells.update( std::move(well) );
            }

            handlerContext.affected_well(wname);
            handlerContext.record_well_structure_change();

            handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
        }
    }
}

void handleWINJGAS(HandlerContext& handlerContext)
{
    // \Note: we do not support the item 4 MAKEUPGAS and item 5 STAGE in WINJGAS keyword yet
    for (const auto& record : handlerContext.keyword) {
        const std::string fluid_nature = record.getItem<ParserKeywords::WINJGAS::FLUID>().getTrimmedString(0);

        // \Note: technically, only the first two characters are significant
        // with some testing, we can determine whether we want to enforce this.
        // at the moment, we only support full string STREAM for fluid nature
        if (fluid_nature != "STREAM") {
            const std::string msg = fmt::format("The fluid nature '{}' is not supported in WINJGAS keyword.", fluid_nature);
            throw OpmInputError(msg, handlerContext.keyword.location());
        }

        const std::string stream_name = record.getItem<ParserKeywords::WINJGAS::STREAM>().getTrimmedString(0);
        // we make sure the stream is defined in WELLSTRE keyword
        const auto& inj_streams = handlerContext.state().inj_streams;
        if (!inj_streams.has(stream_name)) {
            const std::string msg = fmt::format("The stream '{}' is not defined in WELLSTRE keyword.", stream_name);
            throw OpmInputError(msg, handlerContext.keyword.location());
        }

        const std::string wellNamePattern = record.getItem<ParserKeywords::WINJGAS::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);
        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells.get(well_name);
            auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());

            const auto& inj_stream = inj_streams.get(stream_name);
            injection->setGasInjComposition(inj_stream);

            if (well2.updateInjection(injection)) {
                handlerContext.state().wells.update(std::move(well2));
            }
        }
    }
}

void handleWELSPECS(HandlerContext& handlerContext)
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
        const auto existingWells = handlerContext.wellNames(wellName, allowEmptyWellList);

        if (groupName == "FIELD") {
            if (existingWells.empty()) {
                fieldWells.push_back(wellName);
            }
            else {
                fieldWells.insert(fieldWells.end(), existingWells.begin(), existingWells.end());
            }
        }

        if (! handlerContext.state().groups.has(groupName)) {
            handlerContext.addGroup(groupName);
        }

        if (existingWells.empty()) {
            // 'wellName' does not match any existing wells.  Create a
            // new Well object for this well.
            handlerContext.welspecsCreateNewWell(record,
                                                 wellName,
                                                 groupName);
        }
        else {
            // 'wellName' matches one or more existing wells.  Assign
            // new properties for those wells.
            handlerContext.welspecsUpdateExistingWells(record,
                                                       existingWells,
                                                       groupName);
        }
    }

    if (! fieldWells.empty()) {
        std::ranges::sort(fieldWells);
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


void handleWELSPECL(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::WELSPECL;

    auto getTrimmedName = [&handlerContext](const auto& item)
    {
        return trim_wgname(handlerContext.keyword,
                           item.template get<std::string>(0),
                           handlerContext.parseContext,
                           handlerContext.errors);
    };
    handleWELSPECS(handlerContext);
    std::size_t index = 0;
    std::unordered_map<std::string, int> lgr_well_seq_map;

    for (const auto& record : handlerContext.keyword) {
        const auto wellName = getTrimmedName(record.getItem<Kw::WELL>());
        const auto lgrTag = getTrimmedName(record.getItem<Kw::LGR>());
        const auto& [tagPos, inserted] = lgr_well_seq_map.try_emplace(lgrTag, 0);
        if (! inserted) {
            // lgrTag already exists in the map, increase sequence number.
            ++tagPos->second;
        }
        auto& well = handlerContext.state().wells.get(wellName);
        well.setInsertIndexLGR(tagPos->second);
        well.setInsertIndexAllLGR(index);
        well.flag_lgr_well();
        well.set_lgr_well_tag(lgrTag);
        index++;
    }
}

// The documentation for the WELTARG keyword says that the well must have
// been fully specified and initialized using one of the WCONxxxx keywords
// prior to modifying the well using the WELTARG keyword.
//
// The following implementation of handling the WELTARG keyword does not
// check or enforce in any way that this is done (i.e., it is not checked or
// verified that the well is initialized with any WCONxxxx keyword).
//
// Update: See the discussion following the definitions of the SI factors,
// due to a bad design we currently need the well to be specified with
// WCONPROD / WCONHIST before WELTARG is applied.  Otherwise the units for
// the rates will be wrong.

void handleWELTARG(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::WELTARG;

    const auto SiFactorP = handlerContext.static_schedule()
        .m_unit_system.parse("Pressure").getSIScaling();

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<Kw::WELL>().getTrimmedString(0);

        const auto is_wlist = handlerContext.state()
            .wlist_manager().hasList(wellNamePattern);

        const auto well_names = handlerContext.wellNames(wellNamePattern, is_wlist);

        if (well_names.empty()) {
            if (is_wlist) {
                // wellNamePattern names an empty well list.  This is okay,
                // so issue a warning and continue.
                const auto msg_format =
                    fmt::format("Empty WLIST '{}' in '{{keyword}}', "
                                "in {{file}} line {{line}}.", wellNamePattern);

                const auto msg = OpmInputError::format
                    (msg_format, handlerContext.keyword.location());

                OpmLog::warning("WELTARG:EmptyWLIST", msg);
                continue;
            }
            else {
                handlerContext.invalidNamePattern(wellNamePattern);
            }
        }

        const auto cmode = WellWELTARGCModeFromString(record.getItem<Kw::CMODE>().getTrimmedString(0));
        const auto new_arg = record.getItem<Kw::NEW_VALUE>().get<UDAValue>(0);

        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells.get(well_name);
            bool update = false;
            if (well2.isProducer()) {
                auto prop = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                prop->handleWELTARG(cmode, new_arg, SiFactorP);
                update = well2.updateProduction(prop);
                if (cmode == Well::WELTARGCMode::GUID) {
                    update |= well2.updateWellGuideRate(new_arg.get<double>());
                }

                auto udq_active = handlerContext.state().udq_active.get();
                if (prop->updateUDQActive(handlerContext.state().udq.get(), cmode, udq_active)) {
                    handlerContext.state().udq_active.update( std::move(udq_active));
                }
            }
            else {
                auto inj = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                inj->handleWELTARG(cmode, new_arg, SiFactorP);
                update = well2.updateInjection(inj);
                if (cmode == Well::WELTARGCMode::GUID) {
                    update |= well2.updateWellGuideRate(new_arg.get<double>());
                }

                auto udq_active = handlerContext.state().udq_active.get();
                if (inj->updateUDQActive(handlerContext.state().udq.get(), cmode, udq_active)) {
                    handlerContext.state().udq_active.update(std::move(udq_active));
                }
            }

            if (update) {
                if (well2.isProducer()) {
                    handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::PRODUCTION_UPDATE);
                    handlerContext.state().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                } else {
                    handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                    handlerContext.state().events().addEvent( ScheduleEvents::INJECTION_UPDATE );
                }
                handlerContext.state().wells.update( std::move(well2) );
                handlerContext.affected_well(well_name);
            }
        }
    }
}


void handleWHISTCTL(HandlerContext& handlerContext)
{
    const auto& record = handlerContext.keyword.getRecord(0);
    const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
    const auto controlMode = WellProducerCModeFromString(cmodeString);

    if (controlMode != Well::ProducerCMode::NONE && !Well::WellProductionProperties::effectiveHistoryProductionControl(controlMode) ) {
        std::string msg = "The WHISTCTL keyword specifies an un-supported control mode " + cmodeString
            + ", which makes WHISTCTL keyword not affect the simulation at all";
        OpmLog::warning(msg);
    }
    handlerContext.state().update_whistctl( controlMode );

    const std::string bhp_terminate = record.getItem("BPH_TERMINATE").getTrimmedString(0);
    if (bhp_terminate == "YES") {
        std::string msg_fmt = "Problem with {keyword}\n"
                              "In {file} line {line}\n"
                              "Setting item 2 in {keyword} to 'YES' to stop the run is not supported";
        handlerContext.parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg_fmt, handlerContext.keyword.location(), handlerContext.errors );
    }

    for (const auto& well_ref : handlerContext.state().wells()) {
        auto well2 = well_ref.get();
        auto prop = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());

        if (prop->whistctl_cmode != controlMode) {
            prop->whistctl_cmode = controlMode;
            well2.updateProduction(prop);
            handlerContext.state().wells.update( std::move(well2) );
        }
    }
}

// ---------------------------------------------------------------------------

/// Intermediate layer for canonicalising well list inputs and applying the
/// resulting well list operation.
class WListOperation
{
public:
    /// Constructor.
    ///
    /// \param[in] handlerContext Schedule state and ancillary information
    /// at the current time point.
    explicit WListOperation(HandlerContext& handlerContext)
        : hctx_ { std::ref(handlerContext) }
    {}

    /// Internalise well list name, operation, and well name data for a
    /// single WLIST keyword record.
    ///
    /// Throws an exception if any of the input data is incorrect and cannot
    /// be properly interpreted in context.
    ///
    /// \param[in] record Single record from a WLIST keyword.
    void parse(const DeckRecord& record);

    /// Apply previously parsed input data to the current set of well lists.
    ///
    /// Depending on the operation, this function will create a new well
    /// list with an initial set of wells (NEW), add a set of wells to an
    /// existing (or new) well list (ADD), move a set of wells to an
    /// existing (or new) well list (MOV), or remove a set of wells from an
    /// existing well list (DEL).
    ///
    /// \param[in,out] wlm Run's currently known well lists.  On exit,
    /// updated according to the specification previously internalised in
    /// parse().
    void apply(WListManager& wlm);

    /// Whether or not any well lists have changed as a result of the
    /// current set of operations.
    bool wellListsChanged() const
    {
        return this->well_lists_changed_;
    }

private:
    // Note to maintainers: If you change the 'Action' enumeration, then you
    // must also update the apply() member function accordingly.

    /// Well list operation.
    enum class Action : std::size_t {
        /// Create a new well list (NEW).
        New,

        /// Add wells to an existing or new well list (ADD).
        Add,

        /// Remove a set of wells from an existing well list (DEL).
        Del,

        /// Move all specified wells onto an existing or new well list (MOV).
        Mov,

        /// Parse error.
        Invalid,
    };

    /// Schedule state and ancillary dynamic information at the current time
    /// point.
    std::reference_wrapper<HandlerContext> hctx_;

    /// Name of well list that is currently being manipulated.
    std::string wlist_name_{};

    /// Current well list operation.
    Action action_{ Action::Invalid };

    /// Set of wells involved in current operation.
    std::vector<std::string> wells_{};

    /// Whether or not any well lists changed as a result of the current set
    /// of operations.
    bool well_lists_changed_{};

    /// Internalise well list name.
    ///
    /// Updates \c wlist_name_.  Will throw an exception if the well list
    /// item does not name a well list--i.e., if the name in the well list
    /// item does not begin with a leading asterisk.
    ///
    /// \param[in] record Single record from a WLIST keyword.
    void parseWListName(const DeckRecord& record);

    /// Internalise well list operation.
    ///
    /// Updates \c action_.  Will throw an exception if the well list
    /// operation item is not a known operation--i.e., if the string in the
    /// well list operation item is not among the set supported by Action.
    ///
    /// \param[in] record Single record from a WLIST keyword.
    void parseWListAction(const DeckRecord& record);

    /// Internalise collection of wells
    ///
    /// Updates \c wells_.  Will throw an exception if any of the named
    /// wells have not been previously defined.
    ///
    /// \param[in] record Single record from a WLIST keyword.
    void parseWListWells(const DeckRecord& record);

    /// Create a new well list.
    ///
    /// Implements the NEW operation.
    ///
    /// \param[in,out] wlm Run's currently known well lists.  On exit, will
    /// have a new, or reset, well list by the name of wlist_name_,
    /// containing the wells in wells_.
    void newList(WListManager& wlm);

    /// Add wells to new or existing well list.
    ///
    /// Implements the ADD operation.
    ///
    /// \param[in,out] wlm Run's currently known well lists.  On exit, the
    /// well list named in wlist_name_ will contain at least the wells named
    /// in wells_.
    void add(WListManager& wlm);

    /// Move wells to new or existing well list.
    ///
    /// Implements the MOV operation.
    ///
    /// \param[in,out] wlm Run's currently known well lists.  On exit, the
    /// well list named in wlist_name_ will contain at least the wells named
    /// in wells_.  Moreover, those wells will no longer be included in any
    /// other well list known to \p wlm.
    void move(WListManager& wlm);

    /// Remove wells from existing well list.
    ///
    /// Implements the DEL operation.
    ///
    /// Will throw an exception if the well list named in well_list_ does
    /// not exist.
    ///
    /// \param[in,out] wlm Run's currently known well lists.  On exit, the
    /// wells named in wells_ will no longer be included in the well list
    /// named in well_list_.
    void del(WListManager& wlm);

    /// Record that at least one well list changed as a result of the
    /// current operation.
    void recordWellListChange();

    /// Report a parsing error through the run's parse context/error guard
    ///
    /// Includes the normal keyword and location information.
    ///
    /// \param[in] msg Specific message pertaining to a particular context
    /// such as an invalid well or well list name.
    void errorInvalidName(std::string_view msg) const;
};

void WListOperation::parse(const DeckRecord& record)
{
    this->parseWListName(record);
    this->parseWListAction(record);
    this->parseWListWells(record);
}

void WListOperation::apply(WListManager& wlm)
{
    // Note: Member function order must match Action enumerator order.
    const auto op = std::array {
        &WListOperation::newList,
        &WListOperation::add,
        &WListOperation::del,
        &WListOperation::move,
    }[ static_cast<std::underlying_type_t<Action>>(this->action_) ];

    (this->*op)(wlm);
}

void WListOperation::parseWListName(const DeckRecord& record)
{
    this->wlist_name_ = record
        .getItem<ParserKeywords::WLIST::NAME>()
        .getTrimmedString(0);

    if (this->wlist_name_.empty() || (this->wlist_name_.front() != '*')) {
        this->errorInvalidName
            (fmt::format("Well list name '{}' does not "
                         "have a leading asterisk ('*')",
                         record.getItem<ParserKeywords::WLIST::NAME>()
                         .get<std::string>(0)));
    }
}

void WListOperation::parseWListAction(const DeckRecord& record)
{
    this->action_ = Action::Invalid;

    const auto action = record
        .getItem<ParserKeywords::WLIST::ACTION>()
        .getTrimmedString(0);

    if      (action == "NEW") { this->action_ = Action::New; }
    else if (action == "ADD") { this->action_ = Action::Add; }
    else if (action == "DEL") { this->action_ = Action::Del; }
    else if (action == "MOV") { this->action_ = Action::Mov; }

    if (this->action_ == Action::Invalid) {
        throw OpmInputError {
            fmt::format(R"(Problem with {{keyword}}
In {{file}} line {{line}}
"Action '{}' is not recognized.)", action),
            this->hctx_.get().keyword.location()
        };
    }
}

void WListOperation::parseWListWells(const DeckRecord& record)
{
    this->wells_.clear();

    const auto& well_args = record
        .getItem<ParserKeywords::WLIST::WELLS>()
        .getData<std::string>();

    for (const auto& well_arg : well_args) {
        // Does not use overload for context to avoid throw
        const auto well_names = this->hctx_.get().wellNames(well_arg, true);

        if (well_names.empty() && (well_arg.find("*") == std::string::npos)) {
            this->errorInvalidName(fmt::format("Well '{}' has not been defined "
                                               "with WELSPECS and will not be "
                                               "added to the list.", well_arg));

            continue;
        }

        this->wells_.insert(this->wells_.end(),
                            well_names.begin(),
                            well_names.end());
    }
}

void WListOperation::newList(WListManager& wlm)
{
    wlm.newList(this->wlist_name_, this->wells_);

    this->recordWellListChange();
}

void WListOperation::add(WListManager& wlm)
{
    wlm.addOrCreateWellList(this->wlist_name_, this->wells_);

    this->recordWellListChange();
}

void WListOperation::move(WListManager& wlm)
{
    for (const auto& well : this->wells_) {
        wlm.delWell(well);
    }

    this->add(wlm);
}

void WListOperation::del(WListManager& wlm)
{
    if (! wlm.hasList(this->wlist_name_)) {
        this->errorInvalidName(fmt::format("Well list '{}' is unknown "
                                           "and cannot be used in DEL "
                                           "operation.", this->wlist_name_));

        return;
    }

    for (const auto& well : this->wells_) {
        if (wlm.delWListWell(well, this->wlist_name_)) {
            this->recordWellListChange();
        }
    }
}

void WListOperation::recordWellListChange()
{
    this->well_lists_changed_ = true;
}

void WListOperation::errorInvalidName(std::string_view message) const
{
    const auto msg_fmt = fmt::format(R"(Problem with {{keyword}}
In {{file}} line {{line}}
{})", message);

    this->hctx_.get().parseContext
        .handleError(ParseContext::SCHEDULE_INVALID_NAME,
                     msg_fmt,
                     this->hctx_.get().keyword.location(),
                     this->hctx_.get().errors);
}

void handleWLIST(HandlerContext& handlerContext)
{
    auto wlistOperation = WListOperation { handlerContext };

    for (const auto& record : handlerContext.keyword) {
        // Will throw an exception if input is unexpected.
        wlistOperation.parse(record);

        // If we get here, then the input data is meaningful and we can
        // proceed to apply the operation.
        //
        // Note: We need an independent WListManager for each record to
        // handle the case that subsequent records are influenced by the
        // operation in earlier records.
        auto wlm = handlerContext.state().wlist_manager();

        wlistOperation.apply(wlm);

        handlerContext.state().wlist_manager.update(std::move(wlm));
    }

    if (! wlistOperation.wellListsChanged()) {
        return;
    }

    auto tracker = handlerContext.state().wlist_tracker();

    if (handlerContext.action_mode) {
        tracker.recordActionChangedLists();
    }
    else {
        tracker.recordStaticChangedLists();
    }

    handlerContext.state().wlist_tracker.update(std::move(tracker));
}

// ---------------------------------------------------------------------------

void handleWPAVE(HandlerContext& handlerContext)
{
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

    for (const auto& wname : handlerContext.state().well_order()) {
        const auto& well = handlerContext.state().wells.get(wname);
        if (well.pavg() != wpave) {
            auto new_well = handlerContext.state().wells.get(wname);
            new_well.updateWPAVE(wpave);
            handlerContext.state().wells.update(std::move(new_well));
        }
    }

    handlerContext.state().pavg.update(std::move(wpave));
}

void handleWPAVEDEP(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem<ParserKeywords::WPAVEDEP::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        const auto& item = record.getItem<ParserKeywords::WPAVEDEP::REFDEPTH>();
        if (item.hasValue(0)) {
            auto ref_depth = item.getSIDouble(0);
            for (const auto& well_name : well_names) {
                auto well = handlerContext.state().wells.get(well_name);
                well.updateWPaveRefDepth( ref_depth );
                handlerContext.state().wells.update( std::move(well) );
            }
        }
    }
}

/// Handler function for well fracturing seeds
///
/// Keyword structure:
///
///   WSEED
///     WellName  I  J  K  nx  ny  nz  ev  eh  wd /
///     WellName  I  J  K  nx  ny  nz  ev  eh  wd /
///     WellName  I  J  K  nx  ny  nz  ev  eh  wd /
///   /
///
/// in which 'WellName' is a well, well list, well template or well list
/// template.  I,J,K are regular well connection coordinates, nx,ny,nz are
/// the components of the fracturing plane's normal vector, and ev,eh,wd are
/// the vertical and horizontal extents along with the initial fracture
/// width at the seed point.
void handleWSEED(HandlerContext& handlerContext)
{
    const auto* grid = handlerContext.grid.get_grid();
    if (grid == nullptr) {
        return;
    }

    auto& seeds = handlerContext.state().wseed;
    auto updated_seed_wells = std::unordered_set<std::string>{};

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<ParserKeywords::WSEED::WELL>()
            .getTrimmedString(0);

        const auto well_names = handlerContext.wellNames(wellNamePattern, false);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
            continue;
        }

        // Subtract one to convert one-based input indices to zero-based
        // internal processing indices.
        const auto cellSeedIndex = grid->getGlobalIndex
            (record.getItem<ParserKeywords::WSEED::I>().get<int>(0) - 1,
             record.getItem<ParserKeywords::WSEED::J>().get<int>(0) - 1,
             record.getItem<ParserKeywords::WSEED::K>().get<int>(0) - 1);

        const auto cellSeedNormal = WellFractureSeeds::NormalVector {
            record.getItem<ParserKeywords::WSEED::NORMAL_X>().getSIDouble(0),
            record.getItem<ParserKeywords::WSEED::NORMAL_Y>().getSIDouble(0),
            record.getItem<ParserKeywords::WSEED::NORMAL_Z>().getSIDouble(0),
        };

        const auto cellSeedSize = WellFractureSeeds::SeedSize {}
            .verticalExtent  (record.getItem<ParserKeywords::WSEED::SIZE_Z>().getSIDouble(0))
            .horizontalExtent(record.getItem<ParserKeywords::WSEED::SIZE_H>().getSIDouble(0))
            .width           (record.getItem<ParserKeywords::WSEED::WIDTH> ().getSIDouble(0));

        for (const auto& well_name : well_names) {
            const auto hasConn = handlerContext.state()
                .wells(well_name)
                .getConnections()
                .hasGlobalIndex(cellSeedIndex);

            if (! hasConn) { continue; }

            auto seed = seeds.has(well_name)
                ? std::make_shared<WellFractureSeeds>(seeds(well_name))
                : std::make_shared<WellFractureSeeds>(well_name);

            if (seed->updateSeed(cellSeedIndex, cellSeedNormal, cellSeedSize)) {
                updated_seed_wells.insert(well_name);
                seeds.update(well_name, std::move(seed));
            }
        }
    }

    for (const auto& updated_seed_well : updated_seed_wells) {
        seeds.get(updated_seed_well).finalizeSeeds();
    }
}

void handleWTEST(HandlerContext& handlerContext)
{
    auto new_config = handlerContext.state().wtest_config.get();
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

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
    handlerContext.state().wtest_config.update( std::move(new_config) );
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getWellHandlers()
{
    return {
        { "WCONHIST", &handleWCONHIST },
        { "WCONINJE", &handleWCONINJE },
        { "WCONINJH", &handleWCONINJH },
        { "WCONPROD", &handleWCONPROD },
        { "WCYCLE",   &handleWCYCLE   },
        { "WELOPEN" , &handleWELOPEN  },
        { "WELLSTRE", &handleWELLSTRE },
        { "WELSPECS", &handleWELSPECS },
        { "WELSPECL", &handleWELSPECL },
        { "WELTARG" , &handleWELTARG  },
        { "WHISTCTL", &handleWHISTCTL },
        { "WINJGAS",  &handleWINJGAS  },
        { "WLIST"   , &handleWLIST    },
        { "WPAVE"   , &handleWPAVE    },
        { "WPAVEDEP", &handleWPAVEDEP },
        { "WSEED"   , &handleWSEED    },
        { "WTEST"   , &handleWTEST    },
    };
}

}
