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
#include <opm/material/fluidsystems/blackoilpvt/ConstantRsDeadOilPvt.hpp>

#include <opm/common/ErrorMacros.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RsconstTable.hpp>
#endif

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar>
void ConstantRsDeadOilPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    const auto& pvdoTables = eclState.getTableManager().getPvdoTables();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    if (pvdoTables.size() != densityTable.size()) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Table sizes mismatch. PVDO: {}, DensityTable: {}\n",
                              pvdoTables.size(), densityTable.size()));
    }

    std::size_t regions = pvdoTables.size();
    setNumRegions(regions);

    // Check for RSCONST keyword - single global value
    const auto& rsConstTables = eclState.getTableManager().getRsconstTables();
    if (!rsConstTables.empty()) {
        // RSCONST has  Rs and Pb
        const auto& rsConst = rsConstTables[0];
       setConstantRs(rsConst.getColumn(0)[0]);
       setBubblePointPressure(rsConst.getColumn(1)[0]);
    }

    for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
        Scalar rhoRefO = densityTable[regionIdx].oil;
        Scalar rhoRefG = densityTable[regionIdx].gas;
        Scalar rhoRefW = densityTable[regionIdx].water;

        setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);

        const auto& pvdoTable = pvdoTables.getTable<PvdoTable>(regionIdx);

        // Set up Bo and μo from PVDO (Rs doesn't affect these)
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
void ConstantRsDeadOilPvt<Scalar>::setNumRegions(std::size_t numRegions)
{
    oilReferenceDensity_.resize(numRegions);
    gasReferenceDensity_.resize(numRegions);
    inverseOilB_.resize(numRegions);
    inverseOilBMu_.resize(numRegions);
    oilMu_.resize(numRegions);
}

template<class Scalar>
void ConstantRsDeadOilPvt<Scalar>::initEnd()
{
    // Calculate invBo/μo table for efficiency
    std::size_t regions = oilMu_.size();
    for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
        const auto& oilmu = oilMu_[regionIdx];
        const auto& invOilB = inverseOilB_[regionIdx];
        
        // Should have same number of samples
        if (oilmu.numSamples() != invOilB.numSamples()) {
            OPM_THROW(std::runtime_error,
                     fmt::format("Table sizes mismatch in region {}. Bo: {}, μo: {}\n",
                                 regionIdx, invOilB.numSamples(), oilmu.numSamples()));
        }

        std::vector<Scalar> invBMuColumn(oilmu.numSamples());
        std::vector<Scalar> pressureColumn(oilmu.numSamples());

        for (unsigned pIdx = 0; pIdx < oilmu.numSamples(); ++pIdx) {
            pressureColumn[pIdx] = invOilB.xAt(pIdx);
            invBMuColumn[pIdx] = invOilB.valueAt(pIdx) / oilmu.valueAt(pIdx);
        }

        inverseOilBMu_[regionIdx].setXYArrays(pressureColumn.size(),
                                              pressureColumn,
                                              invBMuColumn);
    }
}

template class ConstantRsDeadOilPvt<double>;
template class ConstantRsDeadOilPvt<float>;

} // namespace Opm
