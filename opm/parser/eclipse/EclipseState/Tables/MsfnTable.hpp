/*
  Copyright (C) 2015 Statoil ASA.
  2015 IRIS AS

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
#ifndef OPM_PARSER_MSFN_TABLE_HPP
#define	OPM_PARSER_MSFN_TABLE_HPP

#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>

namespace Opm {
    class MsfnTable : public SimpleTable {
    public:
        MsfnTable(Opm::DeckItemConstPtr item)
        {
            m_schema = std::make_shared<TableSchema>( );
            m_schema->addColumn( ColumnSchema( "GasPhaseFraction", Table::STRICTLY_INCREASING  , Table::DEFAULT_NONE));
            m_schema->addColumn( ColumnSchema( "GasSolventRelpermMultiplier", Table::INCREASING  , Table::DEFAULT_NONE));
            m_schema->addColumn( ColumnSchema( "OilRelpermMultiplier", Table::DECREASING  , Table::DEFAULT_NONE));

            SimpleTable::init( item );
            {
                auto& column = getColumn("GasPhaseFraction");
                column.assertUnitRange();
            }
        }


        const TableColumn& getGasPhaseFractionColumn() const
        { return SimpleTable::getColumn(0); }

        const TableColumn& getGasSolventRelpermMultiplierColumn() const
        { return SimpleTable::getColumn(1); }

        const TableColumn& getOilRelpermMultiplierColumn() const
        { return SimpleTable::getColumn(2); }
    };
}

#endif
