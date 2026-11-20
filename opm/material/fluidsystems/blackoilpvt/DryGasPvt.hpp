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
 * \copydoc Opm::DryGasPvt
 */
#ifndef OPM_DRY_GAS_PVT_HPP
#define OPM_DRY_GAS_PVT_HPP

#include <opm/material/Constants.hpp>

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>

#include <opm/material/common/ConditionalStorage.hpp>
#include <opm/material/common/GpuTableHelpers.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm {

class EclipseState;
class Schedule;

// Forward declaration, so that copy_to_gpu() and make_view() can be declared before
// the class definition. The default template arguments are given here (and only here).
template <class Scalar,
          template<class> class Storage = VectorWithDefaultAllocator,
          template<class> class TableStorage = VectorWithDefaultAllocator>
class DryGasPvt;

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
    DryGasPvt<ScalarT, GpuBuffer, GpuView>
    copy_to_gpu(const DryGasPvt<ScalarT>&);

    template <class ScalarT>
    DryGasPvt<ScalarT, GpuView, GpuView>
    make_view(DryGasPvt<ScalarT, GpuBuffer, GpuView>&);
} // namespace gpuistl
#endif // HAVE_CUDA

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 *        without vaporized oil.
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
class DryGasPvt
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar, TableStorage>;

    //! True for the CPU object. Used to restrict the host-only setters that would otherwise
    //! compile for a GPU instantiation and then write device memory from the host
    //! (setReferenceDensities() and setGasViscosity()). The other host-only methods are
    //! defined in the .cpp file for CPU storage only, so misusing them already fails at
    //! link time.
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
                  "DryGasPvt: unsupported combination of Storage and TableStorage");

    DryGasPvt() = default;

    /*!
     * \brief Initialize the parameters for dry gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVDG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule&);

    void setNumRegions(std::size_t numRegions);

    OPM_HOST_DEVICE void setVapPars(const Scalar, const Scalar)
    {
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar /*rhoRefOil*/,
                               Scalar rhoRefGas,
                               Scalar /*rhoRefWater*/)
        requires isCpuStorage
    {
        gasReferenceDensity_[regionIdx] = rhoRefGas;
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setMolarMasses(unsigned /*regionIdx*/,
                        Scalar /*MOil*/,
                        Scalar /*MGas*/,
                        Scalar /*MWater*/)
    { }

    /*!
     * \brief Initialize the viscosity of the gas phase.
     *
     * This is a function of \f$(p_g)\f$...
     */
    void setGasViscosity(unsigned regionIdx, const TabulatedOneDFunction& mug)
        requires isCpuStorage
    { gasMu_[regionIdx] = mug; }

    /*!
     * \brief Initialize the function for the formation volume factor of dry gas
     *
     * \param regionIdx Region index to use
     * \param samplePoints A container of \f$(p_g, B_g)\f$ values
     */
    void setGasFormationVolumeFactor(unsigned regionIdx,
                                     const SamplingPoints& samplePoints);

    /*!
     * \brief Finish initializing the oil phase PVT properties.
     */
    void initEnd();

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    OPM_HOST_DEVICE unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation internalEnergy(unsigned,
                                              const Evaluation&,
                                              const Evaluation&,
                                              const Evaluation&,
                                              const Evaluation&) const
    {
        OPM_THROW(std::runtime_error, "Requested the enthalpy of gas but the thermal "
                                      "option is not enabled");
    }

    OPM_HOST_DEVICE Scalar hVap(unsigned) const
    {
        OPM_THROW(std::runtime_error, "Requested the hvap of gas but the thermal "
                                      "option is not enabled");
    }
    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation viscosity(unsigned regionIdx,
                                         const Evaluation& temperature,
                                         const Evaluation& pressure,
                                         const Evaluation& /*Rv*/,
                                         const Evaluation& /*Rvw*/) const
    { return saturatedViscosity(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at given pressure.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedViscosity(unsigned regionIdx,
                                                  const Evaluation& /*temperature*/,
                                                  const Evaluation& pressure) const
    {
        const Evaluation& invBg = inverseGasB_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseGasBMu_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBg / invMugBg;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                                            const Evaluation& temperature,
                                                            const Evaluation& pressure,
                                                            const Evaluation& /*Rv*/,
                                                            const Evaluation& /*Rvw*/) const
    { return saturatedInverseFormationVolumeFactor(regionIdx, temperature, pressure); }

    /*!
     * \brief Returns the formation volume factor [-] and viscosity [Pa s] of the fluid phase.
     */
    template <class FluidState, class LhsEval = typename FluidState::ValueType>
    OPM_HOST_DEVICE std::pair<LhsEval, LhsEval>
    inverseFormationVolumeFactorAndViscosity(const FluidState& fluidState, unsigned regionIdx) const
    {
        const LhsEval& p = decay<LhsEval>(fluidState.pressure(FluidState::gasPhaseIdx));
        const auto segIdx = this->inverseGasB_[regionIdx].findSegmentIndex(p, /*extrapolate=*/ true);
        const auto& invBg = this->inverseGasB_[regionIdx].eval(p, SegmentIndex{segIdx});
        const auto& invMugBg = this->inverseGasBMu_[regionIdx].eval(p, SegmentIndex{segIdx});
        return { invBg, invBg / invMugBg };
    }

    /*!
     * \brief Returns the formation volume factor [-] of oil saturated gas at given pressure.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                                     const Evaluation& /*temperature*/,
                                                                     const Evaluation& pressure) const
    { return inverseGasB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the oil component
     *
     * \param Rv The surface volume of oil component dissolved in what
     *           will yield one cubic meter of gas at the surface [-]
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturationPressure(unsigned /*regionIdx*/,
                                                  const Evaluation& /*temperature*/,
                                                  const Evaluation& /*Rv*/) const
    { return 0.0; /* this is dry gas! */ }

    /*!
     * \brief Returns the water vaporization factor \f$R_v\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedWaterVaporizationFactor(unsigned /*regionIdx*/,
                                                                const Evaluation& /*temperature*/,
                                                                const Evaluation& /*pressure*/) const
    { return 0.0; /* this is non-humid gas! */ }

    /*!
    * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of water saturated gas.
    */
    template <class Evaluation = Scalar>
    OPM_HOST_DEVICE Evaluation saturatedWaterVaporizationFactor(unsigned /*regionIdx*/,
                                                                const Evaluation& /*temperature*/,
                                                                const Evaluation& /*pressure*/,
                                                                const Evaluation& /*saltConcentration*/) const
    { return 0.0; }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedOilVaporizationFactor(unsigned /*regionIdx*/,
                                                              const Evaluation& /*temperature*/,
                                                              const Evaluation& /*pressure*/,
                                                              const Evaluation& /*oilSaturation*/,
                                                              const Evaluation& /*maxOilSaturation*/) const
    { return 0.0; /* this is dry gas! */ }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the oil phase.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedOilVaporizationFactor(unsigned /*regionIdx*/,
                                                              const Evaluation& /*temperature*/,
                                                              const Evaluation& /*pressure*/) const
    { return 0.0; /* this is dry gas! */ }

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

    const Storage<TabulatedOneDFunction>& inverseGasB() const
    { return inverseGasB_; }

    const Storage<TabulatedOneDFunction>& gasMu() const
    { return gasMu_; }

    const Storage<TabulatedOneDFunction>& inverseGasBMu() const
    { return inverseGasBMu_; }

private:
    //! The device table owner. Only the owning GPU object stores one; the table views in
    //! the owning GPU object and in the GPU view point into device memory owned by its
    //! tables. Held through a unique_ptr so that the owning GPU object is move-only.
    using GpuTables = DryGasPvt<Scalar, VectorWithDefaultAllocator, gpuistl::GpuBuffer>;
    using GpuTablesStorage = ConditionalStorage<isOwningGpu, std::unique_ptr<GpuTables>>;

    GpuTablesStorage gpuTables_{};

    Storage<Scalar> gasReferenceDensity_{};
    Storage<TabulatedOneDFunction> inverseGasB_{};
    Storage<TabulatedOneDFunction> gasMu_{};
    Storage<TabulatedOneDFunction> inverseGasBMu_{};

#if HAVE_CUDA
    /*!
     * \brief Constructor for the GPU objects, used by gpuistl::copy_to_gpu() and
     *        gpuistl::make_view().
     *
     * \param gpuTables The device table owner for the owning GPU object. The table
     *                  views passed in the other arguments must point into its tables.
     *                  For the other GPU objects this is the empty storage; pass {}.
     */
    DryGasPvt(GpuTablesStorage&& gpuTables,
              Storage<Scalar>&& gasReferenceDensity,
              Storage<TabulatedOneDFunction>&& inverseGasB,
              Storage<TabulatedOneDFunction>&& gasMu,
              Storage<TabulatedOneDFunction>&& inverseGasBMu)
        requires (!isCpuStorage)
        : gpuTables_(std::move(gpuTables))
        , gasReferenceDensity_(std::move(gasReferenceDensity))
        , inverseGasB_(std::move(inverseGasB))
        , gasMu_(std::move(gasMu))
        , inverseGasBMu_(std::move(inverseGasBMu))
    { }

    template <class ScalarT>
    friend DryGasPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>
    gpuistl::copy_to_gpu(const DryGasPvt<ScalarT>&);

    template <class ScalarT>
    friend DryGasPvt<ScalarT, gpuistl::GpuView, gpuistl::GpuView>
    gpuistl::make_view(DryGasPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>&);
#endif // HAVE_CUDA
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

/*!
 * \brief Copy a CPU DryGasPvt to the GPU.
 *
 * Each per-region table is uploaded to its own GpuBuffer-based table. These tables are
 * held by a device table owner (DryGasPvt<ScalarT, VectorWithDefaultAllocator,
 * GpuBuffer>), which the returned object owns. The views into these tables are then
 * collected on the host and uploaded as one GpuBuffer per table member.
 *
 * \param cpu A fully initialized CPU object (i.e. initEnd() has been called).
 * \return An owning GPU object. It is move-only. Use make_view() to obtain an object
 *         that can be passed to kernels; the owning GPU object must outlive every
 *         kernel that uses such a view.
 */
template <class ScalarT>
DryGasPvt<ScalarT, GpuBuffer, GpuView>
copy_to_gpu(const DryGasPvt<ScalarT>& cpu)
{
    using GpuTables = DryGasPvt<ScalarT, VectorWithDefaultAllocator, GpuBuffer>;
    using GpuPvt = DryGasPvt<ScalarT, GpuBuffer, GpuView>;

    // inverseGasBMu_ is only filled by initEnd(); after setNumRegions() alone its
    // tables are empty.
    const bool initialized = cpu.numRegions() > 0
        && std::ranges::none_of(cpu.inverseGasBMu(),
                                [](const auto& table) { return table.numSamples() == 0; });
    OPM_ERROR_IF(!initialized,
                 "copy_to_gpu: DryGasPvt is not initialized (call initFromState() or initEnd() first).");

    // Upload the tables to the GPU.
    // Not std::make_unique: the constructor is private, and std::make_unique is not a friend.
    std::unique_ptr<GpuTables> tables(new GpuTables(typename GpuTables::GpuTablesStorage{},
                                                    std::vector<ScalarT>(cpu.gasReferenceDensity_),
                                                    copy_each_to_gpu(cpu.inverseGasB_),
                                                    copy_each_to_gpu(cpu.gasMu_),
                                                    copy_each_to_gpu(cpu.inverseGasBMu_)));

    // Upload vectors of GPU views to the GPU.
    // Everything that reads through `tables` is done before `tables` is moved into the
    // returned object: the order in which function arguments are evaluated is
    // unspecified, so reading and moving `tables` in the same call would be a bug.
    auto gasReferenceDensity = GpuBuffer<ScalarT>(tables->gasReferenceDensity_);
    auto inverseGasB = make_view_buffer(tables->inverseGasB_);
    auto gasMu = make_view_buffer(tables->gasMu_);
    auto inverseGasBMu = make_view_buffer(tables->inverseGasBMu_);

    return GpuPvt(typename GpuPvt::GpuTablesStorage(std::move(tables)),
                  std::move(gasReferenceDensity),
                  std::move(inverseGasB),
                  std::move(gasMu),
                  std::move(inverseGasBMu));
}

/*!
 * \brief Create a GPU view of an owning GPU DryGasPvt.
 *
 * The view can be passed by value to kernels. It does not own any memory, so
 * \p gpuPvt must outlive every use of the view.
 */
template <class ScalarT>
DryGasPvt<ScalarT, GpuView, GpuView>
make_view(DryGasPvt<ScalarT, GpuBuffer, GpuView>& gpuPvt)
{
    using GpuPvtView = DryGasPvt<ScalarT, GpuView, GpuView>;
    static_assert(std::is_trivially_copyable_v<GpuPvtView>,
                  "The view is passed by value to kernels.");

    return GpuPvtView(typename GpuPvtView::GpuTablesStorage{},
                      make_view(gpuPvt.gasReferenceDensity_),
                      make_view(gpuPvt.inverseGasB_),
                      make_view(gpuPvt.gasMu_),
                      make_view(gpuPvt.inverseGasBMu_));
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA


#endif
