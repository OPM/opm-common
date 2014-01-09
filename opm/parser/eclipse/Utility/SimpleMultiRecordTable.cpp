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
#include "SimpleMultiRecordTable.hpp"

namespace Opm {
// create table from first few items of multiple records (i.e. getSIDoubleData() throws an exception)
SimpleMultiRecordTable::SimpleMultiRecordTable(Opm::DeckKeywordConstPtr keyword,
                                               const std::vector<std::string> &columnNames,
                                               int firstEntityOffset)
{
    createColumns_(columnNames);
    for (int rowIdx = 0; rowIdx < keyword->size(); ++ rowIdx) {
        // extract the actual data from the records of the keyword of
        // the deck
        Opm::DeckRecordConstPtr deckRecord =
            keyword->getRecord(rowIdx);

        if ( (getNumFlatItems_(deckRecord) - firstEntityOffset) < numColumns())
            throw std::runtime_error("Number of columns in the data file is"
                                     "inconsistent with the ones specified");

        for (int colIdx = 0; colIdx < numColumns(); ++colIdx) {
            int deckItemIdx = colIdx + firstEntityOffset;
            m_columns[colIdx].push_back(getFlatSiDoubleData_(deckRecord, deckItemIdx));
        }
    }
}

int SimpleMultiRecordTable::getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord) const
{
    int result = 0;
    for (int i = 0; i < deckRecord->size(); ++ i) {
        result += deckRecord->getItem(i)->size();
    }

    return result;
}

double SimpleMultiRecordTable::getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, int flatItemIdx) const
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
