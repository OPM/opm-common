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
#include "SimpleTable.hpp"

namespace Opm {
// create table from single record
SimpleTable::SimpleTable(Opm::DeckKeywordConstPtr keyword,
                         const std::vector<std::string> &columnNames,
                         int recordIdx,
                         int firstEntityOffset)
{
    createColumns_(columnNames);

    // extract the actual data from the deck
    Opm::DeckRecordConstPtr deckRecord =
        keyword->getRecord(recordIdx);

    int numFlatItems = getNumFlatItems_(deckRecord);
    if ( (numFlatItems - firstEntityOffset) % numColumns() != 0)
        throw std::runtime_error("Number of columns in the data file is"
                                 "inconsistent with the ones specified");

    for (int rowIdx = 0;
         rowIdx*numColumns() < numFlatItems - firstEntityOffset;
         ++rowIdx)
    {
        for (int colIdx = 0; colIdx < numColumns(); ++colIdx) {
            int deckItemIdx = rowIdx*numColumns() + firstEntityOffset + colIdx;
            m_columns[colIdx].push_back(getFlatSiDoubleData_(deckRecord, deckItemIdx));
        }
    }
}

void SimpleTable::createColumns_(const std::vector<std::string> &columnNames)
{
    // Allocate column names. TODO (?): move the column names into
    // the json description of the keyword.
    auto columnNameIt = columnNames.begin();
    const auto &columnNameEndIt = columnNames.end();
    int columnIdx = 0;
    for (; columnNameIt != columnNameEndIt; ++columnNameIt, ++columnIdx) {
        m_columnNames[*columnNameIt] = columnIdx;
    }
    m_columns.resize(columnIdx);
}

int SimpleTable::getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord) const
{
    int result = 0;
    for (int i = 0; i < deckRecord->size(); ++ i) {
        result += deckRecord->getItem(i)->size();
    }

    return result;
}

double SimpleTable::getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, int flatItemIdx) const
{
    int itemFirstFlatIdx = 0;
    for (int i = 0; i < deckRecord->size(); ++ i) {
        Opm::DeckItemConstPtr item = deckRecord->getItem(i);
        if (itemFirstFlatIdx + item->size() > flatItemIdx)
            return item->getSIDouble(flatItemIdx - itemFirstFlatIdx);
        else
            itemFirstFlatIdx += item->size();
    }

    throw std::range_error("Tried to access out-of-range flat item");
}
}
