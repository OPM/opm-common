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

#include <config.h>
#include <opm/material/fluidsystems/blackoilpvt/DeadOilPvt.hpp>

#include <opm/common/ErrorMacros.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar>
void DeadOilPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& pvdoTables = eclState.getTableManager().getPvdoTables();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    if (pvdoTables.size() != densityTable.size()) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Table sizes mismatch. PVDO: {}, DensityTable: {}\n",
                              pvdoTables.size(), densityTable.size()));
    }

    size_t numRegions = pvdoTables.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefO = densityTable[regionIdx].oil;
        Scalar rhoRefG = densityTable[regionIdx].gas;
        Scalar rhoRefW = densityTable[regionIdx].water;

        setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);

        const auto& pvdoTable = pvdoTables.getTable<PvdoTable>(regionIdx);

        const auto& BColumn(pvdoTable.getFormationFactorColumn());
        std::vector<Scalar> invBColumn(BColumn.size());
        for (unsigned i = 0; i < invBColumn.size(); ++i)
            invBColumn[i] = 1.0 / BColumn[i];

        inverseOilB_[regionIdx].setXYArrays(pvdoTable.numRows(),
                                            pvdoTable.getPressureColumn(),
                                            invBColumn);
        oilMu_[regionIdx].setXYArrays(pvdoTable.numRows(),
                                      pvdoTable.getPressureColumn(),
                                      pvdoTable.getViscosityColumn());
    }

    initEnd();
}
#endif

template<class Scalar>
void DeadOilPvt<Scalar>::setNumRegions(size_t numRegions)
{
    oilReferenceDensity_.resize(numRegions);
    inverseOilB_.resize(numRegions);
    inverseOilBMu_.resize(numRegions);
    oilMu_.resize(numRegions);
}

template<class Scalar>
void DeadOilPvt<Scalar>::initEnd()
{
    // calculate the final 2D functions which are used for interpolation.
    size_t numRegions = oilMu_.size();
    for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
        // calculate the table which stores the inverse of the product of the oil
        // formation volume factor and the oil viscosity
        const auto& oilMu = oilMu_[regionIdx];
        const auto& invOilB = inverseOilB_[regionIdx];
        assert(oilMu.numSamples() == invOilB.numSamples());

        std::vector<Scalar> invBMuColumn;
        std::vector<Scalar> pressureColumn;
        invBMuColumn.resize(oilMu.numSamples());
        pressureColumn.resize(oilMu.numSamples());

        for (unsigned pIdx = 0; pIdx < oilMu.numSamples(); ++pIdx) {
            pressureColumn[pIdx] = invOilB.xAt(pIdx);
            invBMuColumn[pIdx] = invOilB.valueAt(pIdx) / oilMu.valueAt(pIdx);
        }

        inverseOilBMu_[regionIdx].setXYArrays(pressureColumn.size(),
                                              pressureColumn,
                                              invBMuColumn);
    }
}

template class DeadOilPvt<double>;
template class DeadOilPvt<float>;

} // namespace Opm
