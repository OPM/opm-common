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
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityOilPvt.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <cassert>

namespace Opm {

template<class Scalar>
void ConstantCompressibilityOilPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& pvcdoTable = eclState.getTableManager().getPvcdoTable();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    assert(pvcdoTable.size() == densityTable.size());

    size_t numRegions = pvcdoTable.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefO = densityTable[regionIdx].oil;
        Scalar rhoRefG = densityTable[regionIdx].gas;
        Scalar rhoRefW = densityTable[regionIdx].water;

        setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);

        oilReferencePressure_[regionIdx] = pvcdoTable[regionIdx].reference_pressure;
        oilReferenceFormationVolumeFactor_[regionIdx] = pvcdoTable[regionIdx].volume_factor;
        oilCompressibility_[regionIdx] = pvcdoTable[regionIdx].compressibility;
        oilViscosity_[regionIdx] = pvcdoTable[regionIdx].viscosity;
        oilViscosibility_[regionIdx] = pvcdoTable[regionIdx].viscosibility;
    }

    initEnd();
}

template class ConstantCompressibilityOilPvt<double>;
template class ConstantCompressibilityOilPvt<float>;

} // namespace Opm
