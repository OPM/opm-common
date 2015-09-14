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
#ifndef OPM_PARSER_MULTI_RECORD_TABLE_HPP
#define	OPM_PARSER_MULTI_RECORD_TABLE_HPP

#include <opm/parser/eclipse/EclipseState/Tables/SimpleTable.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

namespace Opm {
    // forward declaration
    class EclipseState;

    // create table from first few items of multiple records (i.e. getSIDoubleData() throws an exception)
    class MultiRecordTable : public SimpleTable {
    protected:
        /*!
         * \brief Read simple tables from multi-item keywords like PVTW
         *
         * This creates a table out of the first N items of each of
         * the keyword's records. (N is the number of columns.)
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  const std::vector<std::string> &columnNames,
                  size_t tableIndex);

    public:
        MultiRecordTable() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckKeywordConstPtr keyword,
                                 const std::vector<std::string> &columnNames,
                                 size_t tableIndex)
        { init(keyword, columnNames, tableIndex); }
#endif

        /*!
         * \brief Returns the number of tables which can be found in a
         *        given keyword.
         */
        static size_t numTables(Opm::DeckKeywordConstPtr keyword);
        static std::vector<std::pair<size_t , size_t> > recordRanges(Opm::DeckKeywordConstPtr keyword);
        /*!
         * \brief Return the index of the first record which applies
         *        for this table object.
         */
        size_t firstRecordIndex() const;

        /*!
         * \brief Return the number of records which are used by this
         *        this table object.
         */
        size_t numRecords() const;

    private:
        std::pair<size_t, size_t> m_recordRange;
    };

    typedef std::shared_ptr<MultiRecordTable> MultiRecordTablePtr;
    typedef std::shared_ptr<const MultiRecordTable> MultiRecordTableConstPtr;

}

#endif

