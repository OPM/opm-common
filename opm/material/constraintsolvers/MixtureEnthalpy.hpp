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
 * \brief Mixture enthalpy for the compositional (flash) path — the property
 *        an isenthalpic (P-H) flash inverts for temperature.
 *
 * Two models behind one seam (EnthalpyModel):
 *
 * - caloric (ideal-gas): per-phase enthalpy is the composition-weighted
 *   ideal-gas enthalpy of the components, and the mixture sums the phases
 *   weighted by their mole fractions,
 *
 *       H = L*H_oil + (1-L)*H_gas,   H_phase = sum_i w_i * int_{T0}^{T} cp_i dT'.
 *
 *   With this model the phase split cancels exactly (H = sum_i z_i*h_i(T) by
 *   material balance), so H is independent of the flash result.
 *
 * - eos_departure: adds the residual (departure) enthalpy per phase,
 *
 *       H_res = -R * T^2 * sum_i w_i * dln(phi_i)/dT      (fixed P, w),
 *
 *   which makes the enthalpy consistent with the same cubic EoS that computes
 *   the phase split. dln(phi_i)/dT is obtained from the fluid system's own
 *   fugacityCoefficient evaluated on a densead state with temperature seeded
 *   as the single AD variable — no hand-derived EoS calculus. The residual
 *   differs per phase (the liquid's is of vaporization-enthalpy scale), so
 *   with this model the enthalpy genuinely depends on the flash result.
 *
 * Units: SI, molar enthalpy [J/mol]; H(T0) = 0 at the IdealGasCaloricData datum
 * (the caloric part carries the datum; the residual vanishes as P -> 0).
 */
#ifndef OPM_MIXTURE_ENTHALPY_HPP
#define OPM_MIXTURE_ENTHALPY_HPP

#include <opm/material/constraintsolvers/IdealGasCaloricData.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/Constants.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <stdexcept>
#include <string>
#include <string_view>

namespace Opm {

//! Which enthalpy model the P-H stack evaluates.
enum class EnthalpyModel { caloric, eos_departure };

//! Parse an enthalpy-model name ("caloric" / "eos_departure"); throws
//! std::runtime_error on anything else. Lives beside the enum so every
//! runtime-parameter consumer shares one parser.
inline EnthalpyModel enthalpyModelFromString(const std::string_view name)
{
    if (name == "caloric")
        return EnthalpyModel::caloric;
    if (name == "eos_departure" || name == "eos-departure")
        return EnthalpyModel::eos_departure;
    throw std::runtime_error("unknown enthalpy model '" + std::string(name)
                             + "' (valid: caloric, eos_departure)");
}

//! The canonical name of an enthalpy model (inverse of enthalpyModelFromString).
inline std::string enthalpyModelToString(const EnthalpyModel model)
{
    return model == EnthalpyModel::caloric ? "caloric" : "eos_departure";
}

/*!
 * \brief Molar mixture enthalpy of a flashed compositional state, under the
 *        caloric (ideal-gas) or EoS-consistent departure model — the property
 *        an isenthalpic (P-H) flash inverts for temperature.
 *
 * See the file documentation for the model definitions, the caloric
 * split-cancellation property, and the unit/datum conventions.
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
                  const CpTable<Scalar, numComponents>& cpTable,
                  const Scalar refTemperature)
    {
        using ValueType = typename FluidState::ValueType;

        const ValueType& T = fluidState.temperature(phaseIdx);
        ValueType h = 0.0;
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            h += fluidState.moleFraction(phaseIdx, compIdx)
                 * cpTable[compIdx].enthalpyIntegral(T, refTemperature);
        }
        return h;
    }

    /*!
     * \brief Molar enthalpy of the flashed mixture [J/mol]: phase enthalpies
     *        weighted by the liquid mole fraction L from the fluid state.
     *
     * Precondition: fluidState is a FLASHED state — L() set and the per-phase
     * compositions consistent with it (both phases at the equilibrium
     * temperature), i.e. the state PTFlash::solve returns.
     */
    template <class FluidState>
    static typename FluidState::ValueType
    mixtureEnthalpy(const FluidState& fluidState,
                    const CpTable<Scalar, numComponents>& cpTable,
                    const Scalar refTemperature)
    {
        const auto& L = fluidState.L();
        return L * phaseEnthalpy(fluidState, FluidSystem::oilPhaseIdx, cpTable, refTemperature)
             + (1.0 - L) * phaseEnthalpy(fluidState, FluidSystem::gasPhaseIdx, cpTable, refTemperature);
    }

    /*!
     * \brief Total molar heat capacity dH/dT [J/(mol K)] of the mixture,
     *        analytic, CALORIC model only.
     *
     * With an EoS-consistent (departure) enthalpy dH/dT gains a residual term
     * that this function does NOT include — do not wire it into a Newton step
     * on a departure-mode enthalpy.
     *
     * Precondition: same as mixtureEnthalpy — a flashed, L-consistent state.
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

    // ── EoS-consistent departure (residual) enthalpy ────────────────────────

    /*!
     * \brief dln(phi_i)/dT of one component in one phase at fixed pressure
     *        and phase composition [1/K].
     *
     * Evaluates the fluid system's own fugacityCoefficient on a one-variable
     * densead state (temperature seeded at slot 0; pressure and composition
     * held constant) and reads the partial: phi.derivative(0)/phi.value().
     * Exposed separately so tests can verify the AD derivative against a
     * finite difference of ln(phi).
     */
    template <class FluidState>
    static Scalar phaseDLnPhiDT(const FluidState& fluidState,
                                const unsigned phaseIdx,
                                const unsigned compIdx,
                                const EOSType& eosType)
    {
        const auto seeded = temperatureSeededCopy_(fluidState, phaseIdx);

        using Eval1 = DenseAd::Evaluation<Scalar, 1>;
        using ParamCache = typename FluidSystem::template ParameterCache<Eval1>;
        ParamCache paramCache(eosType);
        paramCache.updatePhase(seeded, phaseIdx);

        const Eval1 phi = FluidSystem::fugacityCoefficient(seeded, paramCache, phaseIdx, compIdx);
        return phi.derivative(0) / phi.value();
    }

    /*!
     * \brief Residual (departure) molar enthalpy of one phase [J/mol]:
     *        H_res = -R * T^2 * sum_i w_i * dln(phi_i)/dT  at fixed (P, w).
     *
     * Returns a plain Scalar: the residual's derivatives w.r.t. any outer AD
     * variables are NOT propagated (values only — propagating them is the
     * simulator-coupling stage's concern, not this model's).
     */
    template <class FluidState>
    static Scalar phaseResidualEnthalpy(const FluidState& fluidState,
                                        const unsigned phaseIdx,
                                        const EOSType& eosType)
    {
        const auto seeded = temperatureSeededCopy_(fluidState, phaseIdx);

        using Eval1 = DenseAd::Evaluation<Scalar, 1>;
        using ParamCache = typename FluidSystem::template ParameterCache<Eval1>;
        ParamCache paramCache(eosType);
        paramCache.updatePhase(seeded, phaseIdx);

        Scalar HresOverR = 0.0;
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            const Eval1 phi = FluidSystem::fugacityCoefficient(seeded, paramCache, phaseIdx, compIdx);
            const Scalar dLnPhiDT = phi.derivative(0) / phi.value();
            const Scalar w_i = Opm::getValue(fluidState.moleFraction(phaseIdx, compIdx));
            HresOverR -= w_i * dLnPhiDT;
        }
        const Scalar T = Opm::getValue(fluidState.temperature(phaseIdx));
        return Constants<Scalar>::R * HresOverR * T * T;
    }

    /*!
     * \brief Molar enthalpy of one phase [J/mol] under the selected model:
     *        caloric ideal-gas part, plus the EoS departure when
     *        model == eos_departure.
     */
    template <class FluidState>
    static typename FluidState::ValueType
    phaseEnthalpy(const FluidState& fluidState,
                  const unsigned phaseIdx,
                  const CpTable<Scalar, numComponents>& cpTable,
                  const Scalar refTemperature,
                  const EOSType& eosType,
                  const EnthalpyModel model)
    {
        typename FluidState::ValueType h =
            phaseEnthalpy(fluidState, phaseIdx, cpTable, refTemperature);
        if (model == EnthalpyModel::eos_departure)
            h += phaseResidualEnthalpy(fluidState, phaseIdx, eosType);
        return h;
    }

    /*!
     * \brief Molar enthalpy of the flashed mixture [J/mol] under the selected
     *        model. Precondition as for the caloric overload: a flashed,
     *        L-consistent state.
     *
     * Absent phases (weight 0) skip the residual evaluation: their
     * composition is degenerate and their contribution vanishes anyway.
     */
    template <class FluidState>
    static typename FluidState::ValueType
    mixtureEnthalpy(const FluidState& fluidState,
                    const CpTable<Scalar, numComponents>& cpTable,
                    const Scalar refTemperature,
                    const EOSType& eosType,
                    const EnthalpyModel model)
    {
        using ValueType = typename FluidState::ValueType;

        const auto& L = fluidState.L();
        const Scalar Lval = Opm::getValue(L);

        ValueType hOil = phaseEnthalpy(fluidState, FluidSystem::oilPhaseIdx, cpTable, refTemperature);
        ValueType hGas = phaseEnthalpy(fluidState, FluidSystem::gasPhaseIdx, cpTable, refTemperature);
        if (model == EnthalpyModel::eos_departure) {
            if (Lval > 0.0)
                hOil += phaseResidualEnthalpy(fluidState, FluidSystem::oilPhaseIdx, eosType);
            if (Lval < 1.0)
                hGas += phaseResidualEnthalpy(fluidState, FluidSystem::gasPhaseIdx, eosType);
        }
        return L * hOil + (1.0 - L) * hGas;
    }

private:
    /*!
     * \brief AD-typed copy of one phase with temperature as the single seeded
     *        variable (slot 0); pressure and composition enter as constants,
     *        so derivative(0) is a partial at fixed (P, w).
     */
    template <class FluidState>
    static CompositionalFluidState<DenseAd::Evaluation<Scalar, 1>, FluidSystem>
    temperatureSeededCopy_(const FluidState& fluidState, const unsigned phaseIdx)
    {
        using Eval1 = DenseAd::Evaluation<Scalar, 1>;

        CompositionalFluidState<Eval1, FluidSystem> seeded;
        seeded.setTemperature(
            Eval1::createVariable(Opm::getValue(fluidState.temperature(phaseIdx)), 0));
        const Eval1 p = Eval1::createConstant(Opm::getValue(fluidState.pressure(phaseIdx)));
        seeded.setPressure(FluidSystem::oilPhaseIdx, p);
        seeded.setPressure(FluidSystem::gasPhaseIdx, p);
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            seeded.setMoleFraction(phaseIdx, compIdx,
                Eval1::createConstant(Opm::getValue(fluidState.moleFraction(phaseIdx, compIdx))));
        }
        return seeded;
    }
};

} // namespace Opm

#endif // OPM_MIXTURE_ENTHALPY_HPP
