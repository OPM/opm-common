// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::BlackOilDefaultFluidSystemIndices
 */
#ifndef OPM_BLACK_OIL_DEFAULT_FLUID_SYSTEM_INDICES_HPP
#define OPM_BLACK_OIL_DEFAULT_FLUID_SYSTEM_INDICES_HPP

#include <cassert>

namespace Opm {

/*!
 * \brief The class which specifies the default phase and component indices for the
 *        black-oil fluid system.
 */
class BlackOilDefaultFluidSystemIndices
{
public:
    //! Total number of phases
    static constexpr unsigned numPhases = 3;

    //! Index of the water phase
    static constexpr unsigned waterPhaseIdx = 0;
    //! Index of the oil phase
    static constexpr unsigned oilPhaseIdx = 1;
    //! Index of the gas phase
    static constexpr unsigned gasPhaseIdx = 2;

    //! Total number of components
    static constexpr unsigned numComponents = 3;

    //! Index of the oil component
    static constexpr int oilCompIdx = 0;
    //! Index of the water component
    static constexpr int waterCompIdx = 1;
    //! Index of the gas component
    static constexpr int gasCompIdx = 2;

    // TODO: the following two functions are for black oil only
    // it remains to be formulated to be more generic
    // it is likely moved out of the Indices class
    //! Map component index to phase index
    static constexpr int componentToPhaseIdx(int compIdx) {
        assert(compIdx >= 0 && compIdx < static_cast<int>(numComponents));
        switch (compIdx) {
            case oilCompIdx:   return oilPhaseIdx;
            case waterCompIdx: return waterPhaseIdx;
            case gasCompIdx:   return gasPhaseIdx;
            default:           return -1; // invalid index
        }
    }

    //! Map phase index to component index
    static constexpr int phaseToComponentIdx(int phaseIdx) {
        assert(phaseIdx >= 0 && phaseIdx < static_cast<int>(numPhases));
        switch (phaseIdx) {
            case waterPhaseIdx: return waterCompIdx;
            case oilPhaseIdx:   return oilCompIdx;
            case gasPhaseIdx:   return gasCompIdx;
            default:            return -1; // invalid index
        }
    }
};

} // namespace Opm

#endif
