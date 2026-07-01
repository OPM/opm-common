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

#include <opm/input/eclipse/Schedule/Action/Actdims.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>

#include <fmt/format.h>

#include <cstddef>
#include <limits>
#include <stdexcept>

namespace Opm {

Actdims::Actdims()
    : keywords_   { ParserKeywords::ACTDIMS::MAX_ACTION::defaultValue }
    , line_count_ { ParserKeywords::ACTDIMS::MAX_ACTION_LINES::defaultValue }
    , characters_ { ParserKeywords::ACTDIMS::MAX_ACTION_LINE_CHARACTERS::defaultValue }
    , conditions_ { ParserKeywords::ACTDIMS::MAX_ACTION_COND::defaultValue }
{}

Actdims::Actdims(const Deck& deck)
    : Actdims{}
{
    if (!deck.hasKeyword<ParserKeywords::ACTDIMS>()) {
        return;
    }

    const auto& keyword = deck.get<ParserKeywords::ACTDIMS>().back();
    const auto& record = keyword.getRecord(0);

    if (const auto max_actions = record.getItem<ParserKeywords::ACTDIMS::MAX_ACTION>().get<int>(0);
        max_actions < 0)
    {
        throw std::runtime_error {
            "Maximum number of actions in ACTDIMS must be non-negative."
        };
    }
    else if (max_actions > 10'000) {
        throw std::runtime_error {
            "Value for MAX_ACTION in ACTDIMS exceeds maximum permissible value 10000."
        };
    }
    else {
        this->keywords_ = static_cast<std::size_t>(max_actions);
    }

    if (const auto max_lines = record.getItem<ParserKeywords::ACTDIMS::MAX_ACTION_LINES>().get<int>(0);
        max_lines < 0)
    {
        throw std::runtime_error {
            "Maximum number of keyword lines in "
            "ACTDIMS must be non-negative."
        };
    }
    else if (max_lines > 10'000) {
        throw std::runtime_error {
            "Maximum number of keyword lines in "
            "ACTDIMS exceeds maximum permissible value 10000."
        };
    }
    else {
        this->line_count_ = static_cast<std::size_t>(max_lines);
    }

    if (const auto max_chars = record.getItem<ParserKeywords::ACTDIMS::MAX_ACTION_LINE_CHARACTERS>().get<int>(0);
        max_chars <= 0)
    {
        throw std::runtime_error {
            "Maximum number of characters per keyword line "
            "in ACTDIMS must be positive."
        };
    }
    else if (max_chars > 128) {
        throw std::runtime_error {
            "Maximum number of characters per keyword line in "
            "ACTDIMS exceeds maximum permissible value 128."
        };
    }
    else {
        this->characters_ = static_cast<std::size_t>(max_chars);
    }

    if (const auto max_conditions = record.getItem<ParserKeywords::ACTDIMS::MAX_ACTION_COND>().get<int>(0);
        max_conditions <= 0)
    {
        throw std::runtime_error {
            "Maximum number of conditions in ACTDIMS must be positive."
        };
    }
    else if (max_conditions > 10'000) {
        throw std::runtime_error {
            "Maximum number of conditions in ACTDIMS exceeds maximum permissible value 10000."
        };
    }
    else {
        this->conditions_ = static_cast<std::size_t>(max_conditions);
    }
}

Actdims Actdims::serializationTestObject()
{
    Actdims result{};

    result.keywords_ = 1;
    result.line_count_ = 2;
    result.characters_ = 3;
    result.conditions_ = 4;

    return result;
}

std::size_t Actdims::line_size() const
{
    constexpr std::size_t chars_per_string = 8;
    constexpr std::size_t max_chars_per_line =
        std::numeric_limits<std::size_t>::max() - (chars_per_string - 1);

    const std::size_t total_chars = this->max_characters();

    // Guard against overflow: if total_chars is close to SIZE_MAX,
    // adding chars_per_string - 1 would wrap around.
    if (total_chars > max_chars_per_line) {
        throw std::overflow_error {
            fmt::format("Overflow in Actdims::line_size(): "
                        "max_characters() ({:#x}) is too large.",
                        total_chars)
        };
    }

    // Divide by chars_per_string, rounding up to the nearest whole integer
    return (total_chars + chars_per_string - 1) / chars_per_string;
}

bool Actdims::operator==(const Actdims& data) const
{
    return (this->max_keywords() == data.max_keywords())
        && (this->max_line_count() == data.max_line_count())
        && (this->max_characters() == data.max_characters())
        && (this->max_conditions() == data.max_conditions())
        ;
}

} // namespace Opm
