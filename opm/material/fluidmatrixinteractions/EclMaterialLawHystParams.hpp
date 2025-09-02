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

#ifndef OPM_ECL_MATERIAL_LAW_HYST_PARAMS_HPP
#define OPM_ECL_MATERIAL_LAW_HYST_PARAMS_HPP

#include <opm/material/fluidmatrixinteractions/EclMaterialLawTwoPhaseTypes.hpp>

#include <functional>
#include <memory>
#include <tuple>

namespace Opm {
    class EclEpsGridProperties;
class EclipseState;
}

namespace Opm::EclMaterialLaw {

template<class Traits> class Manager;

template<class Traits>
class HystParams
{
public:
    using Scalar = typename Traits::Scalar;
    using GasOilHystParams = typename TwoPhaseTypes<Traits>::GasOilHystParams;
    using GasWaterHystParams = typename TwoPhaseTypes<Traits>::GasWaterHystParams;
    using OilWaterHystParams = typename TwoPhaseTypes<Traits>::OilWaterHystParams;

    HystParams(std::vector<EclEpsScalingPointsInfo<Scalar>>& oilWaterScaledEpsInfoDrainage,
               const EclEpsGridProperties& epsGridProperties,
               const EclEpsGridProperties& epsImbGridProperties,
               const EclipseState& eclState,
               const Manager<Traits>& parent);

    void finalize();

    std::shared_ptr<GasOilHystParams> getGasOilParams()
    { return gasOilParams_; }

    std::shared_ptr<OilWaterHystParams> getOilWaterParams()
    { return oilWaterParams_; }

    std::shared_ptr<GasWaterHystParams> getGasWaterParams()
    { return gasWaterParams_; }

    void setConfig(unsigned satRegionIdx);

    // Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
    // leaf gridview cell with index 'elemIdx', its 'lookupIdx'
    // (index of the parent/equivalent cell on level zero).
    using LookupFunction = std::function<unsigned(unsigned)>;

    void setDrainageParamsOilGas(unsigned elemIdx, unsigned satRegionIdx,
                                 const LookupFunction& lookupIdxOnLevelZeroAssigner);

    void setDrainageParamsOilWater(unsigned elemIdx, unsigned satRegionIdx,
                                   const LookupFunction& lookupIdxOnLevelZeroAssigner);

    void setDrainageParamsGasWater(unsigned elemIdx, unsigned satRegionIdx,
                                   const LookupFunction& lookupIdxOnLevelZeroAssigner);

    void setImbibitionParamsOilGas(unsigned elemIdx, unsigned satRegionIdx,
                                   const LookupFunction& lookupIdxOnLevelZeroAssigner);

    void setImbibitionParamsOilWater(unsigned elemIdx, unsigned satRegionIdx,
                                     const LookupFunction& lookupIdxOnLevelZeroAssigner);
    void setImbibitionParamsGasWater(unsigned elemIdx, unsigned satRegionIdx,
                                     const LookupFunction& lookupIdxOnLevelZeroAssigner);

private:
    bool hasGasWater_();
    bool hasGasOil_();
    bool hasOilWater_();

    // Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
    // leaf gridview cell with index 'elemIdx', its 'lookupIdx'
    // (index of the parent/equivalent cell on level zero).
    std::tuple<EclEpsScalingPointsInfo<Scalar>, EclEpsScalingPoints<Scalar>>
    readScaledEpsPoints_(const EclEpsGridProperties& epsGridProperties,
                         unsigned elemIdx,
                         EclTwoPhaseSystemType type,
                         const LookupFunction& lookupIdxOnLevelZeroAssigner);

    std::tuple<EclEpsScalingPointsInfo<Scalar>, EclEpsScalingPoints<Scalar>>
    readScaledEpsPointsDrainage_(unsigned elemIdx,
                                 EclTwoPhaseSystemType type,
                                 const LookupFunction& lookupIdxOnLevelZeroAssigner);

    std::tuple<EclEpsScalingPointsInfo<Scalar>, EclEpsScalingPoints<Scalar>>
    readScaledEpsPointsImbibition_(unsigned elemIdx,
                                   EclTwoPhaseSystemType type,
                                   const LookupFunction& lookupIdxOnLevelZeroAssigner);

    std::shared_ptr<GasOilHystParams> gasOilParams_;
    std::shared_ptr<OilWaterHystParams> oilWaterParams_;
    std::shared_ptr<GasWaterHystParams> gasWaterParams_;

    std::vector<EclEpsScalingPointsInfo<Scalar>>& oilWaterScaledEpsInfoDrainage_;
    const EclEpsGridProperties& epsGridProperties_;
    const EclEpsGridProperties& epsImbGridProperties_;
    const EclipseState& eclState_;
    const Manager<Traits>& parent_;
};

} // namespace Opm::EclMaterialLaw

#endif
