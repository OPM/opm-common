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

#ifndef ACTDIMS_HPP_
#define ACTDIMS_HPP_

#include <cstddef>

namespace Opm {
class Deck;
} // namespace Opm

namespace Opm {

/// Class representing the ACTDIMS keyword in an Eclipse deck.
///
/// The ACTDIMS keyword specifies the maximum number of actions, lines,
/// characters, and conditions that can be present in the deck.  This class
/// provides methods to access these values and to perform related calculations.
class Actdims
{
public:
    /// Default constructor.
    ///
    /// Initializes the Actdims object with default values for the maximum
    /// number of actions, lines, characters, and conditions.
    Actdims();

    /// Constructor that initializes the Actdims object from a Deck.
    ///
    /// \param[in] deck The Deck object from which to extract the ACTDIMS
    /// keyword.  Default values used if the keyword is not present.
    explicit Actdims(const Deck& deck);

    /// Create a serialization test object.
    ///
    /// \return An Actdims object with test values for serialization.
    static Actdims serializationTestObject();

    /// Get the maximum number of ACTIONX keywords in the deck.
    ///
    /// \return The maximum number of ACTIONX keywords.
    std::size_t max_keywords() const
    {
        return this->keywords_;
    }

    /// Get the maximum number of lines in a single action block.
    ///
    /// \return The maximum number of lines in a single action block.
    std::size_t max_line_count() const
    {
        return this->line_count_;
    }

    /// Get the maximum number of characters in a single line.
    ///
    /// \return The maximum number of characters in a single line.
    std::size_t max_characters() const
    {
        return this->characters_;
    }

    /// Get the maximum number of conditions in a single action.
    ///
    /// \return The maximum number of conditions in a single action.
    std::size_t max_conditions() const
    {
        return this->conditions_;
    }

    /// Get the maximum number of characters in a single line of
    /// action block keywords, rounded up to the nearest multiple of 8.
    /// This is used to determine the number of 8-character strings
    /// needed to store a line of action block keywords in the restart file.
    ///
    /// \details The number of 8-character strings is calculated as
    /// (max_characters + 7) / 8, which effectively rounds up to the
    /// nearest multiple of 8.
    ///
    /// \return The maximum size of a line in terms of 8-character strings.
    std::size_t line_size() const;

    /// Equality operator.
    ///
    /// \param[in] data The Actdims object to compare with.
    ///
    /// \return True if the objects are equal, false otherwise.
    bool operator==(const Actdims& data) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->keywords_);
        serializer(this->line_count_);
        serializer(this->characters_);
        serializer(this->conditions_);
    }

private:
    /// Maximum number of actions in a single deck.
    std::size_t keywords_{};

    /// Maximum number of lines in a single action.
    std::size_t line_count_{};

    /// Maximum number of characters in a single line.
    std::size_t characters_{};

    /// Maximum number of conditions in a single action.
    std::size_t conditions_{};
};

} // namespace Opm

#endif // ACTDIMS_HPP_
