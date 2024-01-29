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
#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

namespace Opm {

template<class Scalar>
void BrineCo2Pvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{

    bool co2sol = eclState.runspec().co2Sol();


    if (!co2sol && (eclState.getTableManager().hasTables("PVDO") ||
        !eclState.getTableManager().getPvtoTables().empty())) {
        OpmLog::warning("CO2STORE is enabled but PVDO or PVTO is in the deck.\n"
                        "BRINE PVT properties are computed based on the Hu et al. "
                        "pvt model and PVDO/PVTO input is ignored.");
    }
    if (eclState.getTableManager().hasTables("PVTW")) {
        OpmLog::warning("CO2STORE or CO2SOL is enabled but PVTW is in the deck.\n"
                        "BRINE PVT properties are computed based on the Hu et al. "
                        "pvt model and PVTW input is ignored.");
    }
    // enable co2 dissolution into brine for co2sol case with DISGASW
    // or co2store case with DISGASW or DISGAS
    bool co2sol_dis = co2sol && eclState.getSimulationConfig().hasDISGASW();
    bool co2storage_dis = eclState.runspec().co2Storage() && (eclState.getSimulationConfig().hasDISGASW() || eclState.getSimulationConfig().hasDISGAS());
    setEnableDissolvedGas(co2sol_dis || co2storage_dis);
    setEnableSaltConcentration(eclState.runspec().phases().active(Phase::BRINE));
    setActivityModelSalt(eclState.getTableManager().actco2s());
    // We only supported single pvt region for the co2-brine module
    size_t numRegions = 1;
    setNumRegions(numRegions);

    size_t regionIdx = 0;
    // Currently we only support constant salinity
    const Scalar molality = eclState.getTableManager().salinity(); // mol/kg
    const Scalar MmNaCl = 58.44e-3; // molar mass of NaCl [kg/mol]
    // convert to mass fraction
    salinity_[regionIdx] = 1 / ( 1 + 1 / (molality*MmNaCl));

    if (co2sol) {
        brineReferenceDensity_[regionIdx] = eclState.getTableManager().getDensityTable()[regionIdx].water;
        Scalar T_ref = eclState.getTableManager().stCond().temperature;
        Scalar P_ref = eclState.getTableManager().stCond().pressure;
        // Throw an error if STCOND is not (T, p) = (15.56 C, 1 atm) = (288.71 K, 1.01325e5 Pa)
        if (T_ref != Scalar(288.71) || P_ref != Scalar(1.01325e5)) {
            OPM_THROW(std::runtime_error, "CO2SOL can only be used with default values for STCOND!");
        }
        co2ReferenceDensity_[regionIdx] = CO2::gasDensity(T_ref, P_ref, extrapolate);
    } else if (!eclState.getTableManager().getDensityTable().empty()) {
        brineReferenceDensity_[regionIdx] = eclState.getTableManager().getDensityTable()[regionIdx].water;
        co2ReferenceDensity_[regionIdx] = eclState.getTableManager().getDensityTable()[regionIdx].gas;
    } else {
        Scalar T_ref = eclState.getTableManager().stCond().temperature;
        Scalar P_ref = eclState.getTableManager().stCond().pressure;
        // Throw an error if STCOND is not (T, p) = (15.56 C, 1 atm) = (288.71 K, 1.01325e5 Pa)
        if (T_ref != Scalar(288.71) || P_ref != Scalar(1.01325e5)) {
            OPM_THROW(std::runtime_error, "CO2STORE can only be used with default values for STCOND!");
        }
        co2ReferenceDensity_[regionIdx] = CO2::gasDensity(T_ref, P_ref, extrapolate);
        brineReferenceDensity_[regionIdx] = Brine::liquidDensity(T_ref, P_ref, salinity_[regionIdx], extrapolate);
    }

}

template class BrineCo2Pvt<double>;
template class BrineCo2Pvt<float>;

} // namespace Opm
