/*
  Copyright (C) 2014 by Andreas Lauser

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
#ifndef OPM_PARSER_SWOF_TABLE_HPP
#define	OPM_PARSER_SWOF_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {
    class SwofTable : protected SimpleTable {
        typedef SimpleTable ParentType;

    public:
        using ParentType::numTables;

        /*!
         * \brief Read the SWOF keyword and provide some convenience
         *        methods for it.
         */
        SwofTable(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx = 0,
                  int firstEntityOffset = 0)
            : SimpleTable(keyword,
                          std::vector<std::string>{"SW", "KRW", "KROW", "PCOW"},
                          recordIdx, firstEntityOffset)
        {}

        int numRows() const
        { return ParentType::numRows(); };

        int numColumns() const
        { return ParentType::numColumns(); };

        const std::vector<double> &getSwColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getKrwColumn() const
        { return ParentType::getColumn(1); }

        const std::vector<double> &getKrowColumn() const
        { return ParentType::getColumn(2); }

        // this column is p_o - p_w (non-wetting phase pressure minus
        // wetting phase pressure for a given water saturation)
        const std::vector<double> &getPcowColumn() const
        { return ParentType::getColumn(3); }
    };
}

#endif	// OPM_PARSER_SIMPLE_TABLE_HPP

