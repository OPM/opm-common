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

#ifndef GSATPROD_H
#define GSATPROD_H

#include <map>
#include <string>
#include <array>

namespace Opm {

    /// Group level satellite production
    class GSatProd
    {
    public:
        /// Satellite production rates for a single group.
        struct GSatProdGroup
        {
            /// Satellite production rate items
            enum Rate { Oil, Gas, Water, Resv, GLift, };

            /// Satellite production rates.
            ///
            /// One rate for each enumerated item.
            std::array<double, 5> rate{};

            /// Equality predicate
            ///
            /// \param[in] data Object against which \code *this \endcode
            /// will be tested for equality.
            ///
            /// \return Whether or not \code *this \endcode is the same as
            /// \p data.
            bool operator==(const GSatProdGroup& data) const
            {
                return rate == data.rate;
            }

            /// Convert between byte array and object representation.
            ///
            /// \tparam Serializer Byte array conversion protocol.
            ///
            /// \param[in,out] serializer Byte array conversion object.
            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(rate);
            }
        };

        /// Create a serialisation test object.
        static GSatProd serializationTestObject();

        /// Define satellite production rates for named group.
        ///
        /// Effectively internalises items from the GSATPROD keyword.
        ///
        /// \param[in] name Group name.
        ///
        /// \param[in] oil_rate Satellite production oil surface rate for
        /// group \p name.
        ///
        /// \param[in] gas_rate Satellite production gas surface rate for
        /// group \p name.
        ///
        /// \param[in] water_rate Satellite production water surface rate
        /// for group \p name.
        ///
        /// \param[in] resv_rate Satellite production reservoir voidage rate
        /// for group \p name.
        ///
        /// \param[in] glift_rate Satellite production gas lift rate for
        /// group \p name.
        void assign(const std::string& name,
                    const double       oil_rate,
                    const double       gas_rate,
                    const double       water_rate,
                    const double       resv_rate,
                    const double       glift_rate);

        /// Whether or not satellite production rates have been defined for
        /// a named group.
        ///
        /// \param[in] name Group name.
        ///
        /// \return Whether or not satellite production rates have been
        /// defined for group \p name.
        bool has(const std::string& name) const;

        /// Retrieve satellite production rates for named group.
        ///
        /// Throws an exception of type \code std::invalid_argument \endcode
        /// if there are no satellite production rates defined for that
        /// named group.  Clients should invoke the predicate has() to
        /// determine availability of satellite production rates for a
        /// particular named group.
        ///
        /// \param[in] name Group name.
        ///
        /// \return Satellite production rates for group \p name.
        const GSatProdGroup& get(const std::string& name) const;

        /// Number of groups for which satellite production rates have been
        /// defined.
        std::size_t size() const;

        /// Equality predicate
        ///
        /// \param[in] data Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// data.
        bool operator==(const GSatProd& data) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(groups_);
        }

    private:
        /// Satellite production rates for all pertinent groups.
        std::map<std::string, GSatProdGroup> groups_;
    };

}

#endif
