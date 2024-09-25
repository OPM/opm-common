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

#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
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

    unsigned regions = isothermalPvt_->numRegions();
    setNumRegions(regions);

    if (enableThermalDensity_) {
        const auto& watDenT = tables.WatDenT();

        if (watDenT.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. WATDENT: {}, numRegions: {}\n",
                                  watDenT.size(), regions));
        }

        for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
            const auto& record = watDenT[regionIdx];

            watdentRefTemp_[regionIdx] = record.T0;
            watdentCT1_[regionIdx] = record.C1;
            watdentCT2_[regionIdx] = record.C2;
        }

        const auto& pvtwTables = tables.getPvtwTable();

        if (pvtwTables.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. PVTW: {}, numRegions: {}\n",
                                  pvtwTables.size(), regions));
        }

        for (unsigned regionIdx = 0; regionIdx < regions; ++ regionIdx) {
            pvtwRefPress_[regionIdx] = pvtwTables[regionIdx].reference_pressure;
            pvtwRefB_[regionIdx] = pvtwTables[regionIdx].volume_factor;
        }
    }

    // Joule Thomson
    if (enableJouleThomson_) {
         const auto& watJT = tables.WatJT();

        if (watJT.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. WATJT: {}, numRegions: {}\n",
                                  watJT.size(), regions));
        }
        for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
            const auto& record = watJT[regionIdx];

            watJTRefPres_[regionIdx] =  record.P0;
            watJTC_[regionIdx] = record.C1;
        }
    }

    if (enableThermalViscosity_) {
        if (tables.getViscrefTable().empty())
            OPM_THROW(std::runtime_error, "VISCREF is required when WATVISCT is present");

        const auto& watvisctTables = tables.getWatvisctTables();
        const auto& viscrefTables = tables.getViscrefTable();

        const auto& pvtwTables = tables.getPvtwTable();

        if (pvtwTables.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. PVTW: {}, numRegions: {}\n",
                                  pvtwTables.size(), regions));
        }
        if (watvisctTables.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. WATVISCT: {}, numRegions: {}\n",
                                  watvisctTables.size(), regions));
        }
        if (viscrefTables.size() != regions) {
            OPM_THROW(std::runtime_error,
                      fmt::format("Table sizes mismatch. VISCREF: {}, numRegions: {}\n",
                                  viscrefTables.size(), regions));
        }

        for (unsigned regionIdx = 0; regionIdx < regions; ++ regionIdx) {
            const auto& T = watvisctTables[regionIdx].getColumn("Temperature").vectorCopy();
            const auto& mu = watvisctTables[regionIdx].getColumn("Viscosity").vectorCopy();
            watvisctCurves_[regionIdx].setXYContainers(T, mu);

            viscrefPress_[regionIdx] = viscrefTables[regionIdx].reference_pressure;
        }

        for (unsigned regionIdx = 0; regionIdx < regions; ++ regionIdx) {
            pvtwViscosity_[regionIdx] = pvtwTables[regionIdx].viscosity;
            pvtwViscosibility_[regionIdx] = pvtwTables[regionIdx].viscosibility;
        }
    }

    if (enableInternalEnergy_) {
        // the specific internal energy of liquid water. be aware that ecl only specifies the heat capacity
        // (via the SPECHEAT keyword) and we need to integrate it ourselfs to get the
        // internal energy
        for (unsigned regionIdx = 0; regionIdx < regions; ++regionIdx) {
            const auto& specHeatTable = tables.getSpecheatTables()[regionIdx];
            const auto& temperatureColumn = specHeatTable.getColumn("TEMPERATURE");
            const auto& cvWaterColumn = specHeatTable.getColumn("CV_WATER");

            std::vector<double> uSamples(temperatureColumn.size());

            Scalar u = temperatureColumn[0]*cvWaterColumn[0];
            for (std::size_t i = 0;; ++i) {
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
#endif

template<class Scalar, bool enableBrine>
void WaterPvtThermal<Scalar,enableBrine>::
setNumRegions(std::size_t numRegions)
{
    pvtwRefPress_.resize(numRegions);
    pvtwRefB_.resize(numRegions);
    pvtwCompressibility_.resize(numRegions);
    pvtwViscosity_.resize(numRegions);
    pvtwViscosibility_.resize(numRegions);
    viscrefPress_.resize(numRegions);
    watvisctCurves_.resize(numRegions);
    watdentRefTemp_.resize(numRegions);
    watdentCT1_.resize(numRegions);
    watdentCT2_.resize(numRegions);
    watJTRefPres_.resize(numRegions);
    watJTC_.resize(numRegions);
    internalEnergyCurves_.resize(numRegions);
    hVap_.resize(numRegions,0.0);
}

template<class Scalar, bool enableBrine>
bool WaterPvtThermal<Scalar,enableBrine>::
operator==(const WaterPvtThermal<Scalar, enableBrine>& data) const
{
    if (isothermalPvt_ && !data.isothermalPvt_) {
        return false;
    }
    if (!isothermalPvt_ && data.isothermalPvt_) {
        return false;
    }

    return this->viscrefPress() == data.viscrefPress() &&
           this->watdentRefTemp() == data.watdentRefTemp() &&
           this->watdentCT1() == data.watdentCT1() &&
           this->watdentCT2() == data.watdentCT2() &&
           this->watJTRefPres() == data.watJTRefPres() &&
           this->watJTC() == data.watJTC() &&
           this->pvtwRefPress() == data.pvtwRefPress() &&
           this->pvtwRefB() == data.pvtwRefB() &&
           this->pvtwCompressibility() == data.pvtwCompressibility() &&
           this->pvtwViscosity() == data.pvtwViscosity() &&
           this->pvtwViscosibility() == data.pvtwViscosibility() &&
           this->watvisctCurves() == data.watvisctCurves() &&
           this->internalEnergyCurves() == data.internalEnergyCurves() &&
           this->enableThermalDensity() == data.enableThermalDensity() &&
           this->enableJouleThomson() == data.enableJouleThomson() &&
           this->enableThermalViscosity() == data.enableThermalViscosity() &&
           this->enableInternalEnergy() == data.enableInternalEnergy();
}

template<class Scalar, bool enableBrine>
WaterPvtThermal<Scalar, enableBrine>&
WaterPvtThermal<Scalar,enableBrine>::
operator=(const WaterPvtThermal<Scalar, enableBrine>& data)
{
    if (data.isothermalPvt_) {
        isothermalPvt_ = new IsothermalPvt(*data.isothermalPvt_);
    }
    else {
        isothermalPvt_ = nullptr;
    }
    viscrefPress_ = data.viscrefPress_;
    watdentRefTemp_ = data.watdentRefTemp_;
    watdentCT1_ = data.watdentCT1_;
    watdentCT2_ = data.watdentCT2_;
    watJTRefPres_ =  data.watJTRefPres_;
    watJTC_ =  data.watJTC_;
    pvtwRefPress_ = data.pvtwRefPress_;
    pvtwRefB_ = data.pvtwRefB_;
    pvtwCompressibility_ = data.pvtwCompressibility_;
    pvtwViscosity_ = data.pvtwViscosity_;
    pvtwViscosibility_ = data.pvtwViscosibility_;
    watvisctCurves_ = data.watvisctCurves_;
    internalEnergyCurves_ = data.internalEnergyCurves_;
    enableThermalDensity_ = data.enableThermalDensity_;
    enableJouleThomson_ = data.enableJouleThomson_;
    enableThermalViscosity_ = data.enableThermalViscosity_;
    enableInternalEnergy_ = data.enableInternalEnergy_;

    return *this;
}

template class WaterPvtThermal<double,false>;
template class WaterPvtThermal<double,true>;
template class WaterPvtThermal<float,false>;
template class WaterPvtThermal<float,true>;

} // namespace Opm
