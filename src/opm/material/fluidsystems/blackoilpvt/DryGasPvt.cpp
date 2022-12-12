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
#include <opm/material/fluidsystems/blackoilpvt/DryGasPvt.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdgTable.hpp>

namespace Opm {

template<class Scalar>
void DryGasPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& pvdgTables = eclState.getTableManager().getPvdgTables();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    assert(pvdgTables.size() == densityTable.size());

    size_t numRegions = pvdgTables.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefO = densityTable[regionIdx].oil;
        Scalar rhoRefG = densityTable[regionIdx].gas;
        Scalar rhoRefW = densityTable[regionIdx].water;

        setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);

        // determine the molar masses of the components
        constexpr Scalar p = 1.01325e5; // surface pressure, [Pa]
        constexpr Scalar T = 273.15 + 15.56; // surface temperature, [K]
        constexpr Scalar MO = 175e-3; // [kg/mol]
        Scalar MG = Constants<Scalar>::R*T*rhoRefG / p; // [kg/mol], consequence of the ideal gas law
        constexpr Scalar MW = 18.0e-3; // [kg/mol]
        // TODO (?): the molar mass of the components can possibly specified
        // explicitly in the deck.
        setMolarMasses(regionIdx, MO, MG, MW);

        const auto& pvdgTable = pvdgTables.getTable<PvdgTable>(regionIdx);

        // say 99.97% of all time: "premature optimization is the root of all
        // evil". Eclipse does this "optimization" for apparently no good reason!
        std::vector<Scalar> invB(pvdgTable.numRows());
        const auto& Bg = pvdgTable.getFormationFactorColumn();
        for (unsigned i = 0; i < Bg.size(); ++i) {
            invB[i] = 1.0 / Bg[i];
        }

        size_t numSamples = invB.size();
        inverseGasB_[regionIdx].setXYArrays(numSamples, pvdgTable.getPressureColumn(), invB);
        gasMu_[regionIdx].setXYArrays(numSamples, pvdgTable.getPressureColumn(), pvdgTable.getViscosityColumn());
    }

    initEnd();
}

template class DryGasPvt<double>;
template class DryGasPvt<float>;

} // namespace Opm
