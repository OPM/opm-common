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
#include <opm/material/thermal/EclThermalLawManager.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/material/fluidsystems/BlackOilDefaultFluidSystemIndices.hpp>
#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>

#include <cassert>
#include <stdexcept>

namespace Opm {

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initParamsForElements(const EclipseState& eclState, size_t numElems,
                      const std::function<std::vector<double>(const FieldPropsManager&, const std::string&)>& fieldPropsDoubleOnLeafAssigner,
                      const std::function<std::vector<unsigned int>(const FieldPropsManager&, const std::string&, bool)>&
                      fieldPropsIntOnLeafAssigner)
{
    const auto& fp = eclState.fieldProps();
    const auto& tableManager = eclState.getTableManager();
    bool has_heatcr = fp.has_double("HEATCR");
    bool has_thconr = fp.has_double("THCONR");
    bool has_thc = fp.has_double("THCROCK")   ||
                   fp.has_double("THCOIL")    ||
                   fp.has_double("THCGAS")    ||
                   fp.has_double("THCWATER");

    if (has_heatcr)
        initHeatcr_(eclState, numElems, fieldPropsDoubleOnLeafAssigner);
    else if (tableManager.hasTables("SPECROCK"))
        initSpecrock_(eclState, fieldPropsIntOnLeafAssigner);
    else
        initNullRockEnergy_();

    if (has_thconr)
        initThconr_(eclState, numElems, fieldPropsDoubleOnLeafAssigner);
    else if (has_thc)
        initThc_(eclState, numElems, fieldPropsDoubleOnLeafAssigner);
    else
        initNullCond_();
}

template<class Scalar, class FluidSystem>
const typename EclThermalLawManager<Scalar,FluidSystem>::SolidEnergyLawParams&
EclThermalLawManager<Scalar,FluidSystem>::
solidEnergyLawParams(unsigned elemIdx) const
{
    switch (solidEnergyApproach_) {
    case EclSolidEnergyApproach::Heatcr:
        assert(elemIdx <  solidEnergyLawParams_.size());
        return solidEnergyLawParams_[elemIdx];

    case EclSolidEnergyApproach::Specrock:
    {
        assert(elemIdx <  elemToSatnumIdx_.size());
        unsigned satnumIdx = elemToSatnumIdx_[elemIdx];
        assert(satnumIdx <  solidEnergyLawParams_.size());
        return solidEnergyLawParams_[satnumIdx];
    }

    case EclSolidEnergyApproach::Null:
        return solidEnergyLawParams_[0];

    default:
        OPM_THROW(std::runtime_error,
                  "Attempting to retrieve solid energy storage parameters "
                  "without a known approach being defined by the deck.");
    }
}

template<class Scalar, class FluidSystem>
const typename EclThermalLawManager<Scalar,FluidSystem>::ThermalConductionLawParams&
EclThermalLawManager<Scalar,FluidSystem>::
thermalConductionLawParams(unsigned elemIdx) const
{
    switch (thermalConductivityApproach_) {
    case EclThermalConductionApproach::Thconr:
    case EclThermalConductionApproach::Thc:
        assert(elemIdx <  thermalConductionLawParams_.size());
        return thermalConductionLawParams_[elemIdx];

    case EclThermalConductionApproach::Null:
        return thermalConductionLawParams_[0];

    default:
        OPM_THROW(std::runtime_error,
                  "Attempting to retrieve thermal conduction parameters without "
                  "a known approach being defined by the deck.");
    }
}

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initHeatcr_(const EclipseState& eclState, size_t numElems,
            const std::function<std::vector<double>(const FieldPropsManager&, const std::string&)>& fieldPropsDoubleOnLeafAssigner)
{
    solidEnergyApproach_ = EclSolidEnergyApproach::Heatcr;
    // actually the value of the reference temperature does not matter for energy
    // conservation. We set it anyway to faciliate comparisons with ECL
    HeatcrLawParams::setReferenceTemperature(FluidSystem::surfaceTemperature);

    const std::vector<double>& heatcrData = fieldPropsDoubleOnLeafAssigner(eclState.fieldProps(), "HEATCR");
    const std::vector<double>& heatcrtData = fieldPropsDoubleOnLeafAssigner(eclState.fieldProps(), "HEATCRT");
    solidEnergyLawParams_.resize(numElems);
    for (unsigned elemIdx = 0; elemIdx < numElems; ++elemIdx) {
        auto& elemParam = solidEnergyLawParams_[elemIdx];
        elemParam.setSolidEnergyApproach(EclSolidEnergyApproach::Heatcr);
        auto& heatcrElemParams = elemParam.template getRealParams<EclSolidEnergyApproach::Heatcr>();

        heatcrElemParams.setReferenceRockHeatCapacity(heatcrData[elemIdx]);
        heatcrElemParams.setDRockHeatCapacity_dT(heatcrtData[elemIdx]);
        heatcrElemParams.finalize();
        elemParam.finalize();
    }
}

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initSpecrock_(const EclipseState& eclState,
              const std::function<std::vector<unsigned int>(const FieldPropsManager&, const std::string&, bool)>& fieldPropsIntOnLeafAssigner)
{
    solidEnergyApproach_ = EclSolidEnergyApproach::Specrock;

    // initialize the element index -> SATNUM index mapping
    elemToSatnumIdx_ = fieldPropsIntOnLeafAssigner(eclState.fieldProps(),"SATNUM", true /*needs translation*/);
    // SATNUM Data contains Fortran-style indices, i.e., they start with 1 instead of 0!

    // internalize the SPECROCK table
    unsigned numSatRegions = eclState.runspec().tabdims().getNumSatTables();
    const auto& tableManager = eclState.getTableManager();
    solidEnergyLawParams_.resize(numSatRegions);
    for (unsigned satnumIdx = 0; satnumIdx < numSatRegions; ++satnumIdx) {
        const auto& specrockTable = tableManager.getSpecrockTables()[satnumIdx];

        auto& multiplexerParams = solidEnergyLawParams_[satnumIdx];

        multiplexerParams.setSolidEnergyApproach(EclSolidEnergyApproach::Specrock);

        auto& specrockParams = multiplexerParams.template getRealParams<EclSolidEnergyApproach::Specrock>();
        const auto& temperatureColumn = specrockTable.getColumn("TEMPERATURE");
        const auto& cvRockColumn = specrockTable.getColumn("CV_ROCK");
        specrockParams.setHeatCapacities(temperatureColumn, cvRockColumn);
        specrockParams.finalize();

        multiplexerParams.finalize();
    }
}

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initNullRockEnergy_()
{
    solidEnergyApproach_ = EclSolidEnergyApproach::Null;

    solidEnergyLawParams_.resize(1);
    solidEnergyLawParams_[0].finalize();
}

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initThconr_(const EclipseState& eclState, size_t numElems,
            const std::function<std::vector<double>(const FieldPropsManager&, const std::string&)>& fieldPropsDoubleOnLeafAssigner)
{
    thermalConductivityApproach_ = EclThermalConductionApproach::Thconr;

    const auto& fp = eclState.fieldProps();
    std::vector<double> thconrData;
    std::vector<double> thconsfData;
    if (fp.has_double("THCONR"))
        thconrData  = fieldPropsDoubleOnLeafAssigner(fp, "THCONR");

    if (fp.has_double("THCONSF"))
        thconsfData =  fieldPropsDoubleOnLeafAssigner(fp, "THCONSF");

    thermalConductionLawParams_.resize(numElems);
    for (unsigned elemIdx = 0; elemIdx < numElems; ++elemIdx) {
        auto& elemParams = thermalConductionLawParams_[elemIdx];
        elemParams.setThermalConductionApproach(EclThermalConductionApproach::Thconr);
        auto& thconrElemParams = elemParams.template getRealParams<EclThermalConductionApproach::Thconr>();

        double thconr = thconrData.empty()   ? 0.0 : thconrData[elemIdx];
        double thconsf = thconsfData.empty() ? 0.0 : thconsfData[elemIdx];
        thconrElemParams.setReferenceTotalThermalConductivity(thconr);
        thconrElemParams.setDTotalThermalConductivity_dSg(thconsf);

        thconrElemParams.finalize();
        elemParams.finalize();
    }
}

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initThc_(const EclipseState& eclState, size_t numElems,
         const std::function<std::vector<double>(const FieldPropsManager&, const std::string&)>& fieldPropsDoubleOnLeafAssigner)
{
    thermalConductivityApproach_ = EclThermalConductionApproach::Thc;

    const auto& fp = eclState.fieldProps();
    std::vector<double> thcrockData;
    std::vector<double> thcoilData;
    std::vector<double> thcgasData;
    std::vector<double> thcwaterData;

    if (fp.has_double("THCROCK"))
        thcrockData = fieldPropsDoubleOnLeafAssigner(fp, "THCROCK");

    if (fp.has_double("THCOIL"))
        thcoilData = fieldPropsDoubleOnLeafAssigner(fp, "THCOIL");

    if (fp.has_double("THCGAS"))
        thcgasData =  fieldPropsDoubleOnLeafAssigner(fp, "THCGAS");

    if (fp.has_double("THCWATER"))
        thcwaterData = fieldPropsDoubleOnLeafAssigner(fp, "THCWATER");

    const std::vector<double>& poroData = fieldPropsDoubleOnLeafAssigner(fp, "PORO");
    const std::vector<double>& ntgData = fieldPropsDoubleOnLeafAssigner(fp, "NTG");

    thermalConductionLawParams_.resize(numElems);
    for (unsigned elemIdx = 0; elemIdx < numElems; ++elemIdx) {
        auto& elemParams = thermalConductionLawParams_[elemIdx];
        elemParams.setThermalConductionApproach(EclThermalConductionApproach::Thc);
        auto& thcElemParams = elemParams.template getRealParams<EclThermalConductionApproach::Thc>();

        thcElemParams.setPorosity(poroData[elemIdx]*ntgData[elemIdx]);
        double thcrock = thcrockData.empty()    ? 0.0 : thcrockData[elemIdx];
        double thcoil = thcoilData.empty()      ? 0.0 : thcoilData[elemIdx];
        double thcgas = thcgasData.empty()      ? 0.0 : thcgasData[elemIdx];
        double thcwater = thcwaterData.empty()  ? 0.0 : thcwaterData[elemIdx];
        thcElemParams.setThcrock(thcrock);
        thcElemParams.setThcoil(thcoil);
        thcElemParams.setThcgas(thcgas);
        thcElemParams.setThcwater(thcwater);

        thcElemParams.finalize();
        elemParams.finalize();
    }
}

template<class Scalar, class FluidSystem>
void EclThermalLawManager<Scalar,FluidSystem>::
initNullCond_()
{
    thermalConductivityApproach_ = EclThermalConductionApproach::Null;

    thermalConductionLawParams_.resize(1);
    thermalConductionLawParams_[0].finalize();
}

using FS = BlackOilFluidSystem<double, BlackOilDefaultFluidSystemIndices>;
template class EclThermalLawManager<double,FS>;

} // namespace Opm
