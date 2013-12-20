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
#ifndef OPM_PARSER_FULL_TABLE_HPP
#define	OPM_PARSER_FULL_TABLE_HPP

#include "SimpleTable.hpp"

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

namespace Opm {
    class FullTable {
    protected:
        // protected default constructor for the derived classes
        FullTable() {}

    public:
        /*!
         * \brief Read full tables from keywords like PVTO
         *
         * The data for these keywords can be considered a 2D table:
         * The outer one is a multi-record table for a given state,
         * the inner one is a normal table which extends this
         * state. For the PVTO keyword, the outer table represents the
         * gas dissolution factor, pressure, volume factor and
         * viscosity at the oil's saturation point, the inner table is
         * the pressure, volume factor and viscosity of untersaturated
         * oil with the same gas dissolution factor.
         */
        FullTable(Opm::DeckKeywordConstPtr keyword,
                  const std::vector<std::string> &outerColumnNames,
                  const std::vector<std::string> &innerColumnNames);

        Opm::SimpleTableConstPtr getOuterTable() const
        { return m_outerTable; }

        Opm::SimpleTableConstPtr getInnerTable(int rowIdx) const
        {
            assert(0 <= rowIdx && rowIdx < m_innerTables.size());
            return m_innerTables[rowIdx];
        }

    protected:
        Opm::SimpleTableConstPtr m_outerTable;
        std::vector<Opm::SimpleTableConstPtr> m_innerTables;

    };

    typedef std::shared_ptr<FullTable> FullTablePtr;
    typedef std::shared_ptr<const FullTable> FullTableConstPtr;
}

#endif	// OPM_PARSER_FULL_TABLE_HPP

