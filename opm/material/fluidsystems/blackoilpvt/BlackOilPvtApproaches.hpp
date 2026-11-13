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
 * \brief Compile-time choice of the PVT classes held by the black-oil fluid system.
 *
 * Introduced for the GPU fluid system, which cannot hold the PVT multiplexers, but an
 * explicit choice works for any storage, also on the CPU.
 */
#ifndef OPM_BLACK_OIL_PVT_APPROACHES_HPP
#define OPM_BLACK_OIL_PVT_APPROACHES_HPP

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/utility/VectorWithDefaultAllocator.hpp>
#include <opm/common/utility/gpuistl_if_available.hpp>

#include <opm/material/fluidsystems/blackoilpvt/GasPvtMultiplexer.hpp>
#include <opm/material/fluidsystems/blackoilpvt/NullOilPvt.hpp>
#include <opm/material/fluidsystems/blackoilpvt/OilPvtMultiplexer.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>

#include <fmt/format.h>

#include <string_view>
#include <type_traits>
#include <utility>

namespace Opm {

/*!
 * \brief The fluid system holds the PVT multiplexers, which choose the PVT class of each
 *        phase from the deck at runtime.
 *
 * The default of the fluid system. Only supported with CPU storage.
 */
struct MultiplexerPvtApproaches {};

/*!
 * \brief Compile-time choice of the PVT approach for each phase of the fluid system.
 *
 * Each phase holds the one concrete PVT class named here, for any storage, see
 * BlackOilPvtTypes. Required for GPU storage.
 */
template <GasPvtApproach gasV, OilPvtApproach oilV, WaterPvtApproach waterV>
struct BlackOilPvtApproaches
{
    static constexpr GasPvtApproach gas = gasV;
    static constexpr OilPvtApproach oil = oilV;
    static constexpr WaterPvtApproach water = waterV;
};

//! The CO2STORE configuration (gas-water).
using Co2StorePvtApproaches = BlackOilPvtApproaches<GasPvtApproach::Co2Gas,
                                                    OilPvtApproach::NoOil,
                                                    WaterPvtApproach::BrineCo2>;

//! Live oil with dry gas: dissolved gas, no vaporized oil (PVTO, PVDG and PVTW). Used by
//! for example SPE1 and SPE9.
using LiveOilDryGasPvtApproaches = BlackOilPvtApproaches<GasPvtApproach::DryGas,
                                                         OilPvtApproach::LiveOil,
                                                         WaterPvtApproach::ConstantCompressibilityWater>;

//! Live oil with wet gas: dissolved gas and vaporized oil (PVTO, PVTG and PVTW). Used by
//! for example Norne.
using LiveOilWetGasPvtApproaches = BlackOilPvtApproaches<GasPvtApproach::WetGas,
                                                         OilPvtApproach::LiveOil,
                                                         WaterPvtApproach::ConstantCompressibilityWater>;

namespace detail {

template <class>
inline constexpr bool alwaysFalse = false;

//! The CPU PVT class that the gas multiplexer holds for the given approach.
template <class Scalar, GasPvtApproach approach>
struct CpuGasPvt
{
    static_assert(approach != GasPvtApproach::NoGas,
                  "NoGas is not supported by BlackOilPvtApproaches (there is no null gas PVT class).");
    using type = std::remove_cvref_t<decltype(std::declval<const GasPvtMultiplexer<Scalar>&>()
                                                  .template getRealPvt<approach>())>;
};

//! The CPU PVT class that the oil multiplexer holds for the given approach.
template <class Scalar, OilPvtApproach approach>
struct CpuOilPvt
{
    using type = std::remove_cvref_t<decltype(std::declval<const OilPvtMultiplexer<Scalar>&>()
                                                  .template getRealPvt<approach>())>;
};

//! An inactive oil phase is represented by NullOilPvt, for all storages.
template <class Scalar>
struct CpuOilPvt<Scalar, OilPvtApproach::NoOil>
{
    using type = NullOilPvt<Scalar>;
};

//! The CPU PVT class that the water multiplexer holds for the given approach.
template <class Scalar, WaterPvtApproach approach>
struct CpuWaterPvt
{
    static_assert(approach != WaterPvtApproach::NoWater,
                  "NoWater is not supported by BlackOilPvtApproaches (there is no null water PVT class).");
    using type = std::remove_cvref_t<decltype(std::declval<const WaterPvtMultiplexer<Scalar>&>()
                                                  .template getRealPvt<approach>())>;
};

/*!
 * \brief Maps a CPU PVT class to its type for the given storage.
 *
 * The PVT classes do not share one template signature (some take one storage parameter,
 * others two), so the GPU types are taken from what gpuistl::copy_to_gpu() and
 * gpuistl::make_view() return.
 *
 * Storage is compared through Storage<Scalar>, since VectorWithDefaultAllocator is an alias
 * template and cannot be matched by partial specialization.
 */
template <class CpuPvt, class Scalar, template <typename> typename Storage>
constexpr auto pvtTypeForStorage()
{
    if constexpr (std::is_same_v<Storage<Scalar>, VectorWithDefaultAllocator<Scalar>>) {
        return std::type_identity<CpuPvt>{};
    }
#if HAVE_CUDA
    else if constexpr (std::is_same_v<Storage<Scalar>, gpuistl::GpuBuffer<Scalar>>) {
        return std::type_identity<decltype(gpuistl::copy_to_gpu(std::declval<const CpuPvt&>()))>{};
    }
    else if constexpr (std::is_same_v<Storage<Scalar>, gpuistl::GpuView<Scalar>>) {
        using BufferPvt = decltype(gpuistl::copy_to_gpu(std::declval<const CpuPvt&>()));
        return std::type_identity<decltype(gpuistl::make_view(std::declval<BufferPvt&>()))>{};
    }
#endif // HAVE_CUDA
    else {
        static_assert(alwaysFalse<CpuPvt>,
                      "Unsupported storage for the black-oil PVT classes. Expected "
                      "VectorWithDefaultAllocator, gpuistl::GpuBuffer or gpuistl::GpuView.");
    }
}

template <class CpuPvt, class Scalar, template <typename> typename Storage>
using PvtTypeForStorage = typename decltype(pvtTypeForStorage<CpuPvt, Scalar, Storage>())::type;

} // namespace detail

/*!
 * \brief The PVT classes of the fluid system for a PVT approach combination and a storage.
 *
 * For a BlackOilPvtApproaches specialization these are the concrete classes: with
 * VectorWithDefaultAllocator the CPU classes, with gpuistl::GpuBuffer the owning GPU
 * objects returned by gpuistl::copy_to_gpu(), and with gpuistl::GpuView the views returned
 * by gpuistl::make_view(). For MultiplexerPvtApproaches they are the multiplexers.
 *
 * \tparam PvtApproaches MultiplexerPvtApproaches or a BlackOilPvtApproaches specialization.
 */
template <class Scalar, template <typename> typename Storage, class PvtApproaches>
struct BlackOilPvtTypes
{
    using GasPvt = detail::PvtTypeForStorage<typename detail::CpuGasPvt<Scalar, PvtApproaches::gas>::type,
                                             Scalar, Storage>;
    using OilPvt = detail::PvtTypeForStorage<typename detail::CpuOilPvt<Scalar, PvtApproaches::oil>::type,
                                             Scalar, Storage>;
    using WaterPvt = detail::PvtTypeForStorage<typename detail::CpuWaterPvt<Scalar, PvtApproaches::water>::type,
                                               Scalar, Storage>;
};

template <class Scalar, template <typename> typename Storage>
struct BlackOilPvtTypes<Scalar, Storage, MultiplexerPvtApproaches>
{
    static_assert(std::is_same_v<Storage<Scalar>, VectorWithDefaultAllocator<Scalar>>,
                  "The PVT multiplexers are CPU only. Use BlackOilPvtApproaches with GPU storage.");

    using GasPvt = GasPvtMultiplexer<Scalar>;
    using OilPvt = OilPvtMultiplexer<Scalar>;
    using WaterPvt = WaterPvtMultiplexer<Scalar>;
};

#if HAVE_CUDA
namespace gpuistl::detail {

constexpr bool isNoOil(OilPvtApproach approach) { return approach == OilPvtApproach::NoOil; }
constexpr bool isNoOil(GasPvtApproach) { return false; }
constexpr bool isNoOil(WaterPvtApproach) { return false; }

/*!
 * \brief Copy the PVT class of one phase from a multiplexer to the GPU.
 *
 * Throws if the multiplexer does not hold the requested approach. For OilPvtApproach::NoOil
 * the oil multiplexer must hold NoOil, which is the case when the oil phase is inactive, and
 * a NullOilPvt is returned.
 *
 * \tparam approach The approach the GPU fluid system is compiled for.
 * \param pvt The multiplexer of the CPU fluid system.
 * \param phaseName Used in the error message.
 */
template <auto approach, class Scalar, class Multiplexer>
auto copyPvtToGpu(const Multiplexer& pvt, std::string_view phaseName)
{
    OPM_ERROR_IF(pvt.approach() != approach,
                 fmt::format(fmt::runtime("Incompatible {} PVT approach for the GPU fluid system. "
                                          "Given {}, expected {}."),
                             phaseName, static_cast<int>(pvt.approach()), static_cast<int>(approach)));

    if constexpr (isNoOil(approach)) {
        // The oil multiplexer has no getRealPvt() for NoOil.
        return ::Opm::gpuistl::copy_to_gpu(NullOilPvt<Scalar>{});
    } else {
        return ::Opm::gpuistl::copy_to_gpu(pvt.template getRealPvt<approach>());
    }
}

} // namespace gpuistl::detail
#endif // HAVE_CUDA

} // namespace Opm

#endif // OPM_BLACK_OIL_PVT_APPROACHES_HPP
