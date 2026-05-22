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

#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/Network/Balance.hpp>
#include <opm/input/eclipse/Schedule/Network/Branch.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/Network/Node.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {
    // =======================================================================
    // Utilities for GNETINJE processing
    // =======================================================================

    /// Report unsupported injection phase in GNETINJE.
    ///
    /// Records a SCHEDULE_INVALID_INJPHASE condition through the normal
    /// reporting protocol.
    ///
    /// \param[in] record GNETINJE record with an unsupported injection
    /// phase.
    ///
    /// \param[in,out] handlerContext Context for the current report step's
    /// parsing operation.
    void rejectInvalidGNetInjePhase(const Opm::DeckRecord& record,
                                    Opm::HandlerContext&   handlerContext)
    {
        using Kw = Opm::ParserKeywords::GNETINJE;

        const auto& phaseItem = record.getItem<Kw::PHASE>();

        const auto phase = phaseItem.defaultApplied(0)
            ? std::string { "<defaulted>" }
            : phaseItem.getTrimmedString(0);

        const auto msg_fmt = fmt::format(R"(Problem with {{keyword}}
In {{file}} line {{line}}
Injection phase '{}' is not permitted.)", phase);

        handlerContext.parseContext
            .handleError(Opm::ParseContext::SCHEDULE_INVALID_INJPHASE,
                         msg_fmt, handlerContext.keyword.location(),
                         handlerContext.errors);
    }

    /// Extract SI unit terminal pressure from GNETINJE's terminal pressure item.
    ///
    /// \param[in] termPress Terminal pressure item from GNETINJE record.
    ///
    /// \return Terminal pressure converted to SI units (Pascal) if
    /// available, nullopt otherwise--e.g., when the terminal pressure is
    /// defaulted or has a negative value.
    std::optional<double>
    parseGNetInjeTerminalPressure(const Opm::DeckItem& termPress)
    {
        if (termPress.defaultApplied(0) || (termPress.get<double>(0) < 0.0)) {
            // Defaulted or negative terminal pressure => not a terminal pressure node.
            return {};
        }

        return { termPress.getSIDouble(0) };
    }

    /// Expanded and internalised but mostly unprocessed GNETINJE data
    ///
    /// Terminal pressures in SI units and parent nodes inferred from group
    /// tree.
    class GNetInjeRaw
    {
    public:
        /// Raw data for a single node in an injection network.
        ///
        /// Node name used as primary lookup key and therefore store
        /// separately.
        struct Node
        {
            /// Node's parent--i.e., parent group.
            ///
            /// Nullopt for fixed-pressure terminal nodes.
            std::optional<std::string> parent{};

            /// Node's fixed terminal pressure (SI units).
            ///
            /// Nullopt if this node is *not* a fixed-pressure terminal
            /// node.
            std::optional<double> termPress{};

            /// VFP table (keyword VFPINJ) associated with flow line between
            /// this node and its \c parent.
            ///
            /// Nullopt for fixed-pressure terminal nodes.  Special value
            /// 9999 to signify a flow line with no pressure loss.
            /// Otherwise, a positive integer identifying an existing VFP
            /// table.
            std::optional<int> vfpTable{};
        };

        /// Access representation of named injection network node.
        ///
        /// Creates node on first access.  Otherwise, returns existing node.
        ///
        /// \param[in] name Injection network node name (a group name).
        ///
        /// \return Mutable injection network node object.
        Node& node(const std::string& name)
        {
            return this->nodes_.try_emplace(name).first->second;
        }

        /// Beginning of range of injection network nodes.
        ///
        /// Mostly to support range-for and other range-based algorithms.
        ///
        /// \return Beginning of injection network node range.
        auto begin() const { return this->nodes_.begin(); }

        /// End of range of injection network nodes.
        ///
        /// Mostly to support range-for and other range-based algorithms.
        ///
        /// \return One past the end of the injection network node range.
        auto end() const { return this->nodes_.end(); }

        /// Query for an empty collection.
        ///
        /// \return Whether or not the current collection is empty.
        auto empty() const { return this->nodes_.empty(); }

    private:
        /// Collection of injection network nodes.
        std::map<std::string, Node> nodes_{};
    };

    /// Facility for internalising all records of a single GNETINJE keyword.
    ///
    /// We create sequences of raw keyword nodes, split by the injecting
    /// phase and expanded so that there is one node for each group that
    /// matches one of the records.
    class CollectGNetInje
    {
    public:
        /// \param[in,out] handlerContext Context for the current report
        /// step's parsing operation.  Mutated in the case of parse failure,
        /// but is otherwise unchanged by this class.
        explicit CollectGNetInje(Opm::HandlerContext& handlerContext)
            : handlerContext_ { handlerContext }
        {}

        /// Parse GNETINJE keyword into sequence of injection network nodes
        /// split by injecting phase.
        ///
        /// \return Sequence of pairs for which \code .first \endcode is the
        /// injecting phase and \code .second \endcode is the corresponding
        /// raw injection network nodes.
        auto operator()()
        {
            auto nodes = std::array {
                std::pair { Opm::Phase::GAS  , GNetInjeRaw{} },
                std::pair { Opm::Phase::WATER, GNetInjeRaw{} },
            };

            // The unsupported phase must be outside the set of valid keys
            // in 'nodes'.
            this->unsupportedPhase_ = Opm::Phase::OIL;

            this->phaseIndexMap_.clear();
            std::ranges::for_each(nodes, [ix = std::size_t{0}, this]
                                  (const auto& node) mutable
            {
                this->phaseIndexMap_.insert_or_assign(node.first, ix++);
            });

            std::ranges::for_each(this->handlerContext_.keyword,
                                  [&nodes, this](const auto& record)
            {
                this->parseRecord(record);

                if (this->vfpTable_.has_value() &&
                    this->phaseIx_.has_value() &&
                    !this->gnames_.empty())
                {
                    // Valid record; meaning the injection phase is
                    // supported (gas or water), we have a consistent VFP
                    // table/terminal pressure setting, and the record
                    // matches at least one existing group name.
                    //
                    // Create raw nodes for each of the matching groups.
                    this->createNodes(nodes[*this->phaseIx_].second);
                }
            });

            return nodes;
        }

    private:
        /// Context for the current report step's parsing operation.
        ///
        /// Mutated in the case of parse failure, but is otherwise unchanged
        /// by this class.
        Opm::HandlerContext& handlerContext_;

        /// Sequence index for the current record's injecting phase.
        ///
        /// Nullopt in case of unsupported injecting phase--anything other
        /// than gas or water.
        std::optional<std::size_t> phaseIx_{};

        /// Terminal pressure (SI units) for current record.
        ///
        /// Nullopt if the current record does not represent a
        /// fixed-pressure terminal node.
        std::optional<double> termPress_{};

        /// Index of current record's injection flow line table (VFP table).
        ///
        /// Nullopt in the case of parse failure, zero if the current record
        /// represents a fixed-pressure terminal node.
        std::optional<int> vfpTable_{};

        /// Group names matching the current record's name root.
        ///
        /// Empty if there are no matching groups.  In that case, the record
        /// is ignored.
        std::vector<std::string> gnames_{};

        /// Bookkeeping structure for translating phases to sequence
        /// indices.
        std::map<Opm::Phase, std::size_t> phaseIndexMap_{};

        /// Sentinel value for phase parsing failure.
        ///
        /// Must be something other than the supported 'GAS' and 'WAT'
        /// phases.
        Opm::Phase unsupportedPhase_ { Opm::Phase::SOLVENT };

        /// Internalise items of a single GNETINJE keyword record.
        ///
        /// Populates the matching group names (\code gnames_ \endcode), the
        /// injecting phase (\code phaseIx_ \endcode), the terminal pressure
        /// value if applicable (\code termPress_ \endcode), and the flow
        /// line VFP table if applicable (\code vfpTable_ \endcode).
        ///
        /// \param[in] record Single record from a GNETINJE keyword.
        void parseRecord(const Opm::DeckRecord& record);

        /// Internalise injecting phase and matching groups.
        ///
        /// Populates the matching group names (\code gnames_ \endcode) and
        /// the injecting phase (\code phaseIx_ \endcode).
        ///
        /// \param[in] record Single record from a GNETINJE keyword.
        void inferPhaseAndGroups(const Opm::DeckRecord& record);

        /// Internalise items of a single GNETINJE keyword record.
        ///
        /// Populates the terminal pressure value (\code termPress_
        /// \endcode) and the flow line VFP table, if applicable (\code
        /// vfpTable_ \endcode).
        ///
        /// \param[in] record Single record from a GNETINJE keyword.
        void inferTermPressAndVFP(const Opm::DeckRecord& record);

        /// Create raw injection network nodes corresponding to currently
        /// active GNETINJE record.
        ///
        /// \param[in,out] nodes Current injection network nodes for active
        /// record's injecting phase.  On return, holds nodes for all the
        /// current record's matching group names.
        void createNodes(GNetInjeRaw& nodes);
    };

    void CollectGNetInje::parseRecord(const Opm::DeckRecord& record)
    {
        // Keyword structure:
        //
        // GNETINJE
        //    GroupName  Phase   TermPress   VFPTable /
        //    GroupName  Phase   TermPress   VFPTable /
        //    ...                                     /
        //    GroupName  Phase   TermPress   VFPTable /
        // /
        //
        // The GroupName may be a root like "INJ-*", in which case the
        // record applies to all groups whose name matches that name root.
        // The Phase must be 'GAS' or 'WAT'.  The terminal pressure
        // (TermPress) is non-negative for fixed-pressure injection network
        // "terminal" nodes, and negative or defaulted otherwise.  The VFP
        // table (item 4) can be set to the special value 9999 to indicate
        // no pressure loss between the node and its parent.  If the VFP
        // table is defaulted or explicitly set to zero, then there is no
        // flow line connecting this node to its parent node.  That will
        // typically be the case for a terminal node.

        // Note: The conditional in operator()() depends on these data
        // members being empty or nullopt if the parsing process fails, so
        // ensure that they start in that known state.
        this->phaseIx_.reset();
        this->vfpTable_.reset();
        this->gnames_.clear();

        this->inferPhaseAndGroups(record);

        if (!this->phaseIx_.has_value() || this->gnames_.empty()) {
            // Invalid phase or no matching groups.  Ignore record.
            return;
        }

        this->inferTermPressAndVFP(record);
    }

    void CollectGNetInje::inferPhaseAndGroups(const Opm::DeckRecord& record)
    {
        using Kw = Opm::ParserKeywords::GNETINJE;

        const auto ph = [&phaseItem = record.getItem<Kw::PHASE>(),
                         unsuppPhase = this->unsupportedPhase_]()
        {
            try {
                return phaseItem.defaultApplied(0)
                    ? unsuppPhase // Default => phase match failure.
                    : Opm::get_phase(phaseItem.getTrimmedString(0));
            }
            catch (const std::invalid_argument&) {
                // The phaseItem contains something that's not recognised by
                // get_phase().  Return an unsupported phase enumerator to
                // indicate phase matching failure.
                return unsuppPhase;
            }
        }();

        auto phaseIxPos = this->phaseIndexMap_.find(ph);
        if (phaseIxPos == this->phaseIndexMap_.end()) {
            rejectInvalidGNetInjePhase(record, this->handlerContext_);
            return;
        }

        this->phaseIx_.emplace(phaseIxPos->second);

        this->gnames_ = this->handlerContext_.groupNames
            (record.getItem<Kw::GROUP>().getTrimmedString(0));

        if (this->gnames_.empty()) {
            this->handlerContext_.invalidNamePattern
                (record.getItem<Kw::GROUP>().getTrimmedString(0));
        }
    }

    void CollectGNetInje::inferTermPressAndVFP(const Opm::DeckRecord& record)
    {
        using Kw = Opm::ParserKeywords::GNETINJE;

        this->termPress_ = parseGNetInjeTerminalPressure
            (record.getItem<Kw::PRESSURE>());

        const auto vfpTable = record.getItem<Kw::VFP_TABLE>().get<int>(0);

        if (vfpTable < 0) {
            // Negative VFP table ID.  This is an error.
            const auto msg_fmt = fmt::format(R"(Problem with keyword {{keyword}}
In {{file}} line {{line}}
Negative VFP table number {} is not permitted.)", vfpTable);

            this->handlerContext_.parseContext
                .handleError(Opm::ParseContext::SCHEDULE_INVALID_VFPTABLE,
                             msg_fmt,
                             this->handlerContext_.keyword.location(),
                             this->handlerContext_.errors);

            // HandleError() will *typically* throw an exception here, but
            // we return--and ignore the record--if user has downgraded this
            // condition to a warning instead.
            return;
        }

        if ((vfpTable > 0) && this->termPress_.has_value()) {
            // We have a VFP table for a terminal node/group.
            //
            // This is an error condition.
            const auto msg_fmt = fmt::format(R"(Problem with keyword {{keyword}}
In {{file}} line {{line}}
Explicit VFP table {} cannot be assigned to terminal group{} '{}'
with terminal pressure {}.)",
                                             vfpTable,
                                             (this->gnames_.size() != 1) ? "s" : "",
                                             record.getItem<Kw::GROUP>().getTrimmedString(0),
                                             record.getItem<Kw::PRESSURE>().get<double>(0));

            this->handlerContext_.parseContext
                .handleError(Opm::ParseContext::SCHEDULE_INVALID_VFPTABLE,
                             msg_fmt,
                             this->handlerContext_.keyword.location(),
                             this->handlerContext_.errors);

            // HandleError() will *typically* throw an exception here, but
            // we return--and ignore the record--if user has downgraded this
            // condition to a warning instead.
            return;
        }

        // If we get here, the terminal pressure and VFP tables are
        // internally consistent.  Record the VFP table to allow process to
        // proceed to creating network nodes.
        //
        // Note that we intentionally do not verify that the VFP table
        // identified in this record actually exists at this point.  A query
        // of the form
        //
        //     handlerContext_.state().vfpinj.has(vfpTable)
        //
        // checks existence, but if we enforce the requirement that the VFP
        // table exists then all unit tests must meet that criterion as
        // well.  That is too much of a burden so we instead defer VFP table
        // existence checking until the point of use.
        this->vfpTable_.emplace(vfpTable);
    }

    void CollectGNetInje::createNodes(GNetInjeRaw& nodes)
    {
        assert (this->vfpTable_.has_value());

        if ((*this->vfpTable_ == 0) && !this->termPress_.has_value()) {
            // VFP table is defaulted for a non-terminal node/group.
            //
            // That node/group is not part of the network.  In update mode,
            // the previous network is copied first, so clear any existing
            // node/branch state for the affected groups before returning.
            std::ranges::for_each(this->gnames_,
                                  [&nodes, &groups = this->handlerContext_.state().groups]
                                  (const auto& gname)
            {
                auto& node = nodes.node(gname);

                node.parent = groups(gname).flow_group();

                node.vfpTable.reset();
                node.termPress.reset();
            });

            return;
        }

        if (this->termPress_.has_value()) {
            // Define a set of fixed-pressure terminal nodes.
            //
            // Clear any stale non-terminal state first in case the same
            // group was previously seen as a branch node in this keyword
            // expansion or inherited from an earlier update.
            std::ranges::for_each(this->gnames_,
                                  [tpress = *this->termPress_, &nodes]
                                  (const auto& gname)
            {
                 auto& node = nodes.node(gname);

                 node.parent.reset();
                 node.vfpTable.reset();
                 node.termPress.emplace(tpress);
            });

            return;
        }

        // If we get here, the nodes in 'gnames_' are not terminal and
        // therefore define branches in the injection network.
        std::ranges::for_each(this->gnames_,
                              [&nodes, vfpTable = *this->vfpTable_,
                               &groups = this->handlerContext_.state().groups]
                              (const auto& gname)
        {
            const auto parent = groups(gname).flow_group();

            if (! parent.has_value()) {
                return;
            }

            // 'Gname' has a parent group in the group tree.  Form a branch
            // to that parent group and associate the flow line with the
            // current VFP table.
            auto& node = nodes.node(gname);

            node.parent.emplace(*parent);
            node.vfpTable.emplace(vfpTable);
            node.termPress.reset();
        });
    }

    // -----------------------------------------------------------------------

    /// Form or update injection network from sequence of raw GNETINJE nodes.
    ///
    /// \param[in] rawInjNetworkNodes Internalised and expanded injection
    /// network nodes from all records in a single GNETINJE keyword that
    /// pertain to a particular injecting phase.
    ///
    /// \param[in,out] injectionNetwork New or existing injection network
    /// structure.  On exit, updated to hold the nodes and branches implied
    /// by \c rawInjNetworkNodes.
    void constructInjectionNetwork(const GNetInjeRaw&        rawInjNetworkNodes,
                                   Opm::Network::ExtNetwork& injectionNetwork)
    {
        auto deferredNodes = std::vector<Opm::Network::Node>{};

        for (const auto& [nodeName, rawNode] : rawInjNetworkNodes) {
            if (! rawNode.vfpTable.has_value() &&
                ! rawNode.termPress.has_value())
            {
                // VFP table defaulted for non-terminal node.  Node is not
                // part of network.

                if (rawNode.parent.has_value()) {
                    // 'NodeName' is down-tree node of a dropped branch.
                    // Confer dropped branch onto network.
                    injectionNetwork.drop_branch(*rawNode.parent, nodeName);
                }

                continue;
            }
            else if (! rawNode.parent.has_value()) {
                // Typically a fixed-pressure terminal node.  Defer
                // registration until after the raw nodes have been
                // processed so branch updates and stale branch cleanup are
                // handled first.

                if (auto& deferredNode = deferredNodes.emplace_back(nodeName);
                    rawNode.termPress.has_value())
                {
                    deferredNode.terminal_pressure(*rawNode.termPress);
                }

                if (injectionNetwork.has_node(nodeName)) {
                    if (const auto up = injectionNetwork.uptree_branch(nodeName);
                        up.has_value())
                    {
                        // 'NodeName' used to be non-terminal, but its role
                        // has changed.  Drop existing up-tree branch to
                        // avoid stale network topology.
                        injectionNetwork.drop_branch(up->uptree_node(), nodeName);
                    }
                }

                continue;
            }

            // If we get here, the 'nodeName' refers to a non-terminal node
            // in the injection network.  Create a branch to its
            // parent/up-tree node and ensure that there is a network node
            // object for 'nodeName'.  We *assume* that the parent will
            // itself be a node in the network and therefore handled
            // elsewhere.
            //
            // Note: There is no artificial lift in injection networks, so
            // we always assign an ALQ of zero to these branches.
            const auto alq = 0.0;

            injectionNetwork.add_or_replace_branch(Opm::Network::Branch {
                nodeName, *rawNode.parent, *rawNode.vfpTable, alq
            });

            injectionNetwork.update_node(Opm::Network::Node { nodeName });
        }

        for (auto&& deferredNode : deferredNodes) {
            injectionNetwork.update_node(std::move(deferredNode));
        }
    }
}

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

void handleGNETINJE(HandlerContext& handlerContext)
{
    std::ranges::for_each(CollectGNetInje { handlerContext }(),
                          [&input = handlerContext.state().injectionNetwork]
                          (const auto& phaseInjectionNetwork)
    {
        const auto& [phase, rawInjNetworkNodes] = phaseInjectionNetwork;

        if (rawInjNetworkNodes.empty()) {
            // No injection network defined for this 'phase'.
            return;
        }

        auto networkUpdate = input.has(phase)
            ? std::make_shared<Network::ExtNetwork>(input.get(phase))
            : std::make_shared<Network::ExtNetwork>();

        networkUpdate->set_standard_network(true);

        constructInjectionNetwork(rawInjNetworkNodes, *networkUpdate);

        input.update(phase, std::move(networkUpdate));
    });
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
        { "GNETINJE", &handleGNETINJE },
        { "GRUPNET",  &handleGRUPNET  },
        { "NEFAC",    &handleNEFAC    },
        { "NETBALAN", &handleNETBALAN },
        { "NODEPROP", &handleNODEPROP },
    };
}

}
