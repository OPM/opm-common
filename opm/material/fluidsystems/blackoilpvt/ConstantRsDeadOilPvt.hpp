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
 * \copydoc Opm::ConstantRsDeadOilPvt
 */
#ifndef OPM_CONSTANT_RS_DEAD_OIL_PVT_HPP
#define OPM_CONSTANT_RS_DEAD_OIL_PVT_HPP

#include <opm/material/common/Tabulated1DFunction.hpp>

#include <cstddef>
#include <vector>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of dead oil
 *        with constant dissolved gas (RSCONST keyword).
 * 
 * RSCONST provides two global values: constant Rs and constant bubble point pressure.
 * Oil properties (Bo, μo) come from PVDO tables and are independent of Rs.
 */
template <class Scalar>
class ConstantRsDeadOilPvt
{
public:
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the oil parameters via the data specified by the PVDO ECL keyword
     *        with additional constant Rs from RSCONST.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    void setNumRegions(std::size_t numRegions);

    void setVapPars(const Scalar, const Scalar)
    {
        // No vaporization parameters for constant Rs
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/)
    {
        oilReferenceDensity_[regionIdx] = rhoRefOil;
        gasReferenceDensity_[regionIdx] = rhoRefGas;
    }

    /*!
     * \brief Set the constant Rs value (global for all regions)
     */
    void setConstantRs(Scalar rsConst)
    { constantRs_ = rsConst; }

    /*!
     * \brief Set the bubble point pressure (global for all regions)
     */
    void setBubblePointPressure(Scalar pbub)
    { bubblePointPressure_ = pbub; }

    /*!
     * \brief Initialize the function for the oil formation volume factor
     */
    void setInverseOilFormationVolumeFactor(unsigned regionIdx,
                                            const TabulatedOneDFunction& invBo)
    { inverseOilB_[regionIdx] = invBo; }

    /*!
     * \brief Initialize the viscosity of the oil phase.
     */
    void setOilViscosity(unsigned regionIdx, const TabulatedOneDFunction& muo)
    { oilMu_[regionIdx] = muo; }

    /*!
     * \brief Finish initializing the oil phase PVT properties.
     */
    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { return inverseOilBMu_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of oil given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned,
                        const Evaluation&,
                        const Evaluation&,
                        const Evaluation&) const
    {
        throw std::runtime_error("Requested the enthalpy of oil but the thermal "
                                 "option is not enabled");
    }

    Scalar hVap(unsigned) const
    {
        throw std::runtime_error("Requested the hvap of oil but the thermal "
                                 "option is not enabled");
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     *        Rs is ignored - viscosity depends only on pressure via PVDO table.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rs*/) const
    { return saturatedViscosity(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil given a pressure.
     *        Uses PVDO table directly.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& pressure) const
    {
        const Evaluation& invBo = inverseOilB_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMuoBo = inverseOilBMu_[regionIdx].eval(pressure, /*extrapolate=*/true);
        return invBo / invMuoBo;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     *        Rs is ignored - Bo depends only on pressure via PVDO table.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& /*temperature*/,
                                            const Evaluation& pressure,
                                            const Evaluation& /*Rs*/) const
    { return inverseOilB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the formation volume factor [-] and viscosity [Pa s] of the fluid phase.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    std::pair<LhsEval, LhsEval>
    inverseFormationVolumeFactorAndViscosity(const FluidState& fluidState, unsigned regionIdx)
    {
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(FluidState::oilPhaseIdx));
        const auto segIdx = this->inverseOilB_[regionIdx].findSegmentIndex(p, /*extrapolate=*/ true);
        const auto& invBo = this->inverseOilB_[regionIdx].eval(p, SegmentIndex{segIdx});
        const auto& invMuoBo = this->inverseOilBMu_[regionIdx].eval(p, SegmentIndex{segIdx});
        return { invBo, invBo / invMuoBo };
    }

    /*!
     * \brief Returns the formation volume factor [-] of oil.
     *        Uses PVDO table directly.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& /*temperature*/,
                                                     const Evaluation& pressure) const
    { return inverseOilB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the constant gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     *        Same value for all regions.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& /*pressure*/) const
    { return constantRs_; }

    /*!
     * \brief Returns the constant gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     *        Same value for all regions.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& /*pressure*/,
                                             const Evaluation& /*oilSaturation*/,
                                             const Evaluation& /*maxOilSaturation*/) const
    { return constantRs_; }

    /*!
     * \brief Returns the bubble point pressure [Pa] from RSCONST.
     *        Same value for all regions.
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rs*/) const
    { return bubblePointPressure_; }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                    const Evaluation& /*pressure*/,
                                    unsigned /*compIdx*/) const
    {
        throw std::runtime_error("Not implemented: The PVT model does not provide "
                                 "a diffusionCoefficient()");
    }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    { return oilReferenceDensity_[regionIdx]; }

    Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    Scalar constantRs() const
    { return constantRs_; }

    Scalar bubblePointPressure() const
    { return bubblePointPressure_; }

    const std::vector<TabulatedOneDFunction>& inverseOilB() const
    { return inverseOilB_; }

    const std::vector<TabulatedOneDFunction>& oilMu() const
    { return oilMu_; }

    const std::vector<TabulatedOneDFunction>& inverseOilBMu() const
    { return inverseOilBMu_; }

private:
    std::vector<Scalar> oilReferenceDensity_{};
    std::vector<Scalar> gasReferenceDensity_{};
    std::vector<TabulatedOneDFunction> inverseOilB_{};
    std::vector<TabulatedOneDFunction> oilMu_{};
    std::vector<TabulatedOneDFunction> inverseOilBMu_{};
    
    // Global constants from RSCONST (same for all regions)
    Scalar constantRs_ = 0.0;           // Constant Rs value from RSCONST
    Scalar bubblePointPressure_ = 0.0;  // Bubble point pressure from RSCONST
};

} // namespace Opm

#endif
