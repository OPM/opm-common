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

#ifndef OPM_CONNECTIONORDERING_HPP_INCLUDED
#define OPM_CONNECTIONORDERING_HPP_INCLUDED

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include <string>
#include <unordered_map>

namespace Opm {

    /// Manages the ordering of well connections based on specified well names.
    ///
    /// Retrieving the connection ordering method for a particular well is
    /// based on exact string matching for well names.  The typical use case
    /// for this class is internalising the information in the SCHEDULE
    /// section keyword COMPORD.  When employed in that situation, client
    /// code is expected to fully expand well templates (e.g., 'PROD*'),
    /// well lists (e.g., '*PR'), or well list templates (e.g., '*PR*') to
    /// a specific collection of well names and to call the update() member
    /// function for each of these well names.
    class ConnectionOrdering
    {
    public:
        /// \brief Default constructor.
        ConnectionOrdering() = default;

        /// \brief Default destructor.
        ~ConnectionOrdering() = default;

        /// \brief Copy constructor.
        ///
        /// \param[in] rhs Source object.
        ConnectionOrdering(const ConnectionOrdering& rhs) = default;

        /// \brief Assignment operator.
        ///
        /// \param[in] rhs Source object.
        ///
        /// \return Reference to this object.
        ConnectionOrdering& operator=(const ConnectionOrdering& rhs) = default;

        /// \brief Move constructor.
        ///
        /// \param[in] rhs Source object.  Left in a valid but unspecified state on exit.
        ConnectionOrdering(ConnectionOrdering&& rhs) = default;

        /// \brief Move assignment operator.
        ///
        /// \param[in] rhs Source object.  Left in a valid but unspecified state on exit.
        ///
        /// \return Reference to this object.
        ConnectionOrdering& operator=(ConnectionOrdering&& rhs) = default;

        /// \brief Registers a connection order for a particular named well.
        ///
        /// The typical source of the input data is the SCHEDULE section COMPORD keyword.
        ///
        /// \param[in] order The connection order to set.
        ///
        /// \param[in] wellName Name of individual well (e.g., "PROD-1").
        ///
        /// \return True if the input resulted in an update, false otherwise.
        bool update(Connection::Order order, const std::string& wellName);

        /// \brief Determines the connection order for a specific well based on the
        /// registered well names.
        ///
        /// \note If no registered well name matches, the default connection order
        /// (TRACK) is used.
        ///
        /// \param[in] wellName Name of specific well.
        ///
        /// \return The connection order for the specified, named well.
        Connection::Order getConnectionOrder(const std::string& wellName) const;

        /// \brief Creates a test object for serialization purposes.
        ///
        /// \return A ConnectionOrdering object suitable for serialization testing.
        static ConnectionOrdering serializationTestObject();

        /// \brief Equality predicate.
        ///
        /// \param[in] that The object to compare with.
        ///
        /// \return Whether or not \c *this is the same as \c that.
        bool operator==(const ConnectionOrdering& that) const noexcept;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template <typename Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->nonDefaultOrder_);
        }

    private:
        /// Wells whose connection order is not the default (TRACK).
        ///
        /// Wells with TRACK ordering are never stored, so an empty map
        /// means every well uses the default order.
        std::unordered_map<std::string, Connection::Order> nonDefaultOrder_{};

        /// \brief Sets the connection order for a well to the default (TRACK).
        ///
        /// \param[in] wellName Name of the well to set to TRACK order.
        ///
        /// \return True if the order was updated, false otherwise.
        bool setTrackOrder(const std::string& wellName);

        /// \brief Sets the connection order for a well to a non-default value.
        ///
        /// \param[in] order The connection order to set.
        ///
        /// \param[in] wellName Name of the well to set the non-default order for.
        ///
        /// \return True if the order was updated, false otherwise.
        bool setNonDefaultOrder(Connection::Order  order,
                                const std::string& wellName);
    };

} // namespace Opm

#endif // OPM_CONNECTIONORDERING_HPP_INCLUDED
