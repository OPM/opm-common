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
 * \copydoc Opm::WetGasPvt
 */
#ifndef OPM_WET_GAS_PVT_HPP
#define OPM_WET_GAS_PVT_HPP

#include <opm/common/Exceptions.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>

#include <opm/material/common/ConditionalStorage.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/UniformXTabulated2DFunctionBuilder.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace Opm {

class EclipseState;
class Schedule;
class SimpleTable;

// Forward declaration, so that copy_to_gpu() and make_view() can be declared before
// the class definition. The default template arguments are given here (and only here).
template <class Scalar,
          template<class> class Storage = VectorWithDefaultAllocator,
          template<class> class TableStorage = VectorWithDefaultAllocator>
class WetGasPvt;

namespace gpuistl
{
template <class T>
class GpuBuffer;

template <class T>
class GpuView;
} // namespace gpuistl

#if HAVE_CUDA
// Declarations of copy_to_gpu() and make_view(), so that they can be declared as
// friends in the class.
namespace gpuistl {
    template <class ScalarT>
    WetGasPvt<ScalarT, GpuBuffer, GpuView>
    copy_to_gpu(const WetGasPvt<ScalarT>&);

    template <class ScalarT>
    WetGasPvt<ScalarT, GpuView, GpuView>
    make_view(WetGasPvt<ScalarT, GpuBuffer, GpuView>&);
} // namespace gpuistl
#endif // HAVE_CUDA

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 *        with vaporized oil.
 *
 * The class is used with four combinations of \c Storage (the per-region arrays) and
 * \c TableStorage (the data inside each table):
 *
 * - \c VectorWithDefaultAllocator, \c VectorWithDefaultAllocator: the CPU object, set up
 *   by initFromState(), or by setNumRegions(), the setters and initEnd().
 * - \c VectorWithDefaultAllocator, \c gpuistl::GpuBuffer: the device table owner, holding
 *   one owning device table per region. It is created by and internal to
 *   gpuistl::copy_to_gpu(), and owned by the owning GPU object.
 * - \c gpuistl::GpuBuffer, \c gpuistl::GpuView: the owning GPU object, returned by
 *   gpuistl::copy_to_gpu(). It holds device arrays of views into the device table
 *   owner's tables, and it is move-only.
 * - \c gpuistl::GpuView, \c gpuistl::GpuView: the GPU view, returned by
 *   gpuistl::make_view(). It owns no memory, is trivially copyable, and is passed to
 *   kernels by value.
 *
 * \code
 * auto gpuPvt = Opm::gpuistl::copy_to_gpu(cpuPvt);  // owning GPU object
 * auto gpuView = Opm::gpuistl::make_view(gpuPvt);   // GPU view
 * kernel<<<...>>>(gpuView);
 * \endcode
 *
 * The owning GPU object must outlive every kernel that uses a view made from it. Moving
 * it does not invalidate such views.
 */
template <class Scalar,
          template<class> class Storage,
          template<class> class TableStorage>
class WetGasPvt
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedTwoDFunction = UniformXTabulated2DFunction<Scalar, TableStorage>;
    using TabulatedTwoDFunctionBuilder = UniformXTabulated2DFunctionBuilder<Scalar>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar, TableStorage>;

    //! True for the CPU object.
    static constexpr bool isCpuStorage
        = std::is_same_v<Storage<Scalar>, VectorWithDefaultAllocator<Scalar>>
       && std::is_same_v<TableStorage<Scalar>, VectorWithDefaultAllocator<Scalar>>;

    //! True for the host-side owner of the per-region device tables. Only created by,
    //! and owned through, gpuistl::copy_to_gpu().
    static constexpr bool isGpuTableOwner
        = std::is_same_v<Storage<Scalar>, VectorWithDefaultAllocator<Scalar>>
       && std::is_same_v<TableStorage<Scalar>, gpuistl::GpuBuffer<Scalar>>;

    //! True if this object owns device memory (created by gpuistl::copy_to_gpu()).
    static constexpr bool isOwningGpu
        = std::is_same_v<Storage<Scalar>, gpuistl::GpuBuffer<Scalar>>
       && std::is_same_v<TableStorage<Scalar>, gpuistl::GpuView<Scalar>>;

    //! True if this object is a non-owning view of device memory (created by gpuistl::make_view()).
    static constexpr bool isGpuView
        = std::is_same_v<Storage<Scalar>, gpuistl::GpuView<Scalar>>
       && std::is_same_v<TableStorage<Scalar>, gpuistl::GpuView<Scalar>>;

    static_assert(isCpuStorage || isGpuTableOwner || isOwningGpu || isGpuView,
                  "WetGasPvt: unsupported combination of Storage and TableStorage");

    WetGasPvt() = default;

    /*!
     * \brief Initialize the parameters for wet gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVTG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);

private:
    void extendPvtgTable_(TabulatedTwoDFunctionBuilder& invGasB,
                          TabulatedTwoDFunctionBuilder& gasmu,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable);

public:
    void setNumRegions(std::size_t numRegions);

    void setVapPars(const Scalar par1, const Scalar)
    {
        vapPar1_ = par1;
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/);

    /*!
     * \brief Initialize the function for the oil vaporization factor \f$R_v\f$
     *
     * \param regionIdx Region index to use
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedGasOilVaporizationFactor(unsigned regionIdx,
                                              const SamplingPoints& samplePoints)
    { saturatedOilVaporizationFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }

    /*!
     * \brief Initialize the function for the gas formation volume factor
     *
     * The gas formation volume factor \f$B_g\f$ is a function of \f$(p_g, X_g^O)\f$ and
     * represents the partial density of the oil component in the gas phase at a given
     * pressure. This method only requires the volume factor of oil-saturated gas (which
     * only depends on pressure) while the dependence on the oil mass fraction is
     * guesstimated...
     */
    void setSaturatedGasFormationVolumeFactor(unsigned regionIdx,
                                              const SamplingPoints& samplePoints);

    /*!
     * \brief Initialize the function for the gas formation volume factor
     *
     * The gas formation volume factor \f$B_g\f$ is a function of \f$(p_g, X_g^O)\f$ and
     * represents the partial density of the oil component in the gas phase at a given
     * pressure.
     *
     * This method sets \f$1/B_g(R_v, p_g)\f$. Note that instead of the mass fraction of
     * the oil component in the gas phase, this function depends on the gas dissolution
     * factor. Also note, that the order of the arguments needs to be \f$(R_s, p_o)\f$
     * and not the other way around.
     */
    void setInverseGasFormationVolumeFactor(unsigned regionIdx,
                                            const TabulatedTwoDFunction& invBg)
    { inverseGasB_[regionIdx] = invBg; }

    /*!
     * \brief Initialize the viscosity of the gas phase.
     *
     * This is a function of \f$(R_s, p_o)\f$...
     */
    void setGasViscosity(unsigned regionIdx, const TabulatedTwoDFunction& mug)
    { gasMu_[regionIdx] = mug; }

    /*!
     * \brief Initialize the phase viscosity for oil saturated gas
     *
     * The gas viscosity is a function of \f$(p_g, X_g^O)\f$, but this method only
     * requires the viscosity of oil-saturated gas (which only depends on pressure) while
     * there is assumed to be no dependence on the gas mass fraction...
     */
    void setSaturatedGasViscosity(unsigned regionIdx,
                                  const SamplingPoints& samplePoints);

    /*!
     * \brief Finish initializing the gas phase PVT properties.
     */
    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned,
                              const Evaluation&,
                              const Evaluation&,
                              const Evaluation&,
                              const Evaluation&) const
    {
        throw std::runtime_error("Requested the enthalpy of gas but the thermal "
                                 "option is not enabled");
    }

    Scalar hVap(unsigned) const
    {
        throw std::runtime_error("Requested the hvap of oil but the thermal "
                                 "option is not enabled");
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& /*temperature*/,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& /*Rvw*/) const
    {
        const Evaluation& invBg = inverseGasB_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseGasBMu_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);

        return invBg / invMugBg;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& pressure) const
    {
        const Evaluation& invBg = inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseSaturatedGasBMu_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBg / invMugBg;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& /*temperature*/,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& /*Rvw*/) const
    { return inverseGasB_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true); }

    /*!
     * \brief Returns the formation volume factor [-] and viscosity [Pa s] of the fluid phase.
     */
    template <class FluidState, class LhsEval = typename FluidState::ValueType>
    std::pair<LhsEval, LhsEval>
    inverseFormationVolumeFactorAndViscosity(const FluidState& fluidState, unsigned regionIdx)
    {
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(FluidState::gasPhaseIdx));
        const LhsEval& Rv = decay<LhsEval>(fluidState.Rv());

        const auto satSegIdx = this->saturatedOilVaporizationFactorTable_[regionIdx].findSegmentIndex(p, /*extrapolate=*/ true);
        const auto RvSat = this->saturatedOilVaporizationFactorTable_[regionIdx].eval(p, SegmentIndex{satSegIdx});
        const bool useSaturatedTables = (fluidState.saturation(FluidState::oilPhaseIdx) > 0.0) && (Rv >= (1.0 - 1e-10) * RvSat);

        if (useSaturatedTables) {
            const LhsEval b = this->inverseSaturatedGasB_[regionIdx].eval(p, SegmentIndex{satSegIdx});
            const LhsEval invBMu = this->inverseSaturatedGasBMu_[regionIdx].eval(p, SegmentIndex{satSegIdx});
            const LhsEval mu = b / invBMu;
            return { b, mu };
        } else {
            unsigned ii, jj1, jj2;
            LhsEval alpha, beta1, beta2;
            this->inverseGasB_[regionIdx].findPoints(ii, jj1, jj2, alpha, beta1, beta2, p, Rv, /*extrapolate =*/ true);
            const LhsEval b = this->inverseGasB_[regionIdx].eval(ii, jj1, jj2, alpha, beta1, beta2);
            const LhsEval invBMu = this->inverseGasBMu_[regionIdx].eval(ii, jj1, jj2, alpha, beta1, beta2);
            const LhsEval mu = b / invBMu;
            return { b, mu };
        }
    }

    /*!
     * \brief Returns the formation volume factor [-] of oil saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& /*temperature*/,
                                                     const Evaluation& pressure) const
    { return inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the gasphase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned /*regionIdx*/,
                                                const Evaluation& /*temperature*/,
                                                const Evaluation& /*pressure*/) const
    { return 0.0; /* this is non-humid gas! */ }

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
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure) const
    {
        return saturatedOilVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the gas phase.
     *
     * This variant of the method prevents all the oil to be vaporized even if the gas
     * phase is still not saturated. This is physically quite dubious but it corresponds
     * to how the Eclipse 100 simulator handles this. (cf the VAPPARS keyword.)
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              Evaluation maxOilSaturation) const
    {
        Evaluation tmp =
            saturatedOilVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        // apply the vaporization parameters for the gas phase (cf. the Eclipse VAPPARS
        // keyword)
        maxOilSaturation = min(maxOilSaturation, Scalar(1.0));
        if (vapPar1_ > 0.0 && maxOilSaturation > 0.01 && oilSaturation < maxOilSaturation) {
            constexpr const Scalar eps = 0.001;
            const Evaluation& So = max(oilSaturation, eps);
            tmp *= max(1e-3, pow(So / maxOilSaturation, vapPar1_));
        }

        return tmp;
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the oil component
     *
     * This method uses the standard blackoil assumptions: This means that the Rv value
     * does not depend on the saturation of oil. (cf. the Eclipse VAPPARS keyword.)
     *
     * \param regionIdx Region index to use
     * \param Rv The surface volume of oil component dissolved in what will yield one
     *           cubic meter of gas at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation&,
                                  const Evaluation& Rv) const
    {
        using Toolbox = MathToolbox<Evaluation>;

        const auto& RvTable = saturatedOilVaporizationFactorTable_[regionIdx];
        constexpr const Scalar eps = std::numeric_limits<typename Toolbox::Scalar>::epsilon() * 1e6;

        // use the tabulated saturation pressure function to get a pretty good initial value
        Evaluation pSat = saturationPressure_[regionIdx].eval(Rv, /*extrapolate=*/true);

        // Newton method to do the remaining work. If the initial
        // value is good, this should only take two to three
        // iterations...
        bool onProbation = false;
        for (unsigned i = 0; i < 20; ++i) {
            const Evaluation& f = RvTable.eval(pSat, /*extrapolate=*/true) - Rv;
            const Evaluation& fPrime = RvTable.evalDerivative(pSat, /*extrapolate=*/true);

            // If the derivative is "zero" Newton will not converge,
            // so simply return our initial guess.
            if (std::abs(scalarValue(fPrime)) < 1.0e-30) {
                return pSat;
            }

            const Evaluation& delta = f / fPrime;

            pSat -= delta;

            if (pSat < 0.0) {
                // if the pressure is lower than 0 Pascals, we set it back to 0. if this
                // happens twice, we give up and just return 0 Pa...
                if (onProbation) {
                    return 0.0;
                }

                onProbation = true;
                pSat = 0.0;
            }

            if (std::abs(scalarValue(delta)) < std::abs(scalarValue(pSat))*eps) {
                return pSat;
            }
        }

        const std::string msg =
            "Finding saturation pressure did not converge: "
            "pSat = " + std::to_string(getValue(pSat)) +
            ", Rv = " + std::to_string(getValue(Rv));
        OpmLog::debug("Wet gas saturation pressure", msg);
        throw NumericalProblem(msg);
    }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                    const Evaluation& /*pressure*/,
                                    unsigned /*compIdx*/) const
    {
        throw std::runtime_error("Not implemented: The PVT model does not provide "
                                 "a diffusionCoefficient()");
    }

    Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    Scalar oilReferenceDensity(unsigned regionIdx) const
    { return oilReferenceDensity_[regionIdx]; }

    const Storage<TabulatedTwoDFunction>& inverseGasB() const
    { return inverseGasB_; }

    const Storage<TabulatedOneDFunction>& inverseSaturatedGasB() const
    { return inverseSaturatedGasB_; }

    const Storage<TabulatedTwoDFunction>& gasMu() const
    { return gasMu_; }

    const Storage<TabulatedTwoDFunction>& inverseGasBMu() const
    { return inverseGasBMu_; }

    const Storage<TabulatedOneDFunction>& inverseSaturatedGasBMu() const
    { return inverseSaturatedGasBMu_; }

    const Storage<TabulatedOneDFunction>& saturatedOilVaporizationFactorTable() const
    { return saturatedOilVaporizationFactorTable_; }

    const Storage<TabulatedOneDFunction>& saturationPressure() const
    { return saturationPressure_; }

    Scalar vapPar1() const
    { return vapPar1_; }

private:
    void updateSaturationPressure_(unsigned regionIdx);

    //! The device table owner. Only the owning GPU object stores one; the table views in
    //! the owning GPU object and in the GPU view point into device memory owned by its
    //! tables. Held through a unique_ptr so that the owning GPU object is move-only.
    using GpuTables = WetGasPvt<Scalar, VectorWithDefaultAllocator, gpuistl::GpuBuffer>;
    using GpuTablesStorage = ConditionalStorage<isOwningGpu, std::unique_ptr<GpuTables>>;

    GpuTablesStorage gpuTables_{};

    Storage<Scalar> gasReferenceDensity_{};
    Storage<Scalar> oilReferenceDensity_{};
    Storage<TabulatedTwoDFunction> inverseGasB_{};
    Storage<TabulatedOneDFunction> inverseSaturatedGasB_{};
    Storage<TabulatedTwoDFunction> gasMu_{};
    Storage<TabulatedTwoDFunction> inverseGasBMu_{};
    Storage<TabulatedOneDFunction> inverseSaturatedGasBMu_{};
    Storage<TabulatedOneDFunction> saturatedOilVaporizationFactorTable_{};
    Storage<TabulatedOneDFunction> saturationPressure_{};

    Scalar vapPar1_ = 0.0;

#if HAVE_CUDA
    /*!
     * \brief Constructor for the GPU objects, used by gpuistl::copy_to_gpu() and
     *        gpuistl::make_view().
     *
     * \param gpuTables The device table owner for the owning GPU object. The table
     *                  views passed in the other arguments must point into its tables.
     *                  For the other GPU objects this is the empty storage; pass {}.
     */
    WetGasPvt(GpuTablesStorage&& gpuTables,
              Storage<Scalar>&& gasReferenceDensity,
              Storage<Scalar>&& oilReferenceDensity,
              Storage<TabulatedTwoDFunction>&& inverseGasB,
              Storage<TabulatedOneDFunction>&& inverseSaturatedGasB,
              Storage<TabulatedTwoDFunction>&& gasMu,
              Storage<TabulatedTwoDFunction>&& inverseGasBMu,
              Storage<TabulatedOneDFunction>&& inverseSaturatedGasBMu,
              Storage<TabulatedOneDFunction>&& saturatedOilVaporizationFactorTable,
              Storage<TabulatedOneDFunction>&& saturationPressure,
              Scalar vapPar1)
        requires (!isCpuStorage)
        : gpuTables_(std::move(gpuTables))
        , gasReferenceDensity_(std::move(gasReferenceDensity))
        , oilReferenceDensity_(std::move(oilReferenceDensity))
        , inverseGasB_(std::move(inverseGasB))
        , inverseSaturatedGasB_(std::move(inverseSaturatedGasB))
        , gasMu_(std::move(gasMu))
        , inverseGasBMu_(std::move(inverseGasBMu))
        , inverseSaturatedGasBMu_(std::move(inverseSaturatedGasBMu))
        , saturatedOilVaporizationFactorTable_(std::move(saturatedOilVaporizationFactorTable))
        , saturationPressure_(std::move(saturationPressure))
        , vapPar1_(vapPar1)
    { }

    template <class ScalarT>
    friend WetGasPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>
    gpuistl::copy_to_gpu(const WetGasPvt<ScalarT>&);

    template <class ScalarT>
    friend WetGasPvt<ScalarT, gpuistl::GpuView, gpuistl::GpuView>
    gpuistl::make_view(WetGasPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>&);
#endif // HAVE_CUDA
};

} // namespace Opm

#endif
