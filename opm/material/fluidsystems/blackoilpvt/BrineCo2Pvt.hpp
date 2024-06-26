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
 * \copydoc Opm::BrineCo2Pvt
 */
#ifndef OPM_BRINE_CO2_PVT_HPP
#define OPM_BRINE_CO2_PVT_HPP

#include <opm/common/Exceptions.hpp>
#include <opm/common/TimingMacros.hpp>
#include <opm/common/ErrorMacros.hpp>

#include <opm/material/Constants.hpp>

#include <opm/material/components/BrineDynamic.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/components/TabulatedComponent.hpp>
#include <opm/material/binarycoefficients/H2O_CO2.hpp>
#include <opm/material/binarycoefficients/Brine_CO2.hpp>

#include <opm/input/eclipse/EclipseState/Co2StoreConfig.hpp>

#include <vector>

namespace Opm {

class EclipseState;
class Schedule;
class Co2StoreConfig;
class EzrokhiTable;

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the liquid phase
 * for a CO2-Brine system
 */
template <class Scalar>
class BrineCo2Pvt
{
    static constexpr bool extrapolate = true;
    //typedef H2O<Scalar> H2O_IAPWS;
    //typedef Brine<Scalar, H2O_IAPWS> Brine_IAPWS;
    //typedef TabulatedComponent<Scalar, H2O_IAPWS> H2O_Tabulated;
    //typedef TabulatedComponent<Scalar, Brine_IAPWS> Brine_Tabulated;

    //typedef H2O_Tabulated H2O;
    //typedef Brine_Tabulated Brine;


public:
    using H2O = SimpleHuDuanH2O<Scalar>;
    using Brine = ::Opm::BrineDynamic<Scalar, H2O>;
    using CO2 = ::Opm::CO2<Scalar>;

    //! The binary coefficients for brine and CO2 used by this fluid system
    using BinaryCoeffBrineCO2 = BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    explicit BrineCo2Pvt() = default;

    BrineCo2Pvt(const std::vector<Scalar>& salinity,
                int activityModel = 3,
                int thermalMixingModelSalt = 1,
                int thermalMixingModelLiquid = 2,
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
        setThermalMixingModel(thermalMixingModelSalt, thermalMixingModelLiquid);
        int num_regions =  salinity_.size();
        co2ReferenceDensity_.resize(num_regions);
        brineReferenceDensity_.resize(num_regions);
        for (int i = 0; i < num_regions; ++i) {
            co2ReferenceDensity_[i] = CO2::gasDensity(T_ref, P_ref, true);
            brineReferenceDensity_[i] = Brine::liquidDensity(T_ref, P_ref, salinity_[i], true);
        }
    }

#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for Brine-CO2 system using an ECL deck.
     *
     */
    void initFromState(const EclipseState& eclState, const Schedule&);
#endif

    void setNumRegions(size_t numRegions)
    {
        brineReferenceDensity_.resize(numRegions);
        co2ReferenceDensity_.resize(numRegions);
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
                               Scalar rhoRefCO2,
                               Scalar /*rhoRefWater*/)
    {
        brineReferenceDensity_[regionIdx] = rhoRefBrine;
        co2ReferenceDensity_[regionIdx] = rhoRefCO2;
    }


    /*!
     * \brief Finish initializing the oil phase PVT properties.
     */
    void initEnd()
    {

    }

    /*!
     * \brief Specify whether the PVT model should consider that the CO2 component can
     *        dissolve in the brine phase
     *
     * By default, dissolved co2 is considered.
     */
    void setEnableDissolvedGas(bool yesno)
    { enableDissolution_ = yesno; }

    /*!
     * \brief Specify whether the PVT model should consider salt concentration from
     * the fluidstate or a fixed salinty
     *
     * By default, fixed salinity is considered
     */
    void setEnableSaltConcentration(bool yesno)
    { enableSaltConcentration_ = yesno; }

    /*!
    * \brief Set activity coefficient model for salt in solubility model
    */
    void setActivityModelSalt(int activityModel)
    {
        // Only 1, 2 and 3 are allowed
        if (activityModel > 3 || activityModel < 1) {
            OPM_THROW(std::runtime_error, "The salt activity model options are 1, 2 or 3");
        }
        activityModel_ = activityModel;
    }

    /*!
    * \brief Set thermal mixing model for co2 in brine
    */
    void setThermalMixingModel(int thermalMixingModelSalt, int thermalMixingModelLiquid)
    {
        if (thermalMixingModelSalt == 0)
            saltMixType_ = Co2StoreConfig::SaltMixingType::NONE;
        else if (thermalMixingModelSalt == 1)
            saltMixType_ = Co2StoreConfig::SaltMixingType::MICHAELIDES;
        else 
            OPM_THROW(std::runtime_error, "The thermal mixing model option for salt are 0 or 1");

        if (thermalMixingModelLiquid == 0)
            liquidMixType_ = Co2StoreConfig::LiquidMixingType::NONE;
        else if (thermalMixingModelLiquid == 1)
            liquidMixType_ = Co2StoreConfig::LiquidMixingType::IDEAL;
        else if (thermalMixingModelLiquid == 2)
            liquidMixType_ = Co2StoreConfig::LiquidMixingType::DUANSUN;
        else 
            OPM_THROW(std::runtime_error, "The thermal mixing model option for liquid are 0, 1 and 2");
    }

    void setEzrokhiDenCoeff(const std::vector<EzrokhiTable>& denaqa)
    {
        if (denaqa.empty())
            return;

        enableEzrokhiDensity_ = true;
        ezrokhiDenNaClCoeff_ = {static_cast<Scalar>(denaqa[0].getC0("NACL")), 
                                static_cast<Scalar>(denaqa[0].getC1("NACL")), 
                                static_cast<Scalar>(denaqa[0].getC2("NACL"))};
        ezrokhiDenCo2Coeff_ = {static_cast<Scalar>(denaqa[0].getC0("CO2")), 
                               static_cast<Scalar>(denaqa[0].getC1("CO2")), 
                               static_cast<Scalar>(denaqa[0].getC2("CO2"))};
    }

    void setEzrokhiViscCoeff(const std::vector<EzrokhiTable>& viscaqa)
    {
        if (viscaqa.empty())
            return;

        enableEzrokhiViscosity_ = true;
        ezrokhiViscNaClCoeff_ = {static_cast<Scalar>(viscaqa[0].getC0("NACL")), 
                                 static_cast<Scalar>(viscaqa[0].getC1("NACL")), 
                                 static_cast<Scalar>(viscaqa[0].getC2("NACL"))};
    }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { return brineReferenceDensity_.size(); }

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
                              const Evaluation& Rs,
                              const Evaluation& saltConcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation salinity = salinityFromConcentration(regionIdx, temperature, pressure, saltConcentration);
        const Evaluation xlCO2 = convertRsToXoG_(Rs,regionIdx);
        return (liquidEnthalpyBrineCO2_(temperature,
                                       pressure,
                                       salinity,
                                       xlCO2)
        - pressure / density(regionIdx, temperature, pressure, Rs, salinity ));
    }
    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& Rs) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation xlCO2 = convertRsToXoG_(Rs,regionIdx);
        return (liquidEnthalpyBrineCO2_(temperature,
                                       pressure,
                                       Evaluation(salinity_[regionIdx]),
                                       xlCO2)
        - pressure / density(regionIdx, temperature, pressure, Rs, Evaluation(salinity_[regionIdx])));
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rs*/) const
    {
        //TODO: The viscosity does not yet depend on the composition
        return saturatedViscosity(regionIdx, temperature, pressure);
    }

        /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                 const Evaluation& temperature,
                                 const Evaluation& pressure,
                                 const Evaluation& saltConcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation salinity = salinityFromConcentration(regionIdx, temperature, pressure, saltConcentration);
        if (enableEzrokhiViscosity_) {
            const Evaluation& mu_pure = H2O::liquidViscosity(temperature, pressure, extrapolate);
            const Evaluation& nacl_exponent = ezrokhiExponent_(temperature, ezrokhiViscNaClCoeff_);
            return mu_pure * pow(10.0, nacl_exponent * salinity);
        }
        else {
            return Brine::liquidViscosity(temperature, pressure, salinity);
        }
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rsw*/,
                         const Evaluation& saltConcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        //TODO: The viscosity does not yet depend on the composition
        return saturatedViscosity(regionIdx, temperature, pressure, saltConcentration);
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        if (enableEzrokhiViscosity_) {
            const Evaluation& mu_pure = H2O::liquidViscosity(temperature, pressure, extrapolate);
            const Evaluation& nacl_exponent = ezrokhiExponent_(temperature, ezrokhiViscNaClCoeff_);
            return mu_pure * pow(10.0, nacl_exponent * Evaluation(salinity_[regionIdx]));
        }
        else {
            return Brine::liquidViscosity(temperature, pressure, Evaluation(salinity_[regionIdx]));
        }
        
    }


    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure,
                                                     const Evaluation& saltconcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation salinity = salinityFromConcentration(regionIdx, temperature, pressure, saltconcentration);
        Evaluation rs_sat = rsSat(regionIdx, temperature, pressure, salinity);
        return (1.0 - convertRsToXoG_(rs_sat,regionIdx)) * density(regionIdx, temperature, pressure, rs_sat, salinity)/brineReferenceDensity_[regionIdx];
    }
    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs,
                                            const Evaluation& saltConcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        const Evaluation salinity = salinityFromConcentration(regionIdx, temperature, pressure, saltConcentration);
        return (1.0 - convertRsToXoG_(Rs,regionIdx)) * density(regionIdx, temperature, pressure, Rs, salinity)/brineReferenceDensity_[regionIdx];
    }
    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rs) const
    {
        return (1.0 - convertRsToXoG_(Rs,regionIdx)) * density(regionIdx, temperature, pressure, Rs, Evaluation(salinity_[regionIdx]))/brineReferenceDensity_[regionIdx];
    }

    /*!
     * \brief Returns the formation volume factor [-] of brine saturated with CO2 at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
    OPM_TIMEFUNCTION_LOCAL();
    Evaluation rs_sat = rsSat(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
        return (1.0 - convertRsToXoG_(rs_sat,regionIdx)) * density(regionIdx, temperature, pressure, rs_sat, Evaluation(salinity_[regionIdx]))/brineReferenceDensity_[regionIdx];
    }

    /*!
     * \brief Returns the saturation pressure of the brine phase [Pa]
     *        depending on its mass fraction of the gas component
     *
     * \param Rs
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rs*/) const
    {
        throw std::runtime_error("Requested the saturation pressure for the brine-co2 pvt module. Not yet implemented.");
    }

    /*!
     * \brief Returns the saturation pressure of the brine phase [Pa]
     *        depending on its mass fraction of the gas component
     *
     * \param Rs
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rs*/,
                                  const Evaluation& /*saltConcentration*/) const
    {
        throw std::runtime_error("Requested the saturation pressure for the brine-co2 pvt module. Not yet implemented.");
    }

    /*!
     * \brief Returns the gas dissoluiton factor \f$R_s\f$ [m^3/m^3] of the liquid phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& /*oilSaturation*/,
                                             const Evaluation& /*maxOilSaturation*/) const
    {
        //TODO support VAPPARS
        return rsSat(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
    }

    /*!
     * \brief Returns the gas dissoluiton factor \f$R_s\f$ [m^3/m^3] of the liquid phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& saltConcentration) const
    {
        const Evaluation salinity = salinityFromConcentration(regionIdx, temperature, pressure, saltConcentration);
        return rsSat(regionIdx, temperature, pressure, salinity);
    }

    /*!
     * \brief Returns thegas dissoluiton factor  \f$R_s\f$ [m^3/m^3] of the liquid phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure) const
    {
        return rsSat(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx]));
    }

    const Scalar oilReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    const Scalar waterReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return co2ReferenceDensity_[regionIdx]; }

    const Scalar salinity(unsigned regionIdx) const
    { return salinity_[regionIdx]; }


    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        //Diffusion coefficient of CO2 in pure water according to (McLachlan and Danckwerts, 1972)
        const Evaluation log_D_H20 = -4.1764 + 712.52 / temperature - 2.5907e5 / (temperature*temperature);

        //Diffusion coefficient of CO2 in the brine phase modified following (Ratcliff and Holdcroft,1963 and Al-Rawajfeh, 2004)
        const Evaluation& mu_H20 = H2O::liquidViscosity(temperature, pressure, extrapolate); // Water viscosity
        Evaluation mu_Brine;
        if (enableEzrokhiViscosity_) {
            const Evaluation& nacl_exponent = ezrokhiExponent_(temperature, ezrokhiViscNaClCoeff_);
            mu_Brine = mu_H20 * pow(10.0, nacl_exponent * Evaluation(salinity_[0]));
        }
        else {
            mu_Brine = Brine::liquidViscosity(temperature, pressure, Evaluation(salinity_[0])); // Brine viscosity
        }
        const Evaluation log_D_Brine = log_D_H20 - 0.87*log10(mu_Brine / mu_H20);

        return pow(Evaluation(10), log_D_Brine) * 1e-4; // convert from cm2/s to m2/s
    }

    template <class Evaluation>
    Evaluation density(unsigned regionIdx,
                       const Evaluation& temperature,
                       const Evaluation& pressure,
                       const Evaluation& Rs,
                       const Evaluation& salinity) const
    {
    OPM_TIMEFUNCTION_LOCAL();
    Evaluation xlCO2 = convertXoGToxoG_(convertRsToXoG_(Rs,regionIdx), salinity);
        Evaluation result = liquidDensity_(temperature,
                                        pressure,
                                        xlCO2,
                                        salinity);

        Valgrind::CheckDefined(result);
        return result;
    }

    template <class Evaluation>
    Evaluation rsSat(unsigned regionIdx,
                     const Evaluation& temperature,
                     const Evaluation& pressure,
                     const Evaluation& salinity) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        if (!enableDissolution_)
            return 0.0;

        // calulate the equilibrium composition for the given
        // temperature and pressure.
        Evaluation xgH2O;
        Evaluation xlCO2;
        BinaryCoeffBrineCO2::calculateMoleFractions(temperature,
                                                    pressure,
                                                    salinity,
                                                    /*knownPhaseIdx=*/-1,
                                                    xlCO2,
                                                    xgH2O,
                                                    activityModel_,
                                                    extrapolate);

        // normalize the phase compositions
        xlCO2 = max(0.0, min(1.0, xlCO2));

        return convertXoGToRs(convertxoGToXoG(xlCO2, salinity), regionIdx);
    }

private:
    std::vector<Scalar> brineReferenceDensity_;
    std::vector<Scalar> co2ReferenceDensity_;
    std::vector<Scalar> salinity_;
    std::vector<Scalar> ezrokhiDenNaClCoeff_;
    std::vector<Scalar> ezrokhiDenCo2Coeff_;
    std::vector<Scalar> ezrokhiViscNaClCoeff_;
    bool enableEzrokhiDensity_ = false;
    bool enableEzrokhiViscosity_ = false;
    bool enableDissolution_ = true;
    bool enableSaltConcentration_ = false;
    int activityModel_;
    Co2StoreConfig::LiquidMixingType liquidMixType_; 
    Co2StoreConfig::SaltMixingType saltMixType_; 

    template <class LhsEval>
    LhsEval ezrokhiExponent_(const LhsEval& temperature,
                             const std::vector<Scalar>& ezrokhiCoeff) const
    {
        const LhsEval& tempC = temperature - 273.15;
        return ezrokhiCoeff[0] + tempC * (ezrokhiCoeff[1] + ezrokhiCoeff[2] * tempC);
    }
    
    template <class LhsEval>
    LhsEval liquidDensity_(const LhsEval& T,
                           const LhsEval& pl,
                           const LhsEval& xlCO2,
                           const LhsEval& salinity) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        Valgrind::CheckDefined(T);
        Valgrind::CheckDefined(pl);
        Valgrind::CheckDefined(xlCO2);

        if(!extrapolate && T < 273.15) {
            const std::string msg =
                "Liquid density for Brine and CO2 is only "
                "defined above 273.15K (is " +
                std::to_string(getValue(T)) + "K)";
            throw NumericalProblem(msg);
        }
        if(!extrapolate && pl >= 2.5e8) {
            const std::string msg  =
                "Liquid density for Brine and CO2 is only "
                "defined below 250MPa (is " +
                std::to_string(getValue(pl)) + "Pa)";
            throw NumericalProblem(msg);
        }

        const LhsEval& rho_pure = H2O::liquidDensity(T, pl, extrapolate);
        if (enableEzrokhiDensity_) {
            const LhsEval& nacl_exponent = ezrokhiExponent_(T, ezrokhiDenNaClCoeff_);
            const LhsEval& co2_exponent = ezrokhiExponent_(T, ezrokhiDenCo2Coeff_);
            const LhsEval& XCO2 = convertxoGToXoG(xlCO2, salinity);
            return rho_pure * pow(10.0, nacl_exponent * salinity + co2_exponent * XCO2);
        }
        else {
            const LhsEval& rho_brine = Brine::liquidDensity(T, pl, salinity, extrapolate);
            const LhsEval& rho_lCO2 = liquidDensityWaterCO2_(T, pl, xlCO2);
            const LhsEval& contribCO2 = rho_lCO2 - rho_pure;
            return rho_brine + contribCO2;
        }
    }

    template <class LhsEval>
    LhsEval liquidDensityWaterCO2_(const LhsEval& temperature,
                                          const LhsEval& pl,
                                          const LhsEval& xlCO2) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        Scalar M_CO2 = CO2::molarMass();
        Scalar M_H2O = H2O::molarMass();

        const LhsEval& tempC = temperature - 273.15;        /* tempC : temperature in °C */
        const LhsEval& rho_pure = H2O::liquidDensity(temperature, pl, extrapolate);
        // calculate the mole fraction of CO2 in the liquid. note that xlH2O is available
        // as a function parameter, but in the case of a pure gas phase the value of M_T
        // for the virtual liquid phase can become very large
        const LhsEval xlH2O = 1.0 - xlCO2;
        const LhsEval& M_T = M_H2O * xlH2O + M_CO2 * xlCO2;
        const LhsEval& V_phi =
            (37.51 +
             tempC*(-9.585e-2 +
                    tempC*(8.74e-4 -
                           tempC*5.044e-7))) / 1.0e6;
        return 1/ (xlCO2 * V_phi/M_T + M_H2O * xlH2O / (rho_pure * M_T));
    }

    /*!
     * \brief Convert a gas dissolution factor to the the corresponding mass fraction
     *        of the gas component in the oil phase.
     */
    template <class LhsEval>
    LhsEval convertRsToXoG_(const LhsEval& Rs, unsigned regionIdx) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        Scalar rho_oRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = co2ReferenceDensity_[regionIdx];

        const LhsEval& rho_oG = Rs*rho_gRef;
        return rho_oG/(rho_oRef + rho_oG);
    }


    /*!
     * \brief Convert a gas mass fraction in the oil phase the corresponding mole fraction.
     */
    template <class LhsEval>
    LhsEval convertXoGToxoG_(const LhsEval& XoG, const LhsEval& salinity) const
    {
        OPM_TIMEFUNCTION_LOCAL();
        Scalar M_CO2 = CO2::molarMass();
        LhsEval M_Brine = Brine::molarMass(salinity);
        return XoG*M_Brine / (M_CO2*(1 - XoG) + XoG*M_Brine);
    }


    /*!
     * \brief Convert a gas mole fraction in the oil phase the corresponding mass fraction.
     */
    template <class LhsEval>
    LhsEval convertxoGToXoG(const LhsEval& xoG, const LhsEval& salinity) const
    {
        OPM_TIMEBLOCK_LOCAL(convertxoGToXoG);
        Scalar M_CO2 = CO2::molarMass();
        LhsEval M_Brine = Brine::molarMass(salinity);

        return xoG*M_CO2 / (xoG*(M_CO2 - M_Brine) + M_Brine);
    }


    /*!
     * \brief Convert the mass fraction of the gas component in the oil phase to the
     *        corresponding gas dissolution factor.
     */
    template <class LhsEval>
    LhsEval convertXoGToRs(const LhsEval& XoG, unsigned regionIdx) const
    {
        Scalar rho_oRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = co2ReferenceDensity_[regionIdx];

        return XoG/(1.0 - XoG)*(rho_oRef/rho_gRef);
    }

    template <class LhsEval>
    LhsEval liquidEnthalpyBrineCO2_(const LhsEval& T,
                                    const LhsEval& p,
                                    const LhsEval& salinity, 
                                    const LhsEval& X_CO2_w) const
    {


        if (liquidMixType_ == Co2StoreConfig::LiquidMixingType::NONE 
            && saltMixType_ == Co2StoreConfig::SaltMixingType::NONE)
            return H2O::liquidEnthalpy(T, p);

        LhsEval hw = H2O::liquidEnthalpy(T, p) /1E3; /* kJ/kg */
        LhsEval h_ls1 = hw;
        // Use mixing model for salt by MICHAELIDES
        if (saltMixType_ == Co2StoreConfig::SaltMixingType::MICHAELIDES) {

            /* X_CO2_w : mass fraction of CO2 in brine */

            /* same function as enthalpy_brine, only extended by CO2 content */

            /*Numerical coefficents from PALLISER*/
            static constexpr Scalar f[] = {
                2.63500E-1, 7.48368E-6, 1.44611E-6, -3.80860E-10
            };

            /*Numerical coefficents from MICHAELIDES for the enthalpy of brine*/
            static constexpr Scalar a[4][3] = {
                { 9633.6, -4080.0, +286.49 },
                { +166.58, +68.577, -4.6856 },
                { -0.90963, -0.36524, +0.249667E-1 },
                { +0.17965E-2, +0.71924E-3, -0.4900E-4 }
            };

            LhsEval theta, h_NaCl;
            LhsEval d_h, delta_h;

            theta = T - 273.15;

            // Regularization
            Scalar scalarTheta = scalarValue(theta);
            Scalar S_lSAT = f[0] + scalarTheta*(f[1] + scalarTheta*(f[2] + scalarTheta*f[3]));

            LhsEval S = salinity;
            if (S > S_lSAT)
                S = S_lSAT;

            /*DAUBERT and DANNER*/
            /*U=*/h_NaCl = (3.6710E4*T + 0.5*(6.2770E1)*T*T - ((6.6670E-2)/3)*T*T*T
                            +((2.8000E-5)/4)*(T*T*T*T))/(58.44E3)- 2.045698e+02; /* kJ/kg */

            LhsEval m = 1E3/58.44 * S/(1-S);
            int i = 0;
            int j = 0;
            d_h = 0;

            for (i = 0; i<=3; i++) {
                for (j=0; j<=2; j++) {
                    d_h = d_h + a[i][j] * pow(theta, static_cast<Scalar>(i)) * pow(m, j);
                }
            }
            /* heat of dissolution for halite according to Michaelides 1971 */
            delta_h = (4.184/(1E3 + (58.44 * m)))*d_h;

            /* Enthalpy of brine without CO2 */
            h_ls1 =(1-S)*hw + S*h_NaCl + S*delta_h; /* kJ/kg */

            // Use Enthalpy of brine without CO2
            if (liquidMixType_ == Co2StoreConfig::LiquidMixingType::NONE)
                return  h_ls1*1E3;
        }

        LhsEval delta_hCO2, hg;
        /* heat of dissolution for CO2 according to Fig. 6 in Duan and Sun 2003. (kJ/kg)
           In the relevant temperature ranges CO2 dissolution is
           exothermal */
        if (liquidMixType_ == Co2StoreConfig::LiquidMixingType::DUANSUN)
            delta_hCO2 = (-57.4375 + T * 0.1325) * 1000/44;
        else 
            delta_hCO2 = 0.0;
            
        /* enthalpy contribution of CO2 (kJ/kg) */
        hg = CO2::gasEnthalpy(T, p, extrapolate)/1E3 + delta_hCO2;

        /* Enthalpy of brine with dissolved CO2 */
        return (h_ls1 - X_CO2_w*hw + hg*X_CO2_w)*1E3; /*J/kg*/
    }

    template <class LhsEval>
    const LhsEval salinityFromConcentration(unsigned regionIdx, const LhsEval&T, const LhsEval& P, const LhsEval& saltConcentration) const
    {
        if (enableSaltConcentration_)
            return saltConcentration/H2O::liquidDensity(T, P, true);

        return salinity(regionIdx);
    }
};

} // namespace Opm

#endif
