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

#ifndef OPM_PARSER_GASVISCT_TABLE_HPP
#define OPM_PARSER_GASVISCT_TABLE_HPP

#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>
#include "SimpleTable.hpp"
#include <opm/parser/eclipse/EclipseState/Tables/TableEnums.hpp>

namespace Opm {
    class GasvisctTable : public SimpleTable {
    public:
        GasvisctTable(const Deck& deck, Opm::DeckItemConstPtr deckItem)
        {
            int numComponents = deck.getKeyword<ParserKeywords::COMPS>()->getRecord(0)->getItem(0)->getInt(0);

            auto temperatureDimension = deck.getActiveUnitSystem()->getDimension("Temperature");
            auto viscosityDimension = deck.getActiveUnitSystem()->getDimension("Viscosity");

            m_schema = std::make_shared<TableSchema>( );
            m_schema->addColumn( ColumnSchema( "Temperature" , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE));
            for (int compIdx = 0; compIdx < numComponents; ++ compIdx) {
                std::string columnName = "Viscosity" + std::to_string(compIdx);
                m_schema->addColumn( ColumnSchema( columnName, Table::INCREASING , Table::DEFAULT_NONE ) );
            }
            SimpleTable::addColumns( );

            {
                size_t numFlatItems = deckItem->size( );
                if ( numFlatItems % numColumns() != 0)
                    throw std::runtime_error("Number of columns in the data file is inconsistent "
                                             "with the expected number for keyword GASVISCT");
            }

            {
                size_t numRows = deckItem->size() / m_schema->size();
                for (size_t columnIndex=0; columnIndex < m_schema->size(); columnIndex++) {
                    auto& column = getColumn( columnIndex );
                    for (size_t rowIdx = 0; rowIdx < numRows; rowIdx++) {
                        size_t deckIndex = rowIdx * m_schema->size() + columnIndex;

                        if (deckItem->defaultApplied( deckIndex ))
                            column.addDefault();
                        else {
                            double rawValue = deckItem->getRawDouble( deckIndex );
                            double SIValue;

                            if (columnIndex == 0)
                                SIValue = temperatureDimension->convertRawToSi(rawValue);
                            else
                                SIValue = viscosityDimension->convertRawToSi(rawValue);

                            column.addValue( SIValue );
                        }
                    }
                }
            }
        }

        const TableColumn& getTemperatureColumn() const
        { return SimpleTable::getColumn(0); }

        const TableColumn& getGasViscosityColumn(size_t compIdx) const
        { return SimpleTable::getColumn(1 + compIdx); }
    };
}

#endif
