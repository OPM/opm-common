// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA.

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
 * \brief Molar enthalpy of a flashed compositional state, the property an
 *        isenthalpic (P-H) flash inverts for temperature.
 *
 * Two models, selected by EnthalpyModel:
 *
 * - caloric (ideal gas): the enthalpy of a phase is the mole-fraction
 *   weighted ideal-gas enthalpy of its components, and the mixture sums the
 *   phases weighted by their mole fractions,
 *
 *       H = L*H_oil + (1-L)*H_gas,   H_phase = sum_i w_i * int_{T0}^{T} cp_i dT'.
 *
 *   The phase split cancels (H = sum_i z_i*h_i(T) by material balance), so H
 *   does not depend on the flash result.
 *
 * - eos_departure: adds the residual (departure) enthalpy of each phase,
 *
 *       H_res = -R * T^2 * d/dT [sum_i w_i * ln(phi_i)]      (fixed P, w),
 *
 *   which makes the enthalpy consistent with the cubic equation of state that
 *   computes the phase split. The residual of the liquid is of the scale of a
 *   vaporization enthalpy, so with this model the enthalpy depends on the
 *   flash result.
 *
 * Units and datum: SI, molar enthalpy [J/mol]. The datum is the ideal gas at
 * T0 = ComponentCp::referenceTemperature(), where every component's ideal-gas
 * enthalpy is zero at any pressure. The caloric model gives H(T0) = 0. The
 * eos_departure model gives H(T0) = the residual enthalpy of the flashed state
 * at (T0, P), which is not zero; it vanishes only as P -> 0. A specified
 * enthalpy handed to an isenthalpic flash has to be computed with the same
 * model.
 *
 * The cp polynomials are valid only inside their fit range
 * (ComponentCp::minTemperature, maxTemperature); the functions here do not
 * check it.
 *
 * Derivatives: the caloric overloads keep the fluid state's value type, and
 * their derivatives are complete. The overloads taking an EnthalpyModel return
 * a plain Scalar, because the residual part is evaluated on values only.
 */
#ifndef OPM_MIXTURE_ENTHALPY_HPP
#define OPM_MIXTURE_ENTHALPY_HPP

#include <opm/material/components/ComponentCp.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/Constants.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

namespace Opm {

//! Which enthalpy model the P-H stack evaluates.
enum class EnthalpyModel { caloric, eos_departure };

/*!
 * \brief Molar mixture enthalpy of a flashed compositional state, under the
 *        caloric (ideal-gas) or the equation-of-state departure model.
 *
 * See the file documentation for the model definitions and the datum.
 */
template <class Scalar, class FluidSystem>
struct MixtureEnthalpy {
    static constexpr int numComponents = FluidSystem::numComponents;

    using EOSType = CompositionalConfig::EOSType;

    /*!
     * \brief Molar enthalpy of one phase [J/mol], caloric model.
     *
     * Generic in the fluid state's value type: works on plain doubles and on
     * DenseAd::Evaluation states alike (the expression is polynomial only).
     */
    template <class FluidState>
    static typename FluidState::ValueType
    phaseEnthalpy(const FluidState& fluidState,
                  const unsigned phaseIdx,
                  const CpTable<Scalar, numComponents>& cpTable)
    {
        using ValueType = typename FluidState::ValueType;

        const ValueType& T = fluidState.temperature(phaseIdx);
        ValueType h = 0.0;
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            h += fluidState.moleFraction(phaseIdx, compIdx)
                 * cpTable[compIdx].enthalpyIntegral(T);
        }
        return h;
    }

    /*!
     * \brief Molar enthalpy of the flashed mixture [J/mol]: phase enthalpies
     *        weighted by the liquid mole fraction L from the fluid state.
     *
     * Precondition: fluidState is a flashed state, with L() set and the phase
     * compositions consistent with it, as PTFlash::solve returns it.
     */
    template <class FluidState>
    static typename FluidState::ValueType
    mixtureEnthalpy(const FluidState& fluidState,
                    const CpTable<Scalar, numComponents>& cpTable)
    {
        const auto& L = fluidState.L();
        return L * phaseEnthalpy(fluidState, FluidSystem::oilPhaseIdx, cpTable)
             + (1.0 - L) * phaseEnthalpy(fluidState, FluidSystem::gasPhaseIdx, cpTable);
    }

    /*!
     * \brief Total molar heat capacity dH/dT [J/(mol K)] of the mixture,
     *        caloric model only.
     *
     * Under the eos_departure model dH/dT has a residual part that this
     * function does not include, so it is not the derivative to use in a
     * Newton step on that model.
     *
     * Precondition: same as mixtureEnthalpy, a flashed state.
     */
    template <class FluidState>
    static typename FluidState::ValueType
    mixtureCp(const FluidState& fluidState,
              const CpTable<Scalar, numComponents>& cpTable)
    {
        using ValueType = typename FluidState::ValueType;

        const auto& L = fluidState.L();
        ValueType cp = 0.0;
        for (unsigned phaseIdx : {static_cast<unsigned>(FluidSystem::oilPhaseIdx),
                                  static_cast<unsigned>(FluidSystem::gasPhaseIdx)}) {
            const ValueType& T = fluidState.temperature(phaseIdx);
            ValueType cpPhase = 0.0;
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                cpPhase += fluidState.moleFraction(phaseIdx, compIdx)
                           * cpTable[compIdx].heatCapacity(T);
            }
            cp += (phaseIdx == static_cast<unsigned>(FluidSystem::oilPhaseIdx) ? L : 1.0 - L) * cpPhase;
        }
        return cp;
    }

    /*!
     * \brief Residual (departure) molar enthalpy of one phase [J/mol]:
     *        H_res = -R * T^2 * d/dT [sum_i w_i * ln(phi_i)]  at fixed (P, w).
     *
     * For the cubic equation of state the sum has the closed form
     *
     *     sum_i w_i ln(phi_i) = Z - 1 - ln(Z - B)
     *                         + A / ((m1 - m2) B) * ln((Z + m2 B) / (Z + m1 B))
     *                         - sum_i w_i s_i B_i,
     *
     * where the last term is the volume translation of fluid systems with a
     * volume shift s_i. It is evaluated on a copy of the phase with temperature
     * seeded as the single AD variable, so the temperature derivative comes
     * from AD. This is the sum of the fluid system's ln(phi_i), without the
     * range CubicEOS limits each phi_i to: at that limit phi_i is held
     * constant, and a heavy component in the liquid would lose its whole
     * contribution to the residual.
     *
     * Returns a plain Scalar: derivatives with respect to outer AD variables
     * are not propagated.
     */
    template <class FluidState>
    static Scalar phaseResidualEnthalpy(const FluidState& fluidState,
                                        const unsigned phaseIdx,
                                        const EOSType& eosType)
    {
        return residualEnthalpy_(fluidState, phaseIdx,
                                 sumLnPhi_(fluidState, phaseIdx, phaseIdx, eosType));
    }

    /*!
     * \brief Molar enthalpy of one phase [J/mol] under the selected model:
     *        the caloric ideal-gas part, plus the residual when
     *        model == eos_departure.
     *
     * Returns a plain Scalar for any fluid-state value type. The residual part
     * carries no derivatives (see phaseResidualEnthalpy), so an AD result
     * would hold a derivative that looks complete but is not. For derivatives
     * of the caloric model, use the caloric overload.
     */
    template <class FluidState>
    static Scalar
    phaseEnthalpy(const FluidState& fluidState,
                  const unsigned phaseIdx,
                  const CpTable<Scalar, numComponents>& cpTable,
                  const EOSType& eosType,
                  const EnthalpyModel model)
    {
        Scalar h = Opm::getValue(phaseEnthalpy(fluidState, phaseIdx, cpTable));
        if (model == EnthalpyModel::eos_departure)
            h += phaseResidualEnthalpy(fluidState, phaseIdx, eosType);
        return h;
    }

    /*!
     * \brief Molar enthalpy of the flashed mixture [J/mol] under the selected
     *        model. Precondition as for the caloric overload: a flashed state.
     *
     * Returns a plain Scalar for any fluid-state value type, for the reason
     * given on the model-switch phaseEnthalpy.
     *
     * An absent phase (weight 0) skips the residual evaluation: its
     * composition is degenerate and its contribution vanishes anyway.
     *
     * For a single-phase state (L = 1 or L = 0) the residual is taken on the
     * cubic root with the lower Gibbs energy, the lower sum_i z_i ln(phi_i),
     * not on the root of the phase label. Where the cubic has three roots the
     * two differ by about a vaporization enthalpy, and the label (Li's
     * correlation in PTFlash) does not look at the pressure. The model-switch
     * phaseEnthalpy keeps the root of the phase it is asked for.
     */
    template <class FluidState>
    static Scalar
    mixtureEnthalpy(const FluidState& fluidState,
                    const CpTable<Scalar, numComponents>& cpTable,
                    const EOSType& eosType,
                    const EnthalpyModel model)
    {
        const Scalar L = Opm::getValue(fluidState.L());

        Scalar hOil = Opm::getValue(phaseEnthalpy(fluidState, FluidSystem::oilPhaseIdx, cpTable));
        Scalar hGas = Opm::getValue(phaseEnthalpy(fluidState, FluidSystem::gasPhaseIdx, cpTable));
        if (model == EnthalpyModel::eos_departure) {
            if (L > 0.0 && L < 1.0) {
                hOil += phaseResidualEnthalpy(fluidState, FluidSystem::oilPhaseIdx, eosType);
                hGas += phaseResidualEnthalpy(fluidState, FluidSystem::gasPhaseIdx, eosType);
            }
            else if (L > 0.0)
                hOil += stableRootResidualEnthalpy_(fluidState, FluidSystem::oilPhaseIdx, eosType);
            else
                hGas += stableRootResidualEnthalpy_(fluidState, FluidSystem::gasPhaseIdx, eosType);
        }
        return L * hOil + (1.0 - L) * hGas;
    }

private:
    /*!
     * \brief AD-typed copy of the composition of phase phaseIdx, stored as
     *        phase rootIdx, with temperature as the single seeded variable
     *        (slot 0); pressure and composition enter as constants, so
     *        derivative(0) is a partial at fixed (P, w).
     */
    template <class FluidState>
    static CompositionalFluidState<DenseAd::Evaluation<Scalar, 1>, FluidSystem>
    temperatureSeededCopy_(const FluidState& fluidState,
                           const unsigned phaseIdx,
                           const unsigned rootIdx)
    {
        using Eval1 = DenseAd::Evaluation<Scalar, 1>;

        CompositionalFluidState<Eval1, FluidSystem> seeded;
        seeded.setTemperature(
            Eval1::createVariable(Opm::getValue(fluidState.temperature(phaseIdx)), 0));
        const Eval1 p = Eval1::createConstant(Opm::getValue(fluidState.pressure(phaseIdx)));
        seeded.setPressure(FluidSystem::oilPhaseIdx, p);
        seeded.setPressure(FluidSystem::gasPhaseIdx, p);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            seeded.setMoleFraction(rootIdx, compIdx,
                Eval1::createConstant(Opm::getValue(fluidState.moleFraction(phaseIdx, compIdx))));
        }
        return seeded;
    }

    /*!
     * \brief sum_i w_i ln(phi_i) of the composition w of phase phaseIdx on the
     *        cubic root of phase rootIdx (smallest root for oil, largest for
     *        gas), in the closed form documented at phaseResidualEnthalpy,
     *        with its temperature derivative at fixed (P, w).
     */
    template <class FluidState>
    static DenseAd::Evaluation<Scalar, 1>
    sumLnPhi_(const FluidState& fluidState,
              const unsigned phaseIdx,
              const unsigned rootIdx,
              const EOSType& eosType)
    {
        using Eval1 = DenseAd::Evaluation<Scalar, 1>;
        using ParamCache = typename FluidSystem::template ParameterCache<Eval1>;

        const auto seeded = temperatureSeededCopy_(fluidState, phaseIdx, rootIdx);
        ParamCache paramCache(eosType);
        paramCache.updatePhase(seeded, rootIdx);

        const Eval1& T = seeded.temperature(rootIdx);
        const Eval1& p = seeded.pressure(rootIdx);
        const Eval1 A = paramCache.A(rootIdx);
        const Eval1 B = paramCache.B(rootIdx);
        const Eval1 m1 = paramCache.m1(rootIdx);
        const Eval1 m2 = paramCache.m2(rootIdx);
        const Eval1 Z = p * paramCache.molarVolume(rootIdx) / (Constants<Scalar>::R * T);

        Eval1 sumLnPhi = Z - 1.0 - log(Z - B)
                       + A / ((m1 - m2) * B) * log((Z + m2 * B) / (Z + m1 * B));
        if constexpr (requires { FluidSystem::volumeShift(0u); }) {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
                sumLnPhi -= seeded.moleFraction(rootIdx, compIdx)
                            * FluidSystem::volumeShift(compIdx)
                            * paramCache.Bi(rootIdx, compIdx);
            }
        }
        return sumLnPhi;
    }

    //! -R T^2 d/dT [sum_i w_i ln(phi_i)] of phase phaseIdx, from sumLnPhi_.
    template <class FluidState>
    static Scalar residualEnthalpy_(const FluidState& fluidState,
                                    const unsigned phaseIdx,
                                    const DenseAd::Evaluation<Scalar, 1>& sumLnPhi)
    {
        const Scalar temperature = Opm::getValue(fluidState.temperature(phaseIdx));
        return -Constants<Scalar>::R * temperature * temperature * sumLnPhi.derivative(0);
    }

    /*!
     * \brief Residual enthalpy of the single phase phaseIdx on the cubic root
     *        with the lower Gibbs energy. At equal (T, P, w) the Gibbs energies
     *        of the two roots differ by RT times the difference of
     *        sum_i w_i ln(phi_i); the volume translation is the same on both.
     */
    template <class FluidState>
    static Scalar stableRootResidualEnthalpy_(const FluidState& fluidState,
                                              const unsigned phaseIdx,
                                              const EOSType& eosType)
    {
        const auto onLiquidRoot = sumLnPhi_(fluidState, phaseIdx, FluidSystem::oilPhaseIdx, eosType);
        const auto onVapourRoot = sumLnPhi_(fluidState, phaseIdx, FluidSystem::gasPhaseIdx, eosType);
        return residualEnthalpy_(fluidState, phaseIdx,
                                 onLiquidRoot.value() <= onVapourRoot.value() ? onLiquidRoot
                                                                              : onVapourRoot);
    }
};

} // namespace Opm

#endif // OPM_MIXTURE_ENTHALPY_HPP
