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
#ifndef OPM_PARSER_PVTO_OUTER_TABLE_HPP
#define	OPM_PARSER_PVTO_OUTER_TABLE_HPP

#include "SimpleMultiRecordTable.hpp"

namespace Opm {
    class PvtoOuterTable : protected SimpleMultiRecordTable {
        typedef SimpleMultiRecordTable ParentType;

    public:
        /*!
         * \brief Read the per record table of the PVTO keyword and
         *        provide some convenience methods for it.
         *
         * The first value of the record (-> Rs) is skipped.
         */
        PvtoOuterTable(Opm::DeckKeywordConstPtr keyword, int tableIdx)
            : ParentType(keyword,
                         std::vector<std::string>{"RS", "P", "BO", "MU"},
                         tableIdx)
        {}

        int numRows() const
        { return ParentType::numRows(); };

        int numColumns() const
        { return ParentType::numColumns(); };

        int firstRecordIndex() const
        { return ParentType::firstRecordIndex(); }

        int numRecords() const
        { return ParentType::numRecords(); }

        const std::vector<double> &getGasSolubilityFactorColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getPressureColumn() const
        { return ParentType::getColumn(1); }

        const std::vector<double> &getFormationFactorColumn() const
        { return ParentType::getColumn(2); }

        const std::vector<double> &getViscosityColumn() const
        { return ParentType::getColumn(3); }
    };
}

#endif	// OPM_PARSER_PVTO_OUTER_TABLE_HPP

