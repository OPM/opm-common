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
#ifndef OPM_PARSER_ROCKTAB_TABLE_HPP
#define	OPM_PARSER_ROCKTAB_TABLE_HPP

#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>

namespace Opm {

    class DeckItem;

    class RocktabTable : public  SimpleTable {
    public:
        RocktabTable(std::shared_ptr< const DeckItem > item,
                     bool isDirectional,
                     bool hasStressOption)
        {
            Table::ColumnOrderEnum POOrder;

            m_isDirectional = isDirectional;
            m_schema = std::make_shared<TableSchema>( );
            if (hasStressOption)
                POOrder = Table::STRICTLY_INCREASING;
            else
                POOrder = Table::STRICTLY_DECREASING;

            m_schema->addColumn( ColumnSchema("PO"      , POOrder      , Table::DEFAULT_NONE ) );
            m_schema->addColumn( ColumnSchema("PV_MULT" , Table::RANDOM , Table::DEFAULT_LINEAR ) );

            if (isDirectional) {
                m_schema->addColumn( ColumnSchema("PV_MULT_TRANX" , Table::RANDOM , Table::DEFAULT_LINEAR ) );
                m_schema->addColumn( ColumnSchema("PV_MULT_TRANY" , Table::RANDOM , Table::DEFAULT_LINEAR ) );
                m_schema->addColumn( ColumnSchema("PV_MULT_TRANZ" , Table::RANDOM , Table::DEFAULT_LINEAR ) );
            } else
                m_schema->addColumn( ColumnSchema("PV_MULT_TRAN" , Table::RANDOM , Table::DEFAULT_LINEAR ) );

            SimpleTable::init(item);
        }


        const TableColumn& getPressureColumn() const
        { return SimpleTable::getColumn(0); }

        const TableColumn& getPoreVolumeMultiplierColumn() const
        { return SimpleTable::getColumn(1); }

        const TableColumn& getTransmissibilityMultiplierColumn() const
        { return SimpleTable::getColumn(2); }

        const TableColumn& getTransmissibilityMultiplierXColumn() const
        { return SimpleTable::getColumn(2); }

        const TableColumn& getTransmissibilityMultiplierYColumn() const
        {
            if (!m_isDirectional)
                return SimpleTable::getColumn(2);
            return SimpleTable::getColumn(3);
        }

        const TableColumn& getTransmissibilityMultiplierZColumn() const
        {
            if (!m_isDirectional)
                return SimpleTable::getColumn(2);
            return SimpleTable::getColumn(4);
        }

    private:
        bool m_isDirectional;
    };
}

#endif
