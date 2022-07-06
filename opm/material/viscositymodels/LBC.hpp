// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 NORCE.
  
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
 * \copydoc Opm::LBC
 */

#ifndef LBC_HPP
#define LBC_HPP

#include <cmath>
#include <vector>

namespace Opm
{
template <class Scalar, class FluidSystem>
class ViscosityModels
{

public:

    // Standard LBC model. (Lohrenz, Bray & Clark: "Calculating Viscosities of Reservoir
    //                      fluids from Their Compositions", JPT 16.10 (1964).
    template <class FluidState, class Params, class LhsEval = typename FluidState::Scalar>
    static LhsEval LBC(const FluidState& fluidState,
                      const Params& /*paramCache*/,
                      unsigned phaseIdx)
    {
        const Scalar MPa_atm = 0.101325;
        const Scalar R = 8.3144598e-3;//Mj/kmol*K
        const auto& T = Opm::decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& rho = Opm::decay<LhsEval>(fluidState.density(phaseIdx));

        LhsEval sumMm = 0.0;
        LhsEval sumVolume = 0.0;
        for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
            const Scalar& p_c = FluidSystem::criticalPressure(compIdx)/1e6; // in Mpa;
            const Scalar& T_c = FluidSystem::criticalTemperature(compIdx);
            const Scalar Mm = FluidSystem::molarMass(compIdx) * 1000; //in kg/kmol;
            const auto& x = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, compIdx));
            const Scalar v_c = FluidSystem::criticalVolume(compIdx);  // in m3/kmol
            sumMm += x*Mm;
            sumVolume += x*v_c;
        }

        LhsEval rho_pc = sumMm/sumVolume; //mixture pseudocritical density
        LhsEval rho_r = rho/rho_pc;


        LhsEval xsum_T_c = 0.0; //mixture pseudocritical temperature
        LhsEval xsum_Mm = 0.0; //mixture molar mass
        LhsEval xsum_p_ca = 0.0;  //mixture pseudocritical pressure
        for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
            const Scalar& p_c = FluidSystem::criticalPressure(compIdx)/1e6; // in Mpa;
            const Scalar& T_c = FluidSystem::criticalTemperature(compIdx);
            const Scalar Mm = FluidSystem::molarMass(compIdx) * 1000; //in kg/kmol;
            const auto& x = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, compIdx));
            Scalar p_ca = p_c / MPa_atm;
            xsum_T_c += x*T_c;
            xsum_Mm += x*Mm;
            xsum_p_ca += x*p_ca;
        }
        LhsEval zeta_tot = Opm::pow(xsum_T_c / (Opm::pow(xsum_Mm,3.0) * Opm::pow(xsum_p_ca,4.0)),1./6);

        LhsEval my0 = 0.0;
        LhsEval sumxrM = 0.0;
        for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
            const Scalar& p_c = FluidSystem::criticalPressure(compIdx)/1e6; // in Mpa;
            const Scalar& T_c = FluidSystem::criticalTemperature(compIdx);
            const Scalar Mm = FluidSystem::molarMass(compIdx) * 1000; //in kg/kmol;
            const auto& x = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, compIdx));
            Scalar p_ca = p_c / MPa_atm;
            Scalar zeta = std::pow(T_c / (std::pow(Mm,3.0) * std::pow(p_ca,4.0)),1./6);
            LhsEval T_r = T/T_c;
            LhsEval xrM = x * std::pow(Mm,0.5);
            LhsEval mys = 0.0;
            if (T_r <=1.5) {
                mys = 34.0e-5*Opm::pow(T_r,0.94)/zeta;
            } else {
                mys = 17.78e-5*Opm::pow(4.58*T_r - 1.67, 0.625)/zeta;
            }
            my0 += xrM*mys;
            sumxrM += xrM;
        }
        my0 /= sumxrM;

        std::vector<Scalar> LBC = {0.10230,
                                   0.023364,
                                   0.058533,
                                   -0.040758,  // typo in 1964-paper: -0.40758
                                   0.0093324};

        LhsEval sumLBC = 0.0;
        for (int i = 0; i < 5; ++i) {
            sumLBC += Opm::pow(rho_r,i)*LBC[i];
        }

        return (my0 + (Opm::pow(sumLBC,4.0) - 1e-4)/zeta_tot)/1e3; // mPas-> Pas
    }

};

}; // namespace Opm

#endif // LBC_HPP
