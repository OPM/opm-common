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
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityWaterPvt.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar>
void ConstantCompressibilityWaterPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& pvtwTable = eclState.getTableManager().getPvtwTable();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    if (pvtwTable.size() != densityTable.size()) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Table sizes mismatch. PVTW: {}, DensityTable: {}\n",
                              pvtwTable.size(), densityTable.size()));
    }

    std::size_t regions = pvtwTable.size();
    setNumRegions(regions);

    for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
        waterReferenceDensity_[regionIdx] = densityTable[regionIdx].water;

        waterReferencePressure_[regionIdx] = pvtwTable[regionIdx].reference_pressure;
        waterReferenceFormationVolumeFactor_[regionIdx] = pvtwTable[regionIdx].volume_factor;
        waterCompressibility_[regionIdx] = pvtwTable[regionIdx].compressibility;
        waterViscosity_[regionIdx] = pvtwTable[regionIdx].viscosity;
        waterViscosibility_[regionIdx] = pvtwTable[regionIdx].viscosibility;
    }

    initEnd();
}
#endif

template<class Scalar>
void ConstantCompressibilityWaterPvt<Scalar>::
setNumRegions(std::size_t numRegions)
{
    waterReferenceDensity_.resize(numRegions);
    waterReferencePressure_.resize(numRegions);
    waterReferenceFormationVolumeFactor_.resize(numRegions);
    waterCompressibility_.resize(numRegions);
    waterViscosity_.resize(numRegions);
    waterViscosibility_.resize(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        setReferenceDensities(regionIdx, 650.0, 1.0, 1000.0);
        setReferenceFormationVolumeFactor(regionIdx, 1.0);
        setReferencePressure(regionIdx, 1e5);
    }
}

template class ConstantCompressibilityWaterPvt<double>;
template class ConstantCompressibilityWaterPvt<float>;

} // namespace Opm
