/*
  Copyright 2026 Equinor ASA.

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

#include "NetworkValidation.hpp"

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/input/eclipse/Schedule/Network/Branch.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/Network/Node.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

namespace {

    /// Names of those nodes which take part in the network.
    ///
    /// A node is part of the network only for as long as it is attached to
    /// at least one branch.  A node mentioned in NODEPROP alone, or a node
    /// whose only branch has been removed by a BRANPROP entry with a zero
    /// VFP table number, is therefore not included here.
    ///
    /// \param[in] network Extended network.
    ///
    /// \return Names of the network's nodes, in order of first appearance
    /// in the input.
    std::vector<std::string>
    connectedNodes(const Opm::Network::ExtNetwork& network)
    {
        auto connected = std::set<std::string>{};
        for (const auto* branch : network.branches()) {
            connected.insert(branch->uptree_node());
            connected.insert(branch->downtree_node());
        }

        auto nodes = std::vector<std::string>{};
        nodes.reserve(connected.size());

        std::ranges::copy_if(network.node_names(), std::back_inserter(nodes),
                             [&connected](const std::string& node)
                             { return connected.contains(node); });

        return nodes;
    }

    /// Whether or not a node acts as a source for the network.
    ///
    /// Flow enters a node through its downtree branches, so a node without
    /// downtree branches has no inlets and must supply its own flow rate.
    ///
    /// \param[in] network Extended network.
    ///
    /// \param[in] node Node name.  Must be a node of \p network.
    ///
    /// \return Whether or not \p node has any inlets.
    bool isSource(const Opm::Network::ExtNetwork& network,
                  const std::string&              node)
    {
        return network.downtree_branches(node).empty();
    }

    /// Report a network inconsistency.
    ///
    /// \param[in] description Description of the inconsistency.  Must not
    /// contain any brace characters.
    ///
    /// \param[in] location Location of the keyword which prompted the
    /// check.  Included in the diagnostic by the error handling protocol.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors.
    void reportInconsistency(const std::string&            description,
                             const Opm::KeywordLocation&   location,
                             const Opm::ParseContext&      parseContext,
                             Opm::ErrorGuard&              errors)
    {
        parseContext.handleError(Opm::ParseContext::SCHEDULE_NETWORK_INVALID,
                                 description, location, errors);
    }

    /// Check that every source node is also a group.
    ///
    /// A source node gets its flow rate from the wells of the group of the
    /// same name.  A source node which is not a group has no way of
    /// contributing to the network.
    ///
    /// \param[in] network Extended network.
    ///
    /// \param[in] nodes Names of the network's nodes.
    ///
    /// \param[in] isGroup Predicate returning whether or not a name is that
    /// of a group.
    ///
    /// \param[in] location Location of the keyword which prompted this
    /// check.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors.
    void checkSourcesAreGroups(const Opm::Network::ExtNetwork&                network,
                               const std::vector<std::string>&                nodes,
                               const std::function<bool(const std::string&)>& isGroup,
                               const Opm::KeywordLocation&                    location,
                               const Opm::ParseContext&                       parseContext,
                               Opm::ErrorGuard&                               errors)
    {
        for (const auto& node : nodes) {
            if (!isSource(network, node) || isGroup(node)) {
                continue;
            }

            reportInconsistency(fmt::format("Network node {0} has no downtree branches, and must "
                                            "therefore supply the network on its own, but no "
                                            "group {0} exists.", node),
                                location, parseContext, errors);
        }
    }

    /// Check that every flow path ends in a fixed pressure node.
    ///
    /// Follows the flow path uptree from each node.  The pressure drop along
    /// the path can be computed only if the path ends in a node of known
    /// pressure.
    ///
    /// Flow paths merge on their way uptree, so a single problem is shared
    /// by every node downtree of it.  To report each problem only once we
    /// stop tracing a path as soon as it reaches a node which some earlier
    /// path already passed through.
    ///
    /// \param[in] network Extended network.
    ///
    /// \param[in] nodes Names of the network's nodes.
    ///
    /// \param[in] location Location of the keyword which prompted this
    /// check.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors.
    void checkFlowPathsAreTerminated(const Opm::Network::ExtNetwork&  network,
                                     const std::vector<std::string>&  nodes,
                                     const Opm::KeywordLocation&      location,
                                     const Opm::ParseContext&         parseContext,
                                     Opm::ErrorGuard&                 errors)
    {
        // Nodes whose flow path has been traced already, either to a fixed
        // pressure node or to a problem which has been reported.
        auto traced = std::unordered_set<std::string>{};

        for (const auto& start : nodes) {
            if (traced.contains(start)) {
                continue;
            }

            auto path = std::vector<std::string> { start };
            auto onPath = std::unordered_set<std::string> { start };

            auto node = start;
            while (! network.node(node).terminal_pressure().has_value()) {
                const auto uptree = network.uptree_branch(node);

                if (! uptree.has_value()) {
                    reportInconsistency(fmt::format("Flow path from network node {} terminates in "
                                                    "node {}, which has neither an uptree branch "
                                                    "nor a fixed pressure.", start, node),
                                        location, parseContext, errors);
                    break;
                }

                node = uptree->uptree_node();

                if (traced.contains(node)) {
                    // Path merges into one which has been traced already.
                    // Whether that path ends well or not, there is nothing
                    // new to report here.
                    break;
                }

                if (! onPath.insert(node).second) {
                    // Cycle in the branch definitions.  Stop here to avoid
                    // looping forever--this path will never reach a fixed
                    // pressure node.
                    reportInconsistency(fmt::format("Flow path from network node {} returns to "
                                                    "node {}: the branches form a loop.",
                                                    start, node),
                                        location, parseContext, errors);
                    break;
                }

                path.push_back(node);
            }

            traced.insert(path.begin(), path.end());
        }
    }

} // Anonymous namespace

void Opm::Network::validateTopology(const ExtNetwork&                              network,
                                    const std::function<bool(const std::string&)>& isGroup,
                                    const KeywordLocation&                         location,
                                    const ParseContext&                            parseContext,
                                    ErrorGuard&                                    errors)
{
    if (!network.active() || network.is_standard_network()) {
        // Nothing to check, or a standard network (GRUPNET) whose nodes are
        // groups by construction.
        return;
    }

    const auto nodes = connectedNodes(network);

    checkSourcesAreGroups(network, nodes, isGroup, location, parseContext, errors);
    checkFlowPathsAreTerminated(network, nodes, location, parseContext, errors);
}
