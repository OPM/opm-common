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
 * \copydoc Opm::LiveOilPvt
 */
#ifndef OPM_LIVE_OIL_PVT_HPP
#define OPM_LIVE_OIL_PVT_HPP

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/Exceptions.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>

#include <opm/material/common/ConditionalStorage.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/UniformXTabulated2DFunctionBuilder.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm {

class EclipseState;
class Schedule;
class SimpleTable;

// Forward declaration, so that copy_to_gpu() and make_view() can be declared before
// the class definition. The default template arguments are given here (and only here).
template <class Scalar,
          template<class> class Storage = VectorWithDefaultAllocator,
          template<class> class TableStorage = VectorWithDefaultAllocator>
class LiveOilPvt;

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
    LiveOilPvt<ScalarT, GpuBuffer, GpuView>
    copy_to_gpu(const LiveOilPvt<ScalarT>&);

    template <class ScalarT>
    LiveOilPvt<ScalarT, GpuView, GpuView>
    make_view(LiveOilPvt<ScalarT, GpuBuffer, GpuView>&);
} // namespace gpuistl
#endif // HAVE_CUDA

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the oil phase
 *        with dissolved gas.
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
class LiveOilPvt
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedTwoDFunction = UniformXTabulated2DFunction<Scalar, TableStorage>;
    using TabulatedTwoDFunctionBuilder = UniformXTabulated2DFunctionBuilder<Scalar>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar, TableStorage>;

    //! True for the CPU object. Used to restrict the host-only methods that would otherwise
    //! compile for a GPU instantiation and then read or write device memory from the host
    //! (setInverseOilFormationVolumeFactor(), setOilViscosity() and saturationPressure()).
    //! The other host-only methods are defined in the .cpp file for CPU storage only, so
    //! misusing them already fails at link time.
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
                  "LiveOilPvt: unsupported combination of Storage and TableStorage");

    LiveOilPvt() = default;

    /*!
     * \brief Initialize the oil parameters via the data specified by the PVTO ECL keyword.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule);

private:
    void extendPvtoTable_(unsigned regionIdx,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable);

public:
    void setNumRegions(std::size_t numRegions);

    void setVapPars(const Scalar, const Scalar par2)
    {
        vapPar2_ = par2;
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/);

    /*!
     * \brief Initialize the function for the gas dissolution factor \f$R_s\f$
     *
     * \param regionIdx Region index to use
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedOilGasDissolutionFactor(unsigned regionIdx,
                                             const SamplingPoints& samplePoints)
    { saturatedGasDissolutionFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }

    /*!
     * \brief Initialize the function for the oil formation volume factor
     *
     * The oil formation volume factor \f$B_o\f$ is a function of \f$(p_o, X_o^G)\f$ and
     * represents the partial density of the oil component in the oil phase at a given
     * pressure. This method only requires the volume factor of gas-saturated oil (which
     * only depends on pressure) while the dependence on the gas mass fraction is
     * guesstimated.
     */
    void setSaturatedOilFormationVolumeFactor(unsigned regionIdx,
                                              const SamplingPoints& samplePoints);

    /*!
     * \brief Initialize the function for the oil formation volume factor
     *
     * The oil formation volume factor \f$B_o\f$ is a function of \f$(p_o, X_o^G)\f$ and
     * represents the partial density of the oil component in the oil phase at a given
     * pressure.
     *
     * This method sets \f$1/B_o(R_s, p_o)\f$. Note that instead of the mass fraction of
     * the gas component in the oil phase, this function depends on the gas dissolution
     * factor. Also note, that the order of the arguments needs to be \f$(R_s, p_o)\f$
     * and not the other way around.
     */
    void setInverseOilFormationVolumeFactor(unsigned regionIdx,
                                            const TabulatedTwoDFunction& invBo)
        requires isCpuStorage
    { inverseOilBTable_[regionIdx] = invBo; }

    /*!
     * \brief Initialize the viscosity of the oil phase.
     *
     * This is a function of \f$(R_s, p_o)\f$...
     */
    void setOilViscosity(unsigned regionIdx, const TabulatedTwoDFunction& muo)
        requires isCpuStorage
    { oilMuTable_[regionIdx] = muo; }

    /*!
     * \brief Initialize the phase viscosity for gas saturated oil
     *
     * The oil viscosity is a function of \f$(p_o, X_o^G)\f$, but this method only
     * requires the viscosity of gas-saturated oil (which only depends on pressure) while
     * there is assumed to be no dependence on the gas mass fraction...
     */
    void setSaturatedOilViscosity(unsigned regionIdx,
                                  const SamplingPoints& samplePoints);

    /*!
     * \brief Finish initializing the oil phase PVT properties.
     */
    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE unsigned numRegions() const
    { return inverseOilBMuTable_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of oil given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation internalEnergy(unsigned,
                                              const Evaluation&,
                                              const Evaluation&,
                                              const Evaluation&) const
    {
        OPM_THROW(std::runtime_error, "Requested the enthalpy of oil but the thermal "
                                      "option is not enabled");
    }

    OPM_HOST_DEVICE Scalar hVap(unsigned) const
    {
        OPM_THROW(std::runtime_error, "Requested the hvap of oil but the thermal "
                                      "option is not enabled");
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation viscosity(unsigned regionIdx,
                                         const Evaluation& /*temperature*/,
                                         const Evaluation& pressure,
                                         const Evaluation& Rs) const
    {
        // ATTENTION: Rs is the first axis!
        const Evaluation& invBo = inverseOilBTable_[regionIdx].eval(Rs, pressure, /*extrapolate=*/true);
        const Evaluation& invMuoBo = inverseOilBMuTable_[regionIdx].eval(Rs, pressure, /*extrapolate=*/true);

        return invBo / invMuoBo;
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedViscosity(unsigned regionIdx,
                                                  const Evaluation& /*temperature*/,
                                                  const Evaluation& pressure) const
    {
        // ATTENTION: Rs is the first axis!
        const Evaluation& invBo = inverseSaturatedOilBTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMuoBo = inverseSaturatedOilBMuTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBo / invMuoBo;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                                            const Evaluation& /*temperature*/,
                                                            const Evaluation& pressure,
                                                            const Evaluation& Rs) const
    {
        // ATTENTION: Rs is represented by the _first_ axis!
        return inverseOilBTable_[regionIdx].eval(Rs, pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the formation volume factor [-] and viscosity [Pa s] of the fluid phase.
     */
    template <class FluidState, class LhsEval = typename FluidState::ValueType>
    OPM_HOST_DEVICE std::pair<LhsEval, LhsEval>
    inverseFormationVolumeFactorAndViscosity(const FluidState& fluidState, unsigned regionIdx) const
    {
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(FluidState::oilPhaseIdx));
        const LhsEval& Rs = decay<LhsEval>(fluidState.Rs());

        const auto satSegIdx = this->saturatedGasDissolutionFactorTable_[regionIdx].findSegmentIndex(p, /*extrapolate=*/ true);
        const auto RsSat = this->saturatedGasDissolutionFactorTable_[regionIdx].eval(p, SegmentIndex{satSegIdx});
        const bool useSaturatedTables = (fluidState.saturation(FluidState::gasPhaseIdx) > 0.0) && (Rs >= (1.0 - 1e-10) * RsSat);

        if (useSaturatedTables) {
            const LhsEval b = this->inverseSaturatedOilBTable_[regionIdx].eval(p, SegmentIndex{satSegIdx});
            const LhsEval invBMu = this->inverseSaturatedOilBMuTable_[regionIdx].eval(p, SegmentIndex{satSegIdx});
            const LhsEval mu = b / invBMu;
            return { b, mu };
        } else {
            unsigned ii, jj1, jj2;
            LhsEval alpha, beta1, beta2;
            this->inverseOilBTable_[regionIdx].findPoints(ii, jj1, jj2, alpha, beta1, beta2, Rs, p, /*extrapolate =*/ true);
            const LhsEval b = this->inverseOilBTable_[regionIdx].eval(ii, jj1, jj2, alpha, beta1, beta2);
            const LhsEval invBMu = this->inverseOilBMuTable_[regionIdx].eval(ii, jj1, jj2, alpha, beta1, beta2);
            const LhsEval mu = b / invBMu;
            return { b, mu };
        }
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                                     const Evaluation& /*temperature*/,
                                                                     const Evaluation& pressure) const
    {
        // ATTENTION: Rs is represented by the _first_ axis!
        return inverseSaturatedOilBTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                                             const Evaluation& /*temperature*/,
                                                             const Evaluation& pressure) const
    { return saturatedGasDissolutionFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the gas dissolution factor \f$R_s\f$ [m^3/m^3] of the oil phase.
     *
     * This variant of the method prevents all the oil to be vaporized even if the gas
     * phase is still not saturated. This is physically quite dubious but it corresponds
     * to how the Eclipse 100 simulator handles this. (cf the VAPPARS keyword.)
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedGasDissolutionFactor(unsigned regionIdx,
                                                             const Evaluation& /*temperature*/,
                                                             const Evaluation& pressure,
                                                             const Evaluation& oilSaturation,
                                                             Evaluation maxOilSaturation) const
    {
        Evaluation tmp =
            saturatedGasDissolutionFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        // apply the vaporization parameters for the gas phase (cf. the Eclipse VAPPARS
        // keyword)
        maxOilSaturation = min(maxOilSaturation, Scalar(1.0));
        if (vapPar2_ > 0.0 && maxOilSaturation > 0.01 && oilSaturation < maxOilSaturation) {
            constexpr const Scalar eps = 0.001;
            const Evaluation& So = max(oilSaturation, eps);
            tmp *= max(1e-3, pow(So / maxOilSaturation, vapPar2_));
        }

        return tmp;
    }

    /*!
     * \brief Returns the saturation pressure of the oil phase [Pa]
     *        depending on its mass fraction of the gas component
     *
     * \param regionIdx Region index to use
     * \param Rs The surface volume of gas component dissolved in what will yield one cubic meter of oil at the surface [-]
     */
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation&,
                                  const Evaluation& Rs) const
        requires isCpuStorage
    {
        using Toolbox = MathToolbox<Evaluation>;

        const auto& RsTable = saturatedGasDissolutionFactorTable_[regionIdx];
        constexpr const Scalar eps = std::numeric_limits<typename Toolbox::Scalar>::epsilon() * 1e6;

        // use the saturation pressure function to get a pretty good initial value
        Evaluation pSat = saturationPressure_[regionIdx].eval(Rs, /*extrapolate=*/true);

        // Newton method to do the remaining work. If the initial
        // value is good, this should only take two to three
        // iterations...
        bool onProbation = false;
        for (int i = 0; i < 20; ++i) {
            const Evaluation& f = RsTable.eval(pSat, /*extrapolate=*/true) - Rs;
            const Evaluation& fPrime = RsTable.evalDerivative(pSat, /*extrapolate=*/true);

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
            " pSat = " + std::to_string(getValue(pSat)) +
            ", Rs = " + std::to_string(getValue(Rs));
        OpmLog::debug("Live oil saturation pressure", msg);
        throw NumericalProblem(msg);
    }

    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                                    const Evaluation& /*pressure*/,
                                                    unsigned /*compIdx*/) const
    {
        OPM_THROW(std::runtime_error, "Not implemented: The PVT model does not provide "
                                      "a diffusionCoefficient()");
    }

    OPM_HOST_DEVICE Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

    OPM_HOST_DEVICE Scalar oilReferenceDensity(unsigned regionIdx) const
    { return oilReferenceDensity_[regionIdx]; }

    const Storage<TabulatedTwoDFunction>& inverseOilBTable() const
    { return inverseOilBTable_; }

    const Storage<TabulatedTwoDFunction>& oilMuTable() const
    { return oilMuTable_; }

    const Storage<TabulatedTwoDFunction>& inverseOilBMuTable() const
    { return inverseOilBMuTable_; }

    const Storage<TabulatedOneDFunction>& saturatedOilMuTable() const
    { return saturatedOilMuTable_; }

    const Storage<TabulatedOneDFunction>& inverseSaturatedOilBTable() const
    { return inverseSaturatedOilBTable_; }

    const Storage<TabulatedOneDFunction>& inverseSaturatedOilBMuTable() const
    { return inverseSaturatedOilBMuTable_; }

    const Storage<TabulatedOneDFunction>& saturatedGasDissolutionFactorTable() const
    { return saturatedGasDissolutionFactorTable_; }

    const Storage<TabulatedOneDFunction>& saturationPressure() const
    { return saturationPressure_; }

    OPM_HOST_DEVICE Scalar vapPar2() const
    { return vapPar2_; }

private:
    void updateSaturationPressure_(unsigned regionIdx);

    //! The device table owner. Only the owning GPU object stores one; the table views in
    //! the owning GPU object and in the GPU view point into device memory owned by its
    //! tables. Held through a unique_ptr so that the owning GPU object is move-only.
    using GpuTables = LiveOilPvt<Scalar, VectorWithDefaultAllocator, gpuistl::GpuBuffer>;
    using GpuTablesStorage = ConditionalStorage<isOwningGpu, std::unique_ptr<GpuTables>>;

    //! The table builders are only needed by the CPU object, between setNumRegions()
    //! and initEnd().
    using BuilderStorage = ConditionalStorage<isCpuStorage, std::vector<TabulatedTwoDFunctionBuilder>>;

    GpuTablesStorage gpuTables_{};
    BuilderStorage inverseOilBTableBuilder_{};
    BuilderStorage oilMuTableBuilder_{};

    Storage<Scalar> gasReferenceDensity_{};
    Storage<Scalar> oilReferenceDensity_{};
    Storage<TabulatedTwoDFunction> inverseOilBTable_{};
    Storage<TabulatedTwoDFunction> oilMuTable_{};
    Storage<TabulatedTwoDFunction> inverseOilBMuTable_{};
    Storage<TabulatedOneDFunction> saturatedOilMuTable_{};
    Storage<TabulatedOneDFunction> inverseSaturatedOilBTable_{};
    Storage<TabulatedOneDFunction> inverseSaturatedOilBMuTable_{};
    Storage<TabulatedOneDFunction> saturatedGasDissolutionFactorTable_{};
    Storage<TabulatedOneDFunction> saturationPressure_{};

    Scalar vapPar2_ = 0.0;

#if HAVE_CUDA
    /*!
     * \brief Constructor for the GPU objects, used by gpuistl::copy_to_gpu() and
     *        gpuistl::make_view().
     *
     * \param gpuTables The device table owner for the owning GPU object. The table
     *                  views passed in the other arguments must point into its tables.
     *                  For the other GPU objects this is the empty storage; pass {}.
     */
    LiveOilPvt(GpuTablesStorage&& gpuTables,
               Storage<Scalar>&& gasReferenceDensity,
               Storage<Scalar>&& oilReferenceDensity,
               Storage<TabulatedTwoDFunction>&& inverseOilBTable,
               Storage<TabulatedTwoDFunction>&& oilMuTable,
               Storage<TabulatedTwoDFunction>&& inverseOilBMuTable,
               Storage<TabulatedOneDFunction>&& saturatedOilMuTable,
               Storage<TabulatedOneDFunction>&& inverseSaturatedOilBTable,
               Storage<TabulatedOneDFunction>&& inverseSaturatedOilBMuTable,
               Storage<TabulatedOneDFunction>&& saturatedGasDissolutionFactorTable,
               Storage<TabulatedOneDFunction>&& saturationPressure,
               Scalar vapPar2)
        requires (!isCpuStorage)
        : gpuTables_(std::move(gpuTables))
        , gasReferenceDensity_(std::move(gasReferenceDensity))
        , oilReferenceDensity_(std::move(oilReferenceDensity))
        , inverseOilBTable_(std::move(inverseOilBTable))
        , oilMuTable_(std::move(oilMuTable))
        , inverseOilBMuTable_(std::move(inverseOilBMuTable))
        , saturatedOilMuTable_(std::move(saturatedOilMuTable))
        , inverseSaturatedOilBTable_(std::move(inverseSaturatedOilBTable))
        , inverseSaturatedOilBMuTable_(std::move(inverseSaturatedOilBMuTable))
        , saturatedGasDissolutionFactorTable_(std::move(saturatedGasDissolutionFactorTable))
        , saturationPressure_(std::move(saturationPressure))
        , vapPar2_(vapPar2)
    { }

    template <class ScalarT>
    friend LiveOilPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>
    gpuistl::copy_to_gpu(const LiveOilPvt<ScalarT>&);

    template <class ScalarT>
    friend LiveOilPvt<ScalarT, gpuistl::GpuView, gpuistl::GpuView>
    gpuistl::make_view(LiveOilPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>&);
#endif // HAVE_CUDA
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

/*!
 * \brief Copy a CPU LiveOilPvt to the GPU.
 *
 * Each per-region table is uploaded to its own GpuBuffer-based table. These tables are
 * held by a device table owner (LiveOilPvt<ScalarT, VectorWithDefaultAllocator,
 * GpuBuffer>), which the returned object owns. The views into these tables are then
 * collected on the host and uploaded as one GpuBuffer per table member.
 *
 * \param cpu A fully initialized CPU object (i.e. initEnd() has been called).
 * \return An owning GPU object. It is move-only. Use make_view() to obtain an object
 *         that can be passed to kernels; the owning GPU object must outlive every
 *         kernel that uses such a view.
 */
template <class ScalarT>
LiveOilPvt<ScalarT, GpuBuffer, GpuView>
copy_to_gpu(const LiveOilPvt<ScalarT>& cpu)
{
    using GpuTables = LiveOilPvt<ScalarT, VectorWithDefaultAllocator, GpuBuffer>;
    using GpuPvt = LiveOilPvt<ScalarT, GpuBuffer, GpuView>;

    // inverseOilBMuTable_ is only filled by initEnd(); after setNumRegions() alone its
    // tables are empty.
    const bool initialized = cpu.numRegions() > 0
        && std::ranges::none_of(cpu.inverseOilBMuTable(),
                                [](const auto& table) { return table.numX() == 0; });
    OPM_ERROR_IF(!initialized,
                 "copy_to_gpu: LiveOilPvt is not initialized (call initFromState() or initEnd() first).");

    // Uploads each per-region table to its own owning table on the GPU.
    auto upload = [](const auto& cpuTables) {
        std::vector<decltype(copy_to_gpu(cpuTables.front()))> ownedTables;
        ownedTables.reserve(cpuTables.size());
        for (const auto& cpuTable : cpuTables) {
            ownedTables.push_back(copy_to_gpu(cpuTable));
        }
        return ownedTables;
    };

    // Not std::make_unique: the constructor is private, and std::make_unique is not a friend.
    std::unique_ptr<GpuTables> tables(new GpuTables(typename GpuTables::GpuTablesStorage{},
                                                    std::vector<ScalarT>(cpu.gasReferenceDensity_),
                                                    std::vector<ScalarT>(cpu.oilReferenceDensity_),
                                                    upload(cpu.inverseOilBTable_),
                                                    upload(cpu.oilMuTable_),
                                                    upload(cpu.inverseOilBMuTable_),
                                                    upload(cpu.saturatedOilMuTable_),
                                                    upload(cpu.inverseSaturatedOilBTable_),
                                                    upload(cpu.inverseSaturatedOilBMuTable_),
                                                    upload(cpu.saturatedGasDissolutionFactorTable_),
                                                    upload(cpu.saturationPressure_),
                                                    cpu.vapPar2_));

    // Returns a GpuBuffer of views into the owning tables.
    auto viewsOf = [](auto& ownedTables) {
        using ViewTable = decltype(make_view(ownedTables.front()));
        static_assert(std::is_trivially_copyable_v<ViewTable>,
                      "The table views are copied to the GPU with memcpy.");

        std::vector<ViewTable> views;
        views.reserve(ownedTables.size());
        for (auto& ownedTable : ownedTables) {
            views.push_back(make_view(ownedTable));
        }
        return GpuBuffer<ViewTable>(views);
    };

    // Everything that reads through `tables` is done before `tables` is moved into the
    // returned object: the order in which function arguments are evaluated is
    // unspecified, so reading and moving `tables` in the same call would be a bug.
    auto gasReferenceDensity = GpuBuffer<ScalarT>(tables->gasReferenceDensity_);
    auto oilReferenceDensity = GpuBuffer<ScalarT>(tables->oilReferenceDensity_);
    auto inverseOilB = viewsOf(tables->inverseOilBTable_);
    auto oilMu = viewsOf(tables->oilMuTable_);
    auto inverseOilBMu = viewsOf(tables->inverseOilBMuTable_);
    auto saturatedOilMu = viewsOf(tables->saturatedOilMuTable_);
    auto inverseSaturatedOilB = viewsOf(tables->inverseSaturatedOilBTable_);
    auto inverseSaturatedOilBMu = viewsOf(tables->inverseSaturatedOilBMuTable_);
    auto saturatedGasDissolutionFactor = viewsOf(tables->saturatedGasDissolutionFactorTable_);
    auto saturationPressure = viewsOf(tables->saturationPressure_);
    const ScalarT vapPar2 = tables->vapPar2_;

    return GpuPvt(typename GpuPvt::GpuTablesStorage(std::move(tables)),
                  std::move(gasReferenceDensity),
                  std::move(oilReferenceDensity),
                  std::move(inverseOilB),
                  std::move(oilMu),
                  std::move(inverseOilBMu),
                  std::move(saturatedOilMu),
                  std::move(inverseSaturatedOilB),
                  std::move(inverseSaturatedOilBMu),
                  std::move(saturatedGasDissolutionFactor),
                  std::move(saturationPressure),
                  vapPar2);
}

/*!
 * \brief Create a GPU view of an owning GPU LiveOilPvt.
 *
 * The view can be passed by value to kernels. It does not own any memory, so
 * \p gpuPvt must outlive every use of the view.
 */
template <class ScalarT>
LiveOilPvt<ScalarT, GpuView, GpuView>
make_view(LiveOilPvt<ScalarT, GpuBuffer, GpuView>& gpuPvt)
{
    using GpuPvtView = LiveOilPvt<ScalarT, GpuView, GpuView>;
    static_assert(std::is_trivially_copyable_v<GpuPvtView>,
                  "The view is passed by value to kernels.");

    return GpuPvtView(typename GpuPvtView::GpuTablesStorage{},
                      make_view(gpuPvt.gasReferenceDensity_),
                      make_view(gpuPvt.oilReferenceDensity_),
                      make_view(gpuPvt.inverseOilBTable_),
                      make_view(gpuPvt.oilMuTable_),
                      make_view(gpuPvt.inverseOilBMuTable_),
                      make_view(gpuPvt.saturatedOilMuTable_),
                      make_view(gpuPvt.inverseSaturatedOilBTable_),
                      make_view(gpuPvt.inverseSaturatedOilBMuTable_),
                      make_view(gpuPvt.saturatedGasDissolutionFactorTable_),
                      make_view(gpuPvt.saturationPressure_),
                      gpuPvt.vapPar2_);
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif
