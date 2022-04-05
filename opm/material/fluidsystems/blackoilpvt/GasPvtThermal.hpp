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

#include <opm/material/Constants.hpp>

#include <opm/material/common/OpmFinal.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>
#include <opm/material/common/Spline.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

namespace Opm {
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
    typedef GasPvtMultiplexer<Scalar, /*enableThermal=*/false> IsothermalPvt;
    typedef Tabulated1DFunction<Scalar> TabulatedOneDFunction;

    GasPvtThermal()
    {
        enableThermalDensity_ = false;
        enableThermalViscosity_ = false;
        enableJouleThomson_ = false;
        isothermalPvt_ = nullptr;
    }

    GasPvtThermal(IsothermalPvt* isothermalPvt,
                  const std::vector<TabulatedOneDFunction>& gasvisctCurves,
                  const std::vector<Scalar>& gasdentRefTemp,
                  const std::vector<Scalar>& gasdentCT1,
                  const std::vector<Scalar>& gasdentCT2,
                  const std::vector<TabulatedOneDFunction>& internalEnergyCurves,
                  bool enableThermalDensity,
                  bool enableThermalViscosity,
                  bool enableInternalEnergy,
                  bool enableJouleThomson)
        : isothermalPvt_(isothermalPvt)
        , gasvisctCurves_(gasvisctCurves)
        , gasdentRefTemp_(gasdentRefTemp)
        , gasdentCT1_(gasdentCT1)
        , gasdentCT2_(gasdentCT2)
        , internalEnergyCurves_(internalEnergyCurves)
        , enableThermalDensity_(enableThermalDensity)
        , enableThermalViscosity_(enableThermalViscosity)
        , enableInternalEnergy_(enableInternalEnergy)
        , enableJouleThomson_(enableJouleThomson)
    { }

    GasPvtThermal(const GasPvtThermal& data)
    { *this = data; }

    ~GasPvtThermal()
    { delete isothermalPvt_; }

#if HAVE_ECL_INPUT
    /*!
     * \brief Implement the temperature part of the gas PVT properties.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule)
    {
        //////
        // initialize the isothermal part
        //////
        isothermalPvt_ = new IsothermalPvt;
        isothermalPvt_->initFromState(eclState, schedule);

        //////
        // initialize the thermal part
        //////
        const auto& tables = eclState.getTableManager();

        enableThermalDensity_ = tables.GasDenT().size() > 0;
        enableThermalViscosity_ = tables.hasTables("GASVISCT");
        enableInternalEnergy_ = tables.hasTables("SPECHEAT");
        enableJouleThomson_ = tables.hasTables("JOULETHOMSON");

        unsigned numRegions = isothermalPvt_->numRegions();
        setNumRegions(numRegions);

        // viscosity
        if (enableThermalViscosity_) {
            const auto& gasvisctTables = tables.getGasvisctTables();
            auto gasCompIdx = tables.gas_comp_index();
            std::string gasvisctColumnName = "Viscosity" + std::to_string(static_cast<long long>(gasCompIdx));

            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& T = gasvisctTables[regionIdx].getColumn("Temperature").vectorCopy();
                const auto& mu = gasvisctTables[regionIdx].getColumn(gasvisctColumnName).vectorCopy();
                gasvisctCurves_[regionIdx].setXYContainers(T, mu);
            }
        }

        // temperature dependence of gas density
        if (tables.GasDenT().size() > 0) {
            const auto& gasDenT = tables.GasDenT();

            assert(gasDenT.size() == numRegions);
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& record = gasDenT[regionIdx];

                gasdentRefTemp_[regionIdx] = record.T0;
                gasdentCT1_[regionIdx] = record.C1;
                gasdentCT2_[regionIdx] = record.C2;
            }
            enableThermalDensity_ = true;
        }
        // temperature dependence of gas density
        if (enableJouleThomson_) {
            //const auto& joulethomsonTable = tables.joulethomsonTable();

            // assert(gasDenT.size() == numRegions);
            // for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
            //     const auto& record = gasDenT[regionIdx];

            //     gasdentRefTemp_[regionIdx] = record.T0;
            //     gasdentCT1_[regionIdx] = record.C1;
            //     gasdentCT2_[regionIdx] = record.C2;
            // }
            // enableThermalDensity_ = true;
        }

        if (enableInternalEnergy_) {
            // the specific internal energy of gas. be aware that ecl only specifies the heat capacity
            // (via the SPECHEAT keyword) and we need to integrate it ourselfs to get the
            // internal energy
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& specHeatTable = tables.getSpecheatTables()[regionIdx];
                const auto& temperatureColumn = specHeatTable.getColumn("TEMPERATURE");
                const auto& cvGasColumn = specHeatTable.getColumn("CV_GAS");

                std::vector<double> uSamples(temperatureColumn.size());

                // the specific enthalpy of vaporization. since ECL does not seem to
                // feature a proper way to specify this quantity, we use the value for
                // methane. A proper model would also need to consider the enthalpy
                // change due to dissolution, i.e. the enthalpies of the gas and oil
                // phases should depend on the phase composition
                const Scalar hVap = 480.6e3; // [J / kg]

                Scalar u = temperatureColumn[0]*cvGasColumn[0] + hVap;
                for (size_t i = 0;; ++i) {
                    uSamples[i] = u;

                    if (i >= temperatureColumn.size() - 1)
                        break;

                    // integrate to the heat capacity from the current sampling point to the next
                    // one. this leads to a quadratic polynomial.
                    Scalar c_v0 = cvGasColumn[i];
                    Scalar c_v1 = cvGasColumn[i + 1];
                    Scalar T0 = temperatureColumn[i];
                    Scalar T1 = temperatureColumn[i + 1];
                    u += 0.5*(c_v0 + c_v1)*(T1 - T0);
                }

                internalEnergyCurves_[regionIdx].setXYContainers(temperatureColumn.vectorCopy(), uSamples);
            }
        }
    }
#endif // HAVE_ECL_INPUT

    /*!
     * \brief Set the number of PVT-regions considered by this object.
     */
    void setNumRegions(size_t numRegions)
    {
        gasvisctCurves_.resize(numRegions);
        internalEnergyCurves_.resize(numRegions);
        gasdentRefTemp_.resize(numRegions);
        gasdentCT1_.resize(numRegions);
        gasdentCT2_.resize(numRegions);
        joulethomsonRefPres_.resize(numRegions);
        joulethomsonGas_.resize(numRegions);
    }

    /*!
     * \brief Finish initializing the thermal part of the gas phase PVT properties.
     */
    void initEnd()
    { }

    size_t numRegions() const
    { return gasvisctCurves_.size(); }

    /*!
     * \brief Returns true iff the density of the gas phase is temperature dependent.
     */
    bool enableThermalDensity() const
    { return enableThermalDensity_; }

    /*!
     * \brief Returns true iff the viscosity of the gas phase is temperature dependent.
     */
    bool enableThermalViscosity() const
    { return enableThermalViscosity_; }

    /*!
     * \brief Returns the specific internal energy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy_JT(unsigned regionIdx,
                              const Evaluation& temperature,
                              const Evaluation& pressure,
                              const Evaluation& Rv) const
    {
        if (!enableInternalEnergy_)
             throw std::runtime_error("Requested the internal energy of gas but it is disabled");

        bool JouleThomson = 1 ; //joulethomsonGas_[regionIdx];

         
        Evaluation Tref = gasdentRefTemp_[regionIdx];// 399.15;
        Evaluation Pref = joulethomsonRefPres_[regionIdx]; //20E5;
  
        Evaluation bref = inverseFormationVolumeFactor(regionIdx, Tref, pressure, Rv);
        Evaluation b = inverseFormationVolumeFactor(regionIdx, temperature, pressure, Rv);
        Evaluation intEn = internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
        Evaluation Cp = (intEn- 480.6E3)/temperature;
       
        Evaluation rho = b * gasReferenceDensity(regionIdx);
        
        Scalar c1T = gasdentCT1_[regionIdx];
        Scalar c2T = gasdentCT2_[regionIdx];

        Evaluation alpha = (c1T + 2*c2T*(temperature - Tref))/
             (1 + c1T*(temperature - Tref) + c2T*(temperature- Tref)*(temperature - Tref));

        Evaluation beta =  bref.derivative(1)/bref; //derivative wrt to p
                
        //Cp = Cv + alpha*alpha/(beta*rho);
        const int N = 1000;
        Evaluation deltaP = (pressure - Pref)/N;
        Evaluation Hp_old = 0;
        Evaluation Hp, JT;

        for (size_t i = 0; i < N; ++i) {
            Evaluation p_new = Pref + i * deltaP;
           
            Evaluation alpha_new = (c1T + 2*c2T*(temperature- Tref))/(1 + c1T*(temperature - Tref) + c2T*(temperature - Tref)*(temperature - Tref));
            // Evaluation rho_new = inverseFormationVolumeFactor(regionIdx, temperature, p_new, Rv) * gasReferenceDensity(regionIdx);
            //JT = -(1.0/Cp)*(1.0 - alpha_new.value() * temperature.value())/rho_new.value();
            Evaluation rho_new = inverseFormationVolumeFactor(regionIdx, temperature, p_new, Rv) * gasReferenceDensity(regionIdx);
            JT = -(1.0/Cp)*(1.0 - alpha_new.value() * temperature.value())/rho_new.value();

            Evaluation dHp = -Cp * JT * deltaP;
            Hp = Hp_old + dHp; 
            Hp_old = Hp;
        }
        std::cout << "P: "<< pressure << " T: " << temperature << " JT: "<< JT << " Hp: " << Hp << "\n"<<std::flush; 

        // compute the specific internal energy for the specified tempature. We use linear
        // interpolation here despite the fact that the underlying heat capacities are
        // piecewise linear (which leads to a quadratic function)
       

        Evaluation a1 = alpha * pressure/rho;
        Evaluation a2 = (beta * pressure -alpha * temperature)/rho;
        return Cp * (temperature-Tref) + Hp - pressure/rho;
       
    }

    template <class Evaluation>
    Evaluation internalEnergy(unsigned regionIdx,
                              const Evaluation& temperature,
                              const Evaluation&,
                              const Evaluation&) const
    {
        if (!enableInternalEnergy_)
            throw std::runtime_error("Requested the internal energy of gas but it is disabled");

        // compute the specific internal energy for the specified tempature. We use linear
        // interpolation here despite the fact that the underlying heat capacities are
        // piecewise linear (which leads to a quadratic function)
        return internalEnergyCurves_[regionIdx].eval(temperature, /*extrapolate=*/true);
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
        if (!enableThermalViscosity())
            return isothermalPvt_->viscosity(regionIdx, temperature, pressure, Rv, Rvw);

        // compute the viscosity deviation due to temperature
        const auto& muGasvisct = gasvisctCurves_[regionIdx].eval(temperature);
        return muGasvisct;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the oil-saturated gas phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        if (!enableThermalViscosity())
            return isothermalPvt_->saturatedViscosity(regionIdx, temperature, pressure);

        // compute the viscosity deviation due to temperature
        const auto& muGasvisct = gasvisctCurves_[regionIdx].eval(temperature, true);
        return muGasvisct;
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

    const std::vector<TabulatedOneDFunction>& gasvisctCurves() const
    { return gasvisctCurves_; }

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

    bool enableJouleThomson() const
    { return enableJouleThomson_; }

    const std::vector<Scalar>& joulethomsonRefPres() const
    { return  joulethomsonRefPres_; }

    const std::vector<Scalar>&  joulethomsonGas() const
    { return joulethomsonGas_; }

    bool operator==(const GasPvtThermal<Scalar>& data) const
    {
        if (isothermalPvt_ && !data.isothermalPvt_)
            return false;
        if (!isothermalPvt_ && data.isothermalPvt_)
            return false;

        return (!this->isoThermalPvt() ||
                (*this->isoThermalPvt() == *data.isoThermalPvt())) &&
                this->gasvisctCurves() == data.gasvisctCurves() &&
                this->gasdentRefTemp() == data.gasdentRefTemp() &&
                this->gasdentCT1() == data.gasdentCT1() &&
                this->gasdentCT2() == data.gasdentCT2() &&
                this->internalEnergyCurves() == data.internalEnergyCurves() &&
                this->enableThermalDensity() == data.enableThermalDensity() &&
                this->enableThermalViscosity() == data.enableThermalViscosity() &&
                this->enableInternalEnergy() == data.enableInternalEnergy();
                this->enableJouleThomson() == data.enableJouleThomson();
    }

    GasPvtThermal<Scalar>& operator=(const GasPvtThermal<Scalar>& data)
    {
        if (data.isothermalPvt_)
            isothermalPvt_ = new IsothermalPvt(*data.isothermalPvt_);
        else
            isothermalPvt_ = nullptr;
        gasvisctCurves_ = data.gasvisctCurves_;
        gasdentRefTemp_ = data.gasdentRefTemp_;
        gasdentCT1_ = data.gasdentCT1_;
        gasdentCT2_ = data.gasdentCT2_;
        internalEnergyCurves_ = data.internalEnergyCurves_;
        enableThermalDensity_ = data.enableThermalDensity_;
        enableThermalViscosity_ = data.enableThermalViscosity_;
        enableInternalEnergy_ = data.enableInternalEnergy_;
        enableJouleThomson_ = data.enableJouleThomson_;

        return *this;
    }

private:
    IsothermalPvt* isothermalPvt_;

    // The PVT properties needed for temperature dependence of the viscosity. We need
    // to store one value per PVT region.
    std::vector<TabulatedOneDFunction> gasvisctCurves_;

    std::vector<Scalar> gasdentRefTemp_;
    std::vector<Scalar> gasdentCT1_;
    std::vector<Scalar> gasdentCT2_;

    std::vector<Scalar> joulethomsonRefPres_;
    std::vector<int> joulethomsonGas_;

    // piecewise linear curve representing the internal energy of gas
    std::vector<TabulatedOneDFunction> internalEnergyCurves_;

    bool enableThermalDensity_;
    bool enableThermalViscosity_;
    bool enableInternalEnergy_;
    bool enableJouleThomson_;
};

} // namespace Opm

#endif
