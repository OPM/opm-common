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
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityBrinePvt.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtwsaltTable.hpp>

namespace Opm {

template<class Scalar>
void ConstantCompressibilityBrinePvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& tableManager = eclState.getTableManager();
    size_t numRegions = tableManager.getTabdims().getNumPVTTables();
    const auto& densityTable = tableManager.getDensityTable();

    formationVolumeTables_.resize(numRegions);
    compressibilityTables_.resize(numRegions);
    viscosityTables_.resize(numRegions);
    viscosibilityTables_.resize(numRegions);
    referencePressure_.resize(numRegions);

    const auto& pvtwsaltTables = tableManager.getPvtwSaltTables();
    if (!pvtwsaltTables.empty()) {
        assert(numRegions == pvtwsaltTables.size());
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& pvtwsaltTable = pvtwsaltTables[regionIdx];
            const auto& c = pvtwsaltTable.getSaltConcentrationColumn();

            const auto& B = pvtwsaltTable.getFormationVolumeFactorColumn();
            formationVolumeTables_[regionIdx].setXYContainers(c, B);

            const auto& compressibility = pvtwsaltTable.getCompressibilityColumn();
            compressibilityTables_[regionIdx].setXYContainers(c, compressibility);

            const auto& viscositytable = pvtwsaltTable.getViscosityColumn();
            viscosityTables_[regionIdx].setXYContainers(c, viscositytable);

            const auto& viscosibility = pvtwsaltTable.getViscosibilityColumn();
            viscosibilityTables_[regionIdx].setXYContainers(c, viscosibility);
            referencePressure_[regionIdx] = pvtwsaltTable.getReferencePressureValue();
        }
    }
    else {
        throw std::runtime_error("PVTWSALT must be specified in BRINE runs\n");
    }


    size_t numPvtwRegions = numRegions;
    setNumRegions(numPvtwRegions);

    for (unsigned regionIdx = 0; regionIdx < numPvtwRegions; ++regionIdx) {

        waterReferenceDensity_[regionIdx] = densityTable[regionIdx].water;
    }

    initEnd();
}

template class ConstantCompressibilityBrinePvt<double>;
template class ConstantCompressibilityBrinePvt<float>;

} // namespace Opm
