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

#include <opm/common/utility/VectorWithDefaultAllocator.hpp>

#include <opm/material/Constants.hpp>
#include <opm/common/TimingMacros.hpp>
#include <opm/common/ErrorMacros.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>

#include <opm/material/components/CO2.hpp>
#include <opm/material/components/BrineDynamic.hpp>
#include <opm/material/components/SimpleHuDuanH2O.hpp>
#include <opm/material/common/UniformTabulated2DFunction.hpp>
#include <opm/material/binarycoefficients/Brine_CO2.hpp>
#include <opm/input/eclipse/EclipseState/Co2StoreConfig.hpp>
#include <opm/material/components/CO2Tables.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <cstddef>
#include <vector>

namespace Opm {

class EclipseState;
class Schedule;
class Co2StoreConfig;

// forward declaration of the class so the function in the next namespace can be declared
template <class Scalar, template<class> class Storage>
class Co2GasPvt;

#if HAVE_CUDA
// declaration of make_view in correct namespace so friend function can be declared in the class
namespace gpuistl {
    template <class Scalar>
    Co2GasPvt<Scalar, GpuView>
    make_view(Co2GasPvt<Scalar, GpuVector>&);
} // namespace gpuistl
#endif // HAVE_CUDA
/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 *        for CO2.
 */
template <class Scalar, template<class> class Storage = VectorWithDefaultAllocator>
class Co2GasPvt
{
    using Params = Opm::CO2Tables<double, Storage>;
    using CO2 = ::Opm::CO2<Scalar, Params>;
    using H2O = SimpleHuDuanH2O<Scalar>;
    using Brine = ::Opm::BrineDynamic<Scalar, H2O>;
    using ContainerT = Storage<Scalar>;
    static constexpr bool extrapolate = true;

public:
    //! The binary coefficients for brine and CO2 used by this fluid system
    using BinaryCoeffBrineCO2 = BinaryCoeff::Brine_CO2<Scalar, H2O, CO2>;

    Co2GasPvt() = default;

    explicit Co2GasPvt(const ContainerT& salinity,
                       int activityModel = 3,
                       int thermalMixingModel = 1,
                       Scalar T_ref = 288.71, //(273.15 + 15.56)
                       Scalar P_ref = 101325);

    Co2GasPvt(const Params& params,
              const ContainerT& brineReferenceDensity,
              const ContainerT& gasReferenceDensity,
              const ContainerT& salinity,
              bool enableEzrokhiDensity,
              bool enableVaporization,
              int activityModel,
              Co2StoreConfig::GasMixingType gastype)
        : brineReferenceDensity_(brineReferenceDensity)
        , gasReferenceDensity_(gasReferenceDensity)
        , salinity_(salinity)
        , enableEzrokhiDensity_(enableEzrokhiDensity)
        , enableVaporization_(enableVaporization)
        , activityModel_(activityModel)
        , gastype_(gastype)
        , co2Tables(params)
{
    assert(enableEzrokhiDensity == false && "Ezrokhi density not supported by GPUs");
}

#if HAVE_ECL_INPUT
    void initFromState(const EclipseState& eclState, const Schedule&);
#endif

    void setNumRegions(std::size_t numRegions);

    OPM_HOST_DEVICE void setVapPars(const Scalar, const Scalar)
    {
    }


    OPM_HOST_DEVICE static constexpr bool isActive()
    {
        return true;
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    OPM_HOST_DEVICE void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefBrine,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/);

    /*!
     * \brief Specify whether the PVT model should consider that the water component can
     *        vaporize in the gas phase
     *
     * By default, vaporized water is considered.
     */
    OPM_HOST_DEVICE void setEnableVaporizationWater(bool yesno)
    { enableVaporization_ = yesno; }

    /*!
    * \brief Set activity coefficient model for salt in solubility model
    */
    OPM_HOST_DEVICE void setActivityModelSalt(int activityModel);

   /*!
    * \brief Set thermal mixing model for co2 in brine
    */
    OPM_HOST_DEVICE void setThermalMixingModel(int thermalMixingModel);

    /*!
     * \brief Finish initializing the co2 phase PVT properties.
     */
    OPM_HOST_DEVICE void initEnd()
    {
    }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    OPM_HOST_DEVICE Scalar hVap(unsigned ) const
    { return 0.0;  }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation internalEnergy(unsigned regionIdx,
                        const Evaluation& temperature,
                        const Evaluation& pressure,
                        const Evaluation& rv,
                        const Evaluation& rvw) const
    {
        OPM_TIMEBLOCK_LOCAL(internalEnergy, Subsystem::PvtProps);
        if (gastype_ == Co2StoreConfig::GasMixingType::NONE) {
            // use the gasInternalEnergy of CO2
            return CO2::gasInternalEnergy(co2Tables, temperature, pressure, extrapolate);
        }

        assert(gastype_ == Co2StoreConfig::GasMixingType::IDEAL);
        // account for H2O in the gas phase
        Evaluation result = 0;
        // The CO2STORE option both works for GAS/WATER and GAS/OIL systems
        // Either rv og rvw should be zero
        assert(rv == 0.0 || rvw == 0.0);
        const Evaluation xBrine = convertRvwToXgW_(max(rvw,rv),regionIdx);
        result += xBrine * H2O::gasInternalEnergy(temperature, pressure);
        result += (1 - xBrine) * CO2::gasInternalEnergy(co2Tables, temperature, pressure, extrapolate);
        return result;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase
     *        given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& temperature,
                         const Evaluation& pressure,
                         const Evaluation& /*Rv*/,
                         const Evaluation& /*Rvw*/) const
    { return saturatedViscosity(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of fluid phase at saturated conditions.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedViscosity(unsigned /*regionIdx*/,
                                  const Evaluation& temperature,
                                  const Evaluation& pressure) const
    {
        OPM_TIMEBLOCK_LOCAL(saturatedViscosity, Subsystem::PvtProps);
        // Neglects impact of vaporized water on the visosity
        return CO2::gasViscosity(co2Tables, temperature, pressure, extrapolate);
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& temperature,
                                            const Evaluation& pressure,
                                            const Evaluation& rv,
                                            const Evaluation& rvw) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        if (!enableVaporization_) {
            return CO2::gasDensity(co2Tables, temperature, pressure, extrapolate) /
                   gasReferenceDensity_[regionIdx];
        }

        // Use CO2 density for the gas phase.
        const auto& rhoCo2 = CO2::gasDensity(co2Tables, temperature, pressure, extrapolate);
        //const auto& rhoH2O = H2O::gasDensity(temperature, pressure);
        //The CO2STORE option both works for GAS/WATER and GAS/OIL systems
        //Either rv og rvw should be zero
        //assert(rv == 0.0 || rvw == 0.0);
        //const Evaluation xBrine = convertRvwToXgW_(max(rvw,rv),regionIdx);
        //const auto rho = 1.0/(xBrine/rhoH2O + (1.0 - xBrine)/rhoCo2);
        return rhoCo2 / (gasReferenceDensity_[regionIdx] +
               max(rvw,rv) * brineReferenceDensity_[regionIdx]);
    }

    /*!
     * \brief Returns the formation volume factor [-] and viscosity [Pa s] of the fluid phase.
     */
    template <class FluidState, class LhsEval = typename FluidState::Scalar>
    std::pair<LhsEval, LhsEval>
    inverseFormationVolumeFactorAndViscosity(const FluidState& fluidState, unsigned regionIdx)
    {
        const LhsEval& T = decay<LhsEval>(fluidState.temperature(FluidState::gasPhaseIdx));
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(FluidState::gasPhaseIdx));
        const LhsEval& Rv = decay<LhsEval>(fluidState.Rv());
        const LhsEval& Rvw = decay<LhsEval>(fluidState.Rvw());
        return { this->inverseFormationVolumeFactor(regionIdx, T, p, Rv, Rvw),
                 this->viscosity(regionIdx, T, p, Rv, Rvw) };
    }

    /*!
     * \brief Returns the formation volume factor [-] of water saturated gas at given pressure.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& temperature,
                                                     const Evaluation& pressure) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        const Evaluation rvw = rvwSat_(regionIdx, temperature, pressure,
                                       Evaluation(salinity_[regionIdx]));
        return inverseFormationVolumeFactor(regionIdx, temperature,
                                            pressure, Evaluation(0.0), rvw);
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the brine component
     *
     * \param Rvw The surface volume of brine component vaporized
     *            in what will yield one cubic meter of water at the surface [-]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturationPressure(unsigned /*regionIdx*/,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& /*Rvw*/) const
    { return 0.0; /* not implemented */ }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx])); }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water phase.
    */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& saltConcentration) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        const Evaluation salinity = salinityFromConcentration(temperature, pressure,
                                                              saltConcentration);
        return rvwSat_(regionIdx, temperature, pressure, salinity);
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure,
                                              const Evaluation& /*oilSaturation*/,
                                              const Evaluation& /*maxOilSaturation*/) const
    { return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx])); }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& temperature,
                                              const Evaluation& pressure) const
    { return rvwSat_(regionIdx, temperature, pressure, Evaluation(salinity_[regionIdx])); }

    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation diffusionCoefficient(const Evaluation& temperature,
                                    const Evaluation& pressure,
                                    unsigned /*compIdx*/) const
    {
        return BinaryCoeffBrineCO2::gasDiffCoeff(co2Tables, temperature, pressure, extrapolate);
    }

    OPM_HOST_DEVICE Scalar gasReferenceDensity(unsigned regionIdx) const
    {
        return gasReferenceDensity_[regionIdx];
    }

    OPM_HOST_DEVICE Scalar oilReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    OPM_HOST_DEVICE Scalar waterReferenceDensity(unsigned regionIdx) const
    { return brineReferenceDensity_[regionIdx]; }

    OPM_HOST_DEVICE Scalar salinity(unsigned regionIdx) const
    { return salinity_[regionIdx]; }

    void setEzrokhiDenCoeff(const std::vector<EzrokhiTable>& denaqa);

    // new get functions that will be needed to move a cpu based Co2GasPvt object to the GPU
    OPM_HOST_DEVICE const ContainerT& getBrineReferenceDensity() const
    { return brineReferenceDensity_; }

    OPM_HOST_DEVICE const ContainerT& getGasReferenceDensity() const
    { return gasReferenceDensity_; }

    OPM_HOST_DEVICE const ContainerT& getSalinity() const
    { return salinity_; }

    OPM_HOST_DEVICE bool getEnableEzrokhiDensity() const
    { return enableEzrokhiDensity_; }

    OPM_HOST_DEVICE bool getEnableVaporization() const
    { return enableVaporization_; }

    OPM_HOST_DEVICE int getActivityModel() const
    { return activityModel_; }

    OPM_HOST_DEVICE Co2StoreConfig::GasMixingType getGasType() const
    { return gastype_; }

    OPM_HOST_DEVICE const Params& getParams() const
    { return co2Tables; }

private:
    template <class LhsEval>
    LhsEval ezrokhiExponent_(const LhsEval& temperature,
                             const ContainerT& ezrokhiCoeff) const
    {
        const LhsEval& tempC = temperature - 273.15;
        return ezrokhiCoeff[0] + tempC * (ezrokhiCoeff[1] + ezrokhiCoeff[2] * tempC);
    }

    template <class LhsEval>
    OPM_HOST_DEVICE LhsEval rvwSat_(unsigned regionIdx,
                    const LhsEval& temperature,
                    const LhsEval& pressure,
                    const LhsEval& salinity) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        if (!enableVaporization_) {
            return 0.0;
        }

        // calulate the equilibrium composition for the given
        // temperature and pressure.
        LhsEval xgH2O;
        LhsEval xlCO2;
        BinaryCoeffBrineCO2::calculateMoleFractions(co2Tables,
                                                    temperature,
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
    OPM_HOST_DEVICE LhsEval convertXgWToRvw(const LhsEval& XgW, unsigned regionIdx) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        Scalar rho_wRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = gasReferenceDensity_[regionIdx];

        return XgW / (1.0 - XgW) * (rho_gRef / rho_wRef);
    }

    /*!
     * \brief Convert a water vaporization factor to the the corresponding mass fraction
     *        of the water component in the gas phase.
     */
    template <class LhsEval>
    OPM_HOST_DEVICE LhsEval convertRvwToXgW_(const LhsEval& Rvw, unsigned regionIdx) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        Scalar rho_wRef = brineReferenceDensity_[regionIdx];
        Scalar rho_gRef = gasReferenceDensity_[regionIdx];

        const LhsEval& rho_wG = Rvw * rho_wRef;
        return rho_wG / (rho_gRef + rho_wG);
    }
    /*!
     * \brief Convert a water mole fraction in the gas phase the corresponding mass fraction.
     */
    template <class LhsEval>
    OPM_HOST_DEVICE LhsEval convertxgWToXgW(const LhsEval& xgW, const LhsEval& salinity) const
    {
        OPM_TIMEFUNCTION_LOCAL(Subsystem::PvtProps);
        Scalar M_CO2 = CO2::molarMass();
        LhsEval M_Brine = Brine::molarMass(salinity);

        return xgW * M_Brine / (xgW * (M_Brine - M_CO2) + M_CO2);
    }

    #if HAVE_CUDA
    template <class ScalarT>
    friend Co2GasPvt<ScalarT, gpuistl::GpuView>
    gpuistl::make_view(Co2GasPvt<ScalarT, gpuistl::GpuBuffer>&);
    #endif // HAVE_CUDA

    template <class LhsEval>
    OPM_HOST_DEVICE const LhsEval salinityFromConcentration(const LhsEval&T, const LhsEval& P,
                                            const LhsEval& saltConcentration) const
    { return saltConcentration/H2O::liquidDensity(T, P, true); }

    ContainerT brineReferenceDensity_{};
    ContainerT gasReferenceDensity_{};
    ContainerT salinity_{};
    ContainerT ezrokhiDenNaClCoeff_{};
    bool enableEzrokhiDensity_ = false;
    bool enableVaporization_ = true;
    int activityModel_{};
    Co2StoreConfig::GasMixingType gastype_{};
    Params co2Tables;
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {
    template<class ScalarT>
    Co2GasPvt<ScalarT, GpuBuffer>
    copy_to_gpu(const Co2GasPvt<ScalarT>& cpuCo2)
    {
        return Co2GasPvt<ScalarT, GpuBuffer>(
            copy_to_gpu(cpuCo2.getParams()),
            GpuBuffer<ScalarT>(cpuCo2.getBrineReferenceDensity()),
            GpuBuffer<ScalarT>(cpuCo2.getGasReferenceDensity()),
            GpuBuffer<ScalarT>(cpuCo2.getSalinity()),
            cpuCo2.getEnableEzrokhiDensity(),
            cpuCo2.getEnableVaporization(),
            cpuCo2.getActivityModel(),
            cpuCo2.getGasType());
    }

    template <class ScalarT>
    Co2GasPvt<ScalarT, GpuView>
    make_view(Co2GasPvt<ScalarT, GpuBuffer>& co2GasPvt)
    {
        using ContainedType = ScalarT;

        auto newBrineReferenceDensity = make_view<ContainedType>(co2GasPvt.brineReferenceDensity_);
        auto newGasReferenceDensity = make_view<ContainedType>(co2GasPvt.gasReferenceDensity_);
        auto newSalinity = make_view<ContainedType>(co2GasPvt.salinity_);

        return Co2GasPvt<ScalarT, GpuView>(
            make_view(co2GasPvt.co2Tables),
            newBrineReferenceDensity,
            newGasReferenceDensity,
            newSalinity,
            co2GasPvt.getEnableEzrokhiDensity(),
            co2GasPvt.getEnableVaporization(),
            co2GasPvt.getActivityModel(),
            co2GasPvt.getGasType());
    }
} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif
