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
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtThermal.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>

namespace Opm {

template<class Scalar, bool enableBrine>
void WaterPvtThermal<Scalar,enableBrine>::
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

    enableThermalDensity_ = tables.WatDenT().size() > 0;
    enableJouleThomson_ = tables.WatJT().size() > 0;
    enableThermalViscosity_ = tables.hasTables("WATVISCT");
    enableInternalEnergy_ = tables.hasTables("SPECHEAT");

    unsigned numRegions = isothermalPvt_->numRegions();
    setNumRegions(numRegions);

    if (enableThermalDensity_) {
        const auto& watDenT = tables.WatDenT();

        assert(watDenT.size() == numRegions);
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& record = watDenT[regionIdx];

            watdentRefTemp_[regionIdx] = record.T0;
            watdentCT1_[regionIdx] = record.C1;
            watdentCT2_[regionIdx] = record.C2;
        }

        const auto& pvtwTables = tables.getPvtwTable();

        assert(pvtwTables.size() == numRegions);

        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            pvtwRefPress_[regionIdx] = pvtwTables[regionIdx].reference_pressure;
            pvtwRefB_[regionIdx] = pvtwTables[regionIdx].volume_factor;
        }
    }

    // Joule Thomson
    if (enableJouleThomson_) {
         const auto& watJT = tables.WatJT();

        assert(watJT.size() == numRegions);
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& record = watJT[regionIdx];

            watJTRefPres_[regionIdx] =  record.P0;
            watJTC_[regionIdx] = record.C1;
        }
    }

    if (enableThermalViscosity_) {
        if (tables.getViscrefTable().empty())
            throw std::runtime_error("VISCREF is required when WATVISCT is present");

        const auto& watvisctTables = tables.getWatvisctTables();
        const auto& viscrefTables = tables.getViscrefTable();

        const auto& pvtwTables = tables.getPvtwTable();

        assert(pvtwTables.size() == numRegions);
        assert(watvisctTables.size() == numRegions);
        assert(viscrefTables.size() == numRegions);

        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            const auto& T = watvisctTables[regionIdx].getColumn("Temperature").vectorCopy();
            const auto& mu = watvisctTables[regionIdx].getColumn("Viscosity").vectorCopy();
            watvisctCurves_[regionIdx].setXYContainers(T, mu);

            viscrefPress_[regionIdx] = viscrefTables[regionIdx].reference_pressure;
        }

        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            pvtwViscosity_[regionIdx] = pvtwTables[regionIdx].viscosity;
            pvtwViscosibility_[regionIdx] = pvtwTables[regionIdx].viscosibility;
        }
    }

    if (enableInternalEnergy_) {
        // the specific internal energy of liquid water. be aware that ecl only specifies the heat capacity
        // (via the SPECHEAT keyword) and we need to integrate it ourselfs to get the
        // internal energy
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            const auto& specHeatTable = tables.getSpecheatTables()[regionIdx];
            const auto& temperatureColumn = specHeatTable.getColumn("TEMPERATURE");
            const auto& cvWaterColumn = specHeatTable.getColumn("CV_WATER");

            std::vector<double> uSamples(temperatureColumn.size());

            Scalar u = temperatureColumn[0]*cvWaterColumn[0];
            for (size_t i = 0;; ++i) {
                uSamples[i] = u;

                if (i >= temperatureColumn.size() - 1)
                    break;

                // integrate to the heat capacity from the current sampling point to the next
                // one. this leads to a quadratic polynomial.
                Scalar c_v0 = cvWaterColumn[i];
                Scalar c_v1 = cvWaterColumn[i + 1];
                Scalar T0 = temperatureColumn[i];
                Scalar T1 = temperatureColumn[i + 1];
                u += 0.5*(c_v0 + c_v1)*(T1 - T0);
            }

            internalEnergyCurves_[regionIdx].setXYContainers(temperatureColumn.vectorCopy(), uSamples);
        }
    }
}

template class WaterPvtThermal<double,false>;
template class WaterPvtThermal<double,true>;
template class WaterPvtThermal<float,false>;
template class WaterPvtThermal<float,true>;

} // namespace Opm
