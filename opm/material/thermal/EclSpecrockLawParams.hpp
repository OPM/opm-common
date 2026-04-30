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

#include <opm/common/utility/VectorWithDefaultAllocator.hpp>
#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>

#include <opm/material/common/EnsureFinalized.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

#include <cassert>
#include <utility>

namespace Opm {

/*!
 * \brief The default implementation of a parameter object for the
 *        ECL thermal law based on SPECROCK.
 *
 * The internal-energy table is stored in a templated \c Storage
 * container so the same class can be used both on the CPU
 * (\c VectorWithDefaultAllocator) and on the GPU (\c GpuBuffer for
 * owning device memory and \c GpuView for a non-owning view usable
 * from a kernel).
 */
template <class ScalarT, template <class> class Storage = ::Opm::VectorWithDefaultAllocator>
class EclSpecrockLawParams : public EnsureFinalized
{
    using InternalEnergyFunction = Tabulated1DFunction<ScalarT, Storage>;

public:
    using Scalar = ScalarT;

    OPM_HOST_DEVICE EclSpecrockLawParams() = default;

    OPM_HOST_DEVICE explicit EclSpecrockLawParams(InternalEnergyFunction internalEnergyFunction)
        : internalEnergyFunction_(std::move(internalEnergyFunction))
    {
        EnsureFinalized::finalize();
    }

    /*!
     * \brief Construct directly from temperature and internal-energy storage
     *        containers (used by the GPU thermal-law manager).
     */
    OPM_HOST_DEVICE EclSpecrockLawParams(Storage<Scalar> temperatureSamples,
                                         Storage<Scalar> internalEnergySamples)
        : internalEnergyFunction_(std::move(temperatureSamples),
                                  std::move(internalEnergySamples))
    {
        EnsureFinalized::finalize();
    }

    /*!
     * \brief Specify the volumetric internal energy of rock via heat capacities.
     */
    template <class Container>
    void setHeatCapacities(const Container& temperature,
                           const Container& heatCapacity)
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
     * \brief Set the temperature/internal-energy sample table directly. Marks
     *        the object as finalized.
     */
    template <class Container>
    void setSamples(const Container& temperature, const Container& internalEnergy)
    {
        internalEnergyFunction_.setXYContainers(temperature, internalEnergy);
        EnsureFinalized::finalize();
    }

    /*!
     * \brief Return the function which maps temparature to the rock's volumetric
     *        internal energy
     *
     * Currently we assume this function to be piecewise linear. (Assuming piecewise
     * linear heat capacity, the real function is quadratic, but the difference should be
     * negligible.)
     */
    OPM_HOST_DEVICE const InternalEnergyFunction& internalEnergyFunction() const
    { EnsureFinalized::check(); return internalEnergyFunction_; }

    InternalEnergyFunction& internalEnergyFunctionMutable()
    { return internalEnergyFunction_; }

    /*! \brief Convenience accessors for the per-region sample tables used by
     *         the GPU thermal-law manager. */
    OPM_HOST_DEVICE const auto& temperatureSamples() const
    { return internalEnergyFunction_.xValues(); }

    OPM_HOST_DEVICE const auto& internalEnergySamples() const
    { return internalEnergyFunction_.yValues(); }

private:
    InternalEnergyFunction internalEnergyFunction_;
};

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

template <class ScalarT>
::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>
copy_to_gpu(const ::Opm::EclSpecrockLawParams<ScalarT>& cpu)
{
    using GpuFunction = ::Opm::Tabulated1DFunction<ScalarT, GpuBuffer>;
    return ::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>(
        GpuFunction(GpuBuffer<ScalarT>(cpu.temperatureSamples()),
                    GpuBuffer<ScalarT>(cpu.internalEnergySamples())));
}

template <class ScalarT>
::Opm::EclSpecrockLawParams<ScalarT, GpuView>
make_view(::Opm::EclSpecrockLawParams<ScalarT, GpuBuffer>& gpuBuffers)
{
    auto& f = gpuBuffers.internalEnergyFunctionMutable();
    using ViewFunction = ::Opm::Tabulated1DFunction<ScalarT, GpuView>;
    return ::Opm::EclSpecrockLawParams<ScalarT, GpuView>(
        ViewFunction(GpuView<ScalarT>(f.xValuesMutable().data(), f.xValuesMutable().size()),
                     GpuView<ScalarT>(f.yValuesMutable().data(), f.yValuesMutable().size())));
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif
