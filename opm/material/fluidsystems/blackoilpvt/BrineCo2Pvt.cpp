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

template<class Scalar, class Params>
BrineCo2Pvt<Scalar, Params>::
BrineCo2Pvt(const std::vector<Scalar>& salinity,
            int activityModel,
            int thermalMixingModelSalt,
            int thermalMixingModelLiquid,
            Scalar T_ref,
            Scalar P_ref)
    : salinity_(salinity)
{
    // Throw an error if reference state is not (T, p) = (15.56 C, 1 atm) = (288.71 K, 1.01325e5 Pa)
    if (T_ref != Scalar(288.71) || P_ref != Scalar(1.01325e5)) {
        OPM_THROW(std::runtime_error,
            "BrineCo2Pvt class can only be used with default reference state (T, P) = (288.71 K, 1.01325e5 Pa)!");
    }
    setActivityModelSalt(activityModel);
    setThermalMixingModel(thermalMixingModelSalt, thermalMixingModelLiquid);
    int num_regions =  salinity_.size();
    co2ReferenceDensity_.resize(num_regions);
    brineReferenceDensity_.resize(num_regions);

    for (int i = 0; i < num_regions; ++i) {
        co2ReferenceDensity_[i] = CO2::gasDensity(co2Tables_, T_ref, P_ref, true);
        brineReferenceDensity_[i] = Brine::liquidDensity(T_ref, P_ref, salinity_[i], true);
    }
}

#if HAVE_ECL_INPUT
template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    bool co2sol = eclState.runspec().co2Sol();
    if (!co2sol && !eclState.getTableManager().getDensityTable().empty()) {
        OpmLog::warning("CO2STORE is enabled but DENSITY is in the deck. \n"
                        "The surface density is computed based on CO2-BRINE "
                        "PVT at standard conditions (STCOND) and DENSITY is ignored");
    }

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
    setActivityModelSalt(eclState.getCo2StoreConfig().actco2s());
    saltMixType_ = eclState.getCo2StoreConfig().brine_type;
    liquidMixType_ = eclState.getCo2StoreConfig().liquid_type;

    // set the surface conditions using the STCOND keyword
    Scalar T_ref = eclState.getTableManager().stCond().temperature;
    Scalar P_ref = eclState.getTableManager().stCond().pressure;

    co2Tables_ = Params();

    // Throw an error if STCOND is not (T, p) = (15.56 C, 1 atm) = (288.71 K, 1.01325e5 Pa)
    if (T_ref != Scalar(288.71) || P_ref != Scalar(1.01325e5)) {
        OPM_THROW(std::runtime_error, "CO2STORE can only be used with default values for STCOND!");
    }
   
    // Check for Ezrokhi tables DENAQA and VISCAQA
    setEzrokhiDenCoeff(eclState.getCo2StoreConfig().getDenaqaTables());
    setEzrokhiViscCoeff(eclState.getCo2StoreConfig().getViscaqaTables());
    std::string ezrokhi_msg;
    if (enableEzrokhiDensity_) {
        ezrokhi_msg = "\nEzrokhi density coefficients : \n\tNaCl = " 
                      + std::to_string(ezrokhiDenNaClCoeff_[0]) + " " + std::to_string(ezrokhiDenNaClCoeff_[1]) 
                      + " " + std::to_string(ezrokhiDenNaClCoeff_[2])
                      + "\n\tCO2 = " + std::to_string(ezrokhiDenCo2Coeff_[0]) + " " + std::to_string(ezrokhiDenCo2Coeff_[1]) 
                      + " " + std::to_string(ezrokhiDenCo2Coeff_[2]);
    }
    else {
        ezrokhi_msg = "";
    }
    if (enableEzrokhiViscosity_) {
        ezrokhi_msg += "\nEzrokhi viscosity coefficients : \n\tNaCl = " 
                       + std::to_string(ezrokhiViscNaClCoeff_[0]) + " " + std::to_string(ezrokhiViscNaClCoeff_[1]) 
                       + " " + std::to_string(ezrokhiViscNaClCoeff_[2]);
    }
    
    std::size_t regions = eclState.runspec().tabdims().getNumPVTTables();
    setNumRegions(regions);
    for (std::size_t regionIdx = 0; regionIdx < regions; ++regionIdx) {
        // Currently we only support constant salinity converted to mass fraction
        salinity_[regionIdx] = eclState.getCo2StoreConfig().salinity();
        if (enableEzrokhiDensity_) {
            const Scalar& rho_pure = H2O::liquidDensity(T_ref, P_ref, extrapolate);
            const Scalar& nacl_exponent = ezrokhiExponent_(T_ref, ezrokhiDenNaClCoeff_);
            brineReferenceDensity_[regionIdx] = rho_pure * pow(10.0, nacl_exponent * salinity_[regionIdx]);
        }
        else {
            brineReferenceDensity_[regionIdx] = Brine::liquidDensity(T_ref, P_ref, salinity_[regionIdx], extrapolate);
        }
        co2ReferenceDensity_[regionIdx] = CO2::gasDensity(co2Tables_, T_ref, P_ref, extrapolate);
    }

    // The reference densities are the same across regions. Only output info for region 0
    OpmLog::info("CO2STORE/CO2SOL is enabled. \n The surface density of CO2 is  " + std::to_string(co2ReferenceDensity_[0])
            + "kg/m3 \n The surface density of Brine is  " + std::to_string(brineReferenceDensity_[0])
            + "kg/m3"
            + "\n The surface densities are computed using the reference pressure ( " + std::to_string(P_ref) 
            + "Pa) and the reference temperature (" +  std::to_string(T_ref) + "K)."
            + ezrokhi_msg
            );
}
#endif

template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
setNumRegions(std::size_t numRegions)
{
    brineReferenceDensity_.resize(numRegions);
    co2ReferenceDensity_.resize(numRegions);
    salinity_.resize(numRegions);
}

template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
setReferenceDensities(unsigned regionIdx,
                      Scalar rhoRefBrine,
                      Scalar rhoRefCO2,
                      Scalar /*rhoRefWater*/)
{
    brineReferenceDensity_[regionIdx] = rhoRefBrine;
    co2ReferenceDensity_[regionIdx] = rhoRefCO2;
}

template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
setActivityModelSalt(int activityModel)
{
    switch (activityModel) {
    case 1:
    case 2:
    case 3:  activityModel_ = activityModel; break;
    default: OPM_THROW(std::runtime_error, "The salt activity model options are 1, 2 or 3");
    }
}

template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
setThermalMixingModel(int thermalMixingModelSalt, int thermalMixingModelLiquid)
{
    switch (thermalMixingModelSalt) {
    case 0: saltMixType_ = Co2StoreConfig::SaltMixingType::NONE; break;
    case 1: saltMixType_ = Co2StoreConfig::SaltMixingType::MICHAELIDES; break;
    default: OPM_THROW(std::runtime_error, "The thermal mixing model option for salt are 0 or 1");
    }

    switch (thermalMixingModelLiquid) {
    case 0: liquidMixType_ = Co2StoreConfig::LiquidMixingType::NONE; break;
    case 1: liquidMixType_ = Co2StoreConfig::LiquidMixingType::IDEAL; break;
    case 2: liquidMixType_ = Co2StoreConfig::LiquidMixingType::DUANSUN; break;
    default: OPM_THROW(std::runtime_error, "The thermal mixing model option for liquid are 0, 1 and 2");
    }
}

template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
setEzrokhiDenCoeff(const std::vector<EzrokhiTable>& denaqa)
{
    if (denaqa.empty())
        return;

    enableEzrokhiDensity_ = true;
    ezrokhiDenNaClCoeff_ = {static_cast<Scalar>(denaqa[0].getC0("NACL")),
                            static_cast<Scalar>(denaqa[0].getC1("NACL")),
                            static_cast<Scalar>(denaqa[0].getC2("NACL"))};
    ezrokhiDenCo2Coeff_ = {static_cast<Scalar>(denaqa[0].getC0("CO2")),
                           static_cast<Scalar>(denaqa[0].getC1("CO2")),
                           static_cast<Scalar>(denaqa[0].getC2("CO2"))};
}

template<class Scalar, class Params>
void BrineCo2Pvt<Scalar, Params>::
setEzrokhiViscCoeff(const std::vector<EzrokhiTable>& viscaqa)
{
    if (viscaqa.empty())
        return;

    enableEzrokhiViscosity_ = true;
    ezrokhiViscNaClCoeff_ = {static_cast<Scalar>(viscaqa[0].getC0("NACL")),
                             static_cast<Scalar>(viscaqa[0].getC1("NACL")),
                             static_cast<Scalar>(viscaqa[0].getC2("NACL"))};
}

template class BrineCo2Pvt<double>;
template class BrineCo2Pvt<float>;

} // namespace Opm
