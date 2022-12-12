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
#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

namespace Opm {

template<class Scalar>
void BrineCo2Pvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    if (!eclState.getTableManager().getDensityTable().empty()) {
        OpmLog::warning("CO2STOR is enabled but DENSITY is in the deck. \n"
                        "The surface density is computed based on CO2-BRINE "
                        "PVT at standard conditions (STCOND) and DENSITY is ignored");
    }

    if (eclState.getTableManager().hasTables("PVDO") ||
        !eclState.getTableManager().getPvtoTables().empty()) {
        OpmLog::warning("CO2STOR is enabled but PVDO or PVTO is in the deck.\n"
                        "BRINE PVT properties are computed based on the Hu et al. "
                        "pvt model and PVDO/PVTO input is ignored.");
    }

    setEnableDissolvedGas(eclState.getSimulationConfig().hasDISGAS());

    // We only supported single pvt region for the co2-brine module
    size_t numRegions = 1;
    setNumRegions(numRegions);
    size_t regionIdx = 0;
    // Currently we only support constant salinity
    const Scalar molality = eclState.getTableManager().salinity(); // mol/kg
    const Scalar MmNaCl = 58e-3; // molar mass of NaCl [kg/mol]
    // convert to mass fraction
    Brine::salinity = 1 / ( 1 + 1 / (molality*MmNaCl)); //
    salinity_[regionIdx] = Brine::salinity;
    // set the surface conditions using the STCOND keyword
    Scalar T_ref = eclState.getTableManager().stCond().temperature;
    Scalar P_ref = eclState.getTableManager().stCond().pressure;

    brineReferenceDensity_[regionIdx] = Brine::liquidDensity(T_ref, P_ref, extrapolate);
    co2ReferenceDensity_[regionIdx] = CO2::gasDensity(T_ref, P_ref, extrapolate);
}

template class BrineCo2Pvt<double>;
template class BrineCo2Pvt<float>;

} // namespace Opm
