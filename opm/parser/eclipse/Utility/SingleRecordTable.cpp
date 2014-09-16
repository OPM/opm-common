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
#include <opm/parser/eclipse/Utility/SingleRecordTable.hpp>

namespace Opm {
size_t SingleRecordTable::numTables(Opm::DeckKeywordConstPtr keyword)
{
    return keyword->size();
}

// create table from single record
void SingleRecordTable::init(Opm::DeckKeywordConstPtr keyword,
                             const std::vector<std::string> &columnNames,
                             size_t recordIdx,
                             size_t firstEntityOffset)
{
    createColumns_(columnNames);

    // extract the actual data from the deck
    Opm::DeckRecordConstPtr deckRecord =
        keyword->getRecord(recordIdx);

    size_t numFlatItems = getNumFlatItems_(deckRecord);
    if ( (numFlatItems - firstEntityOffset) % numColumns() != 0)
        throw std::runtime_error("Number of columns in the data file is"
                                 "inconsistent with the ones specified");

    for (size_t rowIdx = 0;
         rowIdx*numColumns() < numFlatItems - firstEntityOffset;
         ++rowIdx)
    {
        for (size_t colIdx = 0; colIdx < numColumns(); ++colIdx) {
            size_t deckItemIdx = rowIdx*numColumns() + firstEntityOffset + colIdx;
            m_columns[colIdx].push_back(getFlatSiDoubleData_(deckRecord, deckItemIdx));
        }
    }
}

size_t SingleRecordTable::numColumns() const
{ return m_columns.size(); }

size_t SingleRecordTable::numRows() const
{ return m_columns[0].size(); }

const std::vector<double> &SingleRecordTable::getColumn(const std::string &name) const
{
    const auto &colIt = m_columnNames.find(name);
    if (colIt == m_columnNames.end())
        throw std::runtime_error("Unknown column name \""+name+"\"");

    size_t colIdx = colIt->second;
    assert(colIdx < static_cast<size_t>(m_columns.size()));
    return m_columns[colIdx];
}
const std::vector<double> &SingleRecordTable::getColumn(size_t colIdx) const
{
    assert(colIdx < static_cast<size_t>(m_columns.size()));
    return m_columns[colIdx];
}

void SingleRecordTable::createColumns_(const std::vector<std::string> &columnNames)
{
    // Allocate column names. TODO (?): move the column names into
    // the json description of the keyword.
    auto columnNameIt = columnNames.begin();
    const auto &columnNameEndIt = columnNames.end();
    size_t columnIdx = 0;
    for (; columnNameIt != columnNameEndIt; ++columnNameIt, ++columnIdx) {
        m_columnNames[*columnNameIt] = columnIdx;
    }
    m_columns.resize(columnIdx);
}

size_t SingleRecordTable::getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord) const
{
    size_t result = 0;
    for (size_t i = 0; i < deckRecord->size(); ++ i) {
        result += deckRecord->getItem(i)->size();
    }

    return result;
}

double SingleRecordTable::getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, size_t flatItemIdx) const
{
    size_t itemFirstFlatIdx = 0;
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
