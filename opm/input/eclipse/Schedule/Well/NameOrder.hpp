/*
  Copyright 2021 Equinor ASA.

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
#ifndef NAME_ORDER_HPP
#define NAME_ORDER_HPP

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

// The purpose of this small class is to ensure that well and group name
// always come in the order they are defined in the deck.

class NameOrder
{
public:
    NameOrder() = default;
    explicit NameOrder(std::initializer_list<std::string> names);
    explicit NameOrder(const std::vector<std::string>& names);

    void add(const std::string& name);
    std::vector<std::string> sort(std::vector<std::string> names) const;
    const std::vector<std::string>& names() const;
    bool has(const std::string& wname) const;

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_index_map);
        serializer(m_name_list);
    }

    static NameOrder serializationTestObject();

    const std::string& operator[](std::size_t index) const;
    bool operator==(const NameOrder& other) const;

    auto begin() const { return this->m_name_list.begin(); }
    auto end()   const { return this->m_name_list.end(); }
    auto size()  const { return this->m_name_list.size(); }

private:
    std::unordered_map<std::string, std::size_t> m_index_map;
    std::vector<std::string> m_name_list;
};

/// Collection of group names with built-in ordering
class GroupOrder
{
public:
    /// Default constructor
    ///
    /// Mainly useful as a target for object deserialisation.
    GroupOrder() = default;

    /// Constructor
    ///
    /// \param[in] max_groups Maximum number of non-FIELD groups in model.
    /// Typically taken from item 3 of the WELLDIMS keyword.
    explicit GroupOrder(std::size_t max_groups);

    /// Create a serialisation test object.
    static GroupOrder serializationTestObject();

    /// Add a group name to ordered collection.
    ///
    /// \param[in] name Group name.
    void add(const std::string& name);

    /// Retrieve current list of group names
    ///
    /// Includes the FIELD group, as the first element in the sequence (at
    /// index zero).
    const std::vector<std::string>& names() const
    {
        return this->name_list_;
    }

    /// Retrieve list of group names matching a pattern
    ///
    /// Regular wild-card matching only.
    ///
    /// \param[in] pattern Group name or group name template.
    ///
    /// \return List of unique group names matching \p pattern.
    std::vector<std::string> names(const std::string& pattern) const;

    /// Group name existence predicate.
    ///
    /// \param[in] gname Group name
    ///
    /// \return Whether or not \p gname exists in the current collection.
    bool has(const std::string& gname) const;

    /// Group name existence predicate.
    ///
    /// Pattern matching version.
    ///
    /// \param[in] pattern Group name or group name root.
    ///
    /// \return Whether or not any group in the current collection matches
    /// the \p pattern.
    bool anyGroupMatches(const std::string& pattern) const;

    /// Retrieve sequence of group names ordered appropriately for restart
    /// file output.
    ///
    /// Sized according to the naximum number of groups in the model, and
    /// the FIELD group placed last.  Nullopt in "unused" group name slots.
    std::vector<std::optional<std::string>> restart_groups() const;

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p other.
    bool operator==(const GroupOrder& other) const;

    /// Start of group name sequence.
    ///
    /// Includes FIELD group.
    auto begin() const { return this->name_list_.begin(); }

    /// End of group name sequence.
    auto end() const { return this->name_list_.end(); }

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->name_list_);
        serializer(this->max_groups_);
    }

private:
    /// Maximum number of non-FIELD groups in model.
    std::size_t max_groups_{};

    /// Current list of group names, in order of add() function call sequence.
    std::vector<std::string> name_list_{};
};

} // namespace Opm

#endif // NAME_ORDER_HPP
