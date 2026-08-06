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
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

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
 * \c Tabulated1DFunction. The function's storage policy is propagated so
 * this class can be instantiated as a CPU object, an owning GPU object,
 * or a non-owning GPU view usable from a kernel.
 */
template <class ScalarT, template <class> class Storage>
class EclSpecrockLawParams : public EnsureFinalized
{
    using InternalEnergyFunction =
        Tabulated1DFunction<ScalarT, Storage>;

public:
    using Scalar = ScalarT;
    using ValueVector = typename InternalEnergyFunction::ValueVector;

    OPM_HOST_DEVICE EclSpecrockLawParams() = default;

    OPM_HOST_DEVICE EclSpecrockLawParams(const EclSpecrockLawParams<ScalarT, Storage>&) = default;

    OPM_HOST_DEVICE explicit EclSpecrockLawParams(
        InternalEnergyFunction internalEnergyFunction)
        : internalEnergyFunction_(std::move(internalEnergyFunction))
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
        assert(temperature.size() == heatCapacity.size());

        // integrate the heat capacity to compute the internal energy
        Scalar curU = temperature[0]*heatCapacity[0];
        unsigned n = temperature.size();
        std::vector<Scalar> T(n);
        std::vector<Scalar> u(n);
        for (unsigned i = 0; i < temperature.size(); ++ i) {
            T[i] = temperature[i];
            u[i] = curU;

            if (i >= temperature.size() - 1)
                break;

            // integrate to the heat capacity from the current sampling point to the next
            // one. this leads to a quadratic polynomial.
            Scalar c_v0 = heatCapacity[i];
            Scalar c_v1 = heatCapacity[i + 1];
            Scalar T0 = temperature[i];
            Scalar T1 = temperature[i + 1];
            curU += 0.5*(c_v0 + c_v1)*(T1 - T0);
        }

        internalEnergyFunction_.setXYContainers(T, u);
    }

    /*!
    * \brief Set the sample tables directly. Marks the object as finalized.
    *
    * This compatibility helper is kept for callers that construct
    * SPECROCK parameters directly instead of from heat capacities.
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
        internalEnergyFunction_.setXYContainers(temperature, internalEnergy);
        EnsureFinalized::finalize();
    }

    OPM_HOST_DEVICE const InternalEnergyFunction& internalEnergyFunction() const
    { EnsureFinalized::check(); return internalEnergyFunction_; }

    const ValueVector& temperatureSamples() const
    { EnsureFinalized::check(); return internalEnergyFunction_.xValues(); }

    const ValueVector& internalEnergySamples() const
    { EnsureFinalized::check(); return internalEnergyFunction_.yValues(); }

private:
#if HAVE_CUDA
    template <class OtherScalarT>
    friend ::Opm::EclSpecrockLawParams<OtherScalarT, ::Opm::gpuistl::GpuBuffer>
    Opm::gpuistl::copy_to_gpu(const ::Opm::EclSpecrockLawParams<OtherScalarT>& cpu);

    template <class OtherScalarT, template <class> class ContainerT>
    friend ::Opm::EclSpecrockLawParams<OtherScalarT, ::Opm::gpuistl::GpuView>
    Opm::gpuistl::make_view(
        ::Opm::EclSpecrockLawParams<OtherScalarT, ContainerT>& gpuBuffers);
#endif

    InternalEnergyFunction internalEnergyFunction_ {};
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

template <class ScalarT>
::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>
copy_to_gpu(const ::Opm::EclSpecrockLawParams<ScalarT>& cpu)
{
    return ::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>(
        copy_to_gpu(cpu.internalEnergyFunction_));
}

template <class ScalarT, template <class> class ContainerT>
::Opm::EclSpecrockLawParams<ScalarT, GpuView>
make_view(::Opm::EclSpecrockLawParams<ScalarT, ContainerT>& gpuBuffers)
{
    return ::Opm::EclSpecrockLawParams<ScalarT, GpuView>(
        make_view(gpuBuffers.internalEnergyFunction_));
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif
