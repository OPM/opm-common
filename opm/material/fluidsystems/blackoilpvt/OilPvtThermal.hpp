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

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/common/Tabulated1DFunction.hpp>

#include <opm/material/fluidsystems/blackoilpvt/BrineCo2Pvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/ConstantCompressibilityOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/DeadOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/LiveOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/PvtEnums.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

namespace Opm {

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
    using IsothermalPvt = std::variant<std::monostate,
                                       LiveOilPvt<Scalar>,
                                       DeadOilPvt<Scalar>,
                                       ConstantCompressibilityOilPvt<Scalar>,
                                       BrineCo2Pvt<Scalar>>;

    static IsothermalPvt initialize(OilPvtApproach appr)
    {
        switch (appr) {
        case OilPvtApproach::LiveOil:
            return LiveOilPvt<Scalar>{};

        case OilPvtApproach::DeadOil:
            return DeadOilPvt<Scalar>{};

        case OilPvtApproach::ConstantCompressibilityOil:
            return ConstantCompressibilityOilPvt<Scalar>{};

        case OilPvtApproach::BrineCo2:
            return BrineCo2Pvt<Scalar>{};

        default:
            return std::monostate{};
        }
    }

#if HAVE_ECL_INPUT
    static OilPvtApproach chooseApproach(const EclipseState& eclState)
    {
        if (eclState.runspec().co2Storage())
            return OilPvtApproach::BrineCo2;
        else if (!eclState.getTableManager().getPvcdoTable().empty())
            return OilPvtApproach::ConstantCompressibilityOil;
        else if (eclState.getTableManager().hasTables("PVDO"))
            return OilPvtApproach::DeadOil;
        else if (!eclState.getTableManager().getPvtoTables().empty())
            return OilPvtApproach::LiveOil;

        return OilPvtApproach::NoOil;
    }

    /*!
     * \brief Implement the temperature part of the oil PVT properties.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule)
    {
        //////
        // initialize the isothermal part
        //////
        isothermalPvt_ = initialize(chooseApproach(eclState));
        std::visit(VisitorOverloadSet{[&](auto& pvt)
                                      {
                                          pvt.initFromState(eclState, schedule);
                                      }, monoHandler_}, isothermalPvt_);

        //////
        // initialize the thermal part
        //////
        const auto& tables = eclState.getTableManager();

        enableThermalDensity_ = tables.OilDenT().size() > 0;
        enableThermalViscosity_ = tables.hasTables("OILVISCT");
        enableInternalEnergy_ = tables.hasTables("SPECHEAT");

        unsigned numRegions;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          numRegions = pvt.numRegions();
                                      }, monoHandler_}, isothermalPvt_);
        setNumRegions(numRegions);

        // viscosity
        if (enableThermalViscosity_) {
            if (tables.getViscrefTable().empty())
                throw std::runtime_error("VISCREF is required when OILVISCT is present");

            const auto& oilvisctTables = tables.getOilvisctTables();
            const auto& viscrefTable = tables.getViscrefTable();

            assert(oilvisctTables.size() == numRegions);
            assert(viscrefTable.size() == numRegions);

            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& TCol = oilvisctTables[regionIdx].getColumn("Temperature").vectorCopy();
                const auto& muCol = oilvisctTables[regionIdx].getColumn("Viscosity").vectorCopy();
                oilvisctCurves_[regionIdx].setXYContainers(TCol, muCol);

                viscrefPress_[regionIdx] = viscrefTable[regionIdx].reference_pressure;
                viscrefRs_[regionIdx] = viscrefTable[regionIdx].reference_rs;

                // temperature used to calculate the reference viscosity [K]. the
                // value does not really matter if the underlying PVT object really
                // is isothermal...
                constexpr const Scalar Tref = 273.15 + 20;

                // compute the reference viscosity using the isothermal PVT object.
                std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                              {
                                                  viscRef_[regionIdx] = pvt.viscosity(regionIdx,
                                                                                      Tref,
                                                                                      viscrefPress_[regionIdx],
                                                                                      viscrefRs_[regionIdx]);
                                              }, monoHandler_}, isothermalPvt_);
            }
        }

        // temperature dependence of oil density
        const auto& oilDenT = tables.OilDenT();
        if (oilDenT.size() > 0) {
            assert(oilDenT.size() == numRegions);
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& record = oilDenT[regionIdx];

                oildentRefTemp_[regionIdx] = record.T0;
                oildentCT1_[regionIdx] = record.C1;
                oildentCT2_[regionIdx] = record.C2;
            }
        }

        // Joule Thomson 
        if (enableJouleThomson_) {
            const auto& oilJT = tables.OilJT();

            assert(oilJT.size() == numRegions);
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& record = oilJT[regionIdx];

                oilJTRefPres_[regionIdx] =  record.P0;
                oilJTC_[regionIdx] = record.C1;
            }

            const auto& densityTable = eclState.getTableManager().getDensityTable();

            assert(densityTable.size() == numRegions);
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
                 rhoRefG_[regionIdx] = densityTable[regionIdx].gas;
            }
        }

        if (enableInternalEnergy_) {
            // the specific internal energy of liquid oil. be aware that ecl only specifies the
            // heat capacity (via the SPECHEAT keyword) and we need to integrate it
            // ourselfs to get the internal energy
            for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
                const auto& specheatTable = tables.getSpecheatTables()[regionIdx];
                const auto& temperatureColumn = specheatTable.getColumn("TEMPERATURE");
                const auto& cvOilColumn = specheatTable.getColumn("CV_OIL");

                std::vector<double> uSamples(temperatureColumn.size());

                Scalar u = temperatureColumn[0]*cvOilColumn[0];
                for (size_t i = 0;; ++i) {
                    uSamples[i] = u;

                    if (i >= temperatureColumn.size() - 1)
                        break;

                    // integrate to the heat capacity from the current sampling point to the next
                    // one. this leads to a quadratic polynomial.
                    Scalar c_v0 = cvOilColumn[i];
                    Scalar c_v1 = cvOilColumn[i + 1];
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
    bool enableJouleThomsony() const
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
        Evaluation isothermalMu;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          isothermalMu = pvt.viscosity(regionIdx,
                                                                       temperature,
                                                                        pressure,
                                                                        Rs);
                                      }, monoHandler_}, isothermalPvt_);
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
        Evaluation isothermalMu;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          isothermalMu = pvt.saturatedViscosity(regionIdx,
                                                                                temperature,
                                                                                pressure);
                                      }, monoHandler_}, isothermalPvt_);
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
        Evaluation b;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          b = pvt.inverseFormationVolumeFactor(regionIdx,
                                                                               temperature,
                                                                               pressure,
                                                                               Rs);
                                      }, monoHandler_}, isothermalPvt_);

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
        Evaluation b;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          b = pvt.saturatedInverseFormationVolumeFactor(regionIdx,
                                                                                        temperature,
                                                                                        pressure);
                                      }, monoHandler_}, isothermalPvt_);

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
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedGasDissolutionFactor(regionIdx,
                                                                                     temperature,
                                                                                     pressure);
                                      }, monoHandler_}, isothermalPvt_);

        return result;
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
                                             const Evaluation& pressure,
                                             const Evaluation& oilSaturation,
                                             const Evaluation& maxOilSaturation) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturatedGasDissolutionFactor(regionIdx,
                                                                                     temperature,
                                                                                     pressure,
                                                                                     oilSaturation,
                                                                                     maxOilSaturation);
                                      }, monoHandler_}, isothermalPvt_);

        return result;
    }

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
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.saturationPressure(regionIdx,
                                                                          temperature,
                                                                          pressure);
                                      }, monoHandler_}, isothermalPvt_);

        return result;
    }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned compIdx) const
    {
        Evaluation result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.diffusionCoefficient(temperature,
                                                                            pressure,
                                                                            compIdx);
                                      }, monoHandler_}, isothermalPvt_);

        return result;
    }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    {
        Scalar result;
        std::visit(VisitorOverloadSet{[&](const auto& pvt)
                                      {
                                          result = pvt.oilReferenceDensity(regionIdx);
                                      }, monoHandler_}, isothermalPvt_);

        return result;
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
    { return oilJTRefPres_; }

     const std::vector<Scalar>&  oilJTC() const
    { return oilJTC_; }

private:
    IsothermalPvt isothermalPvt_;

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

    // piecewise linear curve representing the internal energy of oil
    std::vector<TabulatedOneDFunction> internalEnergyCurves_;

    bool enableThermalDensity_ = false;
    bool enableJouleThomson_ = false;
    bool enableThermalViscosity_ = false;
    bool enableInternalEnergy_ = false;

    MonoThrowHandler<std::logic_error>
    monoHandler_{"Not implemented: Oil PVT of this deck!"}; // mono state handler
};

} // namespace Opm

#endif
