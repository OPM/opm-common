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
 * \copydoc Opm::LiveOilPvt
 */
#ifndef OPM_LIVE_OIL_PVT_HPP
#define OPM_LIVE_OIL_PVT_HPP

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
 * \brief This class represents the Pressure-Volume-Temperature relations of the oil phas
 *        with dissolved gas.
 */
template <class Scalar>
class LiveOilPvt
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedTwoDFunction = UniformXTabulated2DFunction<Scalar>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the oil parameters via the data specified by the PVTO ECL keyword.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);

private:
    void extendPvtoTable_(unsigned regionIdx,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable);

public:
#endif // HAVE_ECL_INPUT

    void setNumRegions(size_t numRegions);

    void setVapPars(const Scalar, const Scalar par2)
    {
        vapPar2_ = par2;
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/);

    /*!
     * \brief Initialize the function for the gas dissolution factor \f$R_s\f$
     *
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedOilGasDissolutionFactor(unsigned regionIdx, const SamplingPoints& samplePoints)
    { saturatedGasDissolutionFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }

    /*!
     * \brief Initialize the function for the oil formation volume factor
     *
     * The oil formation volume factor \f$B_o\f$ is a function of \f$(p_o, X_o^G)\f$ and
     * represents the partial density of the oil component in the oil phase at a given
     * pressure. This method only requires the volume factor of gas-saturated oil (which
     * only depends on pressure) while the dependence on the gas mass fraction is
     * guesstimated.
     */
    void setSaturatedOilFormationVolumeFactor(unsigned regionIdx,
                                              const SamplingPoints& samplePoints);

    /*!
     * \brief Initialize the function for the oil formation volume factor
     *
     * The oil formation volume factor \f$B_o\f$ is a function of \f$(p_o, X_o^G)\f$ and
     * represents the partial density of the oil component in the oil phase at a given
     * pressure.
     *
     * This method sets \f$1/B_o(R_s, p_o)\f$. Note that instead of the mass fraction of
     * the gas component in the oil phase, this function depends on the gas dissolution
     * factor. Also note, that the order of the arguments needs to be \f$(R_s, p_o)\f$
     * and not the other way around.
     */
    void setInverseOilFormationVolumeFactor(unsigned regionIdx, const TabulatedTwoDFunction& invBo)
    { inverseOilBTable_[regionIdx] = invBo; }

    /*!
     * \brief Initialize the viscosity of the oil phase.
     *
     * This is a function of \f$(R_s, p_o)\f$...
     */
    void setOilViscosity(unsigned regionIdx, const TabulatedTwoDFunction& muo)
    { oilMuTable_[regionIdx] = muo; }

    /*!
     * \brief Initialize the phase viscosity for gas saturated oil
     *
     * The oil viscosity is a function of \f$(p_o, X_o^G)\f$, but this method only
     * requires the viscosity of gas-saturated oil (which only depends on pressure) while
     * there is assumed to be no dependence on the gas mass fraction...
     */
    void setSaturatedOilViscosity(unsigned regionIdx,
                                  const SamplingPoints& samplePoints);

    /*!
     * \brief Finish initializing the oil phase PVT properties.
     */
    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { return inverseOilBMuTable_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of oil given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned,
                        const Evaluation&,
                        const Evaluation&,
                        const Evaluation&) const
    {
        throw std::runtime_error("Requested the enthalpy of oil but the thermal option is not enabled");
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
                         const Evaluation& Rs) const
    {
        // ATTENTION: Rs is the first axis!
        const Evaluation& invBo = inverseOilBTable_[regionIdx].eval(Rs, pressure, /*extrapolate=*/true);
        const Evaluation& invMuoBo = inverseOilBMuTable_[regionIdx].eval(Rs, pressure, /*extrapolate=*/true);

        return invBo/invMuoBo;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& pressure) const
    {
        // ATTENTION: Rs is the first axis!
        const Evaluation& invBo = inverseSaturatedOilBTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMuoBo = inverseSaturatedOilBMuTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBo/invMuoBo;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& /*temperature*/,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs) const
    {
        // ATTENTION: Rs is represented by the _first_ axis!
        return inverseOilBTable_[regionIdx].eval(Rs, pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& /*temperature*/,
                                                     const Evaluation& pressure) const
    {
        // ATTENTION: Rs is represented by the _first_ axis!
        return inverseSaturatedOilBTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& pressure) const
    { return saturatedGasDissolutionFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     *
     * This variant of the method prevents all the oil to be vaporized even if the gas
     * phase is still not saturated. This is physically quite dubious but it corresponds
     * to how the Eclipse 100 simulator handles this. (cf the VAPPARS keyword.)
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& pressure,
                                             const Evaluation& oilSaturation,
                                             Evaluation maxOilSaturation) const
    {
        Evaluation tmp =
            saturatedGasDissolutionFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        // apply the vaporization parameters for the gas phase (cf. the Eclipse VAPPARS
        // keyword)
        maxOilSaturation = min(maxOilSaturation, Scalar(1.0));
        if (vapPar2_ > 0.0 && maxOilSaturation > 0.01 && oilSaturation < maxOilSaturation) {
            constexpr const Scalar eps = 0.001;
            const Evaluation& So = max(oilSaturation, eps);
            tmp *= max(1e-3, pow(So/maxOilSaturation, vapPar2_));
        }

        return tmp;
    }

    /*!
     * \brief Returns the saturation pressure of the oil phase [Pa]
     *        depending on its mass fraction of the gas component
     *
     * \param Rs The surface volume of gas component dissolved in what will yield one cubic meter of oil at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation&,
                                  const Evaluation& Rs) const
    {
        typedef MathToolbox<Evaluation> Toolbox;

        const auto& RsTable = saturatedGasDissolutionFactorTable_[regionIdx];
        constexpr const Scalar eps = std::numeric_limits<typename Toolbox::Scalar>::epsilon()*1e6;

        // use the saturation pressure function to get a pretty good initial value
        Evaluation pSat = saturationPressure_[regionIdx].eval(Rs, /*extrapolate=*/true);

        // Newton method to do the remaining work. If the initial
        // value is good, this should only take two to three
        // iterations...
        bool onProbation = false;
        for (int i = 0; i < 20; ++i) {
            const Evaluation& f = RsTable.eval(pSat, /*extrapolate=*/true) - Rs;
            const Evaluation& fPrime = RsTable.evalDerivative(pSat, /*extrapolate=*/true);

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
            ", Rs = " + std::to_string(getValue(Rs));
        OpmLog::debug("Live oil saturation pressure", msg);
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

    const std::vector<TabulatedTwoDFunction>& inverseOilBTable() const
    { return inverseOilBTable_; }

    const std::vector<TabulatedTwoDFunction>& oilMuTable() const
    { return oilMuTable_; }

    const std::vector<TabulatedTwoDFunction>& inverseOilBMuTable() const
    { return inverseOilBMuTable_; }

    const std::vector<TabulatedOneDFunction>& saturatedOilMuTable() const
    { return saturatedOilMuTable_; }

    const std::vector<TabulatedOneDFunction>& inverseSaturatedOilBTable() const
    { return inverseSaturatedOilBTable_; }

    const std::vector<TabulatedOneDFunction>& inverseSaturatedOilBMuTable() const
    { return inverseSaturatedOilBMuTable_; }

    const std::vector<TabulatedOneDFunction>& saturatedGasDissolutionFactorTable() const
    { return saturatedGasDissolutionFactorTable_; }

    const std::vector<TabulatedOneDFunction>& saturationPressure() const
    { return saturationPressure_; }

    Scalar vapPar2() const
    { return vapPar2_; }

private:
    void updateSaturationPressure_(unsigned regionIdx);

    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> oilReferenceDensity_;
    std::vector<TabulatedTwoDFunction> inverseOilBTable_;
    std::vector<TabulatedTwoDFunction> oilMuTable_;
    std::vector<TabulatedTwoDFunction> inverseOilBMuTable_;
    std::vector<TabulatedOneDFunction> saturatedOilMuTable_;
    std::vector<TabulatedOneDFunction> inverseSaturatedOilBTable_;
    std::vector<TabulatedOneDFunction> inverseSaturatedOilBMuTable_;
    std::vector<TabulatedOneDFunction> saturatedGasDissolutionFactorTable_;
    std::vector<TabulatedOneDFunction> saturationPressure_;

    Scalar vapPar2_ = 0.0;
};

} // namespace Opm

#endif
