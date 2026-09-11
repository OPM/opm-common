/*
  Copyright 2024 Equinor ASA.

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

#ifndef OPM_GROUP_SATELLITE_PRODUCTION_HPP_INCLUDED
#define OPM_GROUP_SATELLITE_PRODUCTION_HPP_INCLUDED

#include <opm/input/eclipse/Deck/UDAValue.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>

namespace Opm {

    class SummaryState;

} // namespace Opm

namespace Opm {

    /// Satellite production rates for a single, named group.
    class GSatProd
    {
    public:
        /// Satellite production rate items.
        enum class Rate : std::size_t
        {
            /// Oil phase production rate.
            Oil,

            /// Gas phase production rate.
            Gas,

            /// Water phase production rate.
            Water,

            /// Reservoir voidage volume production rate.
            Resv,

            /// Gas lift production rate.
            GLift,

            // ---------------------------------------------------------------
            //
            // Bookkeping information.  No other enumerators below this
            // line.
            //
            // ---------------------------------------------------------------

            /// Number of enumerators.
            Num,
        };

        /// Container of satellite production rate values.
        ///
        /// Indexed by \c Rate.
        ///
        /// \tparam T Element type of the satellite production rate values.
        /// Typically \c double or \c UDAValue.
        template <typename T>
        class Values
        {
        private:
            /// Translate a \c Rate to a numeric index.
            ///
            /// Equivalent to C++23's \code std::to_underlying() \endcode.
            ///
            /// \param[in] r Descriptor for a particular satellite
            /// production rate.
            ///
            /// \return Numeric index corresponding to rate descriptor \p r.
            static constexpr auto index(const Rate r) noexcept
                -> std::underlying_type_t<Rate>
            {
                return static_cast<std::underlying_type_t<Rate>>(r);
            }

        public:
            /// Read-only access to individual rate item.
            ///
            /// \param[in] r Rate item descriptor.
            ///
            /// \returns Immutable rate value defined by descriptor \p r.
            [[nodiscard]] constexpr decltype(auto)
            operator[](const Rate r) const noexcept
            {
                return this->v_[index(r)];
            }

            /// Read/write access to individual rate item.
            ///
            /// \param[in] r Rate item descriptor.
            ///
            /// \returns Reference to mutable rate value defined by
            /// descriptor \p r.
            [[nodiscard]] constexpr decltype(auto)
            operator[](const Rate r) noexcept
            {
                return this->v_[index(r)];
            }

            /// Equality predicate.
            ///
            /// \param[in] that Object against which \code *this \endcode
            /// will be tested for equality.
            ///
            /// \return Whether or not \code *this \endcode is the same as
            /// \p that.
            bool operator==(const Values& that) const
            {
                return this->v_ == that.v_;
            }

            /// Convert between byte array and object representation.
            ///
            /// \tparam Serializer Byte array conversion protocol.
            ///
            /// \param[in,out] serializer Byte array conversion object.
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->v_);
            }

        private:
            /// Convenience count for sizing the rate value container.
            static constexpr auto NumComponents =
                static_cast<std::underlying_type_t<Rate>>(Rate::Num);

            /// Rate values.
            std::array<T, NumComponents> v_{};
        };

        /// Default constructor.
        ///
        /// Resulting object is mostly usable as a target for a
        /// serialisation operation.
        GSatProd() = default;

        /// Constructor.
        ///
        /// \param[in] group Name of group to which this collection of
        /// satellite production rates applies.  Mostly needed to meet API
        /// requirements of the ScheduleState::map_member<> class.
        explicit GSatProd(const std::string& group);

        /// Copy constructor.
        ///
        /// \param[in] rhs Source object.
        GSatProd(const GSatProd& rhs) = default;

        /// Move constructor.
        ///
        /// \param[in] rhs Source object.  Left in a valid but unspecificed
        /// state when constructor completes.
        GSatProd(GSatProd&& rhs) = default;

        /// Assignment operator.
        ///
        /// \param[in] rhs Source object.
        ///
        /// \return \code *this \endcode
        GSatProd& operator=(const GSatProd& rhs) = default;

        /// Move-assignment operator.
        ///
        /// \param[in] rhs Source object.  Left in a valid but unspecificed
        /// state when assignment function completes.
        ///
        /// \return \code *this \endcode
        GSatProd& operator=(GSatProd&& rhs) = default;

        /// Create a serialisation test object.
        static GSatProd serializationTestObject();

        /// Define satellite production rates for named group.
        ///
        /// Effectively internalises items from the GSATPROD keyword.
        ///
        /// \param[in] input Satellite production rates as defined by the
        /// GSATPROD keyword.
        void assign(const Values<UDAValue>& input);

        /// Retrieve name of group to which this collection applies.
        ///
        /// Needed to meet API requirements of ScheduleState::map_member<>.
        ///
        /// \return Name of group to which this collection of satellite
        /// production rates applies.
        const std::string& name() const noexcept { return this->group_; }

        /// Name of user-defined quantity backing a particular production rate.
        ///
        /// \param[in] r Rate descriptor.
        ///
        /// \return Name of the UDQ backing satellite production rate \p r.
        /// Nullopt if the production rate \p r is entered as a numeric
        /// value in the GSATPROD keyword.
        std::optional<std::string> udq(const Rate r) const;

        /// Retrieve single satellite production rate.
        ///
        /// \param[in] r Rate descriptor.
        ///
        /// \param[in] st Current summary vectors from which to exctract
        /// applicable UDQ values.
        ///
        /// \returns Current group's satellite production rate for
        /// descriptor \p r.
        double getRate(const Rate r, const SummaryState& st) const;

        /// Retrieve scalar satellite production rates for current group.
        ///
        /// Equivalent to calling getRate(r, st) for all \c Rate descriptors
        /// \c r.
        ///
        /// \param[in] st Summary state.
        ///
        /// \return All satellite production rates for the current group.
        Values<double> getRates(const SummaryState& st) const;

        /// Equality predicate.
        ///
        /// \param[in] that Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// that.
        bool operator==(const GSatProd& that) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->group_);
            serializer(this->rate_);
        }

    private:
        /// Group name.
        ///
        /// Needed to support ScheduleState::map_member<> type requirements
        /// and to simplify UDA evaluation.  Is otherwise mostly redundant.
        std::string group_{};

        /// Satellite production rates.
        ///
        /// One rate for each enumerated item.
        Values<UDAValue> rate_{};
    };

} // namespace Opm

#endif // OPM_GROUP_SATELLITE_PRODUCTION_HPP_INCLUDED
