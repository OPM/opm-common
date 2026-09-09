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
 * \copydoc Opm::ViscosityModels
 */

#ifndef OPM_VISCOSITY_MODELS_HPP
#define OPM_VISCOSITY_MODELS_HPP

#include <opm/common/Exceptions.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <array>
#include <cmath>

namespace Opm
{

template <class Scalar, class FluidSystem>
class ViscosityModels
{

public:

    // Stiel and Thodos (1961) give the dilute-gas viscosity in two branches
    // that meet at this reduced temperature.
    static constexpr Scalar stielThodosSplit = 1.5;

    // Standard coefficients of the Lorentz-Bray-Clark viscosity correlation.
    // (Note the fourth coefficient: typo in 1964-paper has -0.40758.)
    static constexpr std::array<Scalar, 5> defaultLBCCoefficients()
    {
        return {Scalar(0.10230),
                Scalar(0.023364),
                Scalar(0.058533),
                Scalar(-0.040758),
                Scalar(0.0093324)};
    }

    // Standard LBC model. (Lohrenz, Bray & Clark: "Calculating Viscosities of Reservoir
    //                      fluids from Their Compositions", JPT 16.10 (1964).
    template <class FluidState, class Params, class LhsEval = typename FluidState::ValueType>
    static LhsEval LBC(const FluidState& fluidState,
                      const Params& /*paramCache*/,
                      unsigned phaseIdx)
    {
        const Scalar R = Opm::Constants<Scalar>::R;
        const auto& T = Opm::decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& P = Opm::decay<LhsEval>(fluidState.pressure(phaseIdx));
        const auto& Z = Opm::decay<LhsEval>(fluidState.compressFactor(phaseIdx));
        const LhsEval molarDensity = P / (R * T * Z);

        return LBCWithMolarDensity<FluidState, LhsEval, LhsEval>(fluidState, molarDensity, phaseIdx);
    }

    // LBC correlation at the supplied physical molar density [mol/m^3].
    template <class FluidState, class MolarDensity,
              class LhsEval = typename FluidState::ValueType>
    static LhsEval LBCWithMolarDensity(const FluidState& fluidState,
                                       const MolarDensity& molarDensity,
                                       unsigned phaseIdx)
    {
        const auto& T = Opm::decay<LhsEval>(fluidState.temperature(phaseIdx));

        LhsEval sumVolume = 0.0;
        LhsEval xsum_T_c = 0.0; // mixture pseudocritical temperature
        LhsEval xsum_Mm = 0.0; // mixture molar mass
        LhsEval xsum_p_ca = 0.0; // mixture pseudocritical pressure [atm]
        LhsEval my0 = 0.0;
        LhsEval sumxrM = 0.0;
        for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
            const auto& x = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, compIdx));
            const Scalar& T_c = FluidSystem::criticalTemperature(compIdx);
            // Convert kg/mol to kg/kmol (numerically equal to g/mol for the correlations).
            const Scalar Mm = FluidSystem::molarMass(compIdx) * prefix::kilo;
            const Scalar p_ca = FluidSystem::criticalPressure(compIdx) / unit::atm; // Pa -> atm
            // FluidSystem provides m3/kmol; convert to m3/mol to match molarDensity.
            const Scalar v_c = FluidSystem::criticalVolume(compIdx) / prefix::kilo;

            sumVolume += x * v_c;
            xsum_T_c += x * T_c;
            xsum_Mm += x * Mm;
            xsum_p_ca += x * p_ca;

            const LhsEval xrM = x * std::sqrt(Mm);
            const LhsEval mys = stielThodosComponentViscosity_(T, T_c, Mm, p_ca);
            my0 += xrM * mys;
            sumxrM += xrM;
        }
        my0 /= sumxrM;

        const LhsEval rho_pc = 1.0 / sumVolume;
        const LhsEval rho_r = Opm::decay<LhsEval>(molarDensity) / rho_pc;
        const LhsEval zeta_tot
            = Opm::pow(xsum_T_c / (Opm::pow(xsum_Mm, 3.0) * Opm::pow(xsum_p_ca, 4.0)), 1.0 / 6.0);
        const LhsEval sumLBC = evaluateLbcPolynomial_(rho_r);

        // The correlation returns mPa s; viscosity is represented in Pa s.
        const LhsEval mu = (my0 + (Opm::pow(sumLBC, 4.0) - 1e-4) / zeta_tot) * prefix::milli;

        if (mu <= 0.) {
            throw NumericalProblem("LBC correlation produced a non-positive viscosity; "
                                   "check the LBC coefficients");
        }

        return mu;
    }

    // Improved LBC model for CO2 rich mixtures. (Lansangan, Taylor, Smith & Kovarik - 1993)
    template <class FluidState, class Params, class LhsEval = typename FluidState::ValueType>
    static LhsEval LBCco2rich(const FluidState& fluidState,
                              const Params& /*paramCache*/,
                              unsigned phaseIdx)
    {
        const auto& T = Opm::decay<LhsEval>(fluidState.temperature(phaseIdx));
        const auto& rho = Opm::decay<LhsEval>(fluidState.density(phaseIdx));

        LhsEval sumMm = 0.0;
        LhsEval sumVolume = 0.0;
        LhsEval my0 = 0.0;
        LhsEval sumxrM = 0.0;
        for (unsigned compIdx = 0; compIdx < FluidSystem::numComponents; ++compIdx) {
            // Convert kg/mol to kg/kmol (numerically equal to g/mol for the correlations).
            const Scalar Mm = FluidSystem::molarMass(compIdx) * prefix::kilo;
            const auto& x = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, compIdx));
            const Scalar v_c = FluidSystem::criticalVolume(compIdx); // m3/kmol
            const Scalar& T_c = FluidSystem::criticalTemperature(compIdx);
            const Scalar p_ca = FluidSystem::criticalPressure(compIdx) / unit::atm; // Pa -> atm

            sumMm += x * Mm;
            sumVolume += x * v_c;

            const LhsEval xrM = x * std::sqrt(Mm);
            const LhsEval mys = stielThodosComponentViscosity_(T, T_c, Mm, p_ca);
            my0 += xrM * mys;
            sumxrM += xrM;
        }
        my0 /= sumxrM;

        const LhsEval rho_pc = sumMm / sumVolume; // mixture pseudocritical density [kg/m3]
        const LhsEval rho_r = rho / rho_pc;

        LhsEval xxT_p = 0.0; // x*x*T_c/p_c
        LhsEval xxT2_p = 0.0; // x*x*T_c^2/p_c
        // Use atm throughout the mixing rule, as required by the viscosity correlation.
        for (unsigned i_compIdx = 0; i_compIdx < FluidSystem::numComponents; ++i_compIdx) {
            const Scalar& T_c_i = FluidSystem::criticalTemperature(i_compIdx);
            const Scalar p_c_i = FluidSystem::criticalPressure(i_compIdx) / unit::atm; // Pa -> atm
            const auto& x_i = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, i_compIdx));
            for (unsigned j_compIdx = 0; j_compIdx < FluidSystem::numComponents; ++j_compIdx) {
                const Scalar& T_c_j = FluidSystem::criticalTemperature(j_compIdx);
                const Scalar p_c_j
                    = FluidSystem::criticalPressure(j_compIdx) / unit::atm; // Pa -> atm
                const auto& x_j = Opm::decay<LhsEval>(fluidState.moleFraction(phaseIdx, j_compIdx));

                const Scalar T_c_ij = std::sqrt(T_c_i * T_c_j);
                const Scalar p_c_ij = 8.0 * T_c_ij
                    / Opm::pow(Opm::pow(T_c_i / p_c_i, 1.0 / 3.0)
                                   + Opm::pow(T_c_j / p_c_j, 1.0 / 3.0),
                               3.0);
                xxT_p += x_i * x_j * T_c_ij / p_c_ij;
                xxT2_p += x_i * x_j * T_c_ij * T_c_ij / p_c_ij;
            }
        }

        const LhsEval T_pc = xxT2_p / xxT_p; // mixture pseudocritical temperature
        const LhsEval p_pca = T_pc / xxT_p; // mixture pseudocritical pressure [atm]
        const LhsEval zeta_tot
            = Opm::pow(T_pc / (Opm::pow(sumMm, 3.0) * Opm::pow(p_pca, 4.0)), 1.0 / 6.0);
        const LhsEval sumLBC = evaluateLbcPolynomial_(rho_r);

        // The correlation returns mPa s; viscosity is represented in Pa s.
        return (my0 + (Opm::pow(sumLBC, 4.0) - 1e-4) / zeta_tot
                - 1.8366e-8 * Opm::pow(rho_r, 13.992))
            * prefix::milli;
    }

private:
    template <class LhsEval>
    static LhsEval
    stielThodosComponentViscosity_(const LhsEval& T, Scalar T_c, Scalar Mm, Scalar p_ca)
    {
        const Scalar zeta = std::pow(T_c / (std::pow(Mm, 3.0) * std::pow(p_ca, 4.0)), 1.0 / 6.0);
        const LhsEval T_r = T / T_c;
        if (T_r <= stielThodosSplit) {
            return 34.0e-5 * Opm::pow(T_r, 0.94) / zeta;
        }
        return 17.78e-5 * Opm::pow(4.58 * T_r - 1.67, 0.625) / zeta;
    }

    static const std::array<Scalar, 5>& lbcCoefficients_()
    {
        if constexpr (requires { FluidSystem::lbcCoefficients(); }) {
            return FluidSystem::lbcCoefficients();
        } else {
            static constexpr auto default_lbc = defaultLBCCoefficients();
            return default_lbc;
        }
    }

    template <class LhsEval>
    static LhsEval evaluateLbcPolynomial_(const LhsEval& rho_r)
    {
        const auto& LBC = lbcCoefficients_();
        LhsEval sumLBC = 0.0;
        for (int i = 0; i < static_cast<int>(LBC.size()); ++i) {
            sumLBC += Opm::pow(rho_r, i) * LBC[i];
        }
        return sumLBC;
    }
};

} // namespace Opm

#endif // OPM_VISCOSITY_MODELS_HPP
