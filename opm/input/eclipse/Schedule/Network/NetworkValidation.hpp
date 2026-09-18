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

#ifndef NETWORK_VALIDATION_HPP
#define NETWORK_VALIDATION_HPP

#include <functional>
#include <string>

namespace Opm {
    class ErrorGuard;
    class KeywordLocation;
    class ParseContext;
} // namespace Opm

namespace Opm::Network {

class ExtNetwork;

/// Check that the topology of an extended network is internally consistent.
///
/// The network cannot be balanced unless every source--i.e., every node
/// without inlets--supplies a flow rate, and unless every flow path ends in
/// a node of known pressure.  Consequently, this function reports, through
/// the normal error handling protocol,
///
///   -# any node without inlets which is not also a group, and
///
///   -# any flow path which does not end in a fixed pressure node.
///
/// Nodes which are no longer attached to a branch--e.g., because a BRANPROP
/// entry with a zero VFP table number removed the node's only branch--are
/// not part of the network and are therefore not checked.  Neither are
/// standard networks (GRUPNET), the nodes of which are groups by
/// construction.
///
/// \param[in] network Extended network, typically that of the current
/// report step's schedule state.
///
/// \param[in] isGroup Predicate returning whether or not a name is that of
/// a group at the current report step.
///
/// \param[in] location Location of the keyword which prompted this check.
///
/// \param[in] parseContext Error handling controls.
///
/// \param[in,out] errors Collection of parse errors.
void validateTopology(const ExtNetwork& network,
                      const std::function<bool(const std::string&)>& isGroup,
                      const KeywordLocation& location,
                      const ParseContext& parseContext,
                      ErrorGuard& errors);

} // namespace Opm::Network

#endif // NETWORK_VALIDATION_HPP
