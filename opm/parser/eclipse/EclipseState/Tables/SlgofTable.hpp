/*
  Copyright (C) 2015 by Andreas Lauser

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
#ifndef OPM_PARSER_SLGOF_TABLE_HPP
#define	OPM_PARSER_SLGOF_TABLE_HPP

#include "SimpleTable.hpp"

#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>

namespace Opm {

    class DeckItem;

    class SlgofTable : public SimpleTable {

    public:

        SlgofTable(std::shared_ptr< const DeckItem > item)
        {
            m_schema = std::make_shared<TableSchema>();
            m_schema->addColumn( ColumnSchema("SL"   , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ));
            m_schema->addColumn( ColumnSchema("KRG"  , Table::DECREASING , Table::DEFAULT_LINEAR ));
            m_schema->addColumn( ColumnSchema("KROG" , Table::INCREASING , Table::DEFAULT_LINEAR ));
            m_schema->addColumn( ColumnSchema("PCOG" , Table::DECREASING , Table::DEFAULT_LINEAR ));

            SimpleTable::init( item );

            if (getSlColumn().back() != 1.0) {
                throw std::invalid_argument("The last saturation of the SLGOF keyword must be 1!");
            }
        }


        const TableColumn& getSlColumn() const
        { return SimpleTable::getColumn(0); }

        const TableColumn& getKrgColumn() const
        { return SimpleTable::getColumn(1); }

        const TableColumn& getKrogColumn() const
        { return SimpleTable::getColumn(2); }

        // this column is p_g - p_o (non-wetting phase pressure minus
        // wetting phase pressure for a given gas saturation. the name
        // is inconsistent, but it is the one used in the Eclipse
        // manual...)
        const TableColumn& getPcogColumn() const
        { return SimpleTable::getColumn(3); }
    };
}

#endif
