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
#include <opm/material/fluidsystems/blackoilpvt/SolventPvt.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdsTable.hpp>

namespace Opm {

template<class Scalar>
void SolventPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& pvdsTables = eclState.getTableManager().getPvdsTables();
    const auto& sdensityTables = eclState.getTableManager().getSolventDensityTables();

    assert(pvdsTables.size() == sdensityTables.size());

    size_t numRegions = pvdsTables.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefS = sdensityTables[regionIdx].getSolventDensityColumn().front();

        setReferenceDensity(regionIdx, rhoRefS);

        const auto& pvdsTable = pvdsTables.getTable<PvdsTable>(regionIdx);

        // say 99.97% of all time: "premature optimization is the root of all
        // evil". Eclipse does this "optimization" for apparently no good reason!
        std::vector<Scalar> invB(pvdsTable.numRows());
        const auto& Bg = pvdsTable.getFormationFactorColumn();
        for (unsigned i = 0; i < Bg.size(); ++i) {
            invB[i] = 1.0 / Bg[i];
        }

        size_t numSamples = invB.size();
        inverseSolventB_[regionIdx].setXYArrays(numSamples, pvdsTable.getPressureColumn(), invB);
        solventMu_[regionIdx].setXYArrays(numSamples, pvdsTable.getPressureColumn(), pvdsTable.getViscosityColumn());
    }

    initEnd();
}

template class SolventPvt<double>;
template class SolventPvt<float>;

} // namespace Opm
