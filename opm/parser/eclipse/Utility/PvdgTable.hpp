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
#ifndef OPM_PARSER_PVDG_TABLE_HPP
#define	OPM_PARSER_PVDG_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    class PvdgTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

    public:
        PvdgTable() = default;

        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        /*!
         * \brief Read the PVDG keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx)
        {
            ParentType::init(keyword,
                             std::vector<std::string>{"P", "BG", "MUG"},
                             recordIdx,
                             /*firstEntityOffset=*/0);

            ParentType::healDefaultsLinear_("BG");
            ParentType::checkNonDefaultable_("BG", /*isAscending=*/false);

        }

        const std::vector<double> &getPressureColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getFormationFactorColumn() const
        { return ParentType::getColumn(1); }

        const std::vector<double> &getViscosityColumn() const
        { return ParentType::getColumn(2); }
    };
}

#endif
