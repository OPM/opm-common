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
 * \copydoc Opm::WaterPvtThermal
 */
#ifndef OPM_WATER_PVT_THERMAL_HPP
#define OPM_WATER_PVT_THERMAL_HPP

#include <opm/material/common/Tabulated1DFunction.hpp>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
class Schedule;
#endif

template <class Scalar, bool enableThermal, bool enableBrine>
class WaterPvtMultiplexer;

/*!
 * \brief This class implements temperature dependence of the PVT properties of water
 *
 * Note that this _only_ implements the temperature part, i.e., it requires the
 * isothermal properties as input.
 */
template <class Scalar, bool enableBrine>
class WaterPvtThermal
{
public:
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar>;
    using IsothermalPvt = WaterPvtMultiplexer<Scalar, /*enableThermal=*/false, enableBrine>;

    WaterPvtThermal()
    {
        enableThermalDensity_ = false;
        enableJouleThomson_ = false;
        enableThermalViscosity_ = false;
        enableInternalEnergy_ = false;
        isothermalPvt_ = nullptr;
    }

    WaterPvtThermal(IsothermalPvt* isothermalPvt,
                    const std::vector<Scalar>& viscrefPress,
                    const std::vector<Scalar>& watdentRefTemp,
                    const std::vector<Scalar>& watdentCT1,
                    const std::vector<Scalar>& watdentCT2,
                    const std::vector<Scalar>& watJTRefPres,
                    const std::vector<Scalar>& watJTC,
                    const std::vector<Scalar>& pvtwRefPress,
                    const std::vector<Scalar>& pvtwRefB,
                    const std::vector<Scalar>& pvtwCompressibility,
                    const std::vector<Scalar>& pvtwViscosity,
                    const std::vector<Scalar>& pvtwViscosibility,
                    const std::vector<TabulatedOneDFunction>& watvisctCurves,
                    const std::vector<TabulatedOneDFunction>& internalEnergyCurves,
                    bool enableThermalDensity,
                    bool enableJouleThomson,
                    bool enableThermalViscosity,
                    bool enableInternalEnergy)
        : isothermalPvt_(isothermalPvt)
        , viscrefPress_(viscrefPress)
        , watdentRefTemp_(watdentRefTemp)
        , watdentCT1_(watdentCT1)
        , watdentCT2_(watdentCT2)
        , watJTRefPres_(watJTRefPres)
        , watJTC_(watJTC)
        , pvtwRefPress_(pvtwRefPress)
        , pvtwRefB_(pvtwRefB)
        , pvtwCompressibility_(pvtwCompressibility)
        , pvtwViscosity_(pvtwViscosity)
        , pvtwViscosibility_(pvtwViscosibility)
        , watvisctCurves_(watvisctCurves)
        , internalEnergyCurves_(internalEnergyCurves)
        , enableThermalDensity_(enableThermalDensity)
        , enableJouleThomson_(enableJouleThomson)
        , enableThermalViscosity_(enableThermalViscosity)
        , enableInternalEnergy_(enableInternalEnergy)
    { }

    WaterPvtThermal(const WaterPvtThermal& data)
    { *this = data; }

    ~WaterPvtThermal()
    { delete isothermalPvt_; }

#if HAVE_ECL_INPUT
    /*!
     * \brief Implement the temperature part of the water PVT properties.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);
#endif // HAVE_ECL_INPUT

    /*!
     * \brief Set the number of PVT-regions considered by this object.
     */
    void setNumRegions(size_t numRegions)
    {
        pvtwRefPress_.resize(numRegions);
        pvtwRefB_.resize(numRegions);
        pvtwCompressibility_.resize(numRegions);
        pvtwViscosity_.resize(numRegions);
        pvtwViscosibility_.resize(numRegions);
        viscrefPress_.resize(numRegions);
        watvisctCurves_.resize(numRegions);
        watdentRefTemp_.resize(numRegions);
        watdentCT1_.resize(numRegions);
        watdentCT2_.resize(numRegions);
        watJTRefPres_.resize(numRegions);
        watJTC_.resize(numRegions);
        internalEnergyCurves_.resize(numRegions);
        hVap_.resize(numRegions,0.0);
    }

    void setVapPars(const Scalar par1, const Scalar par2)
    {
        isothermalPvt_->setVapPars(par1, par2);
    }

    /*!
     * \brief Finish initializing the thermal part of the water phase PVT properties.
     */
    void initEnd()
    { }

    /*!
     * \brief Returns true iff the density of the water phase is temperature dependent.
     */
    bool enableThermalDensity() const
    { return enableThermalDensity_; }

     /*!
     * \brief Returns true iff Joule-Thomson effect for the water phase is active.
     */
    bool enableJouleThomson() const
    { return enableJouleThomson_; }

    /*!
     * \brief Returns true iff the viscosity of the water phase is temperature dependent.
     */
    bool enableThermalViscosity() const
    { return enableThermalViscosity_; }

    const Scalar hVap(unsigned regionIdx) const {
        return this->hVap_[regionIdx];
    }

    size_t numRegions() const
    { return pvtwRefPress_.size(); }

    /*!
     * \brief Returns the specific internal energy [J/kg] of water given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                              const Evaluation& temperature,
                              const Evaluation& pressure,
                              const Evaluation& Rsw,
                              const Evaluation& saltconcentration) const
    {
        if (!enableInternalEnergy_)
            throw std::runtime_error("Requested the internal energy of water but it is disabled");

        if (!enableJouleThomson_) {
            // compute the specific internal energy for the specified tempature. We use linear
            // interpolation here despite the fact that the underlying heat capacities are
            // piecewise linear (which leads to a quadratic function)
            return internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        }
        else {
            Evaluation Tref = watdentRefTemp_[regionIdx];
            Evaluation Pref = watJTRefPres_[regionIdx];
            Scalar JTC =watJTC_[regionIdx]; // if JTC is default then JTC is calculated

            Evaluation invB = inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rsw, saltconcentration);
            Evaluation Cp = internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true)/temperature;
            Evaluation density = invB * waterReferenceDensity(regionIdx);

            Evaluation enthalpyPres;
            if  (JTC != 0) {
                enthalpyPres = -Cp * JTC * (pressure -Pref);
            }
            else if(enableThermalDensity_) {
                Scalar c1T = watdentCT1_[regionIdx];
                Scalar c2T = watdentCT2_[regionIdx];

                Evaluation alpha = (c1T + 2 * c2T * (temperature - Tref)) /
                    (1 + c1T  *(temperature - Tref) + c2T * (temperature - Tref) * (temperature - Tref));

                constexpr const int N = 100; // value is experimental
                Evaluation deltaP = (pressure - Pref)/N;
                Evaluation enthalpyPresPrev = 0;
                for (size_t i = 0; i < N; ++i) {
                    Evaluation Pnew = Pref + i * deltaP;
                    Evaluation rho = inverseFormationVolumeFactor(regionIdx, temperature, Pnew, Rsw, saltconcentration) * waterReferenceDensity(regionIdx);
                    Evaluation jouleThomsonCoefficient = -(1.0/Cp) * (1.0 - alpha * temperature)/rho;
                    Evaluation deltaEnthalpyPres = -Cp * jouleThomsonCoefficient * deltaP;
                    enthalpyPres = enthalpyPresPrev + deltaEnthalpyPres;
                    enthalpyPresPrev = enthalpyPres;
                }
            }
            else {
                  throw std::runtime_error("Requested Joule-thomson calculation but thermal water density (WATDENT) is not provided");
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
                         const Evaluation& Rsw,
                         const Evaluation& saltconcentration) const
    {
        const auto& isothermalMu = isothermalPvt_->viscosity(regionIdx, temperature, pressure, Rsw, saltconcentration);
        if (!enableThermalViscosity())
            return isothermalMu;

        Scalar x = -pvtwViscosibility_[regionIdx]*(viscrefPress_[regionIdx] - pvtwRefPress_[regionIdx]);
        Scalar muRef = pvtwViscosity_[regionIdx]/(1.0 + x + 0.5*x*x);

        // compute the viscosity deviation due to temperature
        const auto& muWatvisct = watvisctCurves_[regionIdx].eval(temperature, true);
        return isothermalMu * muWatvisct/muRef;
    }

        /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure,
                                  const Evaluation& saltconcentration) const
    {
        const auto& isothermalMu = isothermalPvt_->saturatedViscosity(regionIdx, temperature, pressure, saltconcentration);
        if (!enableThermalViscosity())
            return isothermalMu;

        Scalar x = -pvtwViscosibility_[regionIdx]*(viscrefPress_[regionIdx] - pvtwRefPress_[regionIdx]);
        Scalar muRef = pvtwViscosity_[regionIdx]/(1.0 + x + 0.5*x*x);

        // compute the viscosity deviation due to temperature
        const auto& muWatvisct = watvisctCurves_[regionIdx].eval(temperature, true);
        return isothermalMu * muWatvisct/muRef;
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
        Evaluation Rsw = 0.0;
        return inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rsw, saltconcentration);
    }
    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& Rsw,
                                            const Evaluation& saltconcentration) const
    {
        if (!enableThermalDensity())
            return isothermalPvt_->inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rsw, saltconcentration);

        Scalar BwRef = pvtwRefB_[regionIdx];
        Scalar TRef = watdentRefTemp_[regionIdx];
        const Evaluation& X = pvtwCompressibility_[regionIdx]*(pressure - pvtwRefPress_[regionIdx]);
        Scalar cT1 = watdentCT1_[regionIdx];
        Scalar cT2 = watdentCT2_[regionIdx];
        const Evaluation& Y = temperature - TRef;

        // this is inconsistent with the density calculation of water in the isothermal
        // case (it misses the quadratic pressure term), but it is the equation given in
        // the documentation.
        return 1.0/(((1 - X)*(1 + cT1*Y + cT2*Y*Y))*BwRef);
    }

    /*!
     * \brief Returns the saturation pressure of the water phase [Pa]
     *        depending on its mass fraction of the gas component
     *
     * \param Rs The surface volume of gas component dissolved in what will yield one cubic meter of oil at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rs*/,
                                  const Evaluation& /*saltconcentration*/) const
    { return 0.0; /* this is dead water, so there isn't any meaningful saturation pressure! */ }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                             const Evaluation& /*temperature*/,
                                             const Evaluation& /*pressure*/,
                                             const Evaluation& /*saltconcentration*/) const
    { return 0.0; /* this is dead water! */ }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                    const Evaluation& /*pressure*/,
                                    unsigned /*compIdx*/) const
    {
        throw std::runtime_error("Not implemented: The PVT model does not provide a diffusionCoefficient()");
    }

    const IsothermalPvt* isoThermalPvt() const
    { return isothermalPvt_; }

    const Scalar waterReferenceDensity(unsigned regionIdx) const
    { return isothermalPvt_->waterReferenceDensity(regionIdx); }

    const std::vector<Scalar>& viscrefPress() const
    { return viscrefPress_; }

    const std::vector<Scalar>& watdentRefTemp() const
    { return watdentRefTemp_; }

    const std::vector<Scalar>& watdentCT1() const
    { return watdentCT1_; }

    const std::vector<Scalar>& watdentCT2() const
    { return watdentCT2_; }

    const std::vector<Scalar>& pvtwRefPress() const
    { return pvtwRefPress_; }

    const std::vector<Scalar>& pvtwRefB() const
    { return pvtwRefB_; }

    const std::vector<Scalar>& pvtwCompressibility() const
    { return pvtwCompressibility_; }

    const std::vector<Scalar>& pvtwViscosity() const
    { return pvtwViscosity_; }

    const std::vector<Scalar>& pvtwViscosibility() const
    { return pvtwViscosibility_; }

    const std::vector<TabulatedOneDFunction>& watvisctCurves() const
    { return watvisctCurves_; }

    const std::vector<TabulatedOneDFunction> internalEnergyCurves() const
    { return internalEnergyCurves_; }

    bool enableInternalEnergy() const
    { return enableInternalEnergy_; }

    const std::vector<Scalar>& watJTRefPres() const
    { return  watJTRefPres_; }

     const std::vector<Scalar>&  watJTC() const
    { return watJTC_; }


    bool operator==(const WaterPvtThermal<Scalar, enableBrine>& data) const
    {
        if (isothermalPvt_ && !data.isothermalPvt_)
            return false;
        if (!isothermalPvt_ && data.isothermalPvt_)
            return false;

        return this->viscrefPress() == data.viscrefPress() &&
               this->watdentRefTemp() == data.watdentRefTemp() &&
               this->watdentCT1() == data.watdentCT1() &&
               this->watdentCT2() == data.watdentCT2() &&
               this->watdentCT2() == data.watdentCT2() &&
               this->watJTRefPres() == data.watJTRefPres() &&
               this->watJTC() == data.watJTC() &&
               this->pvtwRefPress() == data.pvtwRefPress() &&
               this->pvtwRefB() == data.pvtwRefB() &&
               this->pvtwCompressibility() == data.pvtwCompressibility() &&
               this->pvtwViscosity() == data.pvtwViscosity() &&
               this->pvtwViscosibility() == data.pvtwViscosibility() &&
               this->watvisctCurves() == data.watvisctCurves() &&
               this->internalEnergyCurves() == data.internalEnergyCurves() &&
               this->enableThermalDensity() == data.enableThermalDensity() &&
               this->enableJouleThomson() == data.enableJouleThomson() &&
               this->enableThermalViscosity() == data.enableThermalViscosity() &&
               this->enableInternalEnergy() == data.enableInternalEnergy();
    }

    WaterPvtThermal<Scalar, enableBrine>& operator=(const WaterPvtThermal<Scalar, enableBrine>& data)
    {
        if (data.isothermalPvt_)
            isothermalPvt_ = new IsothermalPvt(*data.isothermalPvt_);
        else
            isothermalPvt_ = nullptr;
        viscrefPress_ = data.viscrefPress_;
        watdentRefTemp_ = data.watdentRefTemp_;
        watdentCT1_ = data.watdentCT1_;
        watdentCT2_ = data.watdentCT2_;
        watJTRefPres_ =  data.watJTRefPres_;
        watJTC_ =  data.watJTC_;
        pvtwRefPress_ = data.pvtwRefPress_;
        pvtwRefB_ = data.pvtwRefB_;
        pvtwCompressibility_ = data.pvtwCompressibility_;
        pvtwViscosity_ = data.pvtwViscosity_;
        pvtwViscosibility_ = data.pvtwViscosibility_;
        watvisctCurves_ = data.watvisctCurves_;
        internalEnergyCurves_ = data.internalEnergyCurves_;
        enableThermalDensity_ = data.enableThermalDensity_;
        enableJouleThomson_ = data.enableJouleThomson_;
        enableThermalViscosity_ = data.enableThermalViscosity_;
        enableInternalEnergy_ = data.enableInternalEnergy_;

        return *this;
    }

private:
    IsothermalPvt* isothermalPvt_;

    // The PVT properties needed for temperature dependence. We need to store one
    // value per PVT region.
    std::vector<Scalar> viscrefPress_;

    std::vector<Scalar> watdentRefTemp_;
    std::vector<Scalar> watdentCT1_;
    std::vector<Scalar> watdentCT2_;

    std::vector<Scalar> watJTRefPres_;
    std::vector<Scalar> watJTC_;

    std::vector<Scalar> pvtwRefPress_;
    std::vector<Scalar> pvtwRefB_;
    std::vector<Scalar> pvtwCompressibility_;
    std::vector<Scalar> pvtwViscosity_;
    std::vector<Scalar> pvtwViscosibility_;

    std::vector<TabulatedOneDFunction> watvisctCurves_;

    // piecewise linear curve representing the internal energy of water
    std::vector<TabulatedOneDFunction> internalEnergyCurves_;
    std::vector<Scalar> hVap_;

    bool enableThermalDensity_;
    bool enableJouleThomson_;
    bool enableThermalViscosity_;
    bool enableInternalEnergy_;
};

} // namespace Opm

#endif
