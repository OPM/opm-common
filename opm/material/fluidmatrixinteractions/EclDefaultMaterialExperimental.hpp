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
 * \copydoc Opm::EclDefaultMaterial
 */
#ifndef OPM_ECL_DEFAULT_MATERIAL_EXPERIMENTAL_HPP
#define OPM_ECL_DEFAULT_MATERIAL_EXPERIMENTAL_HPP

#include "EclDefaultMaterialParams.hpp"

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>

#include <algorithm>
#include <stdexcept>
#include <type_traits>


namespace Opm {

/*!
 * \ingroup material
 *
 * \brief Expermimental evalutation of relperms by hardcoding static cast
 *
 */
    template<class DefaultMaterial>    
class EclDefaultMaterialExperimental 
{    
public:
    static constexpr int waterPhaseIdx = DefaultMaterial::waterPhaseIdx;
    static constexpr int gasPhaseIdx = DefaultMaterial::gasPhaseIdx;
    static constexpr int oilPhaseIdx = DefaultMaterial::oilPhaseIdx;
    using GasOilMaterialLaw = typename DefaultMaterial::GasOilMaterialLaw;
    using OilWaterMaterialLaw = typename DefaultMaterial::OilWaterMaterialLaw;
    using Scalar = typename GasOilMaterialLaw::Scalar;
    template <class ContainerT, class FluidState,class ParamsOilWater, class ParamsGasOil>
    static void relativePermeabilities(ContainerT& values,
                                       const Scalar& Swco,
                                       const ParamsOilWater& oilwaterparams_tab,
                                       const ParamsGasOil& gasoilparams_tab, 
                                       const FluidState& fluidState)
    // template <class ContainerT, class FluidState>        
    // static void relativePermeabilitiesNew(ContainerT& values,
    //                                       const Params& params,
    //                                       const FluidState& fluidState)        
    {
        // const auto& oilwaterparams = params.oilWaterParams();
        // const auto& gasoilparams = params.gasOilParams();
        // const auto& gasoilparams_tab =params.gasOilParams().drainageParams().effectiveLawParams().template getRealParams<Opm::SatCurveMultiplexerApproach::PiecewiseLinear>();
        // const auto& oilwaterparams_tab =params.oilWaterParams().drainageParams().effectiveLawParams().template getRealParams<Opm::SatCurveMultiplexerApproach::PiecewiseLinear>();
        //Scalar Swco = params.Swl();
        //const Evaluation SwUnscaled = scaledToUnscaledSatKrw(params, SwScaled);
        //const Evaluation krwUnscaled = EffLaw::twoPhaseSatKrw(params.effectiveLawParams(), SwUnscaled);
        //return unscaledToScaledKrw_(SwScaled, params, krwUnscaled);
        using Evaluation = typename std::remove_reference<decltype(values[0])>::type;
        
        
        using PLTwoPhaseLawGasOil  = PiecewiseLinearTwoPhaseMaterial<typename GasOilMaterialLaw::Traits>;
        using PLTwoPhaseLawOilWater  = PiecewiseLinearTwoPhaseMaterial<typename OilWaterMaterialLaw::Traits>;
        
        // values[waterPhaseIdx] = krw<FluidState, Evaluation>(params, fluidState);
        
        const Evaluation Sw = decay<Evaluation>(fluidState.saturation(waterPhaseIdx));
        //const Evaluation krwv = OilWaterMaterialLaw::twoPhaseSatKrw(oilwaterparams, Sw);
        //const auto& oilwaterparams =params.oilWaterParams().drainageParams().effectiveLawParams().template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();
        //const Evaluation krwv = PLTwoPhaseLawOilWater::twoPhaseSatKrw(oilwaterparams, Sw);
        size_t segIdx_ow = PLTwoPhaseLawOilWater::findSegmentIndex(oilwaterparams_tab.SwKrwSamples(), Sw);
        const Evaluation krwv = PLTwoPhaseLawOilWater::eval(oilwaterparams_tab.SwKrwSamples(), oilwaterparams_tab.krwSamples(), Sw, segIdx_ow);
        
        values[waterPhaseIdx] = krwv;
        
        //const Scalar Swco = params.Swl();
        Evaluation krnv;
        const Evaluation Sw_eff = max(Evaluation(Swco), decay<Evaluation>(fluidState.saturation(waterPhaseIdx)));

        const Evaluation Sg = decay<Evaluation>(fluidState.saturation(gasPhaseIdx));
        
        const Evaluation Sw_ow = Sg + Sw_eff;
        // const Evaluation kro_ow = relpermOilInOilWaterSystem<Evaluation>(params, fluidState);
        //const Evaluation kro_ow = OilWaterMaterialLaw::twoPhaseSatKrn(oilwaterparams, Sw_ow);
        //const Evaluation kro_ow = PLTwoPhaseLawOilWater::twoPhaseSatKrn(oilwaterparams, Sw_ow);
        //size_t segIdx_ow = PLTwoPhaseLawOilWater::findSegmentIndex(oilwaterparams.SwKrwSamples(), Sw);
        //NB assumes same index but probalby one should ahve used same x values also
        //const Evaluation kro_ow = PLTwoPhaseLawOilWater::eval(oilwaterparams.SwKrnSamples(), oilwaterparams.krnSamples(), Sw_ow, segIdx_ow);
        size_t segIdx_ow_o = PLTwoPhaseLawOilWater::findSegmentIndex(oilwaterparams_tab.SwKrnSamples(), Sw_ow);
        const Evaluation kro_ow = PLTwoPhaseLawOilWater::eval(oilwaterparams_tab.SwKrnSamples(), oilwaterparams_tab.krnSamples(), Sw_ow, segIdx_ow_o);

// const Evaluation kro_go = relpermOilInOilGasSystem<Evaluation>(params, fluidState);
        const Evaluation So_go = 1.0 - Sw_ow;
        //const Evaluation kro_go = GasOilMaterialLaw::twoPhaseSatKrw(gasoilparams, So_go);
        //const auto& gasoilparams =params.gasOilParams().drainageParams().effectiveLawParams().template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();
        //const Evaluation kro_go = PLTwoPhaseLawGasOil::twoPhaseSatKrw(gasoilparams, So_go);
        size_t segIdx_go = PLTwoPhaseLawGasOil::findSegmentIndex(gasoilparams_tab.SwKrwSamples(), So_go);
        const Evaluation kro_go = PLTwoPhaseLawGasOil::eval(gasoilparams_tab.SwKrwSamples(), gasoilparams_tab.krwSamples(), So_go, segIdx_go);
        // avoid the division by zero: chose a regularized kro which is used if Sw - Swco
        // < epsilon/2 and interpolate between the oridinary and the regularized kro between
        // epsilon and epsilon/2
        constexpr const Scalar epsilon = 1e-5;
        Scalar Sw_ow_wco = scalarValue(Sw_ow) - Swco;
        if ( Sw_ow_wco < epsilon) {
            const Evaluation kro2 = (kro_ow + kro_go) / 2;
            if ( Sw_ow_wco > epsilon / 2) {
                const Evaluation kro1 = (Sg * kro_go + (Sw - Swco) * kro_ow) / (Sw_ow - Swco);
                const Evaluation alpha = (epsilon - (Sw_ow - Swco)) / (epsilon / 2);
                
                krnv = kro2 * alpha + kro1 * (1 - alpha);
            }
            
            krnv = kro2;
        } else {
            krnv = (Sg * kro_go + (Sw - Swco) * kro_ow) / (Sw_ow - Swco);
        }
        
        
        values[oilPhaseIdx] = krnv;
        //values[gasPhaseIdx] = krg<FluidState, Evaluation>(params, fluidState);
        const Evaluation Sw_eff2 = 1.0 - Swco - Sg;
        //Evaluation krgv = GasOilMaterialLaw::twoPhaseSatKrn(gasoilparams, Sw_eff2);
        //size_t segIdx_go = PLTwoPhaseLawGasOil::findSegmentIndex(gasoilparams.SwKrnSamples(), Sw_eff2);
        //const Evaluation krgv = PLTwoPhaseLawGasOil::eval(gasoilparams.SwKrnSamples(), gasoilparams.krnSamples(), Sw_eff2, segIdx_go);
        size_t segIdx_go_g = PLTwoPhaseLawGasOil::findSegmentIndexDescending(gasoilparams_tab.SwKrnSamples(), Sw_eff2);
        const Evaluation krgv = PLTwoPhaseLawGasOil::eval(gasoilparams_tab.SwKrnSamples(), gasoilparams_tab.krnSamples(), Sw_eff2, segIdx_go_g);
        values[gasPhaseIdx] = krgv;
    }
};
}

#endif



