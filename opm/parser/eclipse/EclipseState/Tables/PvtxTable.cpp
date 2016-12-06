/*
  Copyright 2015 Statoil ASA.

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

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtwTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtxTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableSchema.hpp>

namespace Opm {

namespace {
    std::vector< PvtwTable::record > pvtw_records( const DeckKeyword& kw ) {
        std::vector< PvtwTable::record > rs;

        for( const auto& record : kw ) {
            if( record.getItem( 0 ).defaultApplied( 0 ) ) {
                throw std::invalid_argument(
                    "PvtwTable reference pressure cannot be defaulted" );
            }

            rs.emplace_back(
                PvtwTable::record {
                    record.getItem( 0 ).getSIDouble( 0 ),
                    record.getItem( 1 ).getSIDouble( 0 ),
                    record.getItem( 2 ).getSIDouble( 0 ),
                    record.getItem( 3 ).getSIDouble( 0 ),
                    record.getItem( 4 ).getSIDouble( 0 ),
                }
            );
        }

        return rs;
    }

}

    PvtwTable::PvtwTable( const DeckKeyword& kw ) : records( pvtw_records( kw ) ) {}

    const PvtwTable::record& PvtwTable::operator[]( size_t region ) const {
        return this->records.at( region );
    }

    const PvtwTable::record& PvtwTable::at( size_t region ) const {
        return this->records.at( region );
    }

    bool PvtwTable::empty() const {
        return this->records.empty();
    }

    size_t PvtwTable::size() const {
        return this->records.size();
    }

    PvtwTable::const_iterator PvtwTable::begin() const {
        return this->records.begin();
    }

    PvtwTable::const_iterator PvtwTable::end() const {
        return this->records.end();
    }

    PvtxTable::PvtxTable(const std::string& columnName) :
        m_outerColumnSchema( columnName , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ),
        m_outerColumn( m_outerColumnSchema )
    {

    }



    /*
      The Schema pointers m_saturatedSchema and m_underSaturatedSchema must
      have been explicitly set before calling this method.
    */

    void PvtxTable::init( const DeckKeyword& keyword, size_t tableIdx) {
        auto ranges = recordRanges( keyword );
        if (tableIdx >= ranges.size())
            throw std::invalid_argument("Asked for table: " + std::to_string( tableIdx ) + " in keyword + " + keyword.name() + " which only has " + std::to_string( ranges.size() ) + " tables");

        {
            auto range = ranges[ tableIdx ];
            for (size_t  rowIdx = range.first; rowIdx < range.second; rowIdx++) {
                const auto& deckRecord = keyword.getRecord(rowIdx);
                m_outerColumn.addValue( deckRecord.getItem( 0 ).getSIDouble( 0 ));

                const auto& dataItem = deckRecord.getItem(1);
                m_underSaturatedTables.emplace_back( this->m_underSaturatedSchema, dataItem );
            }


            m_saturatedTable = SimpleTable(m_saturatedSchema);
            for (size_t sat_index = 0; sat_index < size(); sat_index++) {
                const auto& underSaturatedTable = getUnderSaturatedTable( sat_index );
                std::vector<double> row(4);
                row[0] = m_outerColumn[sat_index];
                for (size_t col_index = 0; col_index < m_underSaturatedSchema.size(); col_index++)
                    row[col_index + 1] = underSaturatedTable.get( col_index , 0 );

                m_saturatedTable.addRow( row );
            }
        }
    }


    double PvtxTable::evaluate(const std::string& column, double outerArg, double innerArg) const
    {
        TableIndex outerIndex = m_outerColumn.lookup( outerArg );
        const auto& underSaturatedTable1 = getUnderSaturatedTable( outerIndex.getIndex1( ) );
        double weight1 = outerIndex.getWeight1( );
        double value = weight1 * underSaturatedTable1.evaluate( column , innerArg );

        if (weight1 < 1) {
            const auto& underSaturatedTable2 = getUnderSaturatedTable( outerIndex.getIndex2( ) );
            double weight2 = outerIndex.getWeight2( );

            value += weight2 * underSaturatedTable2.evaluate( column , innerArg );
        }

        return value;
    }


    const SimpleTable& PvtxTable::getSaturatedTable() const {
        return this->m_saturatedTable;
    }



    const SimpleTable& PvtxTable::getUnderSaturatedTable(size_t tableNumber) const {
        if (tableNumber >= size())
            throw std::invalid_argument("Invalid table number: " + std::to_string( tableNumber) + " max: " + std::to_string( size() - 1 ));
        return m_underSaturatedTables[ tableNumber ];
    }


    std::vector< SimpleTable >::const_iterator PvtxTable::begin() const {
        return m_underSaturatedTables.begin();
    }

    std::vector< SimpleTable >::const_iterator PvtxTable::end() const {
        return m_underSaturatedTables.end();
    }



    size_t PvtxTable::size() const
    {
        return m_outerColumn.size();
    }


    size_t PvtxTable::numTables( const DeckKeyword& keyword )
    {
        auto ranges = recordRanges(keyword);
        return ranges.size();
    }


    std::vector<std::pair<size_t , size_t> > PvtxTable::recordRanges( const DeckKeyword& keyword ) {
        std::vector<std::pair<size_t,size_t> > ranges;
        size_t startRecord = 0;
        size_t recordIndex = 0;
        while (recordIndex < keyword.size()) {
            const auto& item = keyword.getRecord(recordIndex).getItem(0);
            if (item.size( ) == 0) {
                ranges.push_back( std::make_pair( startRecord , recordIndex ) );
                startRecord = recordIndex + 1;
            }
            recordIndex++;
        }
        ranges.push_back( std::make_pair( startRecord , recordIndex ) );
        return ranges;
    }


    double PvtxTable::getArgValue(size_t index) const {
        if (index < m_outerColumn.size())
            return m_outerColumn[index];
        else
            throw std::invalid_argument("Invalid index");
    }

}

