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
#include <opm/material/fluidsystems/blackoilpvt/GasPvtThermal.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/material/fluidsystems/blackoilpvt/GasPvtMultiplexer.hpp>

namespace Opm {

template<class Scalar>
void GasPvtThermal<Scalar>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    //////
    // initialize the isothermal part
    //////
    isothermalPvt_ = new IsothermalPvt;
    isothermalPvt_->initFromState(eclState, schedule);

    //////
    // initialize the thermal part
    //////
    const auto& tables = eclState.getTableManager();

    enableThermalDensity_ = tables.GasDenT().size() > 0;
    enableJouleThomson_ = tables.GasJT().size() > 0;
    enableThermalViscosity_ = tables.hasTables("GASVISCT");
    enableInternalEnergy_ = tables.hasTables("SPECHEAT");

    unsigned numRegions = isothermalPvt_->numRegions();
    setNumRegions(numRegions);

    // viscosity
    if (enableThermalViscosity_) {
        const auto& gasvisctTables = tables.getGasvisctTables();
        auto gasCompIdx = tables.gas_comp_index();
        std::string gasvisctColumnName = "Viscosity" + std::to_string(static_cast<long long>(gasCompIdx));

        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& T = gasvisctTables[regionIdx].getColumn("Temperature").vectorCopy();
            const auto& mu = gasvisctTables[regionIdx].getColumn(gasvisctColumnName).vectorCopy();
            gasvisctCurves_[regionIdx].setXYContainers(T, mu);
        }
    }

    // temperature dependence of gas density
    if (enableThermalDensity_) {
        const auto& gasDenT = tables.GasDenT();

        assert(gasDenT.size() == numRegions);
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& record = gasDenT[regionIdx];

            gasdentRefTemp_[regionIdx] = record.T0;
            gasdentCT1_[regionIdx] = record.C1;
            gasdentCT2_[regionIdx] = record.C2;
        }
    }

    // Joule Thomson
    if (enableJouleThomson_) {
        const auto& gasJT = tables.GasJT();

        assert(gasJT.size() == numRegions);
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& record = gasJT[regionIdx];

            gasJTRefPres_[regionIdx] =  record.P0;
            gasJTC_[regionIdx] = record.C1;
        }

        const auto& densityTable = eclState.getTableManager().getDensityTable();

        assert(densityTable.size() == numRegions);
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
             rhoRefO_[regionIdx] = densityTable[regionIdx].oil;
        }
    }

    if (enableInternalEnergy_) {
        // the specific internal energy of gas. be aware that ecl only specifies the heat capacity
        // (via the SPECHEAT keyword) and we need to integrate it ourselfs to get the
        // internal energy
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& specHeatTable = tables.getSpecheatTables()[regionIdx];
            const auto& temperatureColumn = specHeatTable.getColumn("TEMPERATURE");
            const auto& cvGasColumn = specHeatTable.getColumn("CV_GAS");

            std::vector<double> uSamples(temperatureColumn.size());

            // the specific enthalpy of vaporization. since ECL does not seem to
            // feature a proper way to specify this quantity, we use the value for
            // methane. A proper model would also need to consider the enthalpy
            // change due to dissolution, i.e. the enthalpies of the gas and oil
            // phases should depend on the phase composition
            const Scalar hVap = 480.6e3; // [J / kg]

            Scalar u = temperatureColumn[0]*cvGasColumn[0] + hVap;
            for (size_t i = 0;; ++i) {
                uSamples[i] = u;

                if (i >= temperatureColumn.size() - 1)
                    break;

                // integrate to the heat capacity from the current sampling point to the next
                // one. this leads to a quadratic polynomial.
                Scalar c_v0 = cvGasColumn[i];
                Scalar c_v1 = cvGasColumn[i + 1];
                Scalar T0 = temperatureColumn[i];
                Scalar T1 = temperatureColumn[i + 1];
                u += 0.5*(c_v0 + c_v1)*(T1 - T0);
            }

            internalEnergyCurves_[regionIdx].setXYContainers(temperatureColumn.vectorCopy(), uSamples);
        }
    }
}

template class GasPvtThermal<double>;
template class GasPvtThermal<float>;

} // namespace Opm
