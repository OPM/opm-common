/*
  Copyright 2026 Equinor ASA
  This file is part of the Open Porous Media project (OPM).
  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
/*!
 * \file
 *
 * \brief Helpers for copying per-region tables to the GPU.
 *
 * Classes that hold one table object per region (e.g. the PVT classes) cannot copy
 * their tables into device memory directly: each table owns GpuBuffers, which are
 * host-side handles. Instead, each table is uploaded to its own owning GPU table, kept
 * in a host-side std::vector, and views into these tables are uploaded as one GpuBuffer.
 *
 * The header of every table type used with these helpers must be included below. When a
 * new table type is ported to the GPU, add its header there.
 */
#ifndef OPM_GPU_TABLE_HELPERS_HPP
#define OPM_GPU_TABLE_HELPERS_HPP

#include <opm/common/utility/gpuistl_if_available.hpp>

// The copy_to_gpu() and make_view() overloads of the tables must be declared before the
// helpers below are defined, since copy_to_gpu() of a CPU table is not found by
// argument-dependent lookup.
#include <opm/material/common/Tabulated1DFunction.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>

#include <type_traits>
#include <vector>

#if HAVE_CUDA
namespace Opm::gpuistl {

/*!
 * \brief Copy each object in a host range to its own owning GPU object.
 *
 * \return A host-side std::vector of the owning GPU objects. It must outlive every view
 *         created from them, e.g. by make_view_buffer().
 */
template <class Range>
auto copy_each_to_gpu(const Range& cpuObjects)
{
    std::vector<decltype(copy_to_gpu(cpuObjects.front()))> ownedObjects;
    ownedObjects.reserve(cpuObjects.size());
    for (const auto& cpuObject : cpuObjects) {
        ownedObjects.push_back(copy_to_gpu(cpuObject));
    }
    return ownedObjects;
}

/*!
 * \brief Upload views into each owning GPU object as one GpuBuffer of views.
 *
 * \param ownedObjects A host range of owning GPU objects, e.g. from copy_each_to_gpu().
 *                     It must outlive the returned buffer.
 */
template <class Range>
auto make_view_buffer(Range& ownedObjects)
{
    using View = decltype(make_view(ownedObjects.front()));
    static_assert(std::is_trivially_copyable_v<View>,
                  "The views are copied to the GPU with memcpy.");

    std::vector<View> views;
    views.reserve(ownedObjects.size());
    for (auto& ownedObject : ownedObjects) {
        views.push_back(make_view(ownedObject));
    }
    return GpuBuffer<View>(views);
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif // OPM_GPU_TABLE_HELPERS_HPP
