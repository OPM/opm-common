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
#ifndef OPM_LGR_CONNECTION_CHECK_HPP
#define OPM_LGR_CONNECTION_CHECK_HPP

#include <array>
#include <vector>

namespace Opm {

class EclipseState;
class LgrCollection;

//! A refinement box in 0-based, end-exclusive cell coordinates.
struct LgrCellBox
{
    std::array<int,3> start{};
    std::array<int,3> end{};
};

//! The GLOBAL-parent boxes of a deck's CARFIN collection (nested boxes lie
//! inside their parent and add nothing).
std::vector<LgrCellBox> lgrCellBoxes(const LgrCollection& lgrs);

/// Refuse the deck connections refinement cannot carry: an explicit NNC (the
/// NNC keyword, a numerical aquifer) naming a cell inside a box, and a MULTREGT
/// multiplier other than 1 on such a connection.  Decided from the deck alone,
/// so every rank decides the same.
void refuseDeckConnectionsInsideBoxes(const EclipseState& eclState,
                                      const std::array<int,3>& cartDims,
                                      const std::vector<LgrCellBox>& boxes);

} // namespace Opm

#endif // OPM_LGR_CONNECTION_CHECK_HPP
