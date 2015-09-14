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
#include <iostream>
#include <opm/parser/eclipse/EclipseState/Tables/MultiRecordTable.hpp>

namespace Opm {
/*!
 * \brief Returns the number of tables which can be found in a
 *        given keyword.
 */

size_t MultiRecordTable::numTables(Opm::DeckKeywordConstPtr keyword)
{
    auto ranges = recordRanges(keyword);
    return ranges.size();
}


std::vector<std::pair<size_t , size_t> > MultiRecordTable::recordRanges(Opm::DeckKeywordConstPtr keyword) {
    std::vector<std::pair<size_t,size_t> > ranges;
    size_t startRecord = 0;
    size_t recordIndex = 0;
    while (recordIndex < keyword->size()) {
        auto item = keyword->getRecord(recordIndex)->getItem(0);
        if (item->size( ) == 0) {
            ranges.push_back( std::make_pair( startRecord , recordIndex ) );
            startRecord = recordIndex + 1;
        }
        recordIndex++;
    }
    ranges.push_back( std::make_pair( startRecord , recordIndex ) );
    return ranges;
}


// create table from first few items of multiple records
void MultiRecordTable::init(Opm::DeckKeywordConstPtr keyword,
                            const std::vector<std::string> &columnNames,
                            size_t tableIdx)
{
    auto ranges = recordRanges(keyword);
    if (tableIdx >= ranges.size())
        throw std::invalid_argument("Asked for table: " + std::to_string( tableIdx ) + " in keyword + " + keyword->name() + " which only has " + std::to_string( ranges.size() ) + " tables");

    createColumns(columnNames);
    m_recordRange = ranges[ tableIdx ];
    for (size_t  rowIdx = m_recordRange.first; rowIdx < m_recordRange.second; rowIdx++) {
        Opm::DeckRecordConstPtr deckRecord = keyword->getRecord(rowIdx);
        Opm::DeckItemConstPtr indexItem = deckRecord->getItem(0);
        Opm::DeckItemConstPtr dataItem = deckRecord->getItem(1);

        m_columns[0].push_back(indexItem->getSIDouble(0));
        m_valueDefaulted[0].push_back(indexItem->defaultApplied(0));
        for (size_t colIdx = 1; colIdx < numColumns(); ++colIdx) {
            m_columns[colIdx].push_back(dataItem->getSIDouble(colIdx - 1));
            m_valueDefaulted[colIdx].push_back(dataItem->defaultApplied(colIdx - 1));
        }
    }
}

size_t MultiRecordTable::firstRecordIndex() const
{
    return m_recordRange.first;
}

size_t MultiRecordTable::numRecords() const
{
    return m_recordRange.second - m_recordRange.first;
}


}
