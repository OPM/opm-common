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
 * \copydoc Opm::EclMaterialLaw::Manager
 */

#ifndef OPM_ECL_MATERIAL_LAW_INIT_PARAMS_HPP
#define OPM_ECL_MATERIAL_LAW_INIT_PARAMS_HPP

#include <opm/material/fluidmatrixinteractions/EclMaterialLawTwoPhaseTypes.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace Opm {
    class EclipseState;
    class FieldPropsManager;
}

namespace Opm::EclMaterialLaw {

template<class Traits> class HystParams;
template<class Traits> class Manager;

template<class Traits>
class InitParams
{
    using Scalar = typename Traits::Scalar;

public:
    InitParams(const Manager<Traits>& parent,
               const EclipseState& eclState,
               std::size_t numCompressedElems);

    using LookupFunction = std::function<unsigned(unsigned)>;
    using IntLookupFunction = std::function<std::vector<int>(const FieldPropsManager&,
                                                             const std::string&, bool)>;
    using MaterialLawParams = typename Manager<Traits>::MaterialLawParams;

    // Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
    // field properties of cells on the leaf grid view for CpGrid with local grid refinement.
    // Function argument 'lookupIdxOnLevelZeroAssigner' is added to lookup, for each
    // leaf gridview cell with index 'elemIdx', its 'lookupIdx'
    // (index of the parent/equivalent cell on level zero).
    void run(const IntLookupFunction& fieldPropIntOnLeafAssigner,
             const LookupFunction& lookupIdxOnLevelZeroAssigner);

    typename Manager<Traits>::Params params_;

private:
    // Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
    // field properties of cells on the leaf grid view for CpGrid with local grid refinement.
    void copySatnumArrays_(const IntLookupFunction& fieldPropIntOnLeafAssigner);

    // Function argument 'fieldPropIntOnLeadAssigner' needed to lookup
    // field properties of cells on the leaf grid view for CpGrid with local grid refinement.
    void copyIntArray_(std::vector<int>& dest,
                       const std::string& keyword,
                       const IntLookupFunction& fieldPropIntOnLeafAssigner) const;

    unsigned imbRegion_(const std::vector<int>& array, unsigned elemIdx) const;

    void initArrays_(std::vector<const std::vector<int>*>& satnumArray,
                     std::vector<const std::vector<int>*>& imbnumArray,
                     std::vector<std::vector<MaterialLawParams>*>& mlpArray);

    void initMaterialLawParamVectors_();

    void initOilWaterScaledEpsInfo_();

    // Function argument 'fieldProptOnLeadAssigner' needed to lookup
    // field properties of cells on the leaf grid view for CpGrid with local grid refinement.
    void initSatnumRegionArray_(const IntLookupFunction& fieldPropIntOnLeafAssigner);

    void initThreePhaseParams_(HystParams<Traits>& hystParams,
                               MaterialLawParams& materialParams,
                               unsigned satRegionIdx,
                               unsigned elemIdx);

    void readEffectiveParameters_();

    void readUnscaledEpsPointsVectors_();

    template <class Container>
    void readUnscaledEpsPoints_(Container& dest,
                                const EclEpsConfig& config,
                                EclTwoPhaseSystemType system_type);

    unsigned satRegion_(const std::vector<int>& array, unsigned elemIdx) const;

    const Manager<Traits>& parent_;
    const EclipseState& eclState_;
    std::size_t numCompressedElems_;

    std::unique_ptr<EclEpsGridProperties> epsImbGridProperties_; // imbibition
    EclEpsGridProperties epsGridProperties_;    // drainage
};

} // namespace Opm::EclMaterialLaw

#endif
