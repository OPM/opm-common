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

#ifndef OPM_NUMERICAL_AQUIFER_MODE_HPP
#define OPM_NUMERICAL_AQUIFER_MODE_HPP

namespace Opm {

/*!
 * \brief How the numerical aquifers of a deck (AQUNUM/AQUCON) are represented.
 *
 * A numerical aquifer is pure flow bookkeeping: a pore volume and a list of
 * connections.  It has no geometry.  ECLIPSE nevertheless expresses it by taking over a
 * grid cell, which is cheap and restart-compatible, and this remains the default so that
 * existing decks are unaffected.
 *
 * That representation costs something, though, for anything that reads the grid as
 * geometry rather than as bookkeeping.  The taken-over cell is forced active and carries
 * an authored depth and pore volume, and its connections become non-neighbour
 * connections, which have no face and therefore no geometry either.  Mechanics is the
 * clearest case: such a cell is not rock, and a connection which is not a face cannot
 * carry a traction.
 *
 * \c AuxiliaryCells asks for the opposite: leave the grid alone entirely, and let the
 * simulator represent the aquifer as degrees of freedom of its own.  The keywords are
 * still parsed and the aquifer data is still available; what changes is that no grid cell
 * is taken over, no field property is overridden and no non-neighbour connection is
 * generated.  A simulator which selects this mode is responsible for representing the
 * aquifers itself -- nothing else will.
 */
enum class NumericalAquiferMode {
    //! Take over a grid cell per aquifer cell (the historical behaviour).
    GridCells,

    //! Leave the grid untouched; the simulator represents the aquifer itself.
    AuxiliaryCells,
};

} // namespace Opm

#endif // OPM_NUMERICAL_AQUIFER_MODE_HPP
