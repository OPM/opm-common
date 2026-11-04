/*
  Copyright 2026 Equinor ASA.

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

#ifndef OPM_IO_BYTEORDER_HPP
#define OPM_IO_BYTEORDER_HPP

#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <ranges>
#include <span>
#include <vector>

/// \file
///
/// Conversion between the host's native byte order and the big-endian byte
/// order of numeric data in "unformatted" (binary) result files.

namespace Opm::EclIO {

    static_assert(CHAR_BIT == 8,
                  "Binary file I/O requires 8-bit bytes");

    static_assert((std::endian::native == std::endian::little) ||
                  (std::endian::native == std::endian::big),
                  "Binary file I/O does not support mixed-endian platforms");

    static_assert(sizeof(int) == 4,
                  "Binary file I/O represents INTE elements as 'int'");

    static_assert(std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<double>::is_iec559,
                  "Binary file I/O requires IEEE 754 floating-point types");

    /// Numeric types which have a direct representation as an element of
    /// a binary file array.
    template <typename T>
    concept FileScalar = (std::integral<T> || std::floating_point<T>)
        && ((sizeof(T) == 4) || (sizeof(T) == 8));

    namespace detail {

        template <std::size_t N> struct UnsignedOfSize;
        template <> struct UnsignedOfSize<4> { using type = std::uint32_t; };
        template <> struct UnsignedOfSize<8> { using type = std::uint64_t; };

        constexpr std::uint32_t byteswap(const std::uint32_t x) noexcept
        {
#if defined(__cpp_lib_byteswap)
            return std::byteswap(x);
#else
            // Mainstream compilers recognise this pattern and emit a single
            // byte swap instruction.
            return ((x & 0x000000FFu) << 24) | ((x & 0x0000FF00u) <<  8)
                |  ((x & 0x00FF0000u) >>  8) | ((x & 0xFF000000u) >> 24);
#endif
        }

        constexpr std::uint64_t byteswap(const std::uint64_t x) noexcept
        {
#if defined(__cpp_lib_byteswap)
            return std::byteswap(x);
#else
            return (std::uint64_t{byteswap(static_cast<std::uint32_t>(x))} << 32)
                | byteswap(static_cast<std::uint32_t>(x >> 32));
#endif
        }

    } // namespace detail

    /// Reverse the order of the bytes in the object representation of a
    /// numeric value, regardless of the host's native byte order.
    ///
    /// \param[in] value Numeric value.
    ///
    /// \return Value whose object representation is that of \p value with
    /// the bytes in reverse order.
    template <FileScalar T>
    constexpr T byteswap(const T value) noexcept
    {
        using U = typename detail::UnsignedOfSize<sizeof(T)>::type;
        return std::bit_cast<T>(detail::byteswap(std::bit_cast<U>(value)));
    }

    /// Convert a numeric value from the host's native byte order to the
    /// big-endian byte order used in binary files.  Identity
    /// operation on big-endian hosts.
    ///
    /// \param[in] value Numeric value in native byte order.
    ///
    /// \return Value whose object representation is \p value's big-endian
    /// representation.
    template <FileScalar T>
    constexpr T toFileOrder(const T value) noexcept
    {
        if constexpr (std::endian::native == std::endian::big) {
            return value;
        }
        else {
            return byteswap(value);
        }
    }

    /// Convert a numeric value from the big-endian byte order used in
    /// binary files to the host's native byte order.  Identity
    /// operation on big-endian hosts.
    ///
    /// \param[in] value Numeric value as read from a binary file.
    ///
    /// \return \p value in the host's native byte order.
    template <FileScalar T>
    constexpr T fromFileOrder(const T value) noexcept
    {
        // Byte swapping is an involution.
        return toFileOrder(value);
    }

    /// Contiguous sequence of numeric values that can be read from or
    /// written to a binary file in a single operation.
    template <typename R>
    concept FileScalarRange = std::ranges::contiguous_range<R>
        && std::ranges::sized_range<R>
        && FileScalar<std::ranges::range_value_t<R>>;

    /// Read a sequence of numeric values stored in big-endian byte order
    /// and convert them to native byte order.
    ///
    /// \param[in,out] is Input stream positioned at the first value.
    ///
    /// \param[out] values Destination of converted values, e.g., a
    /// std::vector<> or a std::span<>.  The number of values to read is
    /// \code std::ranges::size(values) \endcode.
    template <FileScalarRange R>
    void readFileOrder(std::istream& is, R&& values)
    {
        using T = std::ranges::range_value_t<R>;

        is.read(reinterpret_cast<char*>(std::ranges::data(values)),
                static_cast<std::streamsize>(std::ranges::size(values) * sizeof(T)));

        for (auto& value : values) {
            value = fromFileOrder(value);
        }
    }

    /// Read a single numeric value stored in big-endian byte order and
    /// convert it to native byte order.
    ///
    /// \tparam T Numeric type of value.
    ///
    /// \param[in,out] is Input stream positioned at the value.
    ///
    /// \return Value in native byte order.  Value-initialised if the read
    /// fails, in which case \p is's failure bits are set.
    template <FileScalar T>
    T readFileOrder(std::istream& is)
    {
        using U = typename detail::UnsignedOfSize<sizeof(T)>::type;

        U raw{};
        is.read(reinterpret_cast<char*>(&raw), sizeof raw);
        if (!is) {
            return T{};
        }

        return fromFileOrder(std::bit_cast<T>(raw));
    }

    /// Write a sequence of numeric values in big-endian byte order.
    ///
    /// \param[in,out] os Output stream.
    ///
    /// \param[in] values Numeric values in native byte order, e.g., a
    /// std::vector<> or a std::span<>.
    template <FileScalarRange R>
    void writeFileOrder(std::ostream& os, const R& values)
    {
        using T = std::ranges::range_value_t<R>;

        if constexpr (std::endian::native == std::endian::big) {
            os.write(reinterpret_cast<const char*>(std::ranges::data(values)),
                     static_cast<std::streamsize>(std::ranges::size(values) * sizeof(T)));
        }
        else {
            auto converted = std::vector<T>(std::ranges::begin(values),
                                            std::ranges::end(values));
            for (auto& value : converted) {
                value = toFileOrder(value);
            }

            os.write(reinterpret_cast<const char*>(converted.data()),
                     static_cast<std::streamsize>(converted.size() * sizeof(T)));
        }
    }

    /// Write a single numeric value in big-endian byte order.
    ///
    /// \param[in,out] os Output stream.
    ///
    /// \param[in] value Numeric value in native byte order.
    template <FileScalar T>
    void writeFileOrder(std::ostream& os, const T value)
    {
        const auto converted = toFileOrder(value);
        os.write(reinterpret_cast<const char*>(&converted), sizeof converted);
    }

} // namespace Opm::EclIO

#endif // OPM_IO_BYTEORDER_HPP
