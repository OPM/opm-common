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
#include <opm/parser/eclipse/EclipseState/Tables/SimpleTable.hpp>

namespace Opm {
size_t SimpleTable::numTables(Opm::DeckKeywordConstPtr keyword)
{
    return keyword->size();
}

// create table from single record
void SimpleTable::init(Opm::DeckRecordConstPtr deckRecord,
                             const std::vector<std::string> &columnNames,
                             size_t firstEntityOffset)
{
    createColumns(columnNames);

    size_t numFlatItems = getNumFlatItems(deckRecord);
    if ( (numFlatItems - firstEntityOffset) % numColumns() != 0)
        throw std::runtime_error("Number of columns in the data file is"
                                 "inconsistent with the ones specified");

    for (size_t rowIdx = 0;
         rowIdx*numColumns() < numFlatItems - firstEntityOffset;
         ++rowIdx)
    {
        for (size_t colIdx = 0; colIdx < numColumns(); ++colIdx) {
            size_t deckItemIdx = rowIdx*numColumns() + firstEntityOffset + colIdx;
            m_columns[colIdx].push_back(getFlatSiDoubleData(deckRecord, deckItemIdx));
            m_valueDefaulted[colIdx].push_back(getFlatIsDefaulted(deckRecord, deckItemIdx));
        }
    }
}

size_t SimpleTable::numColumns() const
{ return m_columns.size(); }

size_t SimpleTable::numRows() const
{ return m_columns[0].size(); }

const std::vector<double> &SimpleTable::getColumn(const std::string &name) const
{
    const auto &colIt = m_columnNames.find(name);
    if (colIt == m_columnNames.end())
        throw std::runtime_error("Unknown column name \""+name+"\"");

    size_t colIdx = colIt->second;
    assert(colIdx < static_cast<size_t>(m_columns.size()));
    return m_columns[colIdx];
}
const std::vector<double> &SimpleTable::getColumn(size_t colIdx) const
{
    assert(colIdx < static_cast<size_t>(m_columns.size()));
    return m_columns[colIdx];
}

double SimpleTable::evaluate(const std::string& columnName, double xPos) const
{
    const std::vector<double>& xColumn = getColumn(0);
    const std::vector<double>& yColumn = getColumn(columnName);

    bool isDescending = false;
    if (xColumn.front() > xColumn.back())
        isDescending = true;

    // handle the constant interpolation cases
    if (isDescending) {
        if (xColumn.front() < xPos)
            return yColumn.front();
        else if (xPos < xColumn.back())
            return yColumn.back();
    }
    else {
        if (xPos < xColumn.front())
            return yColumn.front();
        else if (xColumn.back() < xPos)
            return yColumn.back();
    }

    // find the interval which contains the x position using interval halfing
    size_t lowIntervalIdx = 0;
    size_t intervalIdx = (xColumn.size() - 1)/2;
    size_t highIntervalIdx = xColumn.size()-1;
    while (lowIntervalIdx + 1 < highIntervalIdx) {
        if (isDescending) {
            if (xColumn[intervalIdx] < xPos)
                highIntervalIdx = intervalIdx;
            else
                lowIntervalIdx = intervalIdx;
        }
        else {
            if (xColumn[intervalIdx] < xPos)
                lowIntervalIdx = intervalIdx;
            else
                highIntervalIdx = intervalIdx;
        }

        intervalIdx = (highIntervalIdx + lowIntervalIdx)/2;
    }

    // use linear interpolation if the x position is in between two non-defaulted
    // values, else use the non-defaulted value
    double alpha = (xPos - xColumn[intervalIdx])/(xColumn[intervalIdx + 1] - xColumn[intervalIdx]);
    return yColumn[intervalIdx]*(1-alpha) + yColumn[intervalIdx + 1]*alpha;
}

void SimpleTable::checkNonDefaultable(const std::string& columnName)
{
    int columnIdx = m_columnNames.at(columnName);

    int nRows = numRows();

    for (int rowIdx = 0; rowIdx < nRows; ++rowIdx) {
        if (m_valueDefaulted[columnIdx][rowIdx])
            throw std::invalid_argument("Column " + columnName + " is not defaultable");
    }
}


void SimpleTable::checkMonotonic(const std::string& columnName,
                                        bool isAscending,
                                        bool isStrictlyMonotonic)
{
    int columnIdx = m_columnNames.at(columnName);

    int nRows = numRows();

    for (int rowIdx = 0; rowIdx < nRows; ++rowIdx) {
        if (rowIdx > 0) {
            if (isAscending && m_columns[columnIdx][rowIdx] < m_columns[columnIdx][rowIdx - 1])
                throw std::invalid_argument("Column " + columnName + " must be monotonically increasing");
            else if (!isAscending && m_columns[columnIdx][rowIdx] > m_columns[columnIdx][rowIdx - 1])
                throw std::invalid_argument("Column " + columnName + " must be monotonically decreasing");

            if (isStrictlyMonotonic) {
                if (m_columns[columnIdx][rowIdx] == m_columns[columnIdx][rowIdx - 1])
                    throw std::invalid_argument("Column " + columnName + " must be strictly monotonic");
            }
        }
    }
}

void SimpleTable::applyDefaultsConstant(const std::string& columnName, double value)
{
    int columnIdx = m_columnNames.at(columnName);
    int nRows = numRows();

    for (int rowIdx = 0; rowIdx < nRows; ++rowIdx) {
        if (m_valueDefaulted[columnIdx][rowIdx]) {
            m_columns[columnIdx][rowIdx] = value;
            m_valueDefaulted[columnIdx][rowIdx] = false;
        }
    }
}

void SimpleTable::applyDefaultsLinear(const std::string& columnName)
{
    int columnIdx = m_columnNames.at(columnName);
    const std::vector<double>& xColumn = m_columns[0];
    std::vector<double>& yColumn = m_columns[columnIdx];
    int nRows = numRows();

    for (int rowIdx = 0; rowIdx < nRows; ++rowIdx) {
        if (m_valueDefaulted[columnIdx][rowIdx]) {
            // find first row which was not defaulted before the current one
            int rowBeforeIdx = rowIdx;
            for (; rowBeforeIdx >= 0; -- rowBeforeIdx)
                if (!m_valueDefaulted[columnIdx][rowBeforeIdx])
                    break;

            // find first row which was not defaulted after the current one
            int rowAfterIdx = rowIdx;
            for (; rowAfterIdx < static_cast<int>(yColumn.size()); ++ rowAfterIdx)
                if (!m_valueDefaulted[columnIdx][rowAfterIdx])
                    break;

            // switch to extrapolation by a constant at the fringes
            if (rowBeforeIdx < 0 && rowAfterIdx >= static_cast<int>(yColumn.size()))
                throw std::invalid_argument("Column " + columnName + " can't be fully defaulted");
            else if (rowBeforeIdx < 0)
                rowBeforeIdx = rowAfterIdx;
            else if (rowAfterIdx >= static_cast<int>(yColumn.size()))
                rowAfterIdx = rowBeforeIdx;

            // linear interpolation
            double alpha = 0.0;
            if (rowBeforeIdx != rowAfterIdx) {
                alpha = xColumn[rowIdx] - xColumn[rowBeforeIdx];
                alpha /= (xColumn[rowAfterIdx] - xColumn[rowBeforeIdx]);
            }

            double value =
                yColumn[rowBeforeIdx]*(1-alpha) + yColumn[rowAfterIdx]*alpha;

            m_columns[columnIdx][rowIdx] = value;
            m_valueDefaulted[columnIdx][rowIdx] = false;
        }
    }
}

void SimpleTable::createColumns(const std::vector<std::string> &columnNames)
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
    m_valueDefaulted.resize(columnIdx);
}

size_t SimpleTable::getNumFlatItems(Opm::DeckRecordConstPtr deckRecord) const
{
    size_t result = 0;
    for (size_t i = 0; i < deckRecord->size(); ++ i) {
        result += deckRecord->getItem(i)->size();
    }

    return result;
}

double SimpleTable::getFlatRawDoubleData(Opm::DeckRecordConstPtr deckRecord, size_t flatItemIdx) const
{
    size_t itemFirstFlatIdx = 0;
    for (unsigned i = 0; i < deckRecord->size(); ++ i) {
        Opm::DeckItemConstPtr item = deckRecord->getItem(i);
        if (itemFirstFlatIdx + item->size() > flatItemIdx)
            return item->getRawDouble(flatItemIdx - itemFirstFlatIdx);
        else
            itemFirstFlatIdx += item->size();
    }

    throw std::range_error("Tried to access out-of-range flat item");
}

double SimpleTable::getFlatSiDoubleData(Opm::DeckRecordConstPtr deckRecord, size_t flatItemIdx) const
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

bool SimpleTable::getFlatIsDefaulted(Opm::DeckRecordConstPtr deckRecord, size_t flatItemIdx) const
{
    size_t itemFirstFlatIdx = 0;
    for (unsigned i = 0; i < deckRecord->size(); ++ i) {
        Opm::DeckItemConstPtr item = deckRecord->getItem(i);
        if (itemFirstFlatIdx + item->size() > flatItemIdx)
            return item->defaultApplied(flatItemIdx - itemFirstFlatIdx);
        else
            itemFirstFlatIdx += item->size();
    }

    throw std::range_error("Tried to access out-of-range flat item");
}
}
