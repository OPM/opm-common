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



template <class Co2Pvt>
double densityGas(const Co2Pvt& co2Pvt, const double p, const double T, const double Rv)
{
    return co2Pvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                  T,
                                                  p,
                                                  Rv) * co2Pvt.gasReferenceDensity(0);
}

template <class BrinePvt>
double densityBrine(const BrinePvt& brinePvt, const double p, const double T, const double Rs)
{
    return brinePvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                  T,
                                                  p,
                                                  Rs) * brinePvt.oilReferenceDensity(0);
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        std::cout << "co2brinepvt prop phase p T salinity rs "<< std::endl;
        std::cout << "prop = {density, invB, viscosity, rsSat}" << std::endl;
        std::cout << "phase = {CO2, brine}" << std::endl;
        std::cout << "p: pressure in pascal" << std::endl;
        std::cout << "T: temperature in kelvin" << std::endl;
        std::cout << "salinity(optional): salinity in molality" << std::endl;
        std::cout << "rs(optional): rs in SM3/SM3" << std::endl;
        return EXIT_FAILURE;
    }

    std::string prop = argv[1];
    std::string phase = argv[2];
    double p = atof(argv[3]);
    double T = atof(argv[4]);
    std::vector<double> salinity = {0.0};
    double rs = 0.0;
    double rv = 0.0; // only support 0.0 for now
    if (argc > 5)
        salinity[0] = atof(argv[5]);
    if (argc > 6)
        rs = atof(argv[6]);

    //double p_ref = 101325;
    //double T_ref = 298; // 25C
    std::vector<double> ref_den_co2 = {1.80914};
    std::vector<double> ref_den_water = {996.206};

    Opm::Co2GasPvt<double> co2Pvt(ref_den_co2);
    Opm::BrineCo2Pvt<double> brineCo2Pvt(ref_den_water, ref_den_co2, salinity);
    double value;
    if (prop == "density") {
        if (phase == "CO2") {
            value = densityGas(co2Pvt, p, T, rv);
        } else if (phase == "brine") {
            value = densityBrine(brineCo2Pvt, p, T, rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "invB") {
        if (phase == "CO2") {
            value = co2Pvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rv);
        } else if (phase == "brine") {
            value = brineCo2Pvt.inverseFormationVolumeFactor(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "viscosity") {
        if (phase == "CO2") {
            value = co2Pvt.viscosity(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rv);
        } else if (phase == "brine") {
            value = brineCo2Pvt.viscosity(/*regionIdx=*/0,
                                                   T,
                                                   p,
                                                   rs);
        } else {
            throw std::runtime_error("phase " + phase + " not recognized. Use either CO2 or brine");
        }
    } else if (prop == "rsSat") {
            value = brineCo2Pvt.saturatedGasDissolutionFactor(/*regionIdx=*/0,
                                                   T,
                                                   p);
    } else {
        throw std::runtime_error("prop " + prop + " not recognized. "
        + "Use either density, visosity or invB");
    }

    std::cout << value << std::endl;

    return 0;
}
