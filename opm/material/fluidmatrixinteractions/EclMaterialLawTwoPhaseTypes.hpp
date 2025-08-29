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
 * \copydoc Opm::EclMaterialLawManager
 */
#ifndef OPM_ECL_MATERIAL_LAW_TYPES_HPP
#define OPM_ECL_MATERIAL_LAW_TYPES_HPP

#include <opm/material/fluidmatrixinteractions/EclHysteresisTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsTwoPhaseLaw.hpp>
#include <opm/material/fluidmatrixinteractions/MaterialTraits.hpp>
#include <opm/material/fluidmatrixinteractions/SatCurveMultiplexer.hpp>

#include <memory>
#include <vector>

namespace Opm::EclMaterialLaw {

/*!
 *  \brief Helper class defining various two-phase types used in
 *         three-phase material laws.
 */

template <class Traits>
class TwoPhaseTypes
{
public:
    using GasOilTraits = TwoPhaseMaterialTraits<typename Traits::Scalar,
                                                Traits::nonWettingPhaseIdx,
                                                Traits::gasPhaseIdx>;
    using OilWaterTraits = TwoPhaseMaterialTraits<typename Traits::Scalar,
                                                  Traits::wettingPhaseIdx,
                                                  Traits::nonWettingPhaseIdx>;
    using GasWaterTraits = TwoPhaseMaterialTraits<typename Traits::Scalar,
                                                  Traits::wettingPhaseIdx,
                                                  Traits::gasPhaseIdx>;

    // the two-phase material law which is defined on effective (unscaled) saturations
    using GasOilEffectiveLaw = SatCurveMultiplexer<GasOilTraits>;
    using OilWaterEffectiveLaw = SatCurveMultiplexer<OilWaterTraits>;
    using GasWaterEffectiveLaw = SatCurveMultiplexer<GasWaterTraits>;

    using GasOilEffectiveParams = typename GasOilEffectiveLaw::Params;
    using OilWaterEffectiveParams = typename OilWaterEffectiveLaw::Params;
    using GasWaterEffectiveParams = typename GasWaterEffectiveLaw::Params;

    // the two-phase material law which is defined on absolute (scaled) saturations
    using GasOilEpsLaw = EclEpsTwoPhaseLaw<GasOilEffectiveLaw>;
    using OilWaterEpsLaw = EclEpsTwoPhaseLaw<OilWaterEffectiveLaw>;
    using GasWaterEpsLaw = EclEpsTwoPhaseLaw<GasWaterEffectiveLaw>;
    using GasOilEpsParams = typename GasOilEpsLaw::Params;
    using OilWaterEpsParams = typename OilWaterEpsLaw::Params;
    using GasWaterEpsParams = typename GasWaterEpsLaw::Params;

    // the scaled two-phase material laws with hysteresis
    using GasOilLaw = EclHysteresisTwoPhaseLaw<GasOilEpsLaw>;
    using OilWaterLaw = EclHysteresisTwoPhaseLaw<OilWaterEpsLaw>;
    using GasWaterLaw = EclHysteresisTwoPhaseLaw<GasWaterEpsLaw>;
    using GasOilHystParams = typename GasOilLaw::Params;
    using OilWaterHystParams = typename OilWaterLaw::Params;
    using GasWaterHystParams = typename GasWaterLaw::Params;

    using GasOilEffectiveParamVector = std::vector<std::shared_ptr<GasOilEffectiveParams>>;
    using OilWaterEffectiveParamVector = std::vector<std::shared_ptr<OilWaterEffectiveParams>>;
    using GasWaterEffectiveParamVector = std::vector<std::shared_ptr<GasWaterEffectiveParams>>;
};

} // namespace Opm::EclMaterialLaw

#endif
