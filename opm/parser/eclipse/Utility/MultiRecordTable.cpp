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
#include <opm/parser/eclipse/Utility/MultiRecordTable.hpp>

namespace Opm {
/*!
 * \brief Returns the number of tables which can be found in a
 *        given keyword.
 */
size_t MultiRecordTable::numTables(Opm::DeckKeywordConstPtr keyword)
{
    size_t result = 0;

    // first, go to the first record of the specified table. For this,
    // we need to skip the right number of empty records...
    for (size_t recordIdx = 0;
         recordIdx < keyword->size();
         ++ recordIdx)
    {
        if (getNumFlatItems_(keyword->getRecord(recordIdx)) == 0)
            // each table ends with an empty record
            ++ result;
    }

    // the last empty record of a keyword seems to go MIA for some
    // strange reason...
    ++result;

    return result;
}

// create table from first few items of multiple records
void MultiRecordTable::init(Opm::DeckKeywordConstPtr keyword,
                            const std::vector<std::string> &columnNames,
                            size_t tableIdx,
                            size_t firstEntityOffset)
{
    createColumns_(columnNames);

    // first, go to the first record of the specified table. For this,
    // we need to skip the right number of empty records...
    size_t curTableIdx = 0;
    for (m_firstRecordIdx = 0;
         curTableIdx < tableIdx;
         ++ m_firstRecordIdx)
    {
        if (getNumFlatItems_(keyword->getRecord(m_firstRecordIdx)) == 0)
            // next table starts with an empty record
            ++ curTableIdx;
    }

    if (curTableIdx != tableIdx) {
        throw std::runtime_error("keyword does not specify enough tables");
    }

    // find the number of records in the table
    for (m_numRecords = 0;
         m_firstRecordIdx + m_numRecords < keyword->size()
             && getNumFlatItems_(keyword->getRecord(m_firstRecordIdx + m_numRecords)) != 0;
         ++ m_numRecords)
    {
    }

    for (size_t  rowIdx = m_firstRecordIdx; rowIdx < m_firstRecordIdx + m_numRecords; ++ rowIdx) {
        // extract the actual data from the records of the keyword of
        // the deck
        Opm::DeckRecordConstPtr deckRecord =
            keyword->getRecord(rowIdx);

        if ( (getNumFlatItems_(deckRecord) - firstEntityOffset) < numColumns())
            throw std::runtime_error("Number of columns in the data file is"
                                     "inconsistent with the ones specified");

        for (size_t colIdx = 0; colIdx < numColumns(); ++colIdx) {
            size_t deckItemIdx = colIdx + firstEntityOffset;
            m_columns[colIdx].push_back(getFlatSiDoubleData_(deckRecord, deckItemIdx));
            m_valueDefaulted[colIdx].push_back(getFlatIsDefaulted(deckRecord, deckItemIdx));
        }
    }
}

size_t MultiRecordTable::firstRecordIndex() const
{
    return m_firstRecordIdx;
}

size_t MultiRecordTable::numRecords() const
{
    return m_numRecords;
}

size_t MultiRecordTable::getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord)
{
    int result = 0;
    for (unsigned i = 0; i < deckRecord->size(); ++ i) {
        Opm::DeckItemConstPtr item(deckRecord->getItem(i));
        if (item->size() == 0 || item->defaultApplied(0))
            return result;
        result += item->size();
    }

    return result;
}

double MultiRecordTable::getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, unsigned flatItemIdx) const
{
    unsigned itemFirstFlatIdx = 0;
    for (unsigned i = 0; i < deckRecord->size(); ++ i) {
        Opm::DeckItemConstPtr item = deckRecord->getItem(i);
        if (itemFirstFlatIdx + item->size() > flatItemIdx)
            return item->getSIDouble(flatItemIdx - itemFirstFlatIdx);
        else
            itemFirstFlatIdx += item->size();
    }

    throw std::range_error("Tried to access out-of-range flat item");
}
}
