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
 * \copydoc Opm::EclSpecrockLawParams
 */
#ifndef OPM_ECL_SPECROCK_LAW_PARAMS_HPP
#define OPM_ECL_SPECROCK_LAW_PARAMS_HPP

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>

#include <opm/material/common/EnsureFinalized.hpp>

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Opm {

template <class ScalarT, template <class> class Storage = ::Opm::VectorWithDefaultAllocator>
class EclSpecrockLawParams;

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

template <class ScalarT>
::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>
copy_to_gpu(const ::Opm::EclSpecrockLawParams<ScalarT>& cpu);

template <class ScalarT, template <class> class ContainerT>
::Opm::EclSpecrockLawParams<ScalarT, GpuView>
make_view(::Opm::EclSpecrockLawParams<ScalarT, ContainerT>& gpuBuffers);

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

namespace Opm {

/*!
 * \brief The default implementation of a parameter object for the
 *        ECL thermal law based on SPECROCK.
 *
 * Stores the temperature-vs-volumetric-internal-energy table in a
 * templated \c Storage container so the same class can be instantiated
 * as a CPU object (\c VectorWithDefaultAllocator), an owning GPU object
 * (\c GpuBuffer) and a non-owning GPU view (\c GpuView) usable from a
 * kernel.
 */
template <class ScalarT, template <class> class Storage>
class EclSpecrockLawParams : public EnsureFinalized
{
public:
    using Scalar = ScalarT;
    using ValueVector = Storage<Scalar>;

    OPM_HOST_DEVICE EclSpecrockLawParams() = default;

    OPM_HOST_DEVICE EclSpecrockLawParams(ValueVector temperatureSamples,
                                         ValueVector internalEnergySamples)
        : temperatureSamples_(std::move(temperatureSamples))
        , internalEnergySamples_(std::move(internalEnergySamples))
    {
        EnsureFinalized::finalize();
    }

    /*!
     * \brief Specify the volumetric internal energy of rock via heat capacities.
     *
     * Available only on the CPU instantiation since GPU storage types are
     * not constructible from arbitrary host containers. Integrates the
     * piecewise-linear heat capacity to obtain the volumetric internal
     * energy at the same temperature samples.
     */
    template <class ContainerT,
              class StorageT = Storage<Scalar>,
              std::enable_if_t<std::is_same_v<StorageT, ::Opm::VectorWithDefaultAllocator<Scalar>>,
                               int> = 0>
    void setHeatCapacities(const ContainerT& temperature,
                           const ContainerT& heatCapacity)
    {
        if (temperature.size() != heatCapacity.size()) {
            OPM_THROW(std::invalid_argument,
                      "EclSpecrockLawParams: temperature and heat-capacity arrays must have "
                      "matching sizes");
        }

        const std::size_t n = temperature.size();
        temperatureSamples_.resize(n);
        internalEnergySamples_.resize(n);

        // Integrate the heat capacity to compute the internal energy.
        Scalar curU = static_cast<Scalar>(temperature[0]) * static_cast<Scalar>(heatCapacity[0]);
        for (std::size_t i = 0; i < n; ++i) {
            temperatureSamples_[i] = static_cast<Scalar>(temperature[i]);
            internalEnergySamples_[i] = curU;

            if (i + 1 >= n) {
                break;
            }

            // Trapezoidal integration of the heat capacity from the
            // current sample to the next one.
            const Scalar c_v0 = static_cast<Scalar>(heatCapacity[i]);
            const Scalar c_v1 = static_cast<Scalar>(heatCapacity[i + 1]);
            const Scalar T0 = static_cast<Scalar>(temperature[i]);
            const Scalar T1 = static_cast<Scalar>(temperature[i + 1]);
            curU += Scalar(0.5) * (c_v0 + c_v1) * (T1 - T0);
        }
    }

    /*!
     * \brief Set the sample tables directly. Marks the object as finalized.
     *
     * Available only on the CPU instantiation.
     */
    template <class ContainerT,
              class StorageT = Storage<Scalar>,
              std::enable_if_t<std::is_same_v<StorageT, ::Opm::VectorWithDefaultAllocator<Scalar>>,
                               int> = 0>
    void setSamples(const ContainerT& temperature, const ContainerT& internalEnergy)
    {
        if (temperature.size() != internalEnergy.size()) {
            OPM_THROW(std::invalid_argument,
                      "EclSpecrockLawParams: temperature and internal-energy arrays must have "
                      "matching sizes");
        }
        const std::size_t n = temperature.size();
        temperatureSamples_.resize(n);
        internalEnergySamples_.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            temperatureSamples_[i] = static_cast<Scalar>(temperature[i]);
            internalEnergySamples_[i] = static_cast<Scalar>(internalEnergy[i]);
        }
        EnsureFinalized::finalize();
    }

    OPM_HOST_DEVICE std::size_t numSamples() const
    {
        return temperatureSamples_.size();
    }

    /*!
     * \brief Linearly interpolate the volumetric internal energy at a
     *        given temperature. The sample table is assumed sorted in
     *        ascending order; values outside the range are extrapolated
     *        linearly using the first/last segment (matching the
     *        previous Tabulated1DFunction::eval(T, true) behaviour.
     */
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation eval(const Evaluation& x) const
    {
        EnsureFinalized::check();
        const std::size_t n = temperatureSamples_.size();
        // n >= 2 by construction (SPECROCK tables always have >= 2 rows).
        std::size_t segIdx = 0;
        if (x <= temperatureSamples_[1]) {
            segIdx = 0;
        } else if (x >= temperatureSamples_[n - 2]) {
            segIdx = n - 2;
        } else {
            std::size_t lo = 1;
            std::size_t hi = n - 2;
            while (lo + 1 < hi) {
                const std::size_t mid = (lo + hi) / 2;
                if (x < temperatureSamples_[mid]) {
                    hi = mid;
                } else {
                    lo = mid;
                }
            }
            segIdx = lo;
        }
        const Scalar x0 = temperatureSamples_[segIdx];
        const Scalar x1 = temperatureSamples_[segIdx + 1];
        const Scalar y0 = internalEnergySamples_[segIdx];
        const Scalar y1 = internalEnergySamples_[segIdx + 1];
        return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
    }

    OPM_HOST_DEVICE const ValueVector& temperatureSamples() const
    {
        EnsureFinalized::check();
        return temperatureSamples_;
    }

    OPM_HOST_DEVICE const ValueVector& internalEnergySamples() const
    {
        EnsureFinalized::check();
        return internalEnergySamples_;
    }

    ValueVector& temperatureSamplesMutable()
    {
        return temperatureSamples_;
    }

    ValueVector& internalEnergySamplesMutable()
    {
        return internalEnergySamples_;
    }

private:
    ValueVector temperatureSamples_ {};
    ValueVector internalEnergySamples_ {};
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

template <class ScalarT>
::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>
copy_to_gpu(const ::Opm::EclSpecrockLawParams<ScalarT>& cpu)
{
    return ::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>(
        GpuBuffer<ScalarT>(cpu.temperatureSamples()),
        GpuBuffer<ScalarT>(cpu.internalEnergySamples()));
}

template <class ScalarT, template <class> class ContainerT>
::Opm::EclSpecrockLawParams<ScalarT, GpuView>
make_view(::Opm::EclSpecrockLawParams<ScalarT, ContainerT>& gpuBuffers)
{
    auto tView = make_view<ScalarT>(gpuBuffers.temperatureSamplesMutable());
    auto eView = make_view<ScalarT>(gpuBuffers.internalEnergySamplesMutable());
    return ::Opm::EclSpecrockLawParams<ScalarT, GpuView>(tView, eView);
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif
