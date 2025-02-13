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
#ifndef CUBIC_EOS_HPP
#define CUBIC_EOS_HPP

#include <opm/material/Constants.hpp>
#include <opm/material/common/PolynomialUtils.hpp>
#include <opm/material/common/Valgrind.hpp>

namespace Opm
{
template<class Scalar, class FluidSystem>
class CubicEOS
{
    enum { numComponents = FluidSystem::numComponents };

    static constexpr Scalar R = Constants<Scalar>::R;

public:
    template <class FluidState, class Params, class LhsEval = typename FluidState::Scalar>
    static LhsEval computeFugacityCoefficient(const FluidState& fs,
                                              const Params& params,
                                              unsigned phaseIdx,
                                              unsigned compIdx)
    {
        // note that we normalize the component mole fractions, so
        // that their sum is 100%. This increases numerical stability
        // considerably if the fluid state is not physical.

        // extract variables
        LhsEval Vm = params.molarVolume(phaseIdx);
        LhsEval T = fs.temperature(phaseIdx);
        LhsEval p = fs.pressure(phaseIdx); // molar volume in [bar]
        LhsEval A = params.A(phaseIdx);
        LhsEval B = params.B(phaseIdx);
        LhsEval Bi = params.Bi(phaseIdx, compIdx);
        LhsEval m1 = params.m1(phaseIdx);
        LhsEval m2 = params.m2(phaseIdx);

        // Calculate Bi / B
        LhsEval Bi_B = Bi / B;

        // Calculate the compressibility factor
        LhsEval RT = R * T;
        LhsEval Z = p * Vm / RT; // compressibility factor

        // calculate sum(A_ij * x_j)
        LhsEval A_s = 0.0;
        for (unsigned compJIdx = 0; compJIdx < numComponents; ++compJIdx) {
            A_s += params.aCache(phaseIdx, compIdx, compJIdx) * fs.moleFraction(phaseIdx, compJIdx);
        }

        // calculate fugacity coefficient
        LhsEval alpha;
        LhsEval beta;
        LhsEval gamma;
        LhsEval ln_phi;
        LhsEval fugCoeff;

        alpha = -log(Z - B) + Bi_B * (Z - 1);
        beta  = log((Z + m2 * B) / (Z + m1 * B)) * A / ((m1 - m2) * B);
        gamma = (2 / A) * A_s - Bi_B;
        ln_phi = alpha + (beta * gamma);

        fugCoeff = exp(ln_phi);

        ////////
        // limit the fugacity coefficient to a reasonable range:
        //
        // on one side, we want the mole fraction to be at
        // least 10^-3 if the fugacity is at the current pressure
        //
        fugCoeff = min(1e10, fugCoeff);
        //
        // on the other hand, if the mole fraction of the component is 100%, we want the
        // fugacity to be at least 10^-3 Pa
        //
        fugCoeff = max(1e-10, fugCoeff);
        ///////////

        return fugCoeff;
    }

    template <class FluidState, class Params>
    static typename FluidState::Scalar computeMolarVolume(const FluidState& fs,
                                                          Params& params,
                                                          unsigned phaseIdx,
                                                          bool isGasPhase)
    {
        Valgrind::CheckDefined(fs.temperature(phaseIdx));
        Valgrind::CheckDefined(fs.pressure(phaseIdx));

        using Evaluation = typename FluidState::Scalar;
        
        // extract variables
        const Evaluation& T = fs.temperature(phaseIdx);
        const Evaluation& p = fs.pressure(phaseIdx);
        const Evaluation& A = params.A(phaseIdx);
        const Evaluation& B = params.B(phaseIdx);
        const Evaluation& m1 = params.m1(phaseIdx);
        const Evaluation& m2 = params.m2(phaseIdx);

        // coefficients in cubic solver
        const Evaluation a1 = 1.0;  // cubic term
        const Evaluation a2 = (m1 + m2 - 1) * B - 1;  // quadratic term
        const Evaluation a3 = A + m1 * m2 * B * B - (m1 + m2) * B * (B + 1);  // linear term
        const Evaluation a4 = -A * B - m1 * m2 * B * B * (B + 1);  // constant term
        Valgrind::CheckDefined(a1);
        Valgrind::CheckDefined(a2);
        Valgrind::CheckDefined(a3);
        Valgrind::CheckDefined(a4);

        // root of cubic equation
        Evaluation Vm = 0;
        Valgrind::SetUndefined(Vm);
        Evaluation Z[3] = {0.0, 0.0, 0.0};
        int numSol = cubicRoots(Z, a1, a2, a3, a4);
        
        // pick correct root
        const Evaluation RT_p = R * T / p;
        if (numSol == 3) {
            // the EOS has three intersections with the pressure,
            // i.e. the molar volume of gas is the largest one and the
            // molar volume of liquid is the smallest one
            if (isGasPhase)
                Vm = max(1e-7, Z[2] * RT_p);
            else
                Vm = max(1e-7, Z[0] * RT_p);
        }
        else if (numSol == 1) {
            // the EOS only has one intersection with the pressure,
            // for the other phase, we take the extremum of the EOS
            // with the largest distance from the intersection.
            Vm = max(1e-7, Z[0] * RT_p);
        }

        Valgrind::CheckDefined(Vm);
        assert(std::isfinite(scalarValue(Vm)));
        assert(Vm > 0);
        return Vm;

    }

};  // class CubicEOS

}  // namespace Opm

#endif
