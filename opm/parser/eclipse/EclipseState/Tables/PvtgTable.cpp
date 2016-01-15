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

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ColumnSchema.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableSchema.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtgTable.hpp>

namespace Opm {
    PvtgTable::PvtgTable(Opm::DeckKeywordConstPtr keyword, size_t tableIdx ) : PvtxTable("P")
    {
        m_underSaturatedSchema = std::make_shared<TableSchema>( );
        m_underSaturatedSchema->addColumn( ColumnSchema( "RV"  , Table::STRICTLY_DECREASING , Table::DEFAULT_NONE ));
        m_underSaturatedSchema->addColumn( ColumnSchema( "BG"  , Table::RANDOM , Table::DEFAULT_LINEAR ));
        m_underSaturatedSchema->addColumn( ColumnSchema( "MUG" , Table::RANDOM , Table::DEFAULT_LINEAR ));


        m_saturatedSchema = std::make_shared<TableSchema>( );
        m_saturatedSchema->addColumn( ColumnSchema( "PG"  , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ));
        m_saturatedSchema->addColumn( ColumnSchema( "RV"  , Table::RANDOM , Table::DEFAULT_NONE ));
        m_saturatedSchema->addColumn( ColumnSchema( "BG"  , Table::RANDOM , Table::DEFAULT_LINEAR ));
        m_saturatedSchema->addColumn( ColumnSchema( "MUG" , Table::RANDOM , Table::DEFAULT_LINEAR ));

        PvtxTable::init(keyword , tableIdx);
    }
}
