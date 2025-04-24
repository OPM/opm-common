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
 *
 * \copydoc Opm::TabulatedComponent
 */
#ifndef OPM_TABULATED_COMPONENT_HPP
#define OPM_TABULATED_COMPONENT_HPP

#include <opm/material/common/MathToolbox.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <cassert>
#include <stdexcept>
#include <vector>

namespace Opm {

template<class Scalar>
struct TabulatedComponentData
{
    // 1D fields with the temperature as degree of freedom
    std::vector<Scalar> vaporPressure_;

    std::vector<Scalar> minLiquidDensity__;
    std::vector<Scalar> maxLiquidDensity__;

    std::vector<Scalar> minGasDensity__;
    std::vector<Scalar> maxGasDensity__;

    // 2D fields with the temperature and pressure as degrees of
    // freedom
    std::vector<Scalar> gasEnthalpy_;
    std::vector<Scalar> liquidEnthalpy_;

    std::vector<Scalar> gasHeatCapacity_;
    std::vector<Scalar> liquidHeatCapacity_;

    std::vector<Scalar> gasDensity_;
    std::vector<Scalar> liquidDensity_;

    std::vector<Scalar> gasViscosity_;
    std::vector<Scalar> liquidViscosity_;

    std::vector<Scalar> gasThermalConductivity_;
    std::vector<Scalar> liquidThermalConductivity_;

    // 2D fields with the temperature and density as degrees of
    // freedom
    std::vector<Scalar> gasPressure_;
    std::vector<Scalar> liquidPressure_;

    // temperature, pressure and density ranges
    Scalar tempMin_;
    Scalar tempMax_;
    unsigned nTemp_;

    Scalar pressMin_;
    Scalar pressMax_;
    unsigned nPress_;

    Scalar densityMin_;
    Scalar densityMax_;
    unsigned nDensity_;

    void init(Scalar tempMin, Scalar tempMax, unsigned nTemp,
              Scalar pressMin, Scalar pressMax, unsigned nPress)
    {
        tempMin_ = tempMin;
        tempMax_ = tempMax;
        nTemp_ = nTemp;
        pressMin_ = pressMin;
        pressMax_ = pressMax;
        nPress_ = nPress;
        nDensity_ = nPress_;

        // allocate the arrays
        vaporPressure_.resize(nTemp_);
        minGasDensity__.resize(nTemp_);
        maxGasDensity__.resize(nTemp_);
        minLiquidDensity__.resize(nTemp_);
        maxLiquidDensity__.resize(nTemp_);

        gasEnthalpy_.resize(nTemp_*nPress_);
        liquidEnthalpy_.resize(nTemp_*nPress_);
        gasHeatCapacity_.resize(nTemp_*nPress_);
        liquidHeatCapacity_.resize(nTemp_*nPress_);
        gasDensity_.resize(nTemp_*nPress_);
        liquidDensity_.resize(nTemp_*nPress_);
        gasViscosity_.resize(nTemp_*nPress_);
        liquidViscosity_.resize(nTemp_*nPress_);
        gasThermalConductivity_.resize(nTemp_*nPress_);
        liquidThermalConductivity_.resize(nTemp_*nPress_);
        gasPressure_.resize(nTemp_*nDensity_);
        liquidPressure_.resize(nTemp_*nDensity_);
    }
};

/*!
 * \ingroup Components
 *
 * \brief A generic class which tabulates all thermodynamic properties
 *        of a given component.
 *
 * At the moment, this class can only handle the sub-critical fluids
 * since it tabulates along the vapor pressure curve.
 *
 * \tparam Scalar The type used for scalar values
 * \tparam RawComponent The component which ought to be tabulated
 * \tparam useVaporPressure If true, tabulate all quantities along the
 *                          vapor pressure curve, if false use the
 *                          pressure range [p_min, p_max]
 */
template <class ScalarT, class RawComponent, bool useVaporPressure=true>
class TabulatedComponent
{
public:
    using Scalar = ScalarT;

    static constexpr bool isTabulated = true;

    /*!
     * \brief Initialize the tables.
     *
     * \param tempMin The minimum of the temperature range in \f$\mathrm{[K]}\f$
     * \param tempMax The maximum of the temperature range in \f$\mathrm{[K]}\f$
     * \param nTemp The number of entries/steps within the temperature range
     * \param pressMin The minimum of the pressure range in \f$\mathrm{[Pa]}\f$
     * \param pressMax The maximum of the pressure range in \f$\mathrm{[Pa]}\f$
     * \param nPress The number of entries/steps within the pressure range
     */
    static void init(Scalar tempMin, Scalar tempMax, unsigned nTemp,
                     Scalar pressMin, Scalar pressMax, unsigned nPress)
    {
        data_.init(tempMin, tempMax, nTemp, pressMin, pressMax, nPress);

        assert(std::numeric_limits<Scalar>::has_quiet_NaN);
        constexpr Scalar NaN = std::numeric_limits<Scalar>::quiet_NaN();

        // fill the temperature-pressure arrays
        for (unsigned iT = 0; iT < data_.nTemp_; ++ iT) {
            const Scalar temperature = iT * (data_.tempMax_ - data_.tempMin_) / (data_.nTemp_ - 1) + data_.tempMin_;

            try {
                data_.vaporPressure_[iT] = RawComponent::vaporPressure(temperature);
            }
            catch (const std::exception&) {
                data_.vaporPressure_[iT] = NaN;
            }

            const Scalar pgMax = maxGasPressure_(iT);
            const Scalar pgMin = minGasPressure_(iT);

            // fill the temperature, pressure gas arrays
            for (unsigned iP = 0; iP < data_.nPress_; ++ iP) {
                const Scalar pressure = iP * (pgMax - pgMin) / (data_.nPress_ - 1) + pgMin;

                const unsigned i = iT + iP * data_.nTemp_;

                try {
                    data_.gasEnthalpy_[i] = RawComponent::gasEnthalpy(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.gasEnthalpy_[i] = NaN;
                }

                try {
                    data_.gasHeatCapacity_[i] = RawComponent::gasHeatCapacity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.gasHeatCapacity_[i] = NaN;
                }

                try {
                    data_.gasDensity_[i] = RawComponent::gasDensity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.gasDensity_[i] = NaN;
                }

                try {
                    data_.gasViscosity_[i] = RawComponent::gasViscosity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.gasViscosity_[i] = NaN;
                }

                try {
                    data_.gasThermalConductivity_[i] = RawComponent::gasThermalConductivity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.gasThermalConductivity_[i] = NaN;
                }
            };

            const Scalar plMin = minLiquidPressure_(iT);
            const Scalar plMax = maxLiquidPressure_(iT);
            for (unsigned iP = 0; iP < data_.nPress_; ++ iP) {
                Scalar pressure = iP * (plMax - plMin) / (data_.nPress_ - 1) + plMin;

                const unsigned i = iT + iP*data_.nTemp_;

                try {
                    data_.liquidEnthalpy_[i] = RawComponent::liquidEnthalpy(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.liquidEnthalpy_[i] = NaN;
                }

                try {
                    data_.liquidHeatCapacity_[i] = RawComponent::liquidHeatCapacity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.liquidHeatCapacity_[i] = NaN;
                }

                try {
                    data_.liquidDensity_[i] = RawComponent::liquidDensity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.liquidDensity_[i] = NaN;
                }

                try {
                    data_.liquidViscosity_[i] = RawComponent::liquidViscosity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.liquidViscosity_[i] = NaN;
                }

                try {
                    data_.liquidThermalConductivity_[i] = RawComponent::liquidThermalConductivity(temperature, pressure);
                }
                catch (const std::exception&) {
                    data_.liquidThermalConductivity_[i] = NaN;
                }
            }
        }

        // fill the temperature-density arrays
        for (unsigned iT = 0; iT < data_.nTemp_; ++ iT) {
            const Scalar temperature = iT * (data_.tempMax_ - data_.tempMin_) / (data_.nTemp_ - 1) + data_.tempMin_;

            // calculate the minimum and maximum values for the gas
            // densities
            data_.minGasDensity__[iT] = RawComponent::gasDensity(temperature, minGasPressure_(iT));
            if (iT < data_.nTemp_ - 1) {
                data_.maxGasDensity__[iT] = RawComponent::gasDensity(temperature, maxGasPressure_(iT + 1));
            }
            else {
                data_.maxGasDensity__[iT] = RawComponent::gasDensity(temperature, maxGasPressure_(iT));
            }

            // fill the temperature, density gas arrays
            for (unsigned iRho = 0; iRho < data_.nDensity_; ++ iRho) {
                const Scalar density =
                    Scalar(iRho) / (data_.nDensity_ - 1) *
                    (data_.maxGasDensity__[iT] - data_.minGasDensity__[iT])
                    +
                    data_.minGasDensity__[iT];

                const unsigned i = iT + iRho * data_.nTemp_;

                try {
                    data_.gasPressure_[i] = RawComponent::gasPressure(temperature, density);
                }
                catch (const std::exception&) {
                    data_.gasPressure_[i] = NaN;
                }
            }

            // calculate the minimum and maximum values for the liquid
            // densities
            data_.minLiquidDensity__[iT] = RawComponent::liquidDensity(temperature, minLiquidPressure_(iT));
            if (iT < data_.nTemp_ - 1) {
                data_.maxLiquidDensity__[iT] = RawComponent::liquidDensity(temperature, maxLiquidPressure_(iT + 1));
            }
            else {
                data_.maxLiquidDensity__[iT] = RawComponent::liquidDensity(temperature, maxLiquidPressure_(iT));
            }

            // fill the temperature, density liquid arrays
            for (unsigned iRho = 0; iRho < data_.nDensity_; ++ iRho) {
                const Scalar density =
                    Scalar(iRho) / (data_.nDensity_ - 1) *
                    (data_.maxLiquidDensity__[iT] - data_.minLiquidDensity__[iT])
                    +
                    data_.minLiquidDensity__[iT];

                const unsigned i = iT + iRho * data_.nTemp_;

                try {
                    data_.liquidPressure_[i] = RawComponent::liquidPressure(temperature, density);
                }
                catch (const std::exception&) {
                    data_.liquidPressure_[i] = NaN;
                }
            }
        }
    }

    /*!
     * \brief A human readable name for the component.
     */
    static std::string_view name()
    { return RawComponent::name(); }

    /*!
     * \brief The molar mass in \f$\mathrm{[kg/mol]}\f$ of the component.
     */
    static Scalar molarMass()
    { return RawComponent::molarMass(); }

    /*!
     * \brief Returns the critical temperature in \f$\mathrm{[K]}\f$ of the component.
     */
    static Scalar criticalTemperature()
    { return RawComponent::criticalTemperature(); }

    /*!
     * \brief Returns the critical pressure in \f$\mathrm{[Pa]}\f$ of the component.
     */
    static Scalar criticalPressure()
    { return RawComponent::criticalPressure(); }

    /*!
     * \brief Returns the acentric factor of the component.
     */
    static Scalar acentricFactor()
    { throw std::runtime_error("Not implemented: acentricFactor of the component"); }

    /*!
     * \brief Returns the critical volume in \f$\mathrm{[m2/kmol]}\f$ of the component.
     */
    static Scalar criticalVolume()
    { throw std::runtime_error("Not implemented: criticalVolume of the compoenent"); }

    /*!
     * \brief Returns the temperature in \f$\mathrm{[K]}\f$ at the component's triple point.
     */
    static Scalar tripleTemperature()
    { return RawComponent::tripleTemperature(); }

    /*!
     * \brief Returns the pressure in \f$\mathrm{[Pa]}\f$ at the component's triple point.
     */
    static Scalar triplePressure()
    { return RawComponent::triplePressure(); }

    /*!
     * \brief The vapor pressure in \f$\mathrm{[Pa]}\f$ of the component at a given
     *        temperature.
     *
     * \param T temperature of component
     */
    template <class Evaluation>
    static Evaluation vaporPressure(const Evaluation& temperature)
    {
        const Evaluation& result = interpolateT_(data_.vaporPressure_, temperature);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::vaporPressure(temperature);
        }
        return result;
    }

    /*!
     * \brief Specific enthalpy of the gas \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasEnthalpy(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateGasTP_(data_.gasEnthalpy_,
                                                     temperature,
                                                     pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::gasEnthalpy(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief Specific enthalpy of the liquid \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidEnthalpy(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateLiquidTP_(data_.liquidEnthalpy_,
                                                        temperature,
                                                        pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::liquidEnthalpy(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief Specific isobaric heat capacity of the gas \f$\mathrm{[J/(kg K)]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasHeatCapacity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateGasTP_(data_.gasHeatCapacity_,
                                                     temperature,
                                                     pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::gasHeatCapacity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief Specific isobaric heat capacity of the liquid \f$\mathrm{[J/(kg K)]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidHeatCapacity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateLiquidTP_(data_.liquidHeatCapacity_,
                                                        temperature,
                                                        pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::liquidHeatCapacity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief Specific internal energy of the gas \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasInternalEnergy(const Evaluation& temperature, const Evaluation& pressure)
    { return gasEnthalpy(temperature, pressure) - pressure / gasDensity(temperature, pressure); }

    /*!
     * \brief Specific internal energy of the liquid \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidInternalEnergy(const Evaluation& temperature, const Evaluation& pressure)
    { return liquidEnthalpy(temperature, pressure) - pressure / liquidDensity(temperature, pressure); }

    /*!
     * \brief The pressure of gas in \f$\mathrm{[Pa]}\f$ at a given density and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param density density of component in \f$\mathrm{[kg/m^3]}\f$
     */
    template <class Evaluation>
    static Evaluation gasPressure(const Evaluation& temperature, Scalar density)
    {
        const Evaluation& result = interpolateGasTRho_(data_.gasPressure_,
                                                       temperature,
                                                       density);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::gasPressure(temperature,
                                             density);
        }
        return result;
    }

    /*!
     * \brief The pressure of liquid in \f$\mathrm{[Pa]}\f$ at a given density and temperature.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param density density of component in \f$\mathrm{[kg/m^3]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidPressure(const Evaluation& temperature, Scalar density)
    {
        const Evaluation& result = interpolateLiquidTRho_(data_.liquidPressure_,
                                                          temperature,
                                                          density);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::liquidPressure(temperature,
                                                density);
        }
        return result;
    }

    /*!
     * \brief Returns true iff the gas phase is assumed to be compressible
     */
    static bool gasIsCompressible()
    { return RawComponent::gasIsCompressible(); }

    /*!
     * \brief Returns true iff the liquid phase is assumed to be compressible
     */
    static bool liquidIsCompressible()
    { return RawComponent::liquidIsCompressible(); }

    /*!
     * \brief Returns true iff the gas phase is assumed to be ideal
     */
    static bool gasIsIdeal()
    { return RawComponent::gasIsIdeal(); }

    /*!
     * \brief The density of gas at a given pressure and temperature
     *        \f$\mathrm{[kg/m^3]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasDensity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateGasTP_(data_.gasDensity_,
                                                     temperature,
                                                     pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::gasDensity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief The density of liquid at a given pressure and
     *        temperature \f$\mathrm{[kg/m^3]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidDensity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateLiquidTP_(data_.liquidDensity_,
                                                        temperature,
                                                        pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::liquidDensity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief The dynamic viscosity \f$\mathrm{[Pa*s]}\f$ of gas.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasViscosity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateGasTP_(data_.gasViscosity_,
                                                     temperature,
                                                     pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::gasViscosity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief The dynamic viscosity \f$\mathrm{[Pa*s]}\f$ of liquid.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidViscosity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateLiquidTP_(data_.liquidViscosity_,
                                                        temperature,
                                                        pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::liquidViscosity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief The thermal conductivity of gaseous water \f$\mathrm{[W / (m K)]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation gasThermalConductivity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateGasTP_(data_.gasThermalConductivity_,
                                                     temperature,
                                                     pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::gasThermalConductivity(temperature, pressure);
        }
        return result;
    }

    /*!
     * \brief The thermal conductivity of liquid water \f$\mathrm{[W / (m K)]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    template <class Evaluation>
    static Evaluation liquidThermalConductivity(const Evaluation& temperature, const Evaluation& pressure)
    {
        const Evaluation& result = interpolateLiquidTP_(data_.liquidThermalConductivity_,
                                                        temperature,
                                                        pressure);
        if (std::isnan(scalarValue(result))) {
            return RawComponent::liquidThermalConductivity(temperature, pressure);
        }
        return result;
    }

private:
    // returns an interpolated value depending on temperature
    template <class Evaluation>
    static Evaluation interpolateT_(const std::vector<Scalar>& values, const Evaluation& T)
    {
        Evaluation alphaT = tempIdx_(T);
        if (alphaT < 0 || alphaT >= data_.nTemp_ - 1) {
            return std::numeric_limits<Scalar>::quiet_NaN();
        }

        const std::size_t iT = static_cast<std::size_t>(scalarValue(alphaT));
        alphaT -= iT;

        return
            values[iT    ]*(1 - alphaT) +
            values[iT + 1]*(    alphaT);
    }

    // returns an interpolated value for liquid depending on
    // temperature and pressure
    template <class Evaluation>
    static Evaluation interpolateLiquidTP_(const std::vector<Scalar>& values,
                                           const Evaluation& T,
                                           const Evaluation& p)
    {
        Evaluation alphaT = tempIdx_(T);
        if (alphaT < 0 || alphaT >= data_.nTemp_ - 1) {
            return std::numeric_limits<Scalar>::quiet_NaN();
        }

        const std::size_t iT = static_cast<std::size_t>(scalarValue(alphaT));
        alphaT -= iT;

        Evaluation alphaP1 = pressLiquidIdx_(p, iT);
        Evaluation alphaP2 = pressLiquidIdx_(p, iT + 1);

        const std::size_t iP1 =
            static_cast<std::size_t>(
                std::max<int>(0, std::min(static_cast<int>(data_.nPress_) - 2,
                                          static_cast<int>(scalarValue(alphaP1)))));
        const std::size_t iP2 =
            static_cast<std::size_t>(
                std::max(0, std::min(static_cast<int>(data_.nPress_) - 2,
                                     static_cast<int>(scalarValue(alphaP2)))));
        alphaP1 -= iP1;
        alphaP2 -= iP2;

        return
            values[(iT    ) + (iP1    ) * data_.nTemp_] * (1 - alphaT) * (1 - alphaP1) +
            values[(iT    ) + (iP1 + 1) * data_.nTemp_] * (1 - alphaT) * (    alphaP1) +
            values[(iT + 1) + (iP2    ) * data_.nTemp_] * (    alphaT) * (1 - alphaP2) +
            values[(iT + 1) + (iP2 + 1) * data_.nTemp_] * (    alphaT) * (    alphaP2);
    }

    // returns an interpolated value for gas depending on
    // temperature and pressure
    template <class Evaluation>
    static Evaluation interpolateGasTP_(const std::vector<Scalar>& values,
                                        const Evaluation& T,
                                        const Evaluation& p)
    {
        Evaluation alphaT = tempIdx_(T);
        if (alphaT < 0 || alphaT >= data_.nTemp_ - 1) {
            return std::numeric_limits<Scalar>::quiet_NaN();
        }

        const std::size_t iT =
            static_cast<std::size_t>(
                std::max(0, std::min(static_cast<int>(data_.nTemp_) - 2,
                                     static_cast<int>(scalarValue(alphaT)))));
        alphaT -= iT;

        Evaluation alphaP1 = pressGasIdx_(p, iT);
        Evaluation alphaP2 = pressGasIdx_(p, iT + 1);
        const std::size_t iP1 =
            static_cast<std::size_t>(
                std::max(0, std::min(static_cast<int>(data_.nPress_) - 2,
                                     static_cast<int>(scalarValue(alphaP1)))));
        const std::size_t iP2 =
            static_cast<std::size_t>(
                std::max(0, std::min(static_cast<int>(data_.nPress_) - 2,
                                     static_cast<int>(scalarValue(alphaP2)))));
        alphaP1 -= iP1;
        alphaP2 -= iP2;

        return
            values[(iT    ) + (iP1    ) * data_.nTemp_] * (1 - alphaT) * (1 - alphaP1) +
            values[(iT    ) + (iP1 + 1) * data_.nTemp_] * (1 - alphaT) * (    alphaP1) +
            values[(iT + 1) + (iP2    ) * data_.nTemp_] * (    alphaT) * (1 - alphaP2) +
            values[(iT + 1) + (iP2 + 1) * data_.nTemp_] * (    alphaT) * (    alphaP2);
    }

    // returns an interpolated value for gas depending on
    // temperature and density
    template <class Evaluation>
    static Evaluation interpolateGasTRho_(const std::vector<Scalar>& values,
                                          const Evaluation& T,
                                          const Evaluation& rho)
    {
        Evaluation alphaT = tempIdx_(T);
        const unsigned iT =
            std::max(0,
                     std::min(static_cast<int>(data_.nTemp_ - 2),
                              static_cast<int>(alphaT)));
        alphaT -= iT;

        Evaluation alphaP1 = densityGasIdx_(rho, iT);
        Evaluation alphaP2 = densityGasIdx_(rho, iT + 1);
        const unsigned iP1 =
            std::max(0,
                     std::min(static_cast<int>(data_.nDensity_ - 2),
                              static_cast<int>(alphaP1)));
        const unsigned iP2 =
            std::max(0,
                     std::min(static_cast<int>(data_.nDensity_ - 2),
                              static_cast<int>(alphaP2)));
        alphaP1 -= iP1;
        alphaP2 -= iP2;

        return
            values[(iT    ) + (iP1    ) * data_.nTemp_] * (1 - alphaT) * (1 - alphaP1) +
            values[(iT    ) + (iP1 + 1) * data_.nTemp_] * (1 - alphaT) * (    alphaP1) +
            values[(iT + 1) + (iP2    ) * data_.nTemp_] * (    alphaT) * (1 - alphaP2) +
            values[(iT + 1) + (iP2 + 1) * data_.nTemp_] * (    alphaT) * (    alphaP2);
    }

    // returns an interpolated value for liquid depending on
    // temperature and density
    template <class Evaluation>
    static Evaluation interpolateLiquidTRho_(const std::vector<Scalar>& values,
                                             const Evaluation& T,
                                             const Evaluation& rho)
    {
        Evaluation alphaT = tempIdx_(T);
        const unsigned iT = std::max<int>(0, std::min<int>(data_.nTemp_ - 2, static_cast<int>(alphaT)));
        alphaT -= iT;

        Evaluation alphaP1 = densityLiquidIdx_(rho, iT);
        Evaluation alphaP2 = densityLiquidIdx_(rho, iT + 1);
        const unsigned iP1 = std::max<int>(0, std::min<int>(data_.nDensity_ - 2, static_cast<int>(alphaP1)));
        const unsigned iP2 = std::max<int>(0, std::min<int>(data_.nDensity_ - 2, static_cast<int>(alphaP2)));
        alphaP1 -= iP1;
        alphaP2 -= iP2;

        return
            values[(iT    ) + (iP1    ) * data_.nTemp_] * (1 - alphaT) * (1 - alphaP1) +
            values[(iT    ) + (iP1 + 1) * data_.nTemp_] * (1 - alphaT) * (    alphaP1) +
            values[(iT + 1) + (iP2    ) * data_.nTemp_] * (    alphaT) * (1 - alphaP2) +
            values[(iT + 1) + (iP2 + 1) * data_.nTemp_] * (    alphaT) * (    alphaP2);
    }

    // returns the index of an entry in a temperature field
    template <class Evaluation>
    static Evaluation tempIdx_(const Evaluation& temperature)
    {
        return (data_.nTemp_ - 1) * (temperature - data_.tempMin_) / (data_.tempMax_ - data_.tempMin_);
    }

    // returns the index of an entry in a pressure field
    template <class Evaluation>
    static Evaluation pressLiquidIdx_(const Evaluation& pressure, std::size_t tempIdx)
    {
        const Scalar plMin = minLiquidPressure_(tempIdx);
        const Scalar plMax = maxLiquidPressure_(tempIdx);

        return (data_.nPress_ - 1) * (pressure - plMin) / (plMax - plMin);
    }

    // returns the index of an entry in a temperature field
    template <class Evaluation>
    static Evaluation pressGasIdx_(const Evaluation& pressure, std::size_t tempIdx)
    {
        const Scalar pgMin = minGasPressure_(tempIdx);
        const Scalar pgMax = maxGasPressure_(tempIdx);

        return (data_.nPress_ - 1) * (pressure - pgMin) / (pgMax - pgMin);
    }

    // returns the index of an entry in a density field
    template <class Evaluation>
    static Evaluation densityLiquidIdx_(const Evaluation& density, std::size_t tempIdx)
    {
        const Scalar densityMin = minLiquidDensity_(tempIdx);
        const Scalar densityMax = maxLiquidDensity_(tempIdx);
        return (data_.nDensity_ - 1) * (density - densityMin) / (densityMax - densityMin);
    }

    // returns the index of an entry in a density field
    template <class Evaluation>
    static Evaluation densityGasIdx_(const Evaluation& density, std::size_t tempIdx)
    {
        const Scalar densityMin = minGasDensity_(tempIdx);
        const Scalar densityMax = maxGasDensity_(tempIdx);
        return (data_.nDensity_ - 1) * (density - densityMin) / (densityMax - densityMin);
    }

    // returns the minimum tabulized liquid pressure at a given
    // temperature index
    static Scalar minLiquidPressure_(std::size_t tempIdx)
    {
        if (!useVaporPressure) {
            return data_.pressMin_;
        }
        else {
            return std::max<Scalar>(data_.pressMin_, data_.vaporPressure_[tempIdx] / 1.1);
        }
    }

    // returns the maximum tabulized liquid pressure at a given
    // temperature index
    static Scalar maxLiquidPressure_(std::size_t tempIdx)
    {
        if (!useVaporPressure) {
            return data_.pressMax_;
        }
        else {
            return std::max<Scalar>(data_.pressMax_, data_.vaporPressure_[tempIdx] * 1.1);
        }
    }

    // returns the minumum tabulized gas pressure at a given
    // temperature index
    static Scalar minGasPressure_(std::size_t tempIdx)
    {
        if (!useVaporPressure) {
            return data_.pressMin_;
        }
        else {
            return std::min<Scalar>(data_.pressMin_, data_.vaporPressure_[tempIdx] / 1.1);
        }
    }

    // returns the maximum tabulized gas pressure at a given
    // temperature index
    static Scalar maxGasPressure_(std::size_t tempIdx)
    {
        if (!useVaporPressure) {
            return data_.pressMax_;
        }
        else {
            return std::min<Scalar>(data_.pressMax_, data_.vaporPressure_[tempIdx] * 1.1);
        }
    }

    // returns the minimum tabulized liquid density at a given
    // temperature index
    static Scalar minLiquidDensity_(std::size_t tempIdx)
    { return data_.minLiquidDensity__[tempIdx]; }

    // returns the maximum tabulized liquid density at a given
    // temperature index
    static Scalar maxLiquidDensity_(std::size_t tempIdx)
    { return data_.maxLiquidDensity__[tempIdx]; }

    // returns the minumum tabulized gas density at a given
    // temperature index
    static Scalar minGasDensity_(std::size_t tempIdx)
    { return data_.minGasDensity__[tempIdx]; }

    // returns the maximum tabulized gas density at a given
    // temperature index
    static Scalar maxGasDensity_(std::size_t tempIdx)
    { return data_.maxGasDensity__[tempIdx]; }

    static TabulatedComponentData<Scalar> data_;
};

template <class Scalar, class RawComponent, bool useVaporPressure>
TabulatedComponentData<Scalar> TabulatedComponent<Scalar, RawComponent, useVaporPressure>::data_;

} // namespace Opm

#endif
