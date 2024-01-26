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
 * \copydoc Opm::WetGasPvt
 */
#ifndef OPM_WET_GAS_PVT_HPP
#define OPM_WET_GAS_PVT_HPP

#include <opm/common/Exceptions.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
class SimpleTable;
#endif

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phas
 *        with vaporized oil.
 */
template <class Scalar>
class WetGasPvt
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedTwoDFunction = UniformXTabulated2DFunction<Scalar>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for wet gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVTG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);

private:
    void extendPvtgTable_(unsigned regionIdx,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable);

public:
#endif // HAVE_ECL_INPUT

    void setNumRegions(size_t numRegions);

    void setVapPars(const Scalar par1, const Scalar)
    {
        vapPar1_ = par1;
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/);

    /*!
     * \brief Initialize the function for the oil vaporization factor \f$R_v\f$
     *
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedGasOilVaporizationFactor(unsigned regionIdx, const SamplingPoints& samplePoints)
    { saturatedOilVaporizationFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }

    /*!
     * \brief Initialize the function for the gas formation volume factor
     *
     * The gas formation volume factor \f$B_g\f$ is a function of \f$(p_g, X_g^O)\f$ and
     * represents the partial density of the oil component in the gas phase at a given
     * pressure. This method only requires the volume factor of oil-saturated gas (which
     * only depends on pressure) while the dependence on the oil mass fraction is
     * guesstimated...
     */
    void setSaturatedGasFormationVolumeFactor(unsigned regionIdx,
                                              const SamplingPoints& samplePoints);

    /*!
     * \brief Initialize the function for the gas formation volume factor
     *
     * The gas formation volume factor \f$B_g\f$ is a function of \f$(p_g, X_g^O)\f$ and
     * represents the partial density of the oil component in the gas phase at a given
     * pressure.
     *
     * This method sets \f$1/B_g(R_v, p_g)\f$. Note that instead of the mass fraction of
     * the oil component in the gas phase, this function depends on the gas dissolution
     * factor. Also note, that the order of the arguments needs to be \f$(R_s, p_o)\f$
     * and not the other way around.
     */
    void setInverseGasFormationVolumeFactor(unsigned regionIdx, const TabulatedTwoDFunction& invBg)
    { inverseGasB_[regionIdx] = invBg; }

    /*!
     * \brief Initialize the viscosity of the gas phase.
     *
     * This is a function of \f$(R_s, p_o)\f$...
     */
    void setGasViscosity(unsigned regionIdx, const TabulatedTwoDFunction& mug)
    { gasMu_[regionIdx] = mug; }

    /*!
     * \brief Initialize the phase viscosity for oil saturated gas
     *
     * The gas viscosity is a function of \f$(p_g, X_g^O)\f$, but this method only
     * requires the viscosity of oil-saturated gas (which only depends on pressure) while
     * there is assumed to be no dependence on the gas mass fraction...
     */
    void setSaturatedGasViscosity(unsigned regionIdx,
                                  const SamplingPoints& samplePoints);

    /*!
     * \brief Finish initializing the gas phase PVT properties.
     */
    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned,
                        const Evaluation&,
                        const Evaluation&,
                        const Evaluation&,
                        const Evaluation&) const
    {
        throw std::runtime_error("Requested the enthalpy of gas but the thermal option is not enabled");
    }
    Scalar hVap(unsigned) const{
        throw std::runtime_error("Requested the hvap of oil but the thermal option is not enabled");
    }
    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& /*temperature*/,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& /*Rvw*/) const
    {
        const Evaluation& invBg = inverseGasB_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseGasBMu_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);

        return invBg/invMugBg;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& pressure) const
    {
        const Evaluation& invBg = inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseSaturatedGasBMu_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBg/invMugBg;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& /*temperature*/,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& /*Rvw*/) const
    { return inverseGasB_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true); }

    /*!
     * \brief Returns the formation volume factor [-] of oil saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& /*temperature*/,
                                                     const Evaluation& pressure) const
    { return inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the gasphase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/) const
    { return 0.0; /* this is non-humid gas! */ }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
    */
    template <class Evaluation = Scalar>
    Evaluation saturatedWaterVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/,
                                              const Evaluation& /*saltConcentration*/) const
    { return 0.0; }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the gas phase.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure) const
    {
        return saturatedOilVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the gas phase.
     *
     * This variant of the method prevents all the oil to be vaporized even if the gas
     * phase is still not saturated. This is physically quite dubious but it corresponds
     * to how the Eclipse 100 simulator handles this. (cf the VAPPARS keyword.)
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              Evaluation maxOilSaturation) const
    {
        Evaluation tmp =
            saturatedOilVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        // apply the vaporization parameters for the gas phase (cf. the Eclipse VAPPARS
        // keyword)
        maxOilSaturation = min(maxOilSaturation, Scalar(1.0));
        if (vapPar1_ > 0.0 && maxOilSaturation > 0.01 && oilSaturation < maxOilSaturation) {
            constexpr const Scalar eps = 0.001;
            const Evaluation& So = max(oilSaturation, eps);
            tmp *= max(1e-3, pow(So/maxOilSaturation, vapPar1_));
        }

        return tmp;
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the oil component
     *
     * This method uses the standard blackoil assumptions: This means that the Rv value
     * does not depend on the saturation of oil. (cf. the Eclipse VAPPARS keyword.)
     *
     * \param Rv The surface volume of oil component dissolved in what will yield one
     *           cubic meter of gas at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation&,
                                  const Evaluation& Rv) const
    {
        typedef MathToolbox<Evaluation> Toolbox;

        const auto& RvTable = saturatedOilVaporizationFactorTable_[regionIdx];
        constexpr const Scalar eps = std::numeric_limits<typename Toolbox::Scalar>::epsilon()*1e6;

        // use the tabulated saturation pressure function to get a pretty good initial value
        Evaluation pSat = saturationPressure_[regionIdx].eval(Rv, /*extrapolate=*/true);

        // Newton method to do the remaining work. If the initial
        // value is good, this should only take two to three
        // iterations...
        bool onProbation = false;
        for (unsigned i = 0; i < 20; ++i) {
            const Evaluation& f = RvTable.eval(pSat, /*extrapolate=*/true) - Rv;
            const Evaluation& fPrime = RvTable.evalDerivative(pSat, /*extrapolate=*/true);

            // If the derivative is "zero" Newton will not converge,
            // so simply return our initial guess.
            if (std::abs(scalarValue(fPrime)) < 1.0e-30) {
                return pSat;
            }

            const Evaluation& delta = f/fPrime;

            pSat -= delta;

            if (pSat < 0.0) {
                // if the pressure is lower than 0 Pascals, we set it back to 0. if this
                // happens twice, we give up and just return 0 Pa...
                if (onProbation)
                    return 0.0;

                onProbation = true;
                pSat = 0.0;
            }

            if (std::abs(scalarValue(delta)) < std::abs(scalarValue(pSat))*eps)
                return pSat;
        }

        const std::string msg =
            "Finding saturation pressure did not converge: "
            "pSat = " + std::to_string(getValue(pSat)) +
            ", Rv = " + std::to_string(getValue(Rv));
        OpmLog::debug("Wet gas saturation pressure", msg);
        throw NumericalProblem(msg);
    }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                    const Evaluation& /*pressure*/,
                                    unsigned /*compIdx*/) const
    {
        throw std::runtime_error("Not implemented: The PVT model does not provide a diffusionCoefficient()");
    }

    Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    { return oilReferenceDensity_[regionIdx]; }

    const std::vector<TabulatedTwoDFunction>& inverseGasB() const {
        return inverseGasB_;
    }

    const std::vector<TabulatedOneDFunction>& inverseSaturatedGasB() const {
        return inverseSaturatedGasB_;
    }

    const std::vector<TabulatedTwoDFunction>& gasMu() const {
        return gasMu_;
    }

    const std::vector<TabulatedTwoDFunction>& inverseGasBMu() const {
        return inverseGasBMu_;
    }

    const std::vector<TabulatedOneDFunction>& inverseSaturatedGasBMu() const {
        return inverseSaturatedGasBMu_;
    }

    const std::vector<TabulatedOneDFunction>& saturatedOilVaporizationFactorTable() const {
        return saturatedOilVaporizationFactorTable_;
    }

    const std::vector<TabulatedOneDFunction>& saturationPressure() const {
        return saturationPressure_;
    }

    Scalar vapPar1() const {
        return vapPar1_;
    }

private:
    void updateSaturationPressure_(unsigned regionIdx);

    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> oilReferenceDensity_;
    std::vector<TabulatedTwoDFunction> inverseGasB_;
    std::vector<TabulatedOneDFunction> inverseSaturatedGasB_;
    std::vector<TabulatedTwoDFunction> gasMu_;
    std::vector<TabulatedTwoDFunction> inverseGasBMu_;
    std::vector<TabulatedOneDFunction> inverseSaturatedGasBMu_;
    std::vector<TabulatedOneDFunction> saturatedOilVaporizationFactorTable_;
    std::vector<TabulatedOneDFunction> saturationPressure_;

    Scalar vapPar1_ = 0.0;
};

} // namespace Opm

#endif
