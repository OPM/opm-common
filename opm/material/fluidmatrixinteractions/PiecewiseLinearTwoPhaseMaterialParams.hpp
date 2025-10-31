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
 * \copydoc Opm::PiecewiseLinearTwoPhaseMaterialParams
 */
#ifndef OPM_PIECEWISE_LINEAR_TWO_PHASE_MATERIAL_PARAMS_HPP
#define OPM_PIECEWISE_LINEAR_TWO_PHASE_MATERIAL_PARAMS_HPP

#include <algorithm>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <type_traits>

#include <opm/common/ErrorMacros.hpp>
#include <opm/material/common/EnsureFinalized.hpp>
#include <vector>

#include <opm/common/utility/gpuDecorators.hpp>

// forward declaration of the class so the function in the next namespace can be declared
template <class TraitsT, class VectorT = std::vector<typename TraitsT::Scalar>>
class PiecewiseLinearTwoPhaseMaterialParams;

// declaration of make_view in correct namespace so friend function can be declared in the class
namespace Opm::gpuistl
{
    template <class ViewType, class TraitsT, class ContainerType>
    PiecewiseLinearTwoPhaseMaterialParams<TraitsT, ViewType> make_view(PiecewiseLinearTwoPhaseMaterialParams<TraitsT, ContainerType>& params);
}

namespace Opm {
/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Specification of the material parameters for a two-phase material law which
 *        uses a table and piecewise constant interpolation.
 */
template <class TraitsT, class VectorT = std::vector<typename TraitsT::Scalar>>
class PiecewiseLinearTwoPhaseMaterialParams : public EnsureFinalized
{
    using Scalar = typename TraitsT::Scalar;

public:
    using ValueVector = VectorT;

    using Traits = TraitsT;

    PiecewiseLinearTwoPhaseMaterialParams()
    {
    }

    PiecewiseLinearTwoPhaseMaterialParams(ValueVector SwPcwnSamples,
                                          ValueVector pcwnSamples,
                                          ValueVector SwKrwSamples,
                                          ValueVector krwSamples,
                                          ValueVector SwKrnSamples,
                                          ValueVector krnSamples)
        : SwPcwnSamples_(SwPcwnSamples)
        , SwKrwSamples_(SwKrwSamples)
        , SwKrnSamples_(SwKrnSamples)
        , pcwnSamples_(pcwnSamples)
        , krwSamples_(krwSamples)
        , krnSamples_(krnSamples)
    {
        if constexpr (std::is_same_v<ValueVector, std::vector<Scalar>>){
            finalize();
        }
        else{
            // safe if we have a GPU type instantiated by copy_to_gpu or make_view
            EnsureFinalized::finalize();
        }
    }

    /*!
     * \brief Calculate all dependent quantities once the independent
     *        quantities of the parameter object have been set.
     */
    void finalize()
    {
        EnsureFinalized::finalize();

        // revert the order of the sampling points if they were given
        // in reverse direction.
        if (SwPcwnSamples_.front() > SwPcwnSamples_.back())
            swapOrderIfPossibleThrowOtherwise_(SwPcwnSamples_, pcwnSamples_);

        if (SwKrwSamples_.front() > SwKrwSamples_.back())
            swapOrderIfPossibleThrowOtherwise_(SwKrwSamples_, krwSamples_);

        if (SwKrnSamples_.front() > SwKrnSamples_.back())
            swapOrderIfPossibleThrowOtherwise_(SwKrnSamples_, krnSamples_);
    }

    /*!
     * \brief Check if the parameter object has been finalized.
     */
    void checkFinalized() const
    {
        EnsureFinalized::check();
    }

    /*!
     * \brief Return the wetting-phase saturation values of all sampling points.
     */
    OPM_HOST_DEVICE const ValueVector& SwKrwSamples() const
    {
        EnsureFinalized::check();
        return SwKrwSamples_;
    }

    /*!
     * \brief Return the wetting-phase saturation values of all sampling points.
     */
    OPM_HOST_DEVICE const ValueVector& SwKrnSamples() const
    {
        EnsureFinalized::check();
        return SwKrnSamples_;
    }

    /*!
     * \brief Return the wetting-phase saturation values of all sampling points.
     */
    OPM_HOST_DEVICE const ValueVector& SwPcwnSamples() const
    {
        EnsureFinalized::check();
        return SwPcwnSamples_;
    }

    /*!
     * \brief Return the sampling points for the capillary pressure curve.
     *
     * This curve is assumed to depend on the wetting phase saturation
     */
    OPM_HOST_DEVICE const ValueVector& pcwnSamples() const
    {
        EnsureFinalized::check();
        return pcwnSamples_;
    }

    /*!
     * \brief Set the sampling points for the capillary pressure curve.
     *
     * This curve is assumed to depend on the wetting phase saturation
     */
    template <class Container>
    void setPcnwSamples(const Container& SwValues, const Container& values)
    {
        assert(SwValues.size() == values.size());

        size_t n = SwValues.size();
        SwPcwnSamples_.resize(n);
        pcwnSamples_.resize(n);

        std::copy(SwValues.begin(), SwValues.end(), SwPcwnSamples_.begin());
        std::copy(values.begin(), values.end(), pcwnSamples_.begin());
    }

    /*!
     * \brief Return the sampling points for the relative permeability
     *        curve of the wetting phase.
     *
     * This curve is assumed to depend on the wetting phase saturation
     */
    OPM_HOST_DEVICE const ValueVector& krwSamples() const
    {
        EnsureFinalized::check();
        return krwSamples_;
    }

    /*!
     * \brief Set the sampling points for the relative permeability
     *        curve of the wetting phase.
     *
     * This curve is assumed to depend on the wetting phase saturation
     */
    template <class Container>
    void setKrwSamples(const Container& SwValues, const Container& values)
    {
        assert(SwValues.size() == values.size());

        size_t n = SwValues.size();
        SwKrwSamples_.resize(n);
        krwSamples_.resize(n);

        std::copy(SwValues.begin(), SwValues.end(), SwKrwSamples_.begin());
        std::copy(values.begin(), values.end(), krwSamples_.begin());
    }

    /*!
     * \brief Return the sampling points for the relative permeability
     *        curve of the non-wetting phase.
     *
     * This curve is assumed to depend on the wetting phase saturation
     */
    OPM_HOST_DEVICE const ValueVector& krnSamples() const
    {
        EnsureFinalized::check();
        return krnSamples_;
    }

    /*!
     * \brief Set the sampling points for the relative permeability
     *        curve of the non-wetting phase.
     *
     * This curve is assumed to depend on the wetting phase saturation
     */
    template <class Container>
    void setKrnSamples(const Container& SwValues, const Container& values)
    {
        assert(SwValues.size() == values.size());

        size_t n = SwValues.size();
        SwKrnSamples_.resize(n);
        krnSamples_.resize(n);

        std::copy(SwValues.begin(), SwValues.end(), SwKrnSamples_.begin());
        std::copy(values.begin(), values.end(), krnSamples_.begin());
    }

private:
    void swapOrderIfPossibleThrowOtherwise_(ValueVector& swValues, ValueVector& values) const
    {
        if (swValues.front() > swValues.back()) {
            // Reverting the order involves swapping which only works for non-consts.
            // The const expr ensures we can create constant parameter views.
            if constexpr (!std::is_const_v<typename ValueVector::value_type> && !std::is_const_v<ValueVector>) {
                for (unsigned origSampleIdx = 0; origSampleIdx < swValues.size() / 2; ++origSampleIdx) {
                    size_t newSampleIdx = swValues.size() - origSampleIdx - 1;

                    std::swap(swValues[origSampleIdx], swValues[newSampleIdx]);
                    std::swap(values[origSampleIdx], values[newSampleIdx]);
                }
            }
            else{
                OPM_THROW(std::logic_error, "Saturation values in interpolation table provided in wrong order, but table is immutable");
            }
        }
    }

    template <class ViewType, class Traits, class Container>
    friend PiecewiseLinearTwoPhaseMaterialParams<Traits, ViewType> Opm::gpuistl::make_view(PiecewiseLinearTwoPhaseMaterialParams<Traits, Container>& params);

    ValueVector SwPcwnSamples_;
    ValueVector SwKrwSamples_;
    ValueVector SwKrnSamples_;
    ValueVector pcwnSamples_;
    ValueVector krwSamples_;
    ValueVector krnSamples_;
};
} // namespace Opm

namespace Opm::gpuistl{

/// @brief Move a PiecewiseLinearTwoPhaseMaterialParams-object to the GPU
/// @tparam TraitsT the same traits as in PiecewiseLinearTwoPhaseMaterialParams
/// @tparam ViewType  typically gpuView<const scalarType>
/// @tparam GPUContainerType typically const gpuBuffer<scalarType>
/// @param params the parameters object living on the CPU
/// @return the GPU PiecewiseLinearTwoPhaseMaterialParams object
template <class GPUContainerType, class TraitsT>
PiecewiseLinearTwoPhaseMaterialParams<TraitsT, GPUContainerType> copy_to_gpu(const PiecewiseLinearTwoPhaseMaterialParams<TraitsT>& params) {

    // only create the GPU object if the CPU object is finalized
    params.checkFinalized();

    auto SwPcwnSamples = GPUContainerType(params.SwPcwnSamples());
    auto pcwnSamples = GPUContainerType(params.pcwnSamples());
    auto SwKrwSamples = GPUContainerType(params.SwKrwSamples());
    auto krwSamples = GPUContainerType(params.krwSamples());
    auto SwKrnSamples = GPUContainerType(params.SwKrnSamples());
    auto krnSamples = GPUContainerType(params.krnSamples());

    return PiecewiseLinearTwoPhaseMaterialParams<TraitsT, GPUContainerType> (SwPcwnSamples,
                                                                        pcwnSamples,
                                                                        SwKrwSamples,
                                                                        krwSamples,
                                                                        SwKrnSamples,
                                                                        krnSamples);
}

/// @brief this function is intented to make a GPU friendly view of the PiecewiseLinearTwoPhaseMaterialParams
/// @tparam TraitsT the same traits as in PiecewiseLinearTwoPhaseMaterialParams
/// @tparam ContainerType typically const gpuBuffer<scalarType>
/// @tparam ViewType  typically gpuView<const scalarType>
/// @param params the parameters object instansiated with gpuBuffers or similar
/// @return the GPU view of the GPU PiecewiseLinearTwoPhaseMaterialParams object
template <class ViewType, class TraitsT, class ContainerType>
PiecewiseLinearTwoPhaseMaterialParams<TraitsT, ViewType> make_view(PiecewiseLinearTwoPhaseMaterialParams<TraitsT, ContainerType>& params) {

    // only create the GPU object if the CPU object is finalized
    params.checkFinalized();

    using ContainedType = typename ViewType::value_type;

    ViewType SwPcwnSamples = make_view<ContainedType>(params.SwPcwnSamples_);
    ViewType pcwnSamples = make_view<ContainedType>(params.pcwnSamples_);
    ViewType SwKrwSamples = make_view<ContainedType>(params.SwKrwSamples_);
    ViewType krwSamples = make_view<ContainedType>(params.krwSamples_);
    ViewType SwKrnSamples = make_view<ContainedType>(params.SwKrnSamples_);
    ViewType krnSamples = make_view<ContainedType>(params.krnSamples_);

    return PiecewiseLinearTwoPhaseMaterialParams<TraitsT, ViewType> (SwPcwnSamples,
                                                                        pcwnSamples,
                                                                        SwKrwSamples,
                                                                        krwSamples,
                                                                        SwKrnSamples,
                                                                        krnSamples);
}

}

#endif
