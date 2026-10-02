/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along
  with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef REQUISITE_SUMMARY_VECTOR_HPP
#define REQUISITE_SUMMARY_VECTOR_HPP

#include <set>
#include <string>
#include <vector>

namespace Opm {

/// A summary vector that a deck expression named in full.
///
/// Both UDQ definitions and ACTIONX conditions report what they need as bare
/// keywords -- see required_summary() -- because a keyword is usually all
/// there is to report: a UDQ over WBHP applies to whichever wells the UDQ
/// itself applies to, and an ACTIONX matching 'OP*' does not know which wells
/// those are until the run gets there.
///
/// Some expressions do name their object outright, though:
///
///     BPR 10 10 3 > 250 /
///     SPR 'P1' 12 < 100 /
///
/// and for block, connection, segment and node quantities that is the only
/// way anybody will ever find out which vector is wanted. The object cannot
/// be enumerated from the keyword, and covering every cell or every segment
/// in the model is not a service anybody wants.
///
/// The arguments are kept as the deck wrote them. What they mean depends on
/// the keyword's category -- I, J and K for a block; a well name followed by
/// I, J and K for a connection; a well name and a segment number for a
/// segment; a node name for a node -- and interpreting them is left to the
/// caller, which knows the grid and the schedule.
struct RequisiteSummaryVector
{
    /// Summary keyword, e.g. BPR.
    std::string keyword{};

    /// Whatever the expression wrote after the keyword, quotes stripped.
    std::vector<std::string> arguments{};

    /// Ordering predicate, so that these can live in a set.
    bool operator<(const RequisiteSummaryVector& that) const
    {
        return (this->keyword < that.keyword)
            || ((this->keyword == that.keyword) &&
                (this->arguments < that.arguments));
    }

    /// Equality predicate.
    bool operator==(const RequisiteSummaryVector& that) const
    {
        return (this->keyword == that.keyword)
            && (this->arguments == that.arguments);
    }
};

/// Collection of summary vectors named in full by deck expressions.
using RequisiteSummaryVectors = std::set<RequisiteSummaryVector>;

} // namespace Opm

#endif // REQUISITE_SUMMARY_VECTOR_HPP
