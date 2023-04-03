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

#include <iostream>

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

int main(int argc, char **argv)
{

    bool help = false;
    for (int i = 1; i < argc; ++i) {
        std::string tmp = argv[i];
        help = help || (tmp  == "--h") || (tmp  == "--help");
    }

    if (argc < 5 || help) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "co2brinepvt <prop> <phase> <p> <T> <salinity> <rs> "<< std::endl;
        std::cout << "prop = {density, invB, B, viscosity, rsSat, internalEnergy, enthalpy, diffusionCoefficient}" << std::endl;
        std::cout << "phase = {CO2, brine}" << std::endl;
        std::cout << "p: pressure in pascal" << std::endl;
        std::cout << "T: temperature in kelvin" << std::endl;
        std::cout << "salinity(optional): salt molality in mol/kg" << std::endl;
        std::cout << "rs(optional): amount of dissolved CO2 in Brine in SM3/SM3" << std::endl;
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
    double p = atof(argv[3]);
    double T = atof(argv[4]);
    double molality = 0.0;
    double rs = 0.0;
    double rv = 0.0; // only support 0.0 for now
    if (argc > 5)
        molality = atof(argv[5]);
    if (argc > 6)
        rs = atof(argv[6]);

    const double MmNaCl = 58e-3; // molar mass of NaCl [kg/mol]
    // convert to mass fraction
    std::vector<double> salinity = {0.0};
    if (molality > 0.0)
        salinity[0] = 1 / ( 1 + 1 / (molality*MmNaCl));
    Opm::BrineCo2Pvt<double> brineCo2Pvt(salinity);

    Opm::Co2GasPvt<double> co2Pvt(salinity);

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
    } else {
        throw std::runtime_error("prop " + prop + " not recognized. "
        + "Use either density, visosity, invB, B, internalEnergy, enthalpy or diffusionCoefficient");
    }

    std::cout << value << std::endl;

    return 0;
}
