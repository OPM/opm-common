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

#ifndef OPM_MAKE_MSWELL_FROM_STANDARD_WELL_HPP
#define OPM_MAKE_MSWELL_FROM_STANDARD_WELL_HPP

namespace Opm {

class Well;
class UnitSystem;

/// Convert a plain (standard) well into a "simple" multisegment well.
///
/// Synthesises a single linear branch-1 tubing with one segment per COMPDAT
/// connection: the top segment sits at the well reference depth, and each
/// connection is attached to its own segment whose depth is the connection's
/// centre depth. The pressure-drop model is hydrostatic-only, so the wellbore
/// head is determined by the segment-depth differences alone (no dependence on
/// synthesised measured-depth / tubing geometry).
///
/// After conversion \c well.isMultiSegment() is true, so the simulator builds a
/// MultisegmentWell for it. Because MSW solves the segment pressures implicitly,
/// the wellbore hydrostatic head becomes a differentiable part of the well
/// system — unlike the frozen explicit head of a StandardWell. This is useful
/// both for ordinary forward runs (a consistently linearised well head) and, in
/// particular, for the adjoint, where the frozen StandardWell head is the source
/// of a well-objective gradient bias on thick multi-perforation wells.
///
/// Returns the input well unchanged if it is already multisegment or has no
/// connections.
///
/// \note One segment *per connection* is deliberate, and is what distinguishes
///   this from a single-segment conversion. A single segment carrying every
///   connection has only one pressure unknown, so the head *between*
///   perforations is not solved for — which is precisely the quantity this
///   conversion exists to make implicit. A single-segment well could still be
///   made implicit, but only with one density derived from well-average
///   properties, and that is in any case not equivalent to a StandardWell
///   either. The two conversions are therefore different tools rather than two
///   spellings of the same one; a single-segment factory is worth adding
///   separately if a cheaper, closer-to-standard conversion is wanted.
///
/// \param well            the well to convert; left untouched
/// \param unit_system     the deck's unit system (for segment processing)
/// \param tubing_diameter nominal tubing inner diameter [m]; only affects the
///                        (unused, hydrostatic-only) friction term. 0.1524 m
///                        (6") is a reasonable choice.
///
/// \return the converted well
Well makeMultiSegmentWellPerConnection(const Well& well,
                                       const UnitSystem& unit_system,
                                       const double tubing_diameter);

} // namespace Opm

#endif // OPM_MAKE_MSWELL_FROM_STANDARD_WELL_HPP
