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
 * \copydoc Opm::Co2GasPvt
 */
#ifndef OPM_CO2_GAS_PVT_HPP
#define OPM_CO2_GAS_PVT_HPP

#include <opm/material/Constants.hpp>

#include <opm/material/components/CO2.hpp>
#include <opm/material/components/Brine.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/binarycoefficients/Brine_CO2.hpp>

#include <vector>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 * for CO2
 */
template <class Scalar>
class Co2GasPvt
{
    using CO2 = ::Opm::CO2<Scalar>;
    using H2O = SimpleHuDuanH2O<Scalar>;
    using Brine = ::Opm::Brine<Scalar, H2O>;
    static constexpr bool extrapolate = true;

public:
    //! The binary coefficients for brine and CO2 used by this fluid system
    using BinaryCoeffBrineCO2 = BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    explicit Co2GasPvt() = default;

    Co2GasPvt(const std::vector<Scalar>& salinity,
              Scalar T_ref = 288.71, //(273.15 + 15.56)
              Scalar P_ref = 101325)
        : salinity_(salinity)
    {
        int num_regions = salinity_.size();
        setNumRegions(num_regions);
        Brine::salinity = salinity[0];
        for (int i = 0; i < num_regions; ++i) {
            gasReferenceDensity_[i] = CO2::gasDensity(T_ref, P_ref, extrapolate);
            brineReferenceDensity_[i] = Brine::liquidDensity(T_ref, P_ref, extrapolate);
        }
    }
#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for CO2 gas using an ECL deck.
     */
    void initFromState(const EclipseState& eclState, const Schedule&);
#endif

    void setNumRegions(size_t numRegions)
    {
        gasReferenceDensity_.resize(numRegions);
        brineReferenceDensity_.resize(numRegions);
        salinity_.resize(numRegions);
    }


    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefBrine,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/)
    {
        gasReferenceDensity_[regionIdx] = rhoRefGas;
        brineReferenceDensity_[regionIdx] = rhoRefBrine;;
    }

    /*!
     * \brief Finish initializing the co2 phase PVT properties.
     */
    void initEnd()
    {

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
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& rv) const
    {
        Evaluation result = 0;
        const Evaluation xBrine = convertRvwToXgW_(rv,regionIdx);
        result += xBrine * Brine::gasEnthalpy(temperature, pressure);
        result += (1 - xBrine) * CO2::gasEnthalpy(temperature, pressure, extrapolate);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rv*/,
                         const Evaluation& /*Rvw*/) const
    { return saturatedViscosity(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of fluid phase at saturated conditions.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned /*regionIdx*/,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        // Neglects impact of vapporized water on the visosity
        return CO2::gasViscosity(temperature, pressure, extrapolate);
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& /*Rv*/,
                                            const Evaluation& /*Rvw*/) const
    { return saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the formation volume factor [-] of water saturated gas at given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        // Neglects impact of vapporized water on the density
        return CO2::gasDensity(temperature, pressure, extrapolate)/gasReferenceDensity_[regionIdx];
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the oil component
     *
     * \param Rv The surface volume of oil component dissolved in what will yield one cubic meter of gas at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rv*/) const
    { return 0.0; /* not implemented */ }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return rvwSat_(regionIdx, temperature, pressure); }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water phase.
    */
    template <class Evaluation = Scalar>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& /*saltConcentration*/) const
    { return rvwSat_(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& /*oilSaturation*/,
                                              const Evaluation& /*maxOilSaturation*/) const
    { return rvwSat_(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return rvwSat_(regionIdx, temperature, pressure); }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        return BinaryCoeffBrineCO2::gasDiffCoeff(temperature, pressure, extrapolate);
    }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    const Scalar oilReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    const Scalar waterReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    const Scalar salinity(unsigned regionIdx) const
    { return salinity_[regionIdx]; }

private:

    template <class LhsEval>
    LhsEval rvwSat_(unsigned regionIdx,
                    const LhsEval& temperature,
                    const LhsEval& pressure) const
    {
        if (!enableDissolution_)
            return 0.0;

        // calulate the equilibrium composition for the given
        // temperature and pressure.
        LhsEval xgH2O;
        LhsEval xlCO2;
        BinaryCoeffBrineCO2::calculateMoleFractions(temperature,
                                                    pressure,
                                                    salinity_[regionIdx],
                                                    /*knownPhaseIdx=*/-1,
                                                    xlCO2,
                                                    xgH2O,
                                                    extrapolate);

        // normalize the phase compositions
        xgH2O = max(0.0, min(1.0, xgH2O));

        return convertXgWToRvw(convertxgWToXgW(xgH2O), regionIdx);
    }

    /*!
     * \brief Convert the mass fraction of the water component in the gas phase to the
     *        corresponding water vaporization factor.
     */
    template <class LhsEval>
    LhsEval convertXgWToRvw(const LhsEval& XgW, unsigned regionIdx) const
    {
        Scalar rho_wRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = gasReferenceDensity_[regionIdx];

        return XgW/(1.0 - XgW)*(rho_gRef/rho_wRef);
    }

    /*!
     * \brief Convert a water vapporization factor to the the corresponding mass fraction
     *        of the water component in the gas phase.
     */
    template <class LhsEval>
    LhsEval convertRvwToXgW_(const LhsEval& Rvw, unsigned regionIdx) const
    {
        Scalar rho_wRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = gasReferenceDensity_[regionIdx];

        const LhsEval& rho_wG = Rvw*rho_gRef;
        return rho_wG/(rho_wRef + rho_wG);
    }
    /*!
     * \brief Convert a water mole fraction in the gas phase the corresponding mass fraction.
     */
    template <class LhsEval>
    LhsEval convertxgWToXgW(const LhsEval& xgW) const
    {
        Scalar M_CO2 = CO2::molarMass();
        Scalar M_Brine = Brine::molarMass();

        return xgW*M_Brine / (xgW*(M_Brine - M_CO2) + M_CO2);
    }

    std::vector<Scalar> brineReferenceDensity_;
    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> salinity_;
    bool enableDissolution_ = true;
};

} // namespace Opm

#endif
