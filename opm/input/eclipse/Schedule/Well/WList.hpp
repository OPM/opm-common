/*
  Copyright 2019 Equinor ASA.

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
#ifndef WLIST_HPP
#define WLIST_HPP

#include <cstddef>
#include <string>
#include <vector>

/// \file Interface of a single well list.

namespace Opm {

/// Named sequence of wells.
class WList
{
public:
    /// Default constructor.
    ///
    /// Creates an object that is mostly useful as a target for a
    /// deserialisation operation.  May nevertheless be populated through
    /// the add() or del() member functions.
    WList() = default;

    /// Constructor.
    ///
    /// \param[in] wlist Initial collection of wells for this well list.
    ///
    /// \param[in] wlname Well list name.
    WList(const std::vector<std::string>& wlist, const std::string& wlname);

    /// Number of wells in this well list.
    std::size_t size() const;

    /// Predicate for an empty well list.
    bool empty() const { return this->size() == 0; }

    /// Remove all wells from this well list.
    void clear();

    /// Add named well to this well list.
    ///
    /// No change if the well already exists in the current well list.
    ///
    /// \param[in] well Well name.
    void add(const std::string& well);

    /// Remove named well from this well list.
    ///
    /// No change if the well is not on the current list.
    ///
    /// \param[in] well Well name.
    void del(const std::string& well);

    /// Whether or not named well is on the current list.
    ///
    /// \param[in] well Well name.
    ///
    /// \return Whether or not \p well is on the current list.
    bool has(const std::string& well) const;

    /// Retrieve name of current well list.
    ///
    /// Returns the \c wlname constructor argument.
    const std::string& getName() const { return this->name; }

    /// Sequence of named wells on current well list.
    const std::vector<std::string>& wells() const;

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const WList& data) const;

    /// Inequality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will be
    /// tested for inequality.
    ///
    /// \return Whether or not \code *this \endcode is different from \p that.
    bool operator!=(const WList& that) const
    {
        return ! (*this == that);
    }

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(well_list);
        serializer(name);
    }

private:
    /// Named wells currently on this well list.
    std::vector<std::string> well_list;

    /// Well list name.
    std::string name;
};

} // namespace Opm

#endif // WLIST_HPP
