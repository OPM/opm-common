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
 * \copydoc Opm::DeadOilPvt
 */
#ifndef OPM_DEAD_OIL_PVT_HPP
#define OPM_DEAD_OIL_PVT_HPP

#include <opm/material/common/Tabulated1DFunction.hpp>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the oil phase
 *        without dissolved gas.
 */
template <class Scalar>
class DeadOilPvt
{
public:
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the oil parameters via the data specified by the PVDO ECL keyword.
     */
    void initFromState(const EclipseState& eclState, const Schedule&);
#endif // HAVE_ECL_INPUT

    void setNumRegions(size_t numRegions);

    void setVapPars(const Scalar, const Scalar)
    {
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar /*rhoRefGas*/,
                               Scalar /*rhoRefWater*/)
    {
        oilReferenceDensity_[regionIdx] = rhoRefOil;
    }

    /*!
     * \brief Initialize the function for the oil formation volume factor
     *
     * The oil formation volume factor \f$B_o\f$ is a function of \f$(p_o, X_o^G)\f$ and
     * represents the partial density of the oil component in the oil phase at a given
     * pressure.
     *
     * This method sets \f$1/B_o(p_o)\f$. Note that the mass fraction of the gas
     * component in the oil phase is missing when assuming dead oil.
     */
    void setInverseOilFormationVolumeFactor(unsigned regionIdx, const TabulatedOneDFunction& invBo)
    { inverseOilB_[regionIdx] = invBo; }

    /*!
     * \brief Initialize the viscosity of the oil phase.
     *
     * This is a function of \f$(R_s, p_o)\f$...
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
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rs*/) const
    { return saturatedViscosity(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of gas saturated oil given a pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& pressure) const
    {
        const Evaluation& invBo = inverseOilB_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMuoBo = inverseOilBMu_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBo/invMuoBo;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& /*temperature*/,
                                            const Evaluation& pressure,
                                            const Evaluation& /*Rs*/) const
    { return inverseOilB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the formation volume factor [-] of saturated oil.
     *
     * Note that by definition, dead oil is always gas saturated.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure) const
    { return inverseOilB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& /*pressure*/) const
    { return 0.0; /* this is dead oil! */ }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& /*pressure*/,
                                             const Evaluation& /*oilSaturation*/,
                                             const Evaluation& /*maxOilSaturation*/) const
    { return 0.0; /* this is dead oil! */ }

    /*!
     * \brief Returns the saturation pressure of the oil phase [Pa]
     *        depending on its mass fraction of the gas component
     *
     * \param Rs The surface volume of gas component dissolved in what will yield one cubic meter of oil at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rs*/) const
    { return 0.0; /* this is dead oil, so there isn't any meaningful saturation pressure! */ }

    template <class Evaluation>
    Evaluation saturatedGasMassFraction(unsigned /*regionIdx*/,
                                        const Evaluation& /*temperature*/,
                                        const Evaluation& /*pressure*/) const
    { return 0.0; /* this is dead oil! */ }

    template <class Evaluation>
    Evaluation saturatedGasMoleFraction(unsigned /*regionIdx*/,
                                        const Evaluation& /*temperature*/,
                                        const Evaluation& /*pressure*/) const
    { return 0.0; /* this is dead oil! */ }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                    const Evaluation& /*pressure*/,
                                    unsigned /*compIdx*/) const
    {
        throw std::runtime_error("Not implemented: The PVT model does not provide a diffusionCoefficient()");
    }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    { return oilReferenceDensity_[regionIdx]; }

    const std::vector<TabulatedOneDFunction>& inverseOilB() const
    { return inverseOilB_; }

    const std::vector<TabulatedOneDFunction>& oilMu() const
    { return oilMu_; }

    const std::vector<TabulatedOneDFunction>& inverseOilBMu() const
    { return inverseOilBMu_; }

private:
    std::vector<Scalar> oilReferenceDensity_;
    std::vector<TabulatedOneDFunction> inverseOilB_;
    std::vector<TabulatedOneDFunction> oilMu_;
    std::vector<TabulatedOneDFunction> inverseOilBMu_;
};

} // namespace Opm

#endif
