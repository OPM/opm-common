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

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class Tables;

    class SwofTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class Tables;

        /*!
         * \brief Read the SWOF keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx)
        {
            ParentType::init(keyword,
                             std::vector<std::string>{"SW", "KRW", "KROW", "PCOW"},
                             recordIdx,
                             /*firstEntityOffset=*/0);

            ParentType::checkNonDefaultable("SW");
            ParentType::checkMonotonic("SW", /*isAscending=*/true);
            ParentType::applyDefaultsLinear("KRW");
            ParentType::applyDefaultsLinear("KROW");
            ParentType::applyDefaultsLinear("PCOW");
        }

    public:
        SwofTable() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckKeywordConstPtr keyword, size_t tableIdx)
        { init(keyword, tableIdx); }
#endif

        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

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

#endif

