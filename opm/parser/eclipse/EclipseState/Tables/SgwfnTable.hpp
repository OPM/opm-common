/*
  Copyright (C) 2015 by Statoil ASA.

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
#ifndef OPM_PARSER_SGWFN_TABLE_HPP
#define	OPM_PARSER_SGWFN_TABLE_HPP

#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableSchema.hpp>

namespace Opm {

    class DeckItem;

    class SgwfnTable : public SimpleTable {

    public:
        SgwfnTable(std::shared_ptr< const DeckItem > item) {
            m_schema = std::make_shared<TableSchema>();

            m_schema->addColumn( ColumnSchema( "SG"   , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ) );
            m_schema->addColumn( ColumnSchema( "KRG"  , Table::RANDOM ,              Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "KRGW" , Table::RANDOM ,              Table::DEFAULT_LINEAR ) );
            m_schema->addColumn( ColumnSchema( "PCGW" , Table::RANDOM ,              Table::DEFAULT_LINEAR ) );

            SimpleTable::init( item );
        }

        const TableColumn& getSgColumn() const
        { return SimpleTable::getColumn(0); }

        const TableColumn& getKrgColumn() const
        { return SimpleTable::getColumn(1); }

        const TableColumn& getKrgwColumn() const
        { return SimpleTable::getColumn(2); }

        // this column is p_g - p_w (non-wetting phase pressure minus
        // wetting phase pressure for a given water saturation)
        const TableColumn& getPcgwColumn() const
        { return SimpleTable::getColumn(3); }
    };
}

#endif

