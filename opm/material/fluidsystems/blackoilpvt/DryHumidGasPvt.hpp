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
 * \copydoc Opm::DryHumidGasPvt
 */
#ifndef OPM_DRY_HUMID_GAS_PVT_HPP
#define OPM_DRY_HUMID_GAS_PVT_HPP

#include <opm/common/Exceptions.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/material/Constants.hpp>
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
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 *        with vaporized water.
 */
template <class Scalar>
class DryHumidGasPvt
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedTwoDFunction = UniformXTabulated2DFunction<Scalar>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for wet gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVTGW keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule&);

private:
    void extendPvtgwTable_(unsigned regionIdx,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable);
public:
#endif // HAVE_ECL_INPUT

    void setNumRegions(size_t numRegions)
    {
        waterReferenceDensity_.resize(numRegions);
        gasReferenceDensity_.resize(numRegions);
        inverseGasB_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        inverseGasBMu_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        inverseSaturatedGasB_.resize(numRegions);
        inverseSaturatedGasBMu_.resize(numRegions);
        gasMu_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        saturatedWaterVaporizationFactorTable_.resize(numRegions);
        saturationPressure_.resize(numRegions);
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar /*rhoRefOil*/,
                               Scalar rhoRefGas,
                               Scalar rhoRefWater)
    {
        waterReferenceDensity_[regionIdx] = rhoRefWater;
        gasReferenceDensity_[regionIdx] = rhoRefGas;
    }

    /*!
     * \brief Initialize the function for the oil vaporization factor \f$R_v\f$
     *
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedGasWaterVaporizationFactor(unsigned regionIdx, const SamplingPoints& samplePoints)
    { saturatedWaterVaporizationFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }

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
    void setSaturatedGasViscosity(unsigned regionIdx, const SamplingPoints& samplePoints  )
    {
        auto& waterVaporizationFac = saturatedWaterVaporizationFactorTable_[regionIdx];

        constexpr const Scalar RwMin = 0.0;
        Scalar RwMax = waterVaporizationFac.eval(saturatedWaterVaporizationFactorTable_[regionIdx].xMax(), /*extrapolate=*/true);

        Scalar poMin = samplePoints.front().first;
        Scalar poMax = samplePoints.back().first;

        constexpr const size_t nRw = 20;
        size_t nP = samplePoints.size()*2;

        TabulatedOneDFunction mugTable;
        mugTable.setContainerOfTuples(samplePoints);

        // calculate a table of estimated densities depending on pressure and gas mass
        // fraction
        for (size_t RwIdx = 0; RwIdx < nRw; ++RwIdx) {
            Scalar Rw = RwMin + (RwMax - RwMin)*RwIdx/nRw;

            gasMu_[regionIdx].appendXPos(Rw);

            for (size_t pIdx = 0; pIdx < nP; ++pIdx) {
                Scalar pg = poMin + (poMax - poMin)*pIdx/nP;
                Scalar mug = mugTable.eval(pg, /*extrapolate=*/true);

                gasMu_[regionIdx].appendSamplePoint(RwIdx, pg, mug);
            }
        }
    }

    /*!
     * \brief Finish initializing the gas phase PVT properties.
     */
    void initEnd()
    {
        // calculate the final 2D functions which are used for interpolation.
        size_t numRegions = gasMu_.size();
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            // calculate the table which stores the inverse of the product of the gas
            // formation volume factor and the gas viscosity
            const auto& gasMu = gasMu_[regionIdx];
            const auto& invGasB = inverseGasB_[regionIdx];
            assert(gasMu.numX() == invGasB.numX());

            auto& invGasBMu = inverseGasBMu_[regionIdx];
            auto& invSatGasB = inverseSaturatedGasB_[regionIdx];
            auto& invSatGasBMu = inverseSaturatedGasBMu_[regionIdx];

            std::vector<Scalar> satPressuresArray;
            std::vector<Scalar> invSatGasBArray;
            std::vector<Scalar> invSatGasBMuArray;
            for (size_t pIdx = 0; pIdx < gasMu.numX(); ++pIdx) {
                invGasBMu.appendXPos(gasMu.xAt(pIdx));

                assert(gasMu.numY(pIdx) == invGasB.numY(pIdx));

                size_t numRw = gasMu.numY(pIdx);
                for (size_t RwIdx = 0; RwIdx < numRw; ++RwIdx)
                    invGasBMu.appendSamplePoint(pIdx,
                                                gasMu.yAt(pIdx, RwIdx),
                                                invGasB.valueAt(pIdx, RwIdx)
                                                / gasMu.valueAt(pIdx, RwIdx));

                // the sampling points in UniformXTabulated2DFunction are always sorted
                // in ascending order. Thus, the value for saturated gas is the last one
                // (i.e., the one with the largest Rw value)
                satPressuresArray.push_back(gasMu.xAt(pIdx));
                invSatGasBArray.push_back(invGasB.valueAt(pIdx, numRw - 1));
                invSatGasBMuArray.push_back(invGasBMu.valueAt(pIdx, numRw - 1));
            }

            invSatGasB.setXYContainers(satPressuresArray, invSatGasBArray);
            invSatGasBMu.setXYContainers(satPressuresArray, invSatGasBMuArray);

            updateSaturationPressure_(regionIdx);
        }
    }

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
                        const Evaluation&) const
    {
        throw std::runtime_error("Requested the enthalpy of gas but the thermal option is not enabled");
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& /*temperature*/,
                         const Evaluation& pressure,
                         const Evaluation& /*Rv*/,
                         const Evaluation& Rvw) const
    {
        const Evaluation& invBg = inverseGasB_[regionIdx].eval(pressure, Rvw, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseGasBMu_[regionIdx].eval(pressure, Rvw, /*extrapolate=*/true);

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
                                            const Evaluation& /*Rv*/,
                                            const Evaluation& Rvw) const
    { return inverseGasB_[regionIdx].eval(pressure, Rvw, /*extrapolate=*/true); }

    /*!
     * \brief Returns the formation volume factor [-] of water saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& /*temperature*/,
                                                     const Evaluation& pressure) const
    { return inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure) const
    {
        return saturatedWaterVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
    */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure,
                                              const Evaluation& saltConcentration) const
    {
        if (enableRwgSalt_)
            return saturatedWaterVaporizationSaltFactorTable_[regionIdx].eval(pressure, saltConcentration, /*extrapolate=*/true);
        else {
            return saturatedWaterVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
        }
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/,
                                              const Evaluation& /*oilSaturation*/,
                                              const Evaluation& /*maxOilSaturation*/) const
    { return 0.0; /* this is dry humid gas! */ }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/) const
    { return 0.0; /* this is dry humid gas! */ }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the water component
     *
     * \param Rw The surface volume of water component dissolved in what will yield one
     *           cubic meter of gas at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation&,
                                  const Evaluation& Rw) const
    {
        typedef MathToolbox<Evaluation> Toolbox;

        const auto& RwTable = saturatedWaterVaporizationFactorTable_[regionIdx];
        const Scalar eps = std::numeric_limits<typename Toolbox::Scalar>::epsilon()*1e6;

        // use the tabulated saturation pressure function to get a pretty good initial value
        Evaluation pSat = saturationPressure_[regionIdx].eval(Rw, /*extrapolate=*/true);

        // Newton method to do the remaining work. If the initial
        // value is good, this should only take two to three
        // iterations...
        bool onProbation = false;
        for (unsigned i = 0; i < 20; ++i) {
            const Evaluation& f = RwTable.eval(pSat, /*extrapolate=*/true) - Rw;
            const Evaluation& fPrime = RwTable.evalDerivative(pSat, /*extrapolate=*/true);

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
            " pSat = " + std::to_string(getValue(pSat)) +
            ", Rw = " + std::to_string(getValue(Rw));
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

    Scalar waterReferenceDensity(unsigned regionIdx) const
    { return waterReferenceDensity_[regionIdx]; }

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

    const std::vector<TabulatedOneDFunction>& saturatedWaterVaporizationFactorTable() const {
        return saturatedWaterVaporizationFactorTable_;
    }

    const std::vector<TabulatedTwoDFunction>& saturatedWaterVaporizationSaltFactorTable() const {
        return saturatedWaterVaporizationSaltFactorTable_;
    }

    const std::vector<TabulatedOneDFunction>& saturationPressure() const {
        return saturationPressure_;
    }

    Scalar vapPar1() const {
        return vapPar1_;
    }

private:
    void updateSaturationPressure_(unsigned regionIdx)
    {
        typedef std::pair<Scalar, Scalar> Pair;
        const auto& waterVaporizationFac = saturatedWaterVaporizationFactorTable_[regionIdx];

        // create the taublated function representing saturation pressure depending of
        // Rw
        size_t n = waterVaporizationFac.numSamples();
        Scalar delta = (waterVaporizationFac.xMax() - waterVaporizationFac.xMin())/Scalar(n + 1);

        SamplingPoints pSatSamplePoints;
        Scalar Rw = 0;
        for (size_t i = 0; i <= n; ++ i) {
            Scalar pSat = waterVaporizationFac.xMin() + Scalar(i)*delta;
            Rw = saturatedWaterVaporizationFactor(regionIdx, /*temperature=*/Scalar(1e30), pSat);

            Pair val(Rw, pSat);
            pSatSamplePoints.push_back(val);
        }

        //Prune duplicate Rv values (can occur, and will cause problems in further interpolation)
        auto x_coord_comparator = [](const Pair& a, const Pair& b) { return a.first == b.first; };
        auto last = std::unique(pSatSamplePoints.begin(), pSatSamplePoints.end(), x_coord_comparator);
        if (std::distance(pSatSamplePoints.begin(), last) > 1) // only remove them if there are more than two points
            pSatSamplePoints.erase(last, pSatSamplePoints.end());

        saturationPressure_[regionIdx].setContainerOfTuples(pSatSamplePoints);
    }

    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> waterReferenceDensity_;
    std::vector<TabulatedTwoDFunction> inverseGasB_;
    std::vector<TabulatedOneDFunction> inverseSaturatedGasB_;
    std::vector<TabulatedTwoDFunction> gasMu_;
    std::vector<TabulatedTwoDFunction> inverseGasBMu_;
    std::vector<TabulatedOneDFunction> inverseSaturatedGasBMu_;
    std::vector<TabulatedOneDFunction> saturatedWaterVaporizationFactorTable_;
    std::vector<TabulatedTwoDFunction> saturatedWaterVaporizationSaltFactorTable_;
    std::vector<TabulatedOneDFunction> saturationPressure_;

    bool enableRwgSalt_ = false;
    Scalar vapPar1_ = 0.0;
};

} // namespace Opm

#endif
