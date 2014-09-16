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
#ifndef OPM_PARSER_SINGLE_RECORD_TABLE_HPP
#define	OPM_PARSER_SINGLE_RECORD_TABLE_HPP

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

namespace Opm {
    class SingleRecordTable {
    public:
        /*!
         * \brief Returns the number of tables in a keyword.
         *
         * For simple tables, that is identical to the number of
         * records.
         */
        static size_t numTables(Opm::DeckKeywordConstPtr keyword);

        /*!
         * \brief Read simple tables from keywords like SWOF
         *
         * This requires all data to be a list of doubles in the first
         * item of a given record index.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  const std::vector<std::string> &columnNames,
                  size_t recordIdx,
                  size_t firstEntityOffset);

        size_t numColumns() const;
        size_t numRows() const;
        const std::vector<double> &getColumn(const std::string &name) const;
        const std::vector<double> &getColumn(size_t colIdx) const;

    protected:
        void createColumns_(const std::vector<std::string> &columnNames);
        bool isDefaulted_(const std::string& columnName, size_t rowIdx) const
        {
            int columnIdx = m_columnNames.at(columnName);
            return m_valueDefaulted[columnIdx][rowIdx];
        }

        size_t getNumFlatItems_(Opm::DeckRecordConstPtr deckRecord) const;
        double getFlatSiDoubleData_(Opm::DeckRecordConstPtr deckRecord, size_t flatItemIdx) const;
        bool getFlatIsDefaulted_(Opm::DeckRecordConstPtr deckRecord, size_t flatItemIdx) const;

        std::map<std::string, size_t> m_columnNames;
        std::vector<std::vector<double> > m_columns;
        std::vector<std::vector<bool> > m_valueDefaulted;
    };

    typedef std::shared_ptr<SingleRecordTable> SingleRecordTablePtr;
    typedef std::shared_ptr<const SingleRecordTable> SingleRecordTableConstPtr;
}

#endif
