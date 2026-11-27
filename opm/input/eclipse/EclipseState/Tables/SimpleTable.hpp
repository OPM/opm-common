/*
  Copyright (C) 2013 by Andreas Lauser

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

#ifndef OPM_PARSER_SIMPLE_TABLE_HPP
#define	OPM_PARSER_SIMPLE_TABLE_HPP

#include <opm/input/eclipse/EclipseState/Tables/TableColumn.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableSchema.hpp>
#include <opm/input/eclipse/EclipseState/Util/OrderedMap.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Opm {

    class SimpleTable
    {
    public:
        SimpleTable() = default;
        SimpleTable(TableSchema, const std::string& tableName, const DeckItem& deckItem, const int tableID);
        explicit SimpleTable(TableSchema);

        virtual ~SimpleTable() = default;

        static SimpleTable serializationTestObject();

        void addColumns();

        /// Initialise table from deck item, using SI values.
        ///
        /// \param[in] tableName Name of table to initialise
        /// \param[in] deckItem item to initialise
        /// \param[in] tableID Index for table to initialise
        void init(const std::string& tableName,
                  const DeckItem&    deckItem,
                  const int          tableID)
        {
            this->init(tableName, deckItem, tableID,
                       [](const DeckItem& item, const std::size_t index)
                       { return item.getSIDouble(index); });
        }

        /// Initialise table from deck item, using custom value extraction.
        ///
        /// \param[in] tableName Name of table to initialise
        /// \param[in] deckItem item to initialise
        /// \param[in] tableID Index for table to initialise
        /// \param[in] readValue Extracts each non-defaulted value from \p deckItem
        template <typename ValueReader>
        void init(const std::string& tableName,
                  const DeckItem&    deckItem,
                  const int          tableID,
                  ValueReader&&      readValue)
        {
            this->addColumns();
            this->verifyConsistentNumberOfColumns(tableName, tableID, deckItem.data_size());

            const auto ncol = this->numColumns();

            const std::size_t rows = deckItem.data_size() / ncol;

            for (std::size_t colIdx = 0; colIdx < ncol; ++colIdx) {
                auto& column = this->getColumn(colIdx);

                for (std::size_t rowIdx = 0; rowIdx < rows; ++rowIdx) {
                    const std::size_t deckItemIdx = rowIdx*ncol + colIdx;

                    if (deckItem.defaultApplied(deckItemIdx)) {
                        column.addDefault(tableName);
                    }
                    else {
                        column.addValue(readValue(deckItem, deckItemIdx), tableName);
                    }
                }

                if (colIdx > 0) {
                    column.applyDefaults(this->getColumn(0), tableName);
                }
            }
        }

        std::size_t numColumns() const;
        std::size_t numRows() const;
        void addRow(const std::vector<double>& row, const std::string& tableName);
        const TableColumn& getColumn(const std::string &name) const;
        const TableColumn& getColumn(std::size_t colIdx) const;
        bool hasColumn(const std::string& name) const;

        TableColumn& getColumn(const std::string &name);
        TableColumn& getColumn(std::size_t colIdx);

        double get(const std::string& column, std::size_t row) const;
        double get(std::size_t column, std::size_t row) const;

        /*!
         * \brief Evaluate a column of the table at a given position.
         *
         * This method uses linear interpolation and always uses the first
         * column as the X coordinate.
         */
        double evaluate(const std::string& columnName, double xPos) const;

        bool operator==(const SimpleTable& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_schema);
            serializer(m_columns);
        }

    protected:
        TableSchema m_schema;
        OrderedMap<TableColumn> m_columns;

    private:
        /// Ensure that input table data matches expected number of table columns.
        ///
        /// Throws an exception of type std::runtime_error for inconsistent
        /// table data sizes.
        ///
        /// \param[in] tableName Table name (input keyword).
        ///
        /// \param[in] tableID Region to which this table applies.
        ///
        /// \param[in] data_size Number of tabulated, raw input values for
        /// this individual table.  Must be a multiple of the table's
        /// configured number of columns, i.e., numColumns().  Exception
        /// thrown if not.
        void verifyConsistentNumberOfColumns(const std::string& tableName,
                                             const int          tableID,
                                             const std::size_t  data_size) const;
    };

} // namespace Opm

#endif // OPM_PARSER_SIMPLE_TABLE_HPP
