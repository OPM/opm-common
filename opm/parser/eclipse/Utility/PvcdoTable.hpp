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
#ifndef OPM_PARSER_PVCDO_TABLE_HPP
#define	OPM_PARSER_PVCDO_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {
    class PvcdoTable : protected SimpleTable {
        typedef SimpleTable ParentType;

    public:
        /*!
         * \brief Read the PVCDO keyword and provide some convenience
         *        methods for it.
         */
        PvcdoTable(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx = 0,
                  int firstEntityOffset = 0)
            : SimpleTable(keyword,
                          std::vector<std::string>{"P", "BW", "CW", "MUW", "CMUW"},
                          recordIdx, firstEntityOffset)
        {}

        int numRows() const
        { return ParentType::numRows(); };

        int numColumns() const
        { return ParentType::numColumns(); };

        const std::vector<double> &getPressureColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getFormationFactorColumn() const
        { return ParentType::getColumn(1); }

        const std::vector<double> &getCompressibilityColumn() const
        { return ParentType::getColumn(2); }

        const std::vector<double> &getViscosityColumn() const
        { return ParentType::getColumn(3); }

        const std::vector<double> &getViscosibilityColumn() const
        { return ParentType::getColumn(4); }
    };
}

#endif	// OPM_PARSER_PVCDO_TABLE_HPP

