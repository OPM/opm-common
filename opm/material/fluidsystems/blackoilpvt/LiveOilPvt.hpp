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
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/UniformXTabulated2DFunctionBuilder.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <cstddef>
#include <stdexcept>
#include <type_traits>
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

namespace detail {

    /*!
     * \brief Holder for the table builders that LiveOilPvt uses during
     *        initialization (initFromState(), the setSaturatedOil*()
     *        methods and initEnd()).
     *
     *        The builders are CPU-only and are never used after initEnd(),
     *        so they only exist when LiveOilPvt uses CPU storage. For GPU
     *        storage (GpuBuffer or GpuView) the holder is empty, which keeps
     *        LiveOilPvt free of std::vector members on the GPU.
     */
    template <class Scalar, bool IsCpu>
    struct LiveOilPvtBuilderHolder {
    };

    template <class Scalar>
    struct LiveOilPvtBuilderHolder<Scalar, true> {
        std::vector<UniformXTabulated2DFunctionBuilder<Scalar>> inverseOilBTableBuilder_{};
        std::vector<UniformXTabulated2DFunctionBuilder<Scalar>> oilMuTableBuilder_{};
    };

    /*!
     * \brief Holder for the per-region device table buffers when LiveOilPvt
     *        owns its device memory (i.e. Storage is GpuBuffer).
     *
     *        In that case the per-region tables of LiveOilPvt are stored as
     *        GpuBuffer<Table<Scalar, GpuView>>, i.e. as non-owning views. The
     *        device arrays these views point into are owned by the
     *        GpuBuffer-based tables in this holder (one per region), so the
     *        holder must outlive every use of the views, including kernels
     *        using a view obtained by make_view().
     *
     *        The holder is filled by copy_to_gpu(). For CPU storage and for
     *        non-owning GPU storage (GpuView) it is empty.
     *
     * \note The holder is move-only, which makes an owning LiveOilPvt
     *       move-only as well. A copy would allocate new buffers, but the
     *       views in the copy would still point into the buffers of the
     *       original. Moving keeps the device pointers, and thereby all
     *       views, valid.
     */
    template <class Scalar, bool IsOwningGpu>
    struct LiveOilPvtGpuTableHolder {
    };

#if HAVE_CUDA
    template <class Scalar>
    struct LiveOilPvtGpuTableHolder<Scalar, true> {
        using TwoD = UniformXTabulated2DFunction<Scalar, gpuistl::GpuBuffer>;
        using OneD = Tabulated1DFunction<Scalar, gpuistl::GpuBuffer>;

        // Ensure that the holder is move-only:
        LiveOilPvtGpuTableHolder() = default;
        LiveOilPvtGpuTableHolder(const LiveOilPvtGpuTableHolder&) = delete;
        LiveOilPvtGpuTableHolder& operator=(const LiveOilPvtGpuTableHolder&) = delete;
        LiveOilPvtGpuTableHolder(LiveOilPvtGpuTableHolder&&) noexcept = default;
        LiveOilPvtGpuTableHolder& operator=(LiveOilPvtGpuTableHolder&&) noexcept = default;

        // One entry per PVT region, in the same order as the corresponding
        // table members of LiveOilPvt (e.g. inverseOilBTableBuffer_[regionIdx]
        // owns the data viewed by inverseOilBTable_[regionIdx]).
        std::vector<TwoD> inverseOilBTableBuffer_;
        std::vector<TwoD> oilMuTableBuffer_;
        std::vector<TwoD> inverseOilBMuTableBuffer_;
        std::vector<OneD> saturatedOilMuTableBuffer_;
        std::vector<OneD> inverseSaturatedOilBTableBuffer_;
        std::vector<OneD> inverseSaturatedOilBMuTableBuffer_;
        std::vector<OneD> saturatedGasDissolutionFactorTableBuffer_;
        std::vector<OneD> saturationPressureBuffer_;
    };
#endif

} // namespace detail

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the oil phase
 *        with dissolved gas.
 *
 * The class is used with three combinations of storage containers:
 *  - \c LiveOilPvt<Scalar> (CPU): initialized from the deck by initFromState() or by
 *    the setters followed by initEnd().
 *  - \c LiveOilPvt<Scalar, gpuistl::GpuBuffer, gpuistl::GpuView> (owning GPU): created
 *    from a CPU object by gpuistl::copy_to_gpu(). Owns all device memory.
 *  - \c LiveOilPvt<Scalar, gpuistl::GpuView, gpuistl::GpuView> (non-owning GPU): created
 *    from an owning GPU object by gpuistl::make_view(). This is the object to pass to
 *    kernels; the owning object must outlive it.
 *
 * \tparam Scalar       Floating point type of the tabulated values.
 * \tparam Storage      Container for the per-region data (reference densities and the
 *                      per-region tables).
 * \tparam TableStorage Container used inside each per-region table. On the GPU this is
 *                      always \c GpuView; the device arrays are owned by
 *                      detail::LiveOilPvtGpuTableHolder of the owning object.
 */
template <class Scalar, template<class> class Storage, template<class> class TableStorage>
class LiveOilPvt
    : private detail::LiveOilPvtBuilderHolder<
        Scalar,
        std::is_same_v<Storage<Scalar>, VectorWithDefaultAllocator<Scalar>>>,
      private detail::LiveOilPvtGpuTableHolder<
        Scalar,
        std::is_same_v<Storage<Scalar>, gpuistl::GpuBuffer<Scalar>>>
{
    using SamplingPoints = std::vector<std::pair<Scalar, Scalar>>;

public:
    using TabulatedTwoDFunction = UniformXTabulated2DFunction<Scalar, TableStorage>;
    using TabulatedTwoDFunctionBuilder = UniformXTabulated2DFunctionBuilder<Scalar>;
    using TabulatedOneDFunction = Tabulated1DFunction<Scalar, TableStorage>;

    //! True if this object owns device memory (created by gpuistl::copy_to_gpu()).
    static constexpr bool isOwningGpu
        = std::is_same_v<Storage<Scalar>, gpuistl::GpuBuffer<Scalar>>;

    //! True if this object is a non-owning view of device memory (created by gpuistl::make_view()).
    static constexpr bool isGpuView
        = std::is_same_v<Storage<Scalar>, gpuistl::GpuView<Scalar>>;

    //! True if this object lives on the CPU. Used to restrict the host-only methods that
    //! would otherwise compile for a GPU instantiation and then read or write device
    //! memory from the host (setInverseOilFormationVolumeFactor(), setOilViscosity() and
    //! saturationPressure()). The other host-only methods are defined in the .cpp file
    //! for CPU storage only, so misusing them already fails at link time.
    static constexpr bool isCpuStorage
        = std::is_same_v<Storage<Scalar>, VectorWithDefaultAllocator<Scalar>>;

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
    using GpuTableHolder = detail::LiveOilPvtGpuTableHolder<Scalar, true>;

    /*!
     * \brief Constructor for the owning GPU object, used by gpuistl::copy_to_gpu().
     *
     * \param holder Owns the per-region device tables. The per-region table views
     *               passed in the other arguments must point into these tables.
     */
    LiveOilPvt(GpuTableHolder&& holder,
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
        requires isOwningGpu
        : GpuTableHolder(std::move(holder))
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

    /*!
     * \brief Constructor for the non-owning GPU object, used by gpuistl::make_view().
     *
     * All arguments are views into the memory of an owning GPU object.
     */
    LiveOilPvt(Storage<Scalar> gasReferenceDensity,
               Storage<Scalar> oilReferenceDensity,
               Storage<TabulatedTwoDFunction> inverseOilBTable,
               Storage<TabulatedTwoDFunction> oilMuTable,
               Storage<TabulatedTwoDFunction> inverseOilBMuTable,
               Storage<TabulatedOneDFunction> saturatedOilMuTable,
               Storage<TabulatedOneDFunction> inverseSaturatedOilBTable,
               Storage<TabulatedOneDFunction> inverseSaturatedOilBMuTable,
               Storage<TabulatedOneDFunction> saturatedGasDissolutionFactorTable,
               Storage<TabulatedOneDFunction> saturationPressure,
               Scalar vapPar2)
        requires isGpuView
        : gasReferenceDensity_(gasReferenceDensity)
        , oilReferenceDensity_(oilReferenceDensity)
        , inverseOilBTable_(inverseOilBTable)
        , oilMuTable_(oilMuTable)
        , inverseOilBMuTable_(inverseOilBMuTable)
        , saturatedOilMuTable_(saturatedOilMuTable)
        , inverseSaturatedOilBTable_(inverseSaturatedOilBTable)
        , inverseSaturatedOilBMuTable_(inverseSaturatedOilBMuTable)
        , saturatedGasDissolutionFactorTable_(saturatedGasDissolutionFactorTable)
        , saturationPressure_(saturationPressure)
        , vapPar2_(vapPar2)
    { }

    template <class ScalarT>
    friend LiveOilPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>
    gpuistl::copy_to_gpu(const LiveOilPvt<ScalarT>&);

    template <class ScalarT>
    friend LiveOilPvt<ScalarT, gpuistl::GpuView, gpuistl::GpuView>
    gpuistl::make_view(LiveOilPvt<ScalarT, gpuistl::GpuBuffer, gpuistl::GpuView>&);
#endif
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

/*!
 * \brief Copy a CPU LiveOilPvt to the GPU.
 *
 * Each per-region table is uploaded to its own GpuBuffer-based table, owned by the
 * detail::LiveOilPvtGpuTableHolder base of the returned object. The per-region table
 * views into these buffers are then collected on the host and uploaded as one
 * GpuBuffer per table member.
 *
 * \param cpu A fully initialized CPU object (i.e. initEnd() has been called).
 * \return An owning GPU object. Use make_view() to obtain an object that can be
 *         passed to kernels. The returned object is move-only (see
 *         detail::LiveOilPvtGpuTableHolder).
 */
template <class ScalarT>
LiveOilPvt<ScalarT, GpuBuffer, GpuView>
copy_to_gpu(const LiveOilPvt<ScalarT>& cpu)
{
    using GpuPvt = LiveOilPvt<ScalarT, GpuBuffer, GpuView>;
    typename GpuPvt::GpuTableHolder holder;

    OPM_ERROR_IF(cpu.numRegions() == 0 || cpu.inverseOilBTable().size() != cpu.numRegions(),
                 "copy_to_gpu: LiveOilPvt is not initialized (call initFromState() or initEnd() first).");

    // Uploads each per-region table in cpuTables to its own owning table in
    // ownedTables, and returns a GpuBuffer of views into these owning tables.
    auto uploadTables = [](const auto& cpuTables, auto& ownedTables) {
        using ViewTable = decltype(make_view(ownedTables.front()));
        static_assert(std::is_trivially_copyable_v<ViewTable>,
                      "The table views are copied to the GPU with memcpy.");

        std::vector<ViewTable> views;
        ownedTables.reserve(cpuTables.size());
        views.reserve(cpuTables.size());
        for (const auto& cpuTable : cpuTables) {
            ownedTables.push_back(copy_to_gpu(cpuTable));
            views.push_back(make_view(ownedTables.back()));
        }
        return GpuBuffer<ViewTable>(views);
    };

    auto inverseOilB = uploadTables(cpu.inverseOilBTable(), holder.inverseOilBTableBuffer_);
    auto oilMu = uploadTables(cpu.oilMuTable(), holder.oilMuTableBuffer_);
    auto inverseOilBMu = uploadTables(cpu.inverseOilBMuTable(), holder.inverseOilBMuTableBuffer_);
    auto saturatedOilMu = uploadTables(cpu.saturatedOilMuTable(), holder.saturatedOilMuTableBuffer_);
    auto inverseSaturatedOilB = uploadTables(cpu.inverseSaturatedOilBTable(), holder.inverseSaturatedOilBTableBuffer_);
    auto inverseSaturatedOilBMu = uploadTables(cpu.inverseSaturatedOilBMuTable(), holder.inverseSaturatedOilBMuTableBuffer_);
    auto saturatedGasDissolutionFactor = uploadTables(cpu.saturatedGasDissolutionFactorTable(), holder.saturatedGasDissolutionFactorTableBuffer_);
    auto saturationPressure = uploadTables(cpu.saturationPressure(), holder.saturationPressureBuffer_);

    return GpuPvt(std::move(holder),
                  GpuBuffer<ScalarT>(cpu.gasReferenceDensity_),
                  GpuBuffer<ScalarT>(cpu.oilReferenceDensity_),
                  std::move(inverseOilB),
                  std::move(oilMu),
                  std::move(inverseOilBMu),
                  std::move(saturatedOilMu),
                  std::move(inverseSaturatedOilB),
                  std::move(inverseSaturatedOilBMu),
                  std::move(saturatedGasDissolutionFactor),
                  std::move(saturationPressure),
                  cpu.vapPar2_);
}

/*!
 * \brief Create a non-owning view of an owning GPU LiveOilPvt.
 *
 * The view can be passed by value to kernels. It does not own any memory, so
 * \p gpuBuffers must outlive every use of the view.
 */
template <class ScalarT>
LiveOilPvt<ScalarT, GpuView, GpuView>
make_view(LiveOilPvt<ScalarT, GpuBuffer, GpuView>& gpuBuffers)
{
    using GpuPvtView = LiveOilPvt<ScalarT, GpuView, GpuView>;
    static_assert(std::is_trivially_copyable_v<GpuPvtView>,
                  "The view is passed by value to kernels.");

    return GpuPvtView(make_view(gpuBuffers.gasReferenceDensity_),
                      make_view(gpuBuffers.oilReferenceDensity_),
                      make_view(gpuBuffers.inverseOilBTable_),
                      make_view(gpuBuffers.oilMuTable_),
                      make_view(gpuBuffers.inverseOilBMuTable_),
                      make_view(gpuBuffers.saturatedOilMuTable_),
                      make_view(gpuBuffers.inverseSaturatedOilBTable_),
                      make_view(gpuBuffers.inverseSaturatedOilBMuTable_),
                      make_view(gpuBuffers.saturatedGasDissolutionFactorTable_),
                      make_view(gpuBuffers.saturationPressure_),
                      gpuBuffers.vapPar2_);
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif
