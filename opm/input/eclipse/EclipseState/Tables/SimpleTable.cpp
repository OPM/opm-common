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

// <iostream> is for std::cerr in assertJFuncPressure().  This should really
// go away or at least use the logging system instead.
#include <iostream>
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
        , m_jfunc (false)
    {
        this->init(tableName, deckItem, tableID);
    }

    SimpleTable::SimpleTable(TableSchema schema)
        : m_schema(std::move(schema))
        , m_jfunc (false)
    {
        this->addColumns();
    }

    SimpleTable SimpleTable::serializationTestObject()
    {
        SimpleTable result;
        result.m_schema = Opm::TableSchema::serializationTestObject();
        result.m_columns.insert({"test3", Opm::TableColumn::serializationTestObject()});
        result.m_jfunc = true;

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

    double SimpleTable::get(const std::string& column, size_t row) const
    {
        return this->getColumn(column)[row];
    }

    double SimpleTable::get(size_t column, size_t row) const
    {
        return this->getColumn(column)[row];
    }

    void SimpleTable::init(const std::string& tableName,
                           const DeckItem&    deckItem,
                           const int          tableID,
                           const double       scaling_factor)
    {
        this->addColumns();

        const auto ncol = this->numColumns();

        if ((deckItem.data_size() % ncol) != 0) {
            throw std::runtime_error {
                fmt::format("For table {} with ID {}: "
                            "Number of input table elements ({}) is "
                            "not a multiple of table's specified number "
                            "of columns ({})",
                            tableName, tableID+1, deckItem.data_size(), this->numColumns())
            };
        }

        const size_t rows = deckItem.data_size() / ncol;

        for (size_t colIdx = 0; colIdx < ncol; ++colIdx) {
            auto& column = this->getColumn(colIdx);

            for (size_t rowIdx = 0; rowIdx < rows; ++rowIdx) {
                const size_t deckItemIdx = rowIdx*ncol + colIdx;

                if (deckItem.defaultApplied(deckItemIdx)) {
                    column.addDefault(tableName);
                }
                else if (m_jfunc) {
                    column.addValue(deckItem.getData<double>()[deckItemIdx], tableName);
                }
                else if (scaling_factor > 0.0) {
                    column.addValue(scaling_factor * deckItem.get<double>(deckItemIdx), tableName);
                }
                else {
                    column.addValue(deckItem.getSIDouble(deckItemIdx), tableName);
                }
            }

            if (colIdx > 0) {
                column.applyDefaults(this->getColumn(0), tableName);
            }
        }
    }

    size_t SimpleTable::numColumns() const
    {
        return m_schema.size();
    }

    size_t SimpleTable::numRows() const
    {
        return this->getColumn(0).size();
    }

    const TableColumn& SimpleTable::getColumn(const std::string& name) const
    {
        if (! this->m_jfunc) {
            return this->m_columns.get(name);
        }

        if ((name == "PCOW") || (name == "PCOG")) {
            assertJFuncPressure(false); // this will throw since m_jfunc=true
        }

        return this->m_columns.get(name);
    }

    const TableColumn& SimpleTable::getColumn(size_t columnIndex) const
    {
        return this->m_columns.iget(columnIndex);
    }

    TableColumn& SimpleTable::getColumn(const std::string& name)
    {
        if (! this->m_jfunc) {
            return this->m_columns.get(name);
        }

        if ((name == "PCOW") || (name == "PCOG")) {
            assertJFuncPressure(false); // this will throw since m_jfunc=true
        }

        return this->m_columns.get(name);
    }

    TableColumn& SimpleTable::getColumn(size_t columnIndex)
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

    void SimpleTable::assertJFuncPressure(const bool jf) const
    {
        if (jf == this->m_jfunc) {
            return;
        }

        // if we reach here, wrong values are read from the deck! (JFUNC is
        // used incorrectly.)  This function writes to std err for now, but
        // will after a grace period be rewritten to throw
        // (std::invalid_argument).
        if (m_jfunc) {
            std::cerr << "Developer warning: Pressure column "
                "is read with JFUNC in deck.\n";
        }
        else {
            std::cerr << "Developer warning: Raw values from JFUNC column "
                "is read, but JFUNC not provided in deck.\n";
        }
    }

    bool SimpleTable::operator==(const SimpleTable& data) const
    {
        return (this->m_schema == data.m_schema)
            && (this->m_columns == data.m_columns)
            && (this->m_jfunc == data.m_jfunc)
            ;
    }
}
