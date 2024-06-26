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
* \copydoc Opm::H2GasPvt
*/
#ifndef OPM_H2_GAS_PVT_HPP
#define OPM_H2_GAS_PVT_HPP

#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/BrineDynamic.hpp>
#include <opm/material/components/H2.hpp>
#include <opm/material/binarycoefficients/Brine_H2.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>

#include <vector>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

/*!
* \brief This class represents the Pressure-Volume-Temperature relations of the gas phase for H2
*/
template <class Scalar>
class H2GasPvt
{
    using H2O = SimpleHuDuanH2O<Scalar>;
    using Brine = ::Opm::BrineDynamic<Scalar, H2O>;
    using H2 = ::Opm::H2<Scalar>;
    static const bool extrapolate = true;

public:
    // The binary coefficients for brine and H2 used by this fluid system
    using BinaryCoeffBrineH2 = BinaryCoeff::Brine_H2<Scalar, H2O, H2>;

    explicit H2GasPvt() = default;

    H2GasPvt(const std::vector<Scalar>& salinity,
              Scalar T_ref = 288.71, //(273.15 + 15.56)
              Scalar P_ref = 101325)
        : salinity_(salinity)
    {
        int numRegions = salinity_.size();
        setNumRegions(numRegions);
        for (int i = 0; i < numRegions; ++i) {
            gasReferenceDensity_[i] = H2::gasDensity(T_ref, P_ref, extrapolate);
            brineReferenceDensity_[i] = Brine::liquidDensity(T_ref, P_ref, salinity_[i], extrapolate);
        }
    }

#if HAVE_ECL_INPUT
    /*!
    * \brief Initialize the parameters for H2 gas using an ECL deck.
    */
    void initFromState(const EclipseState& eclState, const Schedule&);
#endif

    void setNumRegions(size_t numRegions)
    {
        gasReferenceDensity_.resize(numRegions);
        brineReferenceDensity_.resize(numRegions);
        salinity_.resize(numRegions);
    }

    void setVapPars(const Scalar, const Scalar)
    {
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
        brineReferenceDensity_[regionIdx] = rhoRefBrine;
    }

    /*!
    * \brief Specify whether the PVT model should consider that the water component can
    *        vaporize in the gas phase
    *
    * By default, vaporized water is considered.
    */
    void setEnableVaporizationWater(bool yesno)
    { enableVaporization_ = yesno; }

    /*!
    * \brief Finish initializing the oil phase PVT properties.
    */
    void initEnd()
    {
    }

    /*!
    * \brief Return the number of PVT regions which are considered by this PVT-object.
    */
    unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    Scalar hVap(unsigned ) const{
        return 0;
    }
    /*!
    * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
    *
    */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned /*regionIdx*/,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& /*rv*/,
                        const Evaluation& /*rvw*/) const
    {
        // use the gasInternalEnergy of H2
        return H2::gasInternalEnergy(temperature, pressure, extrapolate);

        // TODO: account for H2O in the gas phase
        // Init output
        //Evaluation result = 0;

        // We have to check that one of RV and RVW is zero since H2STORE works with either GAS/WATER or GAS/OIL system
        //assert(rv == 0.0 || rvw == 0.0);

        // Calculate each component contribution and return weighted sum
        //const Evaluation xBrine = convertRvwToXgW_(max(rvw, rv), regionIdx);
        //result += xBrine * H2O::gasInternalEnergy(temperature, pressure);
        //result += (1 - xBrine) * H2::gasInternalEnergy(temperature, pressure, extrapolate);
        //return result;
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
    {
        return saturatedViscosity(regionIdx, temperature, pressure);
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned /*regionIdx*/,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        return H2::gasViscosity(temperature, pressure, extrapolate);
    }

    /*!
    * \brief Returns the formation volume factor [-] of the fluid phase.
    */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& rv,
                                            const Evaluation& rvw) const
    {
        // If vaporization is disabled, return H2 gas volume factor
        if (!enableVaporization_)
            return H2::gasDensity(temperature, pressure, extrapolate)/gasReferenceDensity_[regionIdx];

        // Use CO2 density for the gas phase.
        const auto& rhoH2 = H2::gasDensity(temperature, pressure, extrapolate);
        //const auto& rhoH2O = H2O::gasDensity(temperature, pressure);
        //The H2STORE option both works for GAS/WATER and GAS/OIL systems
        //Either rv og rvw should be zero
        //assert(rv == 0.0 || rvw == 0.0);
        //const Evaluation xBrine = convertRvwToXgW_(max(rvw,rv),regionIdx);
        //const auto rho = 1.0/(xBrine/rhoH2O + (1.0 - xBrine)/rhoH2);
        return rhoH2/(gasReferenceDensity_[regionIdx] + max(rvw,rv)*brineReferenceDensity_[regionIdx]);
    }

    /*!
    * \brief Returns the formation volume factor [-] of oil saturated gas at given pressure.
    */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        const Evaluation rvw = rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
        return inverseFormationVolumeFactor(regionIdx, temperature, pressure, Evaluation(0.0), rvw);
    }

    /*!
    * \brief Returns the saturation pressure of the gas phase [Pa] depending on its mass fraction of the oil component
    *
    * \param Rv The surface volume of oil component dissolved in what will yield one cubic meter of gas at the surface [-]
    */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rv*/) const
    { return 0.0; /* Not implemented! */ }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
    */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    {
        return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
    }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
    */
    template <class Evaluation = Scalar>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& saltConcentration) const
    {
        const Evaluation salinity = salinityFromConcentration(temperature, pressure, saltConcentration);
        return rvwSat_(regionIdx, temperature, pressure, salinity);
     }

    /*!
    * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
    */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& /*oilSaturation*/,
                                              const Evaluation& /*maxOilSaturation*/) const
    {
        return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
    }

    /*!
    * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
    */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    {
        return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
    }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        return BinaryCoeffBrineH2::gasDiffCoeff(temperature, pressure);
    }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    Scalar waterReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    Scalar salinity(unsigned regionIdx) const
    { return salinity_[regionIdx]; }

private:
    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> brineReferenceDensity_;
    std::vector<Scalar> salinity_;
    bool enableVaporization_ = true;

    template <class LhsEval>
    LhsEval rvwSat_(unsigned regionIdx,
                    const LhsEval& temperature,
                    const LhsEval& pressure,
                    const LhsEval& salinity) const
    {
        // If water vaporization is disabled, we return zero
        if (!enableVaporization_)
            return 0.0;

        // From Li et al., Int. J. Hydrogen Energ., 2018, water mole fraction is calculated assuming ideal mixing
        LhsEval pw_sat = H2O::vaporPressure(temperature);
        LhsEval yH2O = pw_sat / pressure;

        // normalize the phase compositions
        yH2O = max(0.0, min(1.0, yH2O));
        return convertXgWToRvw(convertxgWToXgW(yH2O, salinity), regionIdx);
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
    * \brief Convert a water vaporization factor to the the corresponding mass fraction
    *        of the water component in the gas phase.
    */
    template <class LhsEval>
    LhsEval convertRvwToXgW_(const LhsEval& Rvw, unsigned regionIdx) const
    {
        Scalar rho_wRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = gasReferenceDensity_[regionIdx];

        const LhsEval& rho_wG = Rvw*rho_wRef;
        return rho_wG/(rho_gRef + rho_wG);
    }

    /*!
    * \brief Convert a water mole fraction in the gas phase the corresponding mass fraction.
    */
    template <class LhsEval>
    LhsEval convertxgWToXgW(const LhsEval& xgW, const LhsEval& salinity) const
    {
        Scalar M_H2 = H2::molarMass();
        LhsEval M_Brine = Brine::molarMass(salinity);

        return xgW*M_Brine / (xgW*(M_Brine - M_H2) + M_H2);
    }

    template <class LhsEval>
    const LhsEval salinityFromConcentration(const LhsEval&T, const LhsEval& P, const LhsEval& saltConcentration) const
    {
        return saltConcentration / H2O::liquidDensity(T, P, true);
    }

};  // end class H2GasPvt

}  // end namspace Opm

#endif
