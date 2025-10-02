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
 * \brief This file contains helper classes for the material laws.
 *
 * These classes specify the information which connects fluid systems
 * and the fluid-matrix interaction laws. This includes stuff like the
 * index of the wetting and non-wetting phase, etc.
 */
#ifndef OPM_MATERIAL_TRAITS_HPP
#define OPM_MATERIAL_TRAITS_HPP

namespace Opm {

/*!
 * \ingroup material
 *
 * \brief A generic traits class which does not provide any indices.
 *
 * This traits class is intended to be used by the NullMaterial
 */
template <class ScalarT, int numPhasesV>
class NullMaterialTraits
{
public:
    //! The type used for scalar floating point values
    using Scalar = ScalarT;

    //! The number of fluid phases
    static constexpr int numPhases = numPhasesV;
};

/*!
 * \ingroup material
 *
 * \brief A generic traits class for two-phase material laws.
 */
template <class ScalarT, int wettingPhaseIdxV, int nonWettingPhaseIdxV>
class TwoPhaseMaterialTraits
{
public:
    //! The type used for scalar floating point values
    using Scalar = ScalarT;

    //! The number of fluid phases
    static constexpr int numPhases = 2;

    //! The index of the wetting phase
    static constexpr int  wettingPhaseIdx = wettingPhaseIdxV;

    //! The index of the non-wetting phase
    static constexpr int nonWettingPhaseIdx = nonWettingPhaseIdxV;

    // some safety checks...
    static_assert(wettingPhaseIdx != nonWettingPhaseIdx,
                  "wettingPhaseIdx and nonWettingPhaseIdx must be different");
};

/*!
 * \ingroup material
 *
 * \brief A generic traits class for three-phase material laws.
 */
template <class ScalarT, int wettingPhaseIdxV, int nonWettingasPhaseIdxV, int gasPhaseIdxV,
          bool enableHysteresisV, bool enableEndpointScalingV>
class ThreePhaseMaterialTraits
{
public:
    //! The type used for scalar floating point values
    using Scalar = ScalarT;

    //! The number of fluid phases
    static constexpr int numPhases = 3;

    //! The index of the wetting liquid phase
    static constexpr int wettingPhaseIdx = wettingPhaseIdxV;

    //! The index of the non-wetting liquid phase
    static constexpr int nonWettingPhaseIdx = nonWettingasPhaseIdxV;

    //! The index of the gas phase (i.e., the least wetting phase)
    static constexpr int gasPhaseIdx = gasPhaseIdxV;

    //! Is hysteresis enabled
    static constexpr bool enableHysteresis = enableHysteresisV;

    //! Is endpoint scaling enabled
    static constexpr bool enableEndpointScaling = enableEndpointScalingV;

    // some safety checks...
    static_assert(0 <= wettingPhaseIdx && wettingPhaseIdx < numPhases,
                  "wettingPhaseIdx is out of range");
    static_assert(0 <= nonWettingPhaseIdx && nonWettingPhaseIdx < numPhases,
                  "nonWettingPhaseIdx is out of range");
    static_assert(0 <= gasPhaseIdx && gasPhaseIdx < numPhases,
                  "gasPhaseIdx is out of range");

    static_assert(wettingPhaseIdx != nonWettingPhaseIdx,
                  "wettingPhaseIdx and nonWettingPhaseIdx must be different");
    static_assert(wettingPhaseIdx != gasPhaseIdx,
                  "wettingPhaseIdx and gasPhaseIdx must be different");
    static_assert(nonWettingPhaseIdx != gasPhaseIdx,
                  "nonWettingPhaseIdx and gasPhaseIdx must be different");
};

} // namespace Opm

#endif
