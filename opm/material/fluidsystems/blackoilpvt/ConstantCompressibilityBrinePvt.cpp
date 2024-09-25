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

#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtwsaltTable.hpp>

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar>
void ConstantCompressibilityBrinePvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& tableManager = eclState.getTableManager();
    std::size_t regions = tableManager.getTabdims().getNumPVTTables();
    const auto& densityTable = tableManager.getDensityTable();

    formationVolumeTables_.resize(regions);
    compressibilityTables_.resize(regions);
    viscosityTables_.resize(regions);
    viscosibilityTables_.resize(regions);
    referencePressure_.resize(regions);

    const auto& pvtwsaltTables = tableManager.getPvtwSaltTables();
    if (!pvtwsaltTables.empty()) {
        if (pvtwsaltTables.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. PVTWSALT: {}, NumRegions: {}\n",
                                  pvtwsaltTables.size(), regions));
        }
        for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
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
        OPM_THROW(std::runtime_error, "PVTWSALT must be specified in BRINE runs\n");
    }


    std::size_t numPvtwRegions = regions;
    setNumRegions(numPvtwRegions);

    for (unsigned regionIdx = 0; regionIdx < numPvtwRegions; ++regionIdx) {

        waterReferenceDensity_[regionIdx] = densityTable[regionIdx].water;
    }

    initEnd();
}
#endif

template<class Scalar>
void ConstantCompressibilityBrinePvt<Scalar>::
setNumRegions(std::size_t numRegions)
{
    waterReferenceDensity_.resize(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        setReferenceDensities(regionIdx, 650.0, 1.0, 1000.0);
    }
}

template class ConstantCompressibilityBrinePvt<double>;
template class ConstantCompressibilityBrinePvt<float>;

} // namespace Opm
