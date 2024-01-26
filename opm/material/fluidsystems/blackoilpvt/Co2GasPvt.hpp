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
#include <opm/common/ErrorMacros.hpp>

#include <opm/material/components/CO2.hpp>
#include <opm/material/components/BrineDynamic.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/binarycoefficients/Brine_CO2.hpp>
#include <opm/input/eclipse/EclipseState/Co2StoreConfig.hpp>

#include <vector>

namespace Opm {

class EclipseState;
class Schedule;
class Co2StoreConfig;

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 * for CO2
 */
template <class Scalar>
class Co2GasPvt
{
    using CO2 = ::Opm::CO2<Scalar>;
    using H2O = SimpleHuDuanH2O<Scalar>;
    using Brine = ::Opm::BrineDynamic<Scalar, H2O>;
    static constexpr bool extrapolate = true;

public:
    //! The binary coefficients for brine and CO2 used by this fluid system
    using BinaryCoeffBrineCO2 = BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    explicit Co2GasPvt() = default;

    Co2GasPvt(const std::vector<Scalar>& salinity,
              int activityModel = 3,
              int thermalMixingModel = 1,
              Scalar T_ref = 288.71, //(273.15 + 15.56)
              Scalar P_ref = 101325)
        : salinity_(salinity)
    {
        // Throw an error if reference state is not (T, p) = (15.56 C, 1 atm) = (288.71 K, 1.01325e5 Pa)
        if (T_ref != Scalar(288.71) || P_ref != Scalar(1.01325e5)) {
            OPM_THROW(std::runtime_error,
                "BrineCo2Pvt class can only be used with default reference state (T, P) = (288.71 K, 1.01325e5 Pa)!");
        }
        setActivityModelSalt(activityModel);
        setThermalMixingModel(thermalMixingModel);

        int num_regions = salinity_.size();
        setNumRegions(num_regions);
        for (int i = 0; i < num_regions; ++i) {
            gasReferenceDensity_[i] = CO2::gasDensity(T_ref, P_ref, extrapolate);
            brineReferenceDensity_[i] = Brine::liquidDensity(T_ref, P_ref, salinity_[i], extrapolate);
        }
    }
    /*!
     * \brief Initialize the parameters for CO2 gas using an ECL deck.
     */
    void initFromState(const EclipseState& eclState, const Schedule&);

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
        brineReferenceDensity_[regionIdx] = rhoRefBrine;;
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
    * \brief Set activity coefficient model for salt in solubility model
    */
    void setActivityModelSalt(int activityModel)
    {
        // Only 1, 2, and 3 are allowed
        if (activityModel > 3 || activityModel < 1) {
            OPM_THROW(std::runtime_error, "The salt activity model options are 1, 2 or 3");
        }
        activityModel_ = activityModel;
    }

   /*!
    * \brief Set thermal mixing model for co2 in brine
    */
    void setThermalMixingModel(int thermalMixingModel)
    {
        // Only 0 and 1 are allowed
        // 0 = Use pure CO2 entalpy
        // 1 = Account for vapporized water in gas phase (Mass fraction)
        if (thermalMixingModel == 0)
            gastype_ = Co2StoreConfig::GasMixingType::NONE;

        else if (thermalMixingModel == 1)
            gastype_ = Co2StoreConfig::GasMixingType::IDEAL;
        else
            OPM_THROW(std::runtime_error, "The thermal mixing model options are 0 and 1");
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

    Scalar hVap(unsigned ) const{
        return 0;
    }
    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& rv,
                        const Evaluation& rvw) const
    {
        OPM_TIMEBLOCK_LOCAL(internalEnergy);
        if (gastype_ == Co2StoreConfig::GasMixingType::NONE)
            // use the gasInternalEnergy of CO2
            return CO2::gasInternalEnergy(temperature, pressure, extrapolate);

        assert(gastype_ == Co2StoreConfig::GasMixingType::IDEAL);
        // account for H2O in the gas phase
        Evaluation result = 0;
        // The CO2STORE option both works for GAS/WATER and GAS/OIL systems
        // Either rv og rvw should be zero
        assert(rv == 0.0 || rvw == 0.0);
        const Evaluation xBrine = convertRvwToXgW_(max(rvw,rv),regionIdx);
        result += xBrine * H2O::gasInternalEnergy(temperature, pressure);
        result += (1 - xBrine) * CO2::gasInternalEnergy(temperature, pressure, extrapolate);
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
        OPM_TIMEBLOCK_LOCAL(saturatedViscosity);
        // Neglects impact of vaporized water on the visosity
        return CO2::gasViscosity(temperature, pressure, extrapolate);
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
        OPM_TIMEFUNCTION_LOCAL();
        if (!enableVaporization_)
            return CO2::gasDensity(temperature, pressure, extrapolate)/gasReferenceDensity_[regionIdx];

        // Use CO2 density for the gas phase.
        const auto& rhoCo2 = CO2::gasDensity(temperature, pressure, extrapolate);
        //const auto& rhoH2O = H2O::gasDensity(temperature, pressure);
        //The CO2STORE option both works for GAS/WATER and GAS/OIL systems
        //Either rv og rvw should be zero
        //assert(rv == 0.0 || rvw == 0.0);
        //const Evaluation xBrine = convertRvwToXgW_(max(rvw,rv),regionIdx);
        //const auto rho = 1.0/(xBrine/rhoH2O + (1.0 - xBrine)/rhoCo2);
        return rhoCo2/(gasReferenceDensity_[regionIdx] + max(rvw,rv)*brineReferenceDensity_[regionIdx]);
    }

    /*!
     * \brief Returns the formation volume factor [-] of water saturated gas at given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation rvw = rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
        return inverseFormationVolumeFactor(regionIdx,temperature,pressure, Evaluation(0.0), rvw);
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the brine component
     *
     * \param Rvw The surface volume of brine component vaporized in what will yield one cubic meter of water at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rvw*/) const
    { return 0.0; /* not implemented */ }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx])); }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water phase.
    */
    template <class Evaluation = Scalar>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& saltConcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL();
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
    { return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx])); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx])); }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        return BinaryCoeffBrineCO2::gasDiffCoeff(temperature, pressure, extrapolate);
    }

    Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    Scalar waterReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    Scalar salinity(unsigned regionIdx) const
    { return salinity_[regionIdx]; }

    void setEzrokhiDenCoeff(const std::vector<EzrokhiTable>& denaqa)
    {
        if (denaqa.empty())
            return;

        enableEzrokhiDensity_ = true;
        ezrokhiDenNaClCoeff_ = {static_cast<Scalar>(denaqa[0].getC0("NACL")), 
                                static_cast<Scalar>(denaqa[0].getC1("NACL")), 
                                static_cast<Scalar>(denaqa[0].getC2("NACL"))};
    }

private:

    template <class LhsEval>
    LhsEval ezrokhiExponent_(const LhsEval& temperature,
                             const std::vector<Scalar>& ezrokhiCoeff) const
    {
        const LhsEval& tempC = temperature - 273.15;
        return ezrokhiCoeff[0] + tempC * (ezrokhiCoeff[1] + ezrokhiCoeff[2] * tempC);
    }

    template <class LhsEval>
    LhsEval rvwSat_(unsigned regionIdx,
                    const LhsEval& temperature,
                    const LhsEval& pressure,
                    const LhsEval& salinity) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        if (!enableVaporization_)
            return 0.0;

        // calulate the equilibrium composition for the given
        // temperature and pressure.
        LhsEval xgH2O;
        LhsEval xlCO2;
        BinaryCoeffBrineCO2::calculateMoleFractions(temperature,
                                                    pressure,
                                                    salinity,
                                                    /*knownPhaseIdx=*/-1,
                                                    xlCO2,
                                                    xgH2O,
                                                    activityModel_,
                                                    extrapolate);

        // normalize the phase compositions
        xgH2O = max(0.0, min(1.0, xgH2O));

        return convertXgWToRvw(convertxgWToXgW(xgH2O, salinity), regionIdx);
    }

    /*!
     * \brief Convert the mass fraction of the water component in the gas phase to the
     *        corresponding water vaporization factor.
     */
    template <class LhsEval>
    LhsEval convertXgWToRvw(const LhsEval& XgW, unsigned regionIdx) const
    {
        OPM_TIMEFUNCTION_LOCAL();
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
        OPM_TIMEFUNCTION_LOCAL();
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
        OPM_TIMEFUNCTION_LOCAL();
        Scalar M_CO2 = CO2::molarMass();
        LhsEval M_Brine = Brine::molarMass(salinity);

        return xgW*M_Brine / (xgW*(M_Brine - M_CO2) + M_CO2);
    }

    template <class LhsEval>
    const LhsEval salinityFromConcentration(const LhsEval&T, const LhsEval& P, const LhsEval& saltConcentration) const
    { return saltConcentration/H2O::liquidDensity(T, P, true); }

    std::vector<Scalar> brineReferenceDensity_;
    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> salinity_;
    std::vector<Scalar> ezrokhiDenNaClCoeff_;
    bool enableEzrokhiDensity_ = false;
    bool enableVaporization_ = true;
    int activityModel_;
    Co2StoreConfig::GasMixingType gastype_;
};

} // namespace Opm

#endif
