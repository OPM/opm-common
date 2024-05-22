/*
  Copyright 2023 Equinor ASA.

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

#ifndef PAVE_DYNAMIC_SOURCE_DATA_HPP
#define PAVE_DYNAMIC_SOURCE_DATA_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Opm {

/// Dynamic source data for block-average pressure calculations
template <typename Scalar>
class PAvgDynamicSourceData
{
public:
    /// Ad hoc implementation of fixed-width span/view of an underlying
    /// contiguous range of elements.
    ///
    /// \tparam T Element type.  Const or non-const as needed.  Typically \c
    ///   double or \code const double \endcode.
    template<typename T>
    class SourceDataSpan
    {
    private:
        friend class PAvgDynamicSourceData<Scalar>;
    public:
        /// Supported items of dynamic data per source location
        enum class Item
        {
            Pressure,           //< Dynamic pressure value
            MixtureDensity,     //< Dynamic mixture density
            PoreVol,            //< Dynamic pore volume

            // ----------------------------------------

            Last_Do_Not_Use, //< Simplifies item count
        };

        using ElmT = std::remove_cv_t<T>;

        /// Read-only access to numerical value of specified item
        ///
        /// \param[in] i Item of dynamic source data.
        /// \return Numerical value of specified item.
        [[nodiscard]] constexpr ElmT operator[](const Item i) const
        {
            return this->begin_[this->index(i)];
        }

        /// Assign specified item
        ///
        /// Availble only if underlying range is non-const.
        ///
        /// \param[in] i Item of dynamic source data.
        /// \param[in] value Numerical value of specified item.
        /// \return \code *this \endcode to enable chaining.
        template <typename Ret = SourceDataSpan&>
        constexpr std::enable_if_t<! std::is_const_v<T>, Ret>
        set(const Item i, const ElmT value)
        {
            this->begin_[this->index(i)] = value;
            return *this;
        }

        /// Assign all items
        ///
        /// Availble only if underlying range is non-const.
        ///
        /// \param[in] i Item of dynamic source data.
        /// \param[in] value Numerical value of specified item.
        /// \return \code *this \endcode to enable chaining.
        template <typename U, typename Ret = SourceDataSpan&>
        constexpr std::enable_if_t<! std::is_const_v<T>, Ret>
        operator=(const SourceDataSpan<U> src)
        {
            std::copy_n(src.begin_, NumItems, this->begin_);
            return *this;
        }

    private:
        /// Number of data items per source location
        static constexpr auto NumItems =
            static_cast<std::size_t>(Item::Last_Do_Not_Use);

        /// Beginning of items associated to a single source location.
        T* begin_{nullptr};

        /// Constructor.
        ///
        /// \param[in] begin Beginning of items associated to a single
        ///   source location.
        explicit SourceDataSpan(T* begin)
            : begin_{begin}
        {}

        /// Translate item into linear index
        ///
        /// \param[in] i Item of dynamic source data
        /// \return Linear index into span corresponding to specified item.
        constexpr std::size_t index(const Item i) const
        {
            const auto ix = static_cast<std::size_t>(i);
            if (ix >= NumItems) {
                throw std::invalid_argument {
                    "Index out of bounds"
                };
            }

            return ix;
        }
    };

    /// Constructor
    ///
    /// \param[in] sourceLocations Known locations, typically linearised
    ///   global call IDs, for which to enable collecting/reporting dynamic
    ///   source data.
    explicit PAvgDynamicSourceData(const std::vector<std::size_t>& sourceLocations);

    /// Destructor
    ///
    /// Marked virtual because this type is intended for inheritance.
    virtual ~PAvgDynamicSourceData() {}

    /// Acquire read/write span of data items corresponding to a single
    /// source location.
    ///
    /// Mostly intended for assigning values.
    ///
    /// \param[in] source Source location.  Function will \c throw if \p
    ///   source is not one of the known locations registered in the object
    ///   constructor.
    ///
    /// \return Read/write span of data items.
    [[nodiscard]] SourceDataSpan<Scalar>
    operator[](const std::size_t source);

    /// Acquire read-only span of data items corresponding to a single
    /// source location.
    ///
    /// Intended for extracting previously assigned data items.
    ///
    /// \param[in] source Source location.  Function will \c throw if \p
    ///   source is not one of the known locations registered in the object
    ///   constructor.
    ///
    /// \return Read-only span of data items.
    [[nodiscard]] SourceDataSpan<const Scalar>
    operator[](const std::size_t source) const;

protected:
    /// Contiguous array of data items for all source locations.
    ///
    /// Intentionally accessible to derived classes for use in parallel
    /// runs.
    std::vector<Scalar> src_{};

    /// Form mutable data span into non-default backing store.
    ///
    /// Mainly intended for constructing span objects in backing store for
    /// local (on-rank) sources in parallel runs.
    ///
    /// \param[in] ix Logical element index into source term backing store.
    ///
    /// \param[in,out] src Source term backing store.
    ///
    /// \return Mutable view into \p src.
    [[nodiscard]] SourceDataSpan<Scalar>
    sourceTerm(const std::size_t ix, std::vector<Scalar>& src);

    /// Reconstruct Source Data backing storage and internal mapping tables
    ///
    /// Effectively replaces the original object formed by the constructor.
    /// Mainly intended for updating objects as new wells and/or new
    /// reservoir connections are introduced.
    ///
    /// \param[in] sourceLocations Known locations, typically linearised
    ///   global call IDs, for which to enable collecting/reporting dynamic
    ///   source data.
    void reconstruct(const std::vector<std::size_t>& sourceLocations);

    /// Provide number of span items using function syntax
    ///
    /// Marked 'protected' because derived classes might need this
    /// information too.
    ///
    /// \return Number of span items.
    static constexpr std::size_t numSpanItems() noexcept
    {
        return SourceDataSpan<const Scalar>::NumItems;
    }

private:
    /// Translate non-contiguous source locations to starting indices in \c
    /// src_.
    std::unordered_map<std::size_t, typename std::vector<Scalar>::size_type> ix_{};

    /// Form source location to index translation table.
    ///
    /// \param[in] sourceLocations Known locations, typically linearised
    ///   global call IDs, for which to enable collecting/reporting dynamic
    ///   source data.
    ///
    /// \return Whether or not table formation succeeded.  Typical reasons
    ///   for failure is repeated source locations.
    void buildLocationMapping(const std::vector<std::size_t>& sourceLocations);

    /// Translate source location to starting index in \c src_.
    ///
    /// \param[in] source Source location.
    ///
    /// \return Starting index.  Nullopt if no index exists for \p source.
    [[nodiscard]] std::optional<typename std::vector<Scalar>::size_type>
    index(const std::size_t source) const;

    /// Translate element index into storage index.
    ///
    /// This is a customisation point to simplify usage in parallel contexts.
    ///
    /// Default implementation, identity mapping.
    ///
    /// \param[in] elemIndex Source element index.
    ///
    /// \return Storage (starting) index in \c src_.
    [[nodiscard]] virtual typename std::vector<Scalar>::size_type
    storageIndex(typename std::vector<Scalar>::size_type elemIndex) const
    {
        return elemIndex;
    }
};

} // namespace Opm

#endif // PAVE_DYNAMIC_SOURCE_DATA_HPP
