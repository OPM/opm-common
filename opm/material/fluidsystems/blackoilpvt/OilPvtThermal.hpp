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
 * \copydoc Opm::OilPvtThermal
 */
#ifndef OPM_OIL_PVT_THERMAL_HPP
#define OPM_OIL_PVT_THERMAL_HPP

#include <opm/material/common/Tabulated1DFunction.hpp>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

template <class Scalar, bool enableThermal>
class OilPvtMultiplexer;

/*!
 * \brief This class implements temperature dependence of the PVT properties of oil
 *
 * Note that this _only_ implements the temperature part, i.e., it requires the
 * isothermal properties as input.
 */
template <class Scalar>
class OilPvtThermal
{
public:
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;
    using IsothermalPvt = OilPvtMultiplexer<Scalar, /*enableThermal=*/false>;

    OilPvtThermal()
    {
        enableThermalDensity_ = false;
        enableJouleThomson_ = false;
        enableThermalViscosity_ = false;
        enableInternalEnergy_ = false;
        isothermalPvt_ = nullptr;
    }

    OilPvtThermal(IsothermalPvt* isothermalPvt,
                  const std::vector<TabulatedOneDFunction>& oilvisctCurves,
                  const std::vector<Scalar>& viscrefPress,
                  const std::vector<Scalar>& viscrefRs,
                  const std::vector<Scalar>& viscRef,
                  const std::vector<Scalar>& oildentRefTemp,
                  const std::vector<Scalar>& oildentCT1,
                  const std::vector<Scalar>& oildentCT2,
                  const std::vector<Scalar>& oilJTRefPres,
                  const std::vector<Scalar>& oilJTC,
                  const std::vector<TabulatedOneDFunction>& internalEnergyCurves,
                  bool enableThermalDensity,
                  bool enableJouleThomson,
                  bool enableThermalViscosity,
                  bool enableInternalEnergy)
        : isothermalPvt_(isothermalPvt)
        , oilvisctCurves_(oilvisctCurves)
        , viscrefPress_(viscrefPress)
        , viscrefRs_(viscrefRs)
        , viscRef_(viscRef)
        , oildentRefTemp_(oildentRefTemp)
        , oildentCT1_(oildentCT1)
        , oildentCT2_(oildentCT2)
        , oilJTRefPres_(oilJTRefPres)
        , oilJTC_(oilJTC)
        , internalEnergyCurves_(internalEnergyCurves)
        , enableThermalDensity_(enableThermalDensity)
        , enableJouleThomson_(enableJouleThomson)
        , enableThermalViscosity_(enableThermalViscosity)
        , enableInternalEnergy_(enableInternalEnergy)
    { }

    OilPvtThermal(const OilPvtThermal& data)
    { *this = data; }

    ~OilPvtThermal()
    { delete isothermalPvt_; }

#if HAVE_ECL_INPUT
    /*!
     * \brief Implement the temperature part of the oil PVT properties.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    /*!
     * \brief Set the number of PVT-regions considered by this object.
     */
    void setNumRegions(size_t numRegions)
    {
        oilvisctCurves_.resize(numRegions);
        viscrefPress_.resize(numRegions);
        viscrefRs_.resize(numRegions);
        viscRef_.resize(numRegions);
        internalEnergyCurves_.resize(numRegions);
        oildentRefTemp_.resize(numRegions);
        oildentCT1_.resize(numRegions);
        oildentCT2_.resize(numRegions);
        oilJTRefPres_.resize(numRegions);
        oilJTC_.resize(numRegions);
        rhoRefG_.resize(numRegions);
        hVap_.resize(numRegions, 0.0);
    }

    void setVapPars(const Scalar par1, const Scalar par2)
    {
        isothermalPvt_->setVapPars(par1, par2);
    }

    /*!
     * \brief Finish initializing the thermal part of the oil phase PVT properties.
     */
    void initEnd()
    { }

    /*!
     * \brief Returns true iff the density of the oil phase is temperature dependent.
     */
    bool enableThermalDensity() const
    { return enableThermalDensity_; }

    /*!
     * \brief Returns true iff Joule-Thomson effect for the oil phase is active.
     */
    bool enableJouleThomson() const
    { return enableJouleThomson_; }

    /*!
     * \brief Returns true iff the viscosity of the oil phase is temperature dependent.
     */
    bool enableThermalViscosity() const
    { return enableThermalViscosity_; }

    size_t numRegions() const
    { return viscrefRs_.size(); }

    /*!
     * \brief Returns the specific internal energy [J/kg] of oil given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                              const Evaluation& temperature,
                              const Evaluation& pressure,
                              const Evaluation& Rs) const
    {
        if (!enableInternalEnergy_)
             throw std::runtime_error("Requested the internal energy of oil but it is disabled");

        if (!enableJouleThomson_) {
            // compute the specific internal energy for the specified tempature. We use linear
            // interpolation here despite the fact that the underlying heat capacities are
            // piecewise linear (which leads to a quadratic function)
            return internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        }
        else {
            OpmLog::warning("Experimental code for jouleThomson: simulation will be slower");
            Evaluation Tref = oildentRefTemp_[regionIdx];
            Evaluation Pref = oilJTRefPres_[regionIdx];
            Scalar JTC = oilJTC_[regionIdx]; // if JTC is default then JTC is calculated

            Evaluation invB = inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rs);
            Evaluation Cp = internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true)/temperature;
            Evaluation density = invB * (oilReferenceDensity(regionIdx) + Rs * rhoRefG_[regionIdx]);

            Evaluation enthalpyPres;
            if  (JTC != 0) {
                enthalpyPres = -Cp * JTC * (pressure -Pref);
            }
            else if(enableThermalDensity_) {
                Scalar c1T = oildentCT1_[regionIdx];
                Scalar c2T = oildentCT2_[regionIdx];

                Evaluation alpha = (c1T + 2 * c2T * (temperature - Tref)) /
                    (1 + c1T  *(temperature - Tref) + c2T * (temperature - Tref) * (temperature - Tref));

                const int N = 100; // value is experimental
                Evaluation deltaP = (pressure - Pref)/N;
                Evaluation enthalpyPresPrev = 0;
                for (size_t i = 0; i < N; ++i) {
                    Evaluation Pnew = Pref + i * deltaP;
                    Evaluation rho = inverseFormationVolumeFactor(regionIdx, temperature, Pnew, Rs) *
                                     (oilReferenceDensity(regionIdx) + Rs * rhoRefG_[regionIdx]) ;
                    // see e.g.https://en.wikipedia.org/wiki/Joule-Thomson_effect for a derivation of the Joule-Thomson coeff.
                    Evaluation jouleThomsonCoefficient = -(1.0/Cp) * (1.0 - alpha * temperature)/rho;
                    Evaluation deltaEnthalpyPres = -Cp * jouleThomsonCoefficient * deltaP;
                    enthalpyPres = enthalpyPresPrev + deltaEnthalpyPres;
                    enthalpyPresPrev = enthalpyPres;
                }
            }
            else {
                  throw std::runtime_error("Requested Joule-thomson calculation but thermal oil density (OILDENT) is not provided");
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
                         const Evaluation& Rs) const
    {
        const auto& isothermalMu = isothermalPvt_->viscosity(regionIdx, temperature, pressure, Rs);
        if (!enableThermalViscosity())
            return isothermalMu;

        // compute the viscosity deviation due to temperature
        const auto& muOilvisct = oilvisctCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        return muOilvisct/viscRef_[regionIdx]*isothermalMu;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
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
        const auto& muOilvisct = oilvisctCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        return muOilvisct/viscRef_[regionIdx]*isothermalMu;
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
        const auto& b =
            isothermalPvt_->inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rs);

        if (!enableThermalDensity())
            return b;

        // we use the same approach as for the for water here, but with the OPM-specific
        // OILDENT keyword.
        Scalar TRef = oildentRefTemp_[regionIdx];
        Scalar cT1 = oildentCT1_[regionIdx];
        Scalar cT2 = oildentCT2_[regionIdx];
        const Evaluation& Y = temperature - TRef;

        return b/(1 + (cT1 + cT2*Y)*Y);
    }

    /*!
     * \brief Returns the formation volume factor [-] of gas-saturated oil phase.
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
        // OILDENT keyword.
        Scalar TRef = oildentRefTemp_[regionIdx];
        Scalar cT1 = oildentCT1_[regionIdx];
        Scalar cT2 = oildentCT2_[regionIdx];
        const Evaluation& Y = temperature - TRef;

        return b/(1 + (cT1 + cT2*Y)*Y);
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     *
     * This method implements temperature dependence and requires the isothermal gas
     * dissolution factor for gas saturated oil and temperature as inputs. Currently it
     * is just a dummy method which passes through the isothermal gas dissolution factor.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure) const
    { return isothermalPvt_->saturatedGasDissolutionFactor(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     *
     * This method implements temperature dependence and requires the isothermal gas
     * dissolution factor for gas saturated oil and temperature as inputs. Currently it
     * is just a dummy method which passes through the isothermal gas dissolution factor.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                             const Evaluation& temperature,
                                             const Evaluation& pressure,
                                             const Evaluation& oilSaturation,
                                             const Evaluation& maxOilSaturation) const
    { return isothermalPvt_->saturatedGasDissolutionFactor(regionIdx, temperature, pressure, oilSaturation, maxOilSaturation); }

    /*!
     * \brief Returns the saturation pressure of the oil phase [Pa]
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

    const Scalar oilReferenceDensity(unsigned regionIdx) const
    { return isothermalPvt_->oilReferenceDensity(regionIdx); }

    const Scalar hVap(unsigned regionIdx) const {
        return this->hVap_[regionIdx];
    }

    const std::vector<TabulatedOneDFunction>& oilvisctCurves() const
    { return oilvisctCurves_; }

    const std::vector<Scalar>& viscrefPress() const
    { return viscrefPress_; }

    const std::vector<Scalar>& viscrefRs() const
    { return viscrefRs_; }

    const std::vector<Scalar>& viscRef() const
    { return viscRef_; }

    const std::vector<Scalar>& oildentRefTemp() const
    { return oildentRefTemp_; }

    const std::vector<Scalar>& oildentCT1() const
    { return oildentCT1_; }

    const std::vector<Scalar>& oildentCT2() const
    { return oildentCT2_; }

    const std::vector<TabulatedOneDFunction> internalEnergyCurves() const
    { return internalEnergyCurves_; }

    bool enableInternalEnergy() const
    { return enableInternalEnergy_; }

    const std::vector<Scalar>& oilJTRefPres() const
    { return  oilJTRefPres_; }

     const std::vector<Scalar>&  oilJTC() const
    { return oilJTC_; }

    bool operator==(const OilPvtThermal<Scalar>& data) const
    {
        if (isothermalPvt_ && !data.isothermalPvt_)
            return false;
        if (!isothermalPvt_ && data.isothermalPvt_)
            return false;

        return  this->oilvisctCurves() == data.oilvisctCurves() &&
                this->viscrefPress() == data.viscrefPress() &&
                this->viscrefRs() == data.viscrefRs() &&
                this->viscRef() == data.viscRef() &&
                this->oildentRefTemp() == data.oildentRefTemp() &&
                this->oildentCT1() == data.oildentCT1() &&
                this->oildentCT2() == data.oildentCT2() &&
                this->oilJTRefPres() == data.oilJTRefPres() &&
                this->oilJTC() == data.oilJTC() &&
                this->internalEnergyCurves() == data.internalEnergyCurves() &&
                this->enableThermalDensity() == data.enableThermalDensity() &&
                this->enableJouleThomson() == data.enableJouleThomson() &&
                this->enableThermalViscosity() == data.enableThermalViscosity() &&
                this->enableInternalEnergy() == data.enableInternalEnergy();
    }

    OilPvtThermal<Scalar>& operator=(const OilPvtThermal<Scalar>& data)
    {
        if (data.isothermalPvt_)
            isothermalPvt_ = new IsothermalPvt(*data.isothermalPvt_);
        else
            isothermalPvt_ = nullptr;
        oilvisctCurves_ = data.oilvisctCurves_;
        viscrefPress_ = data.viscrefPress_;
        viscrefRs_ = data.viscrefRs_;
        viscRef_ = data.viscRef_;
        oildentRefTemp_ = data.oildentRefTemp_;
        oildentCT1_ = data.oildentCT1_;
        oildentCT2_ = data.oildentCT2_;
        oilJTRefPres_ =  data.oilJTRefPres_;
        oilJTC_ =  data.oilJTC_;
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
    std::vector<TabulatedOneDFunction> oilvisctCurves_;
    std::vector<Scalar> viscrefPress_;
    std::vector<Scalar> viscrefRs_;
    std::vector<Scalar> viscRef_;

    // The PVT properties needed for temperature dependence of the density.
    std::vector<Scalar> oildentRefTemp_;
    std::vector<Scalar> oildentCT1_;
    std::vector<Scalar> oildentCT2_;

    std::vector<Scalar> oilJTRefPres_;
    std::vector<Scalar> oilJTC_;

    std::vector<Scalar> rhoRefG_;
    std::vector<Scalar> hVap_;

    // piecewise linear curve representing the internal energy of oil
    std::vector<TabulatedOneDFunction> internalEnergyCurves_;

    bool enableThermalDensity_;
    bool enableJouleThomson_;
    bool enableThermalViscosity_;
    bool enableInternalEnergy_;
};

} // namespace Opm

#endif
