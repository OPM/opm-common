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

#include <opm/input/eclipse/Schedule/Well/ConnectionOrdering.hpp>

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include <string>
#include <unordered_map>

bool Opm::ConnectionOrdering::update(const Connection::Order order,
                                     const std::string&      wellName)
{
    switch (order) {
    case Connection::Order::TRACK:
        return this->setTrackOrder(wellName);

    case Connection::Order::INPUT:
    case Connection::Order::DEPTH:
        return this->setNonDefaultOrder(order, wellName);
    }

    return false;
}

Opm::Connection::Order
Opm::ConnectionOrdering::getConnectionOrder(const std::string& wellName) const
{
    const auto pos = this->nonDefaultOrder_.find(wellName);

    return (pos == this->nonDefaultOrder_.end())
        ? Connection::Order::TRACK : pos->second;
}

Opm::ConnectionOrdering Opm::ConnectionOrdering::serializationTestObject()
{
    auto obj = ConnectionOrdering{};

    // Add some non-default orders for serialization testing.
    obj.update(Connection::Order::INPUT, "W1-H");
    obj.update(Connection::Order::DEPTH, "PROD");
    obj.update(Connection::Order::TRACK, "INJ");
    obj.update(Connection::Order::DEPTH, "RFT-1729");
    obj.update(Connection::Order::INPUT, "WL2");

    return obj;
}

bool Opm::ConnectionOrdering::operator==(const ConnectionOrdering& that) const noexcept
{
    return this->nonDefaultOrder_ == that.nonDefaultOrder_;
}

// ==========================================================================
// Private member functions below separator
// ==========================================================================

bool Opm::ConnectionOrdering::setTrackOrder(const std::string& wellName)
{
    return this->nonDefaultOrder_.erase(wellName) > 0;
}

bool Opm::ConnectionOrdering::setNonDefaultOrder(const Connection::Order order,
                                                 const std::string&      wellName)
{
    const auto [pos, inserted] = this->nonDefaultOrder_
        .try_emplace(wellName, order);

    if (!inserted && (pos->second != order)) {
        pos->second = order;
        return true;
    }

    return inserted;
}
