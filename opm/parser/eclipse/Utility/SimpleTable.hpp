// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
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
#ifndef OPM_PARSER_SIMPLE_TABLE_HPP
#define	OPM_PARSER_SIMPLE_TABLE_HPP

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

namespace Opm {
    class SimpleTable {
    protected:
        // protected default constructor for the derived classes
        SimpleTable() {}

    public:
        /*!
         * \brief Read simple tables from keywords like SWOF
         *
         * This requires all data to be a list of doubles in the first
         * item of a given record index.
         */
        SimpleTable(Opm::DeckKeywordConstPtr keyword,
                    const std::vector<std::string> &columnNames,
                    int recordIdx = 0,
                    int firstEntityOffset = 0);

        // constructor to make the base class compatible with specialized table implementations
        SimpleTable(Opm::DeckKeywordConstPtr keyword,
                    int recordIdx = 0,
                    int firstEntityOffset = 0)
        {
            throw std::logic_error("The base class of simple tables can't be "
                                   "instantiated without specifying columns!");
        }

        int numColumns() const
        { return m_columns.size(); }

        int numRows() const
        { return m_columns[0].size(); }

        const std::vector<double> &getColumn(const std::string &name) const
        {
            const auto &colIt = m_columnNames.find(name);
            if (colIt == m_columnNames.end())
                throw std::runtime_error("Unknown column name \""+name+"\"");

            int colIdx = colIt->second;
            assert(0 <= colIdx && colIdx < static_cast<int>(m_columns.size()));
            return m_columns[colIdx];
        }
        const std::vector<double> &getColumn(int colIdx) const
        {
            assert(0 <= colIdx && colIdx < static_cast<int>(m_columns.size()));
            return m_columns[colIdx];
        }

    protected:
        void createColumns_(const std::vector<std::string> &columnNames);

        int getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord) const;
        double getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, int flatItemIdx) const;

        std::map<std::string, int> m_columnNames;
        std::vector<std::vector<double> > m_columns;
    };

    typedef std::shared_ptr<SimpleTable> SimpleTablePtr;
    typedef std::shared_ptr<const SimpleTable> SimpleTableConstPtr;

    // create table from first few items of multiple records (i.e. getSIDoubleData() throws an exception)
    class SimpleMultiRecordTable : public SimpleTable {
    public:
        /*!
         * \brief Read simple tables from multi-item keywords like PVTW
         *
         * This creates a table out of the first N items of each of
         * the keyword's records. (N is the number of columns.)
         */
        SimpleMultiRecordTable(Opm::DeckKeywordConstPtr keyword,
                               const std::vector<std::string> &columnNames,
                               int firstEntityOffset = 0);

    private:
        int getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord) const;
        double getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, int flatItemIdx) const;
    };

    typedef std::shared_ptr<SimpleMultiRecordTable> SimpleMultiRecordTablePtr;
    typedef std::shared_ptr<const SimpleMultiRecordTable> SimpleMultiRecordTableConstPtr;

}

#endif	// OPM_PARSER_SIMPLE_TABLE_HPP

