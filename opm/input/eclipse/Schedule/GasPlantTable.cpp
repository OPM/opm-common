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

#include <opm/input/eclipse/Schedule/GasPlantTable.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <unordered_set>
#include <utility>

#include <fmt/format.h>

namespace Opm {

GasPlantTable::GasPlantTable(const DeckRecord& record,
                             const std::size_t numComponents,
                             const KeywordLocation& location)
    : m_num_components(numComponents)
{
    using ParserKeywords::GPTABLE;

    // numComponents is the RUNSPEC COMPS count; the parse sites guarantee a
    // compositional run before a table is constructed.
    assert(numComponents >= 1);

    const auto& table_num_item = record.getItem<GPTABLE::GAS_PLANT_TABLE_NUM>();
    if (table_num_item.defaultApplied(0)) {
        throw OpmInputError {
            "GPTABLE requires a gas plant table number (item 1); it cannot be defaulted.",
            location
        };
    }

    const auto table_num = table_num_item.get<int>(0);
    if (table_num < 1) {
        throw OpmInputError {
            fmt::format("GPTABLE gas plant table number must be positive, but got {}.",
                        table_num),
            location
        };
    }

    // Items 2 and 3 default to the last component (numComponents) when omitted.
    // The deck indices are 1-based component numbers, kept as int.
    const auto last_component = static_cast<int>(numComponents);
    const auto& lower_item = record.getItem<GPTABLE::LOWER_COMPONENT>();
    const auto& upper_item = record.getItem<GPTABLE::UPPER_COMPONENT>();
    const int lower_component = lower_item.defaultApplied(0) ? last_component : lower_item.get<int>(0);
    const int upper_component = upper_item.defaultApplied(0) ? last_component : upper_item.get<int>(0);

    // The heavy-component indices form a bracket [lower, upper]: each must reference a
    // real component (in [1, numComponents]) and the bracket must be non-inverted
    // (lower <= upper; equal denotes a single component).
    if ((lower_component < 1) || (lower_component > last_component) ||
        (upper_component < 1) || (upper_component > last_component))
    {
        throw OpmInputError {
            fmt::format("GPTABLE table {}: heavy-component indices must lie between 1 "
                        "and the number of components ({}), but got lower={}, upper={}.",
                        table_num, numComponents, lower_component, upper_component),
            location
        };
    }

    if (lower_component > upper_component) {
        throw OpmInputError {
            fmt::format("GPTABLE table {}: the lower heavy-component index ({}) must not "
                        "exceed the upper index ({}).",
                        table_num, lower_component, upper_component),
            location
        };
    }

    auto data = record.getItem<GPTABLE::DATA>().getData<double>();

    const auto row_width = numComponents + 1;
    if (data.empty() || (data.size() % row_width != 0)) {
        throw OpmInputError {
            fmt::format("GPTABLE table {}: data length ({}) must be a positive "
                        "multiple of the number of components + 1 ({}).",
                        table_num, data.size(), row_width),
            location
        };
    }

    // Every entry is a dimensionless fraction: the heavy-component mole fraction and
    // each per-component recovery factor all lie in [0, 1]. Reject non-finite or
    // out-of-range values.
    for (const auto& value : data) {
        if (! std::isfinite(value) || (value < 0.0) || (value > 1.0)) {
            throw OpmInputError {
                fmt::format("GPTABLE table {}: every entry must be a finite "
                            "fraction in [0, 1], but got {}.", table_num, value),
                location
            };
        }
    }

    // The heavy-component mole fraction is the key column (column 1) used for
    // interpolation, so it must be strictly increasing across the rows of a
    // multi-row table.
    const auto num_rows = data.size() / row_width;
    if (num_rows >= 2) {
        for (std::size_t row = 1; row < num_rows; ++row) {
            const auto prev_key = data[(row - 1) * row_width];
            const auto curr_key = data[row * row_width];
            if (! (curr_key > prev_key)) {
                throw OpmInputError {
                    fmt::format("GPTABLE table {}: the heavy mole fraction (column 1) "
                                "must be strictly increasing, but row {} ({}) is not "
                                "greater than row {} ({}).",
                                table_num, row + 1, curr_key, row, prev_key),
                    location
                };
            }
        }
    }

    this->m_table_num       = table_num;
    this->m_lower_component = lower_component;
    this->m_upper_component = upper_component;
    this->m_data            = std::move(data);
}

void GasPlantTable::warnOnTableNumber(std::unordered_set<int>& seenTableNumbers,
                                      const int tableNumber,
                                      const std::size_t maxTables,
                                      const KeywordLocation& location)
{
    if (! seenTableNumbers.insert(tableNumber).second) {
        OpmLog::warning(OpmInputError::format(
            fmt::format("GPTABLE: gas plant table number {} is specified more "
                        "than once in a single GPTABLE keyword; last entry wins.",
                        tableNumber),
            location));
    }
    else if ((maxTables > 0) && (tableNumber > static_cast<int>(maxTables))) {
        OpmLog::warning(OpmInputError::format(
            fmt::format("GPTABLE: gas plant table number {} exceeds the maximum "
                        "declared by GPTDIMS ({}).", tableNumber, maxTables),
            location));
    }
}

GasPlantTable GasPlantTable::serializationTestObject()
{
    GasPlantTable result;

    result.m_table_num = 1;
    result.m_lower_component = 2;
    result.m_upper_component = 3;
    result.m_num_components = 3;
    result.m_data = { 0.0, 0.10, 0.20, 0.70,
                      0.2, 0.15, 0.25, 0.60 };

    return result;
}

bool GasPlantTable::operator==(const GasPlantTable& other) const
{
    return (this->m_table_num == other.m_table_num)
        && (this->m_lower_component == other.m_lower_component)
        && (this->m_upper_component == other.m_upper_component)
        && (this->m_num_components == other.m_num_components)
        && (this->m_data == other.m_data);
}

} // namespace Opm
