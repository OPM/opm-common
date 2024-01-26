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
 * \copydoc Opm::GasPvtThermal
 */
#ifndef OPM_GAS_PVT_THERMAL_HPP
#define OPM_GAS_PVT_THERMAL_HPP

#include <opm/material/common/Tabulated1DFunction.hpp>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

template <class Scalar, bool enableThermal>
class GasPvtMultiplexer;

/*!
 * \brief This class implements temperature dependence of the PVT properties of gas
 *
 * Note that this _only_ implements the temperature part, i.e., it requires the
 * isothermal properties as input.
 */
template <class Scalar>
class GasPvtThermal
{
public:
    using IsothermalPvt = GasPvtMultiplexer<Scalar, /*enableThermal=*/false>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;

    GasPvtThermal()
    {
        enableThermalDensity_ = false;
        enableJouleThomson_ = false;
        enableThermalViscosity_ = false;
        enableInternalEnergy_ = false;
        isothermalPvt_ = nullptr;
    }

    GasPvtThermal(IsothermalPvt* isothermalPvt,
                  const std::vector<TabulatedOneDFunction>& gasvisctCurves,
                  const std::vector<Scalar>& viscrefPress,
                  const std::vector<Scalar>& viscRef,
                  const std::vector<Scalar>& gasdentRefTemp,
                  const std::vector<Scalar>& gasdentCT1,
                  const std::vector<Scalar>& gasdentCT2,
                  const std::vector<Scalar>& gasJTRefPres,
                  const std::vector<Scalar>& gasJTC,
                  const std::vector<TabulatedOneDFunction>& internalEnergyCurves,
                  bool enableThermalDensity,
                  bool enableJouleThomson,
                  bool enableThermalViscosity,
                  bool enableInternalEnergy)
        : isothermalPvt_(isothermalPvt)
        , gasvisctCurves_(gasvisctCurves)
        , viscrefPress_(viscrefPress)
        , viscRef_(viscRef)
        , gasdentRefTemp_(gasdentRefTemp)
        , gasdentCT1_(gasdentCT1)
        , gasdentCT2_(gasdentCT2)
        , gasJTRefPres_(gasJTRefPres)
        , gasJTC_(gasJTC)
        , internalEnergyCurves_(internalEnergyCurves)
        , enableThermalDensity_(enableThermalDensity)
        , enableJouleThomson_(enableJouleThomson)
        , enableThermalViscosity_(enableThermalViscosity)
        , enableInternalEnergy_(enableInternalEnergy)
    { }

    GasPvtThermal(const GasPvtThermal& data)
    { *this = data; }

    ~GasPvtThermal()
    { delete isothermalPvt_; }

#if HAVE_ECL_INPUT
    /*!
     * \brief Implement the temperature part of the gas PVT properties.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    /*!
     * \brief Set the number of PVT-regions considered by this object.
     */
    void setNumRegions(size_t numRegions)
    {
        gasvisctCurves_.resize(numRegions);
        viscrefPress_.resize(numRegions);
        viscRef_.resize(numRegions);
        internalEnergyCurves_.resize(numRegions);
        gasdentRefTemp_.resize(numRegions);
        gasdentCT1_.resize(numRegions);
        gasdentCT2_.resize(numRegions);
        gasJTRefPres_.resize(numRegions);
        gasJTC_.resize(numRegions);
        rhoRefO_.resize(numRegions);
        hVap_.resize(numRegions, 0.0);
    }

    void setVapPars(const Scalar par1, const Scalar par2)
    {
        isothermalPvt_->setVapPars(par1, par2);
    }

    /*!
     * \brief Finish initializing the thermal part of the gas phase PVT properties.
     */
    void initEnd()
    { }

    /*!
     * \brief Returns true iff the density of the gas phase is temperature dependent.
     */
    bool enableThermalDensity() const
    { return enableThermalDensity_; }

     /*!
     * \brief Returns true iff Joule-Thomson effect for the gas phase is active.
     */
    bool enableJouleThomson() const
    { return enableJouleThomson_; }

    /*!
     * \brief Returns true iff the viscosity of the gas phase is temperature dependent.
     */
    bool enableThermalViscosity() const
    { return enableThermalViscosity_; }

    size_t numRegions() const
    { return viscrefPress_.size(); }


    /*!
     * \brief Returns the specific internal energy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                              const Evaluation& temperature,
                              const Evaluation& pressure,
                              const Evaluation& Rv,
                              [[maybe_unused]] const Evaluation& RvW) const
    {
        if (!enableInternalEnergy_)
             throw std::runtime_error("Requested the internal energy of gas but it is disabled");

        if (!enableJouleThomson_) {
            // compute the specific internal energy for the specified tempature. We use linear
            // interpolation here despite the fact that the underlying heat capacities are
            // piecewise linear (which leads to a quadratic function)
            return internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        }
        else {
            // NB should probably be revisited with adding more or restrict to linear internal energy
            OpmLog::warning("Experimental code for jouleThomson: simulation will be slower");
            Evaluation Tref = gasdentRefTemp_[regionIdx];
            Evaluation Pref = gasJTRefPres_[regionIdx];
            Scalar JTC = gasJTC_[regionIdx]; // if JTC is default then JTC is calculaited
            Evaluation Rvw = 0.0;

            Evaluation invB = inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rv, Rvw);
            // NB this assumes internalEnergyCurve(0) = 0 derivative should be used add Cp table ??
            Evaluation Cp = (internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true))/temperature;
            Evaluation density = invB * (gasReferenceDensity(regionIdx) + Rv * rhoRefO_[regionIdx]);

            Evaluation enthalpyPres;
            if  (JTC != 0) {
                enthalpyPres = -Cp * JTC * (pressure -Pref);
            }
            else if(enableThermalDensity_) {
                Scalar c1T = gasdentCT1_[regionIdx];
                Scalar c2T = gasdentCT2_[regionIdx];

                Evaluation alpha = (c1T + 2 * c2T * (temperature - Tref)) /
                    (1 + c1T  *(temperature - Tref) + c2T * (temperature - Tref) * (temperature - Tref));

                constexpr const int N = 100; // value is experimental
                Evaluation deltaP = (pressure - Pref)/N;
                Evaluation enthalpyPresPrev = 0;
                for (size_t i = 0; i < N; ++i) {
                    Evaluation Pnew = Pref + i * deltaP;
                    Evaluation rho = inverseFormationVolumeFactor(regionIdx, temperature, Pnew, Rv, Rvw) *
                                     (gasReferenceDensity(regionIdx) + Rv * rhoRefO_[regionIdx]);
                    // see e.g.https://en.wikipedia.org/wiki/Joule-Thomson_effect for a derivation of the Joule-Thomson coeff.
                    Evaluation jouleThomsonCoefficient = -(1.0/Cp) * (1.0 - alpha * temperature)/rho;
                    Evaluation deltaEnthalpyPres = -Cp * jouleThomsonCoefficient * deltaP;
                    enthalpyPres = enthalpyPresPrev + deltaEnthalpyPres;
                    enthalpyPresPrev = enthalpyPres;
                }
            }
            else {
                  throw std::runtime_error("Requested Joule-thomson calculation but thermal gas density (GASDENT) is not provided");
            }

            Evaluation enthalpy = Cp * (temperature - Tref) + enthalpyPres;

            return enthalpy - pressure/density;
        }
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& Rvw) const
    {

        const auto& isothermalMu = isothermalPvt_->viscosity(regionIdx, temperature, pressure, Rv, Rvw);
        if (!enableThermalViscosity())
            return isothermalMu;

        // compute the viscosity deviation due to temperature
        const auto& muGasvisct = gasvisctCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        return muGasvisct/viscRef_[regionIdx]*isothermalMu;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the oil-saturated gas phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        const auto& isothermalMu = isothermalPvt_->saturatedViscosity(regionIdx, temperature, pressure);
        if (!enableThermalViscosity())
            return isothermalMu;

        // compute the viscosity deviation due to temperature
        const auto& muGasvisct = gasvisctCurves_[regionIdx].eval(temperature, true);
        return muGasvisct/viscRef_[regionIdx]*isothermalMu;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& /*Rvw*/) const
    {
        const Evaluation& Rvw = 0.0;
        const auto& b =
            isothermalPvt_->inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rv, Rvw);

        if (!enableThermalDensity())
            return b;

        // we use the same approach as for the for water here, but with the OPM-specific
        // GASDENT keyword.
        //
        // TODO: Since gas is quite a bit more compressible than water, it might be
        //       necessary to make GASDENT to a table keyword. If the current temperature
        //       is relatively close to the reference temperature, the current approach
        //       should be good enough, though.
        Scalar TRef = gasdentRefTemp_[regionIdx];
        Scalar cT1 = gasdentCT1_[regionIdx];
        Scalar cT2 = gasdentCT2_[regionIdx];
        const Evaluation& Y = temperature - TRef;

        return b/(1 + (cT1 + cT2*Y)*Y);
    }

    /*!
     * \brief Returns the formation volume factor [-] of oil-saturated gas.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        const auto& b =
            isothermalPvt_->saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure);

        if (!enableThermalDensity())
            return b;

        // we use the same approach as for the for water here, but with the OPM-specific
        // GASDENT keyword.
        //
        // TODO: Since gas is quite a bit more compressible than water, it might be
        //       necessary to make GASDENT to a table keyword. If the current temperature
        //       is relatively close to the reference temperature, the current approach
        //       should be good enough, though.
        Scalar TRef = gasdentRefTemp_[regionIdx];
        Scalar cT1 = gasdentCT1_[regionIdx];
        Scalar cT2 = gasdentCT2_[regionIdx];
        const Evaluation& Y = temperature - TRef;

        return b/(1 + (cT1 + cT2*Y)*Y);
    }

    /*!
     * \brief Returns the water vaporization factor \f$R_v\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/) const
    { return 0.0; }

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
     *
     * This method implements temperature dependence and requires the gas pressure,
     * temperature and the oil saturation as inputs. Currently it is just a dummy method
     * which passes through the isothermal oil vaporization factor.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return isothermalPvt_->saturatedOilVaporizationFactor(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the gas phase.
     *
     * This method implements temperature dependence and requires the gas pressure,
     * temperature and the oil saturation as inputs. Currently it is just a dummy method
     * which passes through the isothermal oil vaporization factor.
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              const Evaluation& maxOilSaturation) const
    { return isothermalPvt_->saturatedOilVaporizationFactor(regionIdx, temperature, pressure, oilSaturation, maxOilSaturation); }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *
     * This method implements temperature dependence and requires isothermal satuation
     * pressure and temperature as inputs. Currently it is just a dummy method which
     * passes through the isothermal saturation pressure.
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    { return isothermalPvt_->saturationPressure(regionIdx, temperature, pressure); }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
        return isothermalPvt_->diffusionCoefficient(temperature, pressure, compIdx);
    }
    const IsothermalPvt* isoThermalPvt() const
    { return isothermalPvt_; }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return isothermalPvt_->gasReferenceDensity(regionIdx); }

    const Scalar hVap(unsigned regionIdx) const {
        return this->hVap_[regionIdx];
    }

    const std::vector<TabulatedOneDFunction>& gasvisctCurves() const
    { return gasvisctCurves_; }

    const std::vector<Scalar>& viscrefPress() const
    { return viscrefPress_; }

    const std::vector<Scalar>& viscRef() const
    { return viscRef_; }

    const std::vector<Scalar>& gasdentRefTemp() const
    { return gasdentRefTemp_; }

    const std::vector<Scalar>& gasdentCT1() const
    { return gasdentCT1_; }

    const std::vector<Scalar>& gasdentCT2() const
    { return gasdentCT2_; }

    const std::vector<TabulatedOneDFunction>& internalEnergyCurves() const
    { return internalEnergyCurves_; }

    bool enableInternalEnergy() const
    { return enableInternalEnergy_; }

    const std::vector<Scalar>& gasJTRefPres() const
    { return  gasJTRefPres_; }

     const std::vector<Scalar>&  gasJTC() const
    { return gasJTC_; }


    bool operator==(const GasPvtThermal<Scalar>& data) const
    {
        if (isothermalPvt_ && !data.isothermalPvt_)
            return false;
        if (!isothermalPvt_ && data.isothermalPvt_)
            return false;

        return  this->gasvisctCurves() == data.gasvisctCurves() &&
                this->viscrefPress() == data.viscrefPress() &&
                this->viscRef() == data.viscRef() &&
                this->gasdentRefTemp() == data.gasdentRefTemp() &&
                this->gasdentCT1() == data.gasdentCT1() &&
                this->gasdentCT2() == data.gasdentCT2() &&
                this->gasJTRefPres() == data.gasJTRefPres() &&
                this->gasJTC() == data.gasJTC() &&
                this->internalEnergyCurves() == data.internalEnergyCurves() &&
                this->enableThermalDensity() == data.enableThermalDensity() &&
                this->enableJouleThomson() == data.enableJouleThomson() &&
                this->enableThermalViscosity() == data.enableThermalViscosity() &&
                this->enableInternalEnergy() == data.enableInternalEnergy();
    }

    GasPvtThermal<Scalar>& operator=(const GasPvtThermal<Scalar>& data)
    {
        if (data.isothermalPvt_)
            isothermalPvt_ = new IsothermalPvt(*data.isothermalPvt_);
        else
            isothermalPvt_ = nullptr;
        gasvisctCurves_ = data.gasvisctCurves_;
        viscrefPress_ = data.viscrefPress_;
        viscRef_ = data.viscRef_;
        gasdentRefTemp_ = data.gasdentRefTemp_;
        gasdentCT1_ = data.gasdentCT1_;
        gasdentCT2_ = data.gasdentCT2_;
        gasJTRefPres_ =  data.gasJTRefPres_;
        gasJTC_ =  data.gasJTC_;
        internalEnergyCurves_ = data.internalEnergyCurves_;
        enableThermalDensity_ = data.enableThermalDensity_;
        enableJouleThomson_ = data.enableJouleThomson_;
        enableThermalViscosity_ = data.enableThermalViscosity_;
        enableInternalEnergy_ = data.enableInternalEnergy_;

        return *this;
    }

private:
    IsothermalPvt* isothermalPvt_;

    // The PVT properties needed for temperature dependence of the viscosity. We need
    // to store one value per PVT region.
    std::vector<TabulatedOneDFunction> gasvisctCurves_;
    std::vector<Scalar> viscrefPress_;
    std::vector<Scalar> viscRef_;

    std::vector<Scalar> gasdentRefTemp_;
    std::vector<Scalar> gasdentCT1_;
    std::vector<Scalar> gasdentCT2_;

    std::vector<Scalar> gasJTRefPres_;
    std::vector<Scalar> gasJTC_;

    std::vector<Scalar> rhoRefO_;
    std::vector<Scalar> hVap_;

    // piecewise linear curve representing the internal energy of gas
    std::vector<TabulatedOneDFunction> internalEnergyCurves_;

    bool enableThermalDensity_;
    bool enableJouleThomson_;
    bool enableThermalViscosity_;
    bool enableInternalEnergy_;
};

} // namespace Opm

#endif
