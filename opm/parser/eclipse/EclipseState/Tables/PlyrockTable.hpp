/*
  Copyright (C) 2014 by Andreas Lauser

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
#ifndef OPM_PARSER_PLYROCK_TABLE_HPP
#define	OPM_PARSER_PLYROCK_TABLE_HPP

#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ColumnSchema.hpp>

namespace Opm {

    class DeckItem;

    class PlyrockTable : public SimpleTable {
    public:

        // This is not really a table; every column has only one element.
        PlyrockTable(std::shared_ptr< const DeckRecord > record)
        {
            m_schema = std::make_shared<TableSchema>( );
            m_schema->addColumn( ColumnSchema("DeadPoreVolume",            Table::RANDOM , Table::DEFAULT_NONE) );
            m_schema->addColumn( ColumnSchema("ResidualResistanceFactor",  Table::RANDOM , Table::DEFAULT_NONE) );
            m_schema->addColumn( ColumnSchema("RockDensityFactor",         Table::RANDOM , Table::DEFAULT_NONE) );
            m_schema->addColumn( ColumnSchema("AdsorbtionIndex",           Table::RANDOM , Table::DEFAULT_NONE) );
            m_schema->addColumn( ColumnSchema("MaxAdsorbtion",             Table::RANDOM , Table::DEFAULT_NONE) );
            addColumns();
            for (size_t colIdx = 0; colIdx < record->size(); colIdx++) {
                auto item = record->getItem( colIdx );
                auto& column = getColumn( colIdx );

                column.addValue( item->getSIDouble(0) );
            }
        }

        // since this keyword is not necessarily monotonic, it cannot be evaluated!
        //using SimpleTable::evaluate;

        const TableColumn& getDeadPoreVolumeColumn() const
        { return SimpleTable::getColumn(0); }

        const TableColumn& getResidualResistanceFactorColumn() const
        { return SimpleTable::getColumn(1); }

        const TableColumn& getRockDensityFactorColumn() const
        { return SimpleTable::getColumn(2); }

        // is column is actually an integer, but this is not yet
        // supported by opm-parser (yet?) as it would require quite a
        // few changes in the table support classes (read: it would
        // probably require a lot of template vodoo) and some in the
        // JSON-to-C conversion code. In the meantime, the index is
        // just a double which can be converted to an integer in the
        // calling code. (Make sure, that you don't interpolate
        // indices, though!)
        const TableColumn& getAdsorbtionIndexColumn() const
        { return SimpleTable::getColumn(3); }

        const TableColumn& getMaxAdsorbtionColumn() const
        { return SimpleTable::getColumn(4); }
    };
}

#endif
