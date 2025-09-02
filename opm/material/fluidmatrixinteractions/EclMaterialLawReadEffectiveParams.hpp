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

#ifndef OPM_ECL_MATERIAL_LAW_READ_EFFECTIVE_PARAMS_HPP
#define OPM_ECL_MATERIAL_LAW_READ_EFFECTIVE_PARAMS_HPP

#include <opm/material/fluidmatrixinteractions/EclMaterialLawTwoPhaseTypes.hpp>

#include <string>
#include <vector>

namespace Opm {
    class EclipseState;
    class SgfnTable;
    class SgofTable;
    class SlgofTable;
    class TableColumn;
}

namespace Opm::EclMaterialLaw {

template<class Traits> class Manager;


template<class Traits>
class ReadEffectiveParams
{
    using Scalar = typename Traits::Scalar;

    using GasOilEffectiveParams =
        typename TwoPhaseTypes<Traits>::GasOilEffectiveParams;

    using GasOilEffectiveParamVector =
        typename TwoPhaseTypes<Traits>::GasOilEffectiveParamVector;

    using GasWaterEffectiveParams =
        typename TwoPhaseTypes<Traits>::GasWaterEffectiveParams;

    using GasWaterEffectiveParamVector =
        typename TwoPhaseTypes<Traits>::GasWaterEffectiveParamVector;

    using OilWaterEffectiveParams =
        typename TwoPhaseTypes<Traits>::OilWaterEffectiveParams;

    using OilWaterEffectiveParamVector =
        typename TwoPhaseTypes<Traits>::OilWaterEffectiveParamVector;

public:
    ReadEffectiveParams(typename Manager<Traits>::Params& params,
                        const EclipseState& eclState,
                        const Manager<Traits>& parent);

    void read();

private:
    std::vector<double>
    normalizeKrValues_(const double tolcrit,
                       const TableColumn& krValues) const;

    void readGasOilParameters_(unsigned satRegionIdx);

    template <class TableType>
    void readGasOilFamily2_(GasOilEffectiveParams& effParams,
                            const Scalar Swco,
                            const double tolcrit,
                            const TableType& sofTable,
                            const SgfnTable& sgfnTable,
                            const std::string& columnName);

    void readGasOilSgof_(GasOilEffectiveParams& effParams,
                         const Scalar Swco,
                         const double tolcrit,
                         const SgofTable& sgofTable);

    void readGasOilSlgof_(GasOilEffectiveParams& effParams,
                          const Scalar Swco,
                          const double tolcrit,
                          const SlgofTable& slgofTable);

    void readGasWaterParameters_(unsigned satRegionIdx);

    void readOilWaterParameters_(unsigned satRegionIdx);

    typename Manager<Traits>::Params& params_;
    const EclipseState& eclState_;
    const Manager<Traits>& parent_;
};

} // namespace Opm::EclMaterialLaw

#endif
