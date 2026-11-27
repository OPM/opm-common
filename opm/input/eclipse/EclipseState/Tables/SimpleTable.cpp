/*
  Copyright (C) 2013 by Andreas Lauser, 2016 Statoil ASA

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

#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>

#include <opm/input/eclipse/EclipseState/Tables/TableSchema.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm {

    SimpleTable::SimpleTable(TableSchema        schema,
                             const std::string& tableName,
                             const DeckItem&    deckItem,
                             const int          tableID)
        : m_schema(std::move(schema))
    {
        this->init(tableName, deckItem, tableID);
    }

    SimpleTable::SimpleTable(TableSchema schema)
        : m_schema(std::move(schema))
    {
        this->addColumns();
    }

    SimpleTable SimpleTable::serializationTestObject()
    {
        SimpleTable result;
        result.m_schema = Opm::TableSchema::serializationTestObject();
        result.m_columns.insert({"test3", Opm::TableColumn::serializationTestObject()});

        return result;
    }

    void SimpleTable::addRow(const std::vector<double>& row,
                             const std::string& tableName)
    {
        const auto ncol = this->numColumns();

        if (row.size() != ncol) {
            throw std::invalid_argument {
                fmt::format("Mismatched number of columns "
                            "in {}: Expected {}, but got {}.",
                            tableName, ncol, row.size())
            };
        }

        for (auto colIndex = 0*ncol; colIndex < ncol; ++colIndex) {
            this->getColumn(colIndex).addValue(row[colIndex], tableName);
        }
    }

    void SimpleTable::addColumns()
    {
        const auto ncol = this->m_schema.size();

        for (auto colIdx = 0*ncol; colIdx < ncol; ++colIdx) {
            const auto& schemaColumn = m_schema.getColumn(colIdx);
            this->m_columns.insert(std::make_pair(schemaColumn.name(), TableColumn { schemaColumn }));
        }
    }

    double SimpleTable::get(const std::string& column, std::size_t row) const
    {
        return this->getColumn(column)[row];
    }

    double SimpleTable::get(std::size_t column, std::size_t row) const
    {
        return this->getColumn(column)[row];
    }

    std::size_t SimpleTable::numColumns() const
    {
        return m_schema.size();
    }

    std::size_t SimpleTable::numRows() const
    {
        return this->getColumn(0).size();
    }

    const TableColumn& SimpleTable::getColumn(const std::string& name) const
    {
        return this->m_columns.get(name);
    }

    const TableColumn& SimpleTable::getColumn(std::size_t columnIndex) const
    {
        return this->m_columns.iget(columnIndex);
    }

    TableColumn& SimpleTable::getColumn(const std::string& name)
    {
        return this->m_columns.get(name);
    }

    TableColumn& SimpleTable::getColumn(std::size_t columnIndex)
    {
        return this->m_columns.iget(columnIndex);
    }

    bool SimpleTable::hasColumn(const std::string& name) const
    {
        return this->m_schema.hasColumn(name);
    }

    double SimpleTable::evaluate(const std::string& columnName, double xPos) const
    {
        const auto index = this->getColumn(0).lookup(xPos);

        return this->getColumn(columnName).eval(index);
    }

    bool SimpleTable::operator==(const SimpleTable& data) const
    {
        return (this->m_schema == data.m_schema)
            && (this->m_columns == data.m_columns)
            ;
    }

    void SimpleTable::verifyConsistentNumberOfColumns(const std::string& tableName,
                                                      const int          tableID,
                                                      const std::size_t  data_size) const
    {
        const auto ncol = this->numColumns();

        if ((data_size % ncol) != 0) {
            throw std::runtime_error {
                fmt::format("For table {} with ID {}: "
                            "Number of input table elements ({}) is "
                            "not a multiple of table's specified number "
                            "of columns ({}).",
                            tableName, tableID + 1, data_size, ncol)
            };
        }
    }
}
