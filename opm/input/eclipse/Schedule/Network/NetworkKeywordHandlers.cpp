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

#include "NetworkKeywordHandlers.hpp"

#include "../HandlerContext.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>

#include <opm/input/eclipse/Schedule/Network/Balance.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <fmt/format.h>

namespace Opm {

namespace {

void handleBRANPROP(HandlerContext& handlerContext)
{
    auto ext_network = handlerContext.state().network.get();
    if (ext_network.active() && ext_network.is_standard_network()) {
        std::string msg = "Cannot have standard and extended network defined simultaneously.";
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    ext_network.set_standard_network(false);
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

    handlerContext.state().network.update( std::move( ext_network ));
}

void handleGRUPNET(HandlerContext& handlerContext)
{
    auto network = handlerContext.state().network.get();
    if (network.active() && !network.is_standard_network()) {
        std::string msg = "Cannot have standard and extended network defined simultaneously.";
        throw OpmInputError(msg, handlerContext.keyword.location());
    }
    network.set_standard_network(true);

    const std::string info_msg = {
        "\n"
        " PLEASE NOTE:\n"
        "   Flow writes restart data for standard network in extended network format.\n"
        "   Restarting other simulators from Flow output requires conversion to extended network.\n"
    };
    OpmLog::info(info_msg);

    std::vector<Network::Node> nodes;
    for (const auto& record : handlerContext.keyword) {
         const std::string& groupNamePattern = record.getItem<ParserKeywords::GRUPNET::NAME>().getTrimmedString(0);
         const auto group_names = handlerContext.groupNames(groupNamePattern);
         if (group_names.empty()) {
             handlerContext.invalidNamePattern(groupNamePattern);
         }
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
              const auto& group = handlerContext.state().groups.get(group_name);
              const std::string& downtree_node = group_name;
              const std::string& uptree_node = group.parent();
              Network::Node node { group_name };
              node.add_gas_lift_gas(add_gas_lift_gas);
              // A terminal node is a node with a fixed pressure
              const bool is_terminal_node = pressure_item.hasValue(0) && (pressure_item.get<double>(0) >= 0);
              if (is_terminal_node) {
                  if (vfp_table > 0) {
                      std::string msg = fmt::format("The group {} is a terminal node of the network and should not have a vfp table assigned to it. This vfp table will be ignored.", group_name);
                      OpmLog::warning(OpmInputError::format(msg, handlerContext.keyword.location()));
                  }
                  node.terminal_pressure(pressure_item.getSIDouble(0));
                  nodes.push_back(node);
                  // Need to add the flow further up the network in case of other fixed-pressure nodes
                  if (!uptree_node.empty()) {
                    network.add_or_replace_branch(Network::Branch(downtree_node, uptree_node, 9999, 0.0));
                  }
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
    for(const auto& node: nodes) {
          network.update_node(node);
    }

    handlerContext.state().network.update( std::move(network));
}

void handleNEFAC(HandlerContext& handlerContext)
{
    auto ext_network = handlerContext.state().network.get();
    if (!ext_network.active())
        return;
    if (ext_network.is_standard_network()) {
        const std::string& msg = "NEFAC has no effect for a standard network: file {filename} line {lineno}";
        OpmLog::warning(handlerContext.keyword.location().format(msg));
        return;
    }

    bool updated = false;
    for (const auto& record : handlerContext.keyword) {
        const auto& node_name = record.getItem<ParserKeywords::NEFAC::NODE>().get<std::string>(0);
        const auto efficiency = record.getItem<ParserKeywords::NEFAC::EFF_FACTOR>().getSIDouble(0);

        if (ext_network.has_node(node_name)) {
            auto node = ext_network.node(node_name);
            if (node.efficiency() != efficiency) {
                node.set_efficiency(efficiency);
                ext_network.update_node(node);
                updated = true;
            }
        }
    }

    if (updated)
        handlerContext.state().network.update( std::move(ext_network) );
}

void handleNETBALAN(HandlerContext& handlerContext)
{
    handlerContext.state().network_balance
        .update(Network::Balance{ handlerContext.keyword });
}

void handleNODEPROP(HandlerContext& handlerContext)
{
    auto ext_network = handlerContext.state().network.get();
    if (ext_network.active() && ext_network.is_standard_network()) {
        std::string msg = "Cannot have standard and extended network defined simultaneously.";
        throw OpmInputError(msg, handlerContext.keyword.location());
    }

    for (const auto& record : handlerContext.keyword) {
        const auto& name = record.getItem<ParserKeywords::NODEPROP::NAME>().get<std::string>(0);
        const auto& pressure_item = record.getItem<ParserKeywords::NODEPROP::PRESSURE>();

        const bool as_choke = DeckItem::to_bool(record.getItem<ParserKeywords::NODEPROP::AS_CHOKE>().get<std::string>(0));
        const bool add_gas_lift_gas = DeckItem::to_bool(record.getItem<ParserKeywords::NODEPROP::ADD_GAS_LIFT_GAS>().get<std::string>(0));

        Network::Node node { name };

        if (pressure_item.hasValue(0) && (pressure_item.get<double>(0) > 0)) {
            node.terminal_pressure(pressure_item.getSIDouble(0));
        }

        if (handlerContext.state().groups.has(name)) {
            auto& group = handlerContext.state().groups.get(name);
            node.set_efficiency(group.getGroupEfficiencyFactor(/*network*/ true));

            if (as_choke) {
                group.as_choke(name);
                if (group.wellgroup()) {
                    // Wells belong to a group with autochoke enabled are to be run on a common THP and should not have guide rates
                    for (const std::string& wellName : group.wells()) {
                        auto well = handlerContext.state().wells.get(wellName);

                        // Let the wells be operating on a THP Constraint
                        auto properties = std::make_shared<Well::WellProductionProperties>(well.getProductionProperties());
                        // The wells are not to be under GRUP control using guide rates but under THP control
                        properties->addProductionControl(Well::ProducerCMode::THP);
                        properties->controlMode = Well::ProducerCMode::THP;
                        well.updateProduction(properties);

                        // Guide rate availability should be set to false
                        well.updateAvailableForGroupControl(false);
                        auto new_config = handlerContext.state().guide_rate();
                        new_config.update_well(well);
                        handlerContext.state().guide_rate.update( std::move(new_config) );
                        handlerContext.state().wells.update( std::move(well) );
                    }
                    std::string target_group = name;
                    const auto& target_item = record.getItem<ParserKeywords::NODEPROP::CHOKE_GROUP>();

                    if (target_item.hasValue(0)) {
                        target_group = target_item.get<std::string>(0);
                    }
                    if (target_group != name) {
                        const std::string msg = "A manifold group must respond to its own target.";
                        throw OpmInputError(msg, handlerContext.keyword.location());
                    }
                    node.as_choke(target_group);
                }
                else {
                    std::string msg = "The auto-choke option is implemented only for well groups.";
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }
            }
        }

        node.add_gas_lift_gas(add_gas_lift_gas);
        ext_network.update_node(node);
    }

    handlerContext.state().network.update( ext_network );
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getNetworkHandlers()
{
    return {
        { "BRANPROP", &handleBRANPROP },
        { "GRUPNET",  &handleGRUPNET  },
        { "NEFAC",    &handleNEFAC   },
        { "NETBALAN", &handleNETBALAN },
        { "NODEPROP", &handleNODEPROP },
    };
}

}
