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
/*!
 * \file
 *
 * \brief This is the unit test for the co2 brine PVT model
 *
 */
#include "config.h"

#include <opm/material/fluidsystems/blackoilpvt/Co2GasPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>
#include <opm/material/binarycoefficients/Brine_CO2.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/components/CO2Tables.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

namespace {

template <class Co2Pvt>
double densityGas(const Co2Pvt& co2Pvt, const double p, const double T, const double Rv)
{
    return co2Pvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                  T,
                                                  p,
                                                  Rv,
                                                  /*Rvw=*/0.0) * co2Pvt.gasReferenceDensity(0);
}

template <class BrinePvt>
double densityBrine(const BrinePvt& brinePvt, const double p, const double T, const double Rs)
{
    double bo = brinePvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                  T,
                                                  p,
                                                  Rs);
    return bo * (brinePvt.oilReferenceDensity(0) + Rs * brinePvt.gasReferenceDensity(0));
}

std::pair<double, double> moleFractionMutualSolubility(const double p, 
                                                       const double T, 
                                                       const double s,
                                                       const int activityModel)
{
    // Init. output
    double yH2O;
    double xCO2;

    // Calc. mutual solubility
    using H2O = Opm::SimpleHuDuanH2O<double>;
    using CO2 = Opm::CO2<double>;
    using BinaryCoeffBrineCO2 = Opm::BinaryCoeff::Brine_CO2<double, H2O, CO2>;
    Opm::CO2Tables co2Tables;
    BinaryCoeffBrineCO2::calculateMoleFractions(co2Tables, T, p, s, -1, xCO2, yH2O, activityModel, true);

    return {xCO2, yH2O};
}

double moleFractionCO2inBrine(const double p, 
                              const double T, 
                              const double s,
                              const int activityModel)
{
    // Calculate mutual solubilities
    auto [xCO2, yH2O] = moleFractionMutualSolubility(p, T, s, activityModel);
   
    return xCO2;
}

double moleFractionBrineInCO2(const double p, 
                              const double T, 
                              const double s,
                              const int activityModel)
{
    // Calculate mutual solubilities
    auto [xCO2, yH2O] = moleFractionMutualSolubility(p, T, s, activityModel);
   
    return yH2O;
}

double molalityCO2inBrine(const double p, 
                          const double T, 
                          const double m_sal,
                          const int activityModel)
{
    // Mole fraction CO2 in brine
    const double MmNaCl = 58.44e-3; // molar mass of NaCl [kg/mol]
    const double s = 1 / ( 1 + 1 / (m_sal*MmNaCl));
    double xlCO2 = moleFractionCO2inBrine(p, T, s, activityModel);

    // return molal co2
    return xlCO2 * (2 * m_sal + 55.508) / (1 - xlCO2);

}

} // Anonymous namespace

int main(int argc, char **argv)
{

    bool help = false;
    for (int i = 1; i < argc; ++i) {
        std::string tmp = argv[i];
        help = help || (tmp  == "--h") || (tmp  == "--help");
    }

    if (argc < 5 || help) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "co2brinepvt <prop> <phase> <p> <T> <salinity> <rs> <rv> <saltmodel> <thermalmixingmodelgas> <thermalmixingmodelliquid> <thermalmixingmodelsalt>"<< std::endl;
        std::cout << "prop = {density, invB, B, viscosity, rsSat, internalEnergy, enthalpy, diffusionCoefficient}" << std::endl;
        std::cout << "phase = {CO2, brine}" << std::endl;
        std::cout << "p: pressure in bar" << std::endl;
        std::cout << "T: temperature in celcius" << std::endl;
        std::cout << "salinity(optional): salt molality in mol/kg" << std::endl;
        std::cout << "rs(optional): amount of dissolved CO2 in Brine in SM3/SM3" << std::endl;
        std::cout << "rv(optional): amount of vaporized water in Gas in SM3/SM3" << std::endl;
        std::cout << "saltmodel(optional): 0 = no salt activity; 1 = Rumpf et al (1996) [default];"
                     " 2 = Duan-Sun in Spycher & Pruess (2009); 3 = Duan-Sun in Sycher & Pruess (2005)" << std::endl;
        std::cout << "thermalmixingmodelgas(optional): 0 = pure component [default]; 1 = ideal mixing;" << std::endl;
        std::cout << "thermalmixingmodelliquid(optional): 0 = pure component; 1 = ideal mixing; 2 = heat of dissolution according to duan sun [default]"  << std::endl;
        std::cout << "thermalmixingmodelsalt(optional): 0 = pure water; 1 = model in MICHAELIDES [default];" << std::endl;
        std::cout << "OPTIONS:" << std::endl;
        std::cout << "--h/--help Print help and exit." << std::endl;
        std::cout << "DESCRIPTION:" << std::endl;
        std::cout << "co2brinepvt computes PVT properties of a brine/co2 system " << std::endl;
        std::cout << "for a given phase (oil or brine), pressure, temperature, salinity and rs." << std::endl;
        std::cout << "The properties support are: density, the inverse phase formation volume factor (invB), viscosity, " << std::endl;
        std::cout << "saturated dissolution factor (rsSat) " << std::endl;
        std::cout << "See CO2STORE in the OPM manual for more details." << std::endl;
        return EXIT_FAILURE;
    }

    std::string prop = argv[1];
    std::string phase = argv[2];
    double p = atof(argv[3]) * 1e5;
    double T = atof(argv[4]) + 273.15;
    double molality = 0.0;
    double rs = 0.0;
    double rv = 0.0;
    int activityModel = 1;
    int thermalmixgas = 0;
    int thermalmixliquid = 2;
    int thermalmixsalt = 1;
    if (argc > 5)
        molality = atof(argv[5]);
    if (argc > 6)
        rs = atof(argv[6]);
    if (argc > 7)
        rv = atof(argv[7]);

    if (argc > 8)
        activityModel = atoi(argv[8]);
    if (argc > 9)
        thermalmixgas = atoi(argv[9]);
    if (argc > 10)
        thermalmixliquid = atoi(argv[10]);
    if (argc > 11)
        thermalmixsalt = atoi(argv[11]);    

    const double MmNaCl = 58.44e-3; // molar mass of NaCl [kg/mol]
    // convert to mass fraction
    std::vector<double> salinity = {0.0};
    if (molality > 0.0)
        salinity[0] = 1 / ( 1 + 1 / (molality*MmNaCl));
    Opm::BrineCo2Pvt<double> brineCo2Pvt(salinity, activityModel, thermalmixsalt, thermalmixliquid);

    Opm::Co2GasPvt<double> co2Pvt(salinity, activityModel, thermalmixgas);

    double value;
    if (prop == "density") {
        if (phase == "CO2") {
            value = densityGas(co2Pvt, p, T, rv);
        } else if (phase == "brine") {
            value = densityBrine(brineCo2Pvt, p, T, rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "invB" || prop == "B") {
        if (phase == "CO2") {
            value = co2Pvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rv,
                                                   /*Rvw=*/0.0);
        } else if (phase == "brine") {
            value = brineCo2Pvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
        if (prop == "B")
            value = 1 / value;

    } else if (prop == "viscosity") {
        if (phase == "CO2") {
            value = co2Pvt.viscosity(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rv,
                                                   /*Rvw=*/0.0);
        } else if (phase == "brine") {
            value = brineCo2Pvt.viscosity(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "rsSat") {
        if (phase == "CO2") {
            value = co2Pvt.saturatedWaterVaporizationFactor(/*regionIdx=*/0,
                                                   T,
                                                   p);
        } else if (phase == "brine") {
            value = brineCo2Pvt.saturatedGasDissolutionFactor(/*regionIdx=*/0,
                                                   T,
                                                   p);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }


    } else if (prop == "diffusionCoefficient") {
        size_t comp_idx = 0; // not used
        if (phase == "CO2") {
            value = co2Pvt.diffusionCoefficient(T,p, comp_idx);
        } else if (phase == "brine") {
            value = brineCo2Pvt.diffusionCoefficient(T,p, comp_idx);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "internalEnergy") {
        if (phase == "CO2") {
            value = co2Pvt.internalEnergy(/*regionIdx=*/0 ,T,p, rv, 0.0);
        } else if (phase == "brine") {
            value = brineCo2Pvt.internalEnergy(/*regionIdx=*/0 ,T,p, rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "enthalpy") {
        if (phase == "CO2") {
            value = p / densityGas(co2Pvt, p, T, rv) + co2Pvt.internalEnergy(/*regionIdx=*/0 ,T,p, rv, 0.0);
        } else if (phase == "brine") {
            value = p / densityBrine(brineCo2Pvt, p, T, rs) + brineCo2Pvt.internalEnergy(/*regionIdx=*/0 ,T,p, rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "solubility_molal") {
        if (phase == "CO2") {
            // Solubility of CO2 is brine
            value = molalityCO2inBrine(p, T, molality, activityModel);

        }
        else if (phase == "brine") {
            throw std::runtime_error("solubility in molal for brine in CO2 gas not implemented yet!");
        }
        else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "solubility_molefraction") {
        if (phase == "CO2") {
            // Solubility of CO2 is brine
            value = moleFractionCO2inBrine(p, T, salinity[0], activityModel);

        }
        else if (phase == "brine") {
            // Solubility of brine in CO2
            value = moleFractionBrineInCO2(p, T, salinity[0], activityModel);
        }
        else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "solubility_molepercent") {
        if (phase == "CO2") {
            // Solubility of CO2 is brine
            value = moleFractionCO2inBrine(p, T, salinity[0], activityModel) * 100;

        }
        else if (phase == "brine") {
            // Solubility of brine in CO2
            value = moleFractionBrineInCO2(p, T, salinity[0], activityModel) * 100;
        }
        else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    }  
    else {
        throw std::runtime_error("prop " + prop + " not recognized. "
        + "Use either density, visosity, invB, B, internalEnergy, enthalpy or diffusionCoefficient");
    }

    std::cout << std::setprecision (15) << value << std::endl;

    return 0;
}
