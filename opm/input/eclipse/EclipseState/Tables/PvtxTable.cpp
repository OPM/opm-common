/*
  Copyright 2015 Statoil ASA.

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

#include <opm/input/eclipse/EclipseState/Tables/PvtxTable.hpp>

#include <opm/input/eclipse/EclipseState/Tables/FlatTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableSchema.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {

    std::optional<std::vector<std::pair<std::size_t, std::size_t>>::size_type>
    locateSourceTable(const std::size_t                                       tableIdx,
                      const std::vector<std::pair<std::size_t, std::size_t>>& ranges)
    {
        // Source table is *last* non-empty sub-range in 0..tableIdx.

        // Recall: The reverse "begin" iterator, and especially its .base(),
        // is one past the end of the sub-range.  Thus, pass "tableIdx + 1"
        // as the argument to make_reverse_iterator().
        auto srcRange = std::find_if(std::make_reverse_iterator(ranges.cbegin() + tableIdx + 1),
                                     ranges.crend(),
                                     [](const auto& subRange)
                                     { return subRange.first != subRange.second; });

        if (srcRange == ranges.crend()) {
            return {};
        }

        return std::distance(ranges.cbegin(), srcRange.base() - 1);
    }

} // namespace Anonymous

namespace Opm {

    std::size_t PvtxTable::numTables(const DeckKeyword& keyword)
    {
        return std::accumulate(keyword.begin(), keyword.end(), std::size_t{1},
                               [](const auto acc, const auto& record)
                               { return acc + !record.getItem(0).hasValue(0); });
    }

    std::vector<std::pair<std::size_t, std::size_t>>
    PvtxTable::recordRanges(const DeckKeyword& keyword)
    {
        auto ranges = std::vector<std::pair<std::size_t, std::size_t>>{};

        std::size_t startRecord = 0;
        std::size_t recordIndex = startRecord;

        while (recordIndex < keyword.size()) {
            if (! keyword.getRecord(recordIndex).getItem(0).hasValue(0)) {
                ranges.emplace_back(startRecord, recordIndex);
                startRecord = recordIndex + 1;
            }

            ++recordIndex;
        }

        ranges.emplace_back(startRecord, recordIndex);

        return ranges;
    }

    PvtxTable::PvtxTable(const std::string& columnName)
        : m_outerColumnSchema { columnName, Table::STRICTLY_INCREASING, Table::DEFAULT_NONE }
        , m_outerColumn       { m_outerColumnSchema }
    {}

    PvtxTable PvtxTable::serializationTestObject()
    {
        PvtxTable result;

        result.m_outerColumnSchema = ColumnSchema::serializationTestObject();
        result.m_outerColumn = TableColumn::serializationTestObject();

        result.m_underSaturatedSchema = TableSchema::serializationTestObject();
        result.m_saturatedSchema = TableSchema::serializationTestObject();

        result.m_underSaturatedTables = {SimpleTable::serializationTestObject()};
        result.m_saturatedTable = SimpleTable::serializationTestObject();

        return result;
    }

    const SimpleTable& PvtxTable::getSaturatedTable() const
    {
        return this->m_saturatedTable;
    }

    const SimpleTable&
    PvtxTable::getUnderSaturatedTable(const std::size_t tableNumber) const
    {
        if (tableNumber >= this->size()) {
            throw std::invalid_argument {
                fmt::format("Undersaturated table number {} exceeds "
                            "maximum possible value of {}.",
                            tableNumber, this->size() - 1)
            };
        }

        return this->m_underSaturatedTables[tableNumber];
    }

    std::size_t PvtxTable::size() const
    {
        return m_outerColumn.size();
    }

    double PvtxTable::getArgValue(const std::size_t index) const
    {
        if (! (index < m_outerColumn.size())) {
            throw std::invalid_argument {
                fmt::format("Invalid index {}", index)
            };
        }

        return this->m_outerColumn[index];
    }

    bool PvtxTable::operator==(const PvtxTable& data) const
    {
        return (this->m_outerColumnSchema == data.m_outerColumnSchema)
            && (this->m_outerColumn == data.m_outerColumn)
            && (this->m_underSaturatedSchema == data.m_underSaturatedSchema)
            && (this->m_saturatedSchema == data.m_saturatedSchema)
            && (this->m_underSaturatedTables == data.m_underSaturatedTables)
            && (this->m_saturatedTable == data.m_saturatedTable)
            ;
    }

    // The "schema" members m_saturatedSchema and m_underSaturatedSchema
    // must have been explicitly set before calling this method.

    void PvtxTable::init(const DeckKeyword& keyword,
                         const std::size_t  tableIdx0)
    {
        const auto ranges = recordRanges(keyword);

        if (tableIdx0 >= ranges.size()) {
            throw std::invalid_argument {
                fmt::format("Asked for table {} in keyword {} "
                            "which only has {} tables.",
                            tableIdx0, keyword.name(), ranges.size())
            };
        }

        const auto tableIdx = locateSourceTable(tableIdx0, ranges);

        if (! tableIdx.has_value()) {
            throw OpmInputError {
                "Cannot default region 1's table data",
                keyword.location()
            };
        }

        // We identify as 'tableIdx0' even if the real source table is
        // '*tableIdx' (<= tableIdx0).  Input failure diagnostics reported
        // to the user should refer to the actual PVT region we're
        // processing.
        this->populateUndersaturatedTables(keyword, tableIdx0,
                                           ranges[*tableIdx].first,
                                           ranges[*tableIdx].second);

        this->populateMissingUndersaturatedStates();

        this->populateSaturatedTable(keyword.name());
    }

    void PvtxTable::populateUndersaturatedTables(const DeckKeyword& keyword,
                                                 const std::size_t  tableIdx,
                                                 const std::size_t  first,
                                                 const std::size_t  last)
    {
        for (auto rowIdx = first; rowIdx < last; ++rowIdx) {
            const auto& deckRecord = keyword.getRecord(rowIdx);

            this->m_outerColumn
                .addValue(deckRecord.getItem(0).getSIDouble(0), keyword.name());

            this->m_underSaturatedTables
                .emplace_back(this->m_underSaturatedSchema,
                              keyword.name(), deckRecord.getItem(1), tableIdx);
        }
    }

    void PvtxTable::populateSaturatedTable(const std::string& tableName)
    {
        this->m_saturatedTable = SimpleTable { this->m_saturatedSchema };

        auto row = std::vector<double>(this->m_saturatedSchema.size());

        const auto numSaturatedTables = this->size();
        const auto usatTableSrcRow = std::size_t { 0 };

        for (auto tableIdx = 0*numSaturatedTables; tableIdx < numSaturatedTables; ++tableIdx) {
            const auto& usatTable = this->getUnderSaturatedTable(tableIdx);

            row[0] = this->m_outerColumn[tableIdx];

            for (auto colIdx = 0*row.size() + 1; colIdx < row.size(); ++colIdx) {
                row[colIdx] = usatTable.get(colIdx - 1, usatTableSrcRow);
            }

            this->m_saturatedTable.addRow(row, tableName);
        }
    }

    void PvtxTable::populateMissingUndersaturatedStates()
    {
        for (const auto& [src, dest] : this->missingUSatTables()) {
            this->makeScaledUSatTableCopy(src, dest);
        }
    }

    std::vector<std::pair<std::size_t, std::size_t>>
    PvtxTable::missingUSatTables() const
    {
        auto missing = std::vector<std::pair<std::size_t, std::size_t>>{};

        if (this->m_underSaturatedTables.empty()) {
            return missing;
        }

        auto src = this->m_underSaturatedTables.size() - 1;

        for (auto destIx = src + 1; destIx > 0; --destIx) {
            const auto dest = destIx - 1;

            if (this->m_underSaturatedTables[dest].numRows() > 1) {
                // There are undersaturated states in 'dest'.  This is the
                // new 'src'.
                src = dest;
            }
            else {
                // There are no undersaturated states in 'dest'.  Schedule
                // generation of a scaled copy of 'src's undersaturated
                // states in 'dest'.
                missing.emplace_back(src, dest);
            }
        }

        return missing;
    }

    void PvtxTable::makeScaledUSatTableCopy([[maybe_unused]] const std::size_t src,
                                            [[maybe_unused]] const std::size_t dest)
    {
        // Implemented only because we need to be able to create objects of
        // type 'PvtxTable' for serialisation purposes.  Ideally, this would
        // be a pure virtual function.

        throw std::runtime_error {
            "Derived type does not implement makeScaledUSatTableCopy()"
        };
    }

} // namespace Opm
