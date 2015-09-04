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
#ifndef OPM_PARSER_PVTO_INNER_TABLE_HPP
#define	OPM_PARSER_PVTO_INNER_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declarations
    template <class OuterTable, class InnerTable>
    class FullTable;
    class PvtoTable;
    class PvtoOuterTable;
    class PvtoInnerTable;

    class PvtoInnerTable : protected MultiRecordTable {
        

        friend class PvtoTable;
        friend class FullTable<PvtoOuterTable, PvtoInnerTable>;
        PvtoInnerTable() = default;

        /*!
         * \brief Read the per record table of the PVTO keyword and
         *        provide some convenience methods for it.
         *
         * The first value of the record (-> Rs) is skipped.
         */
        void init(Opm::DeckKeywordConstPtr keyword, int recordIdx)
        {
            SingleRecordTable::init(keyword,
                             std::vector<std::string>{"P", "BO", "MU"},
                             recordIdx,
                             /*firstEntityOffset=*/1);

            SingleRecordTable::checkNonDefaultable("P");
            SingleRecordTable::checkMonotonic("P", /*isAscending=*/true);
            SingleRecordTable::applyDefaultsLinear("BO");
            SingleRecordTable::applyDefaultsLinear("MU");
        }

    public:
        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        const std::vector<double> &getPressureColumn() const
        { return SingleRecordTable::getColumn(0); }

        const std::vector<double> &getOilFormationFactorColumn() const
        { return SingleRecordTable::getColumn(1); }

        const std::vector<double> &getOilViscosityColumn() const
        { return SingleRecordTable::getColumn(2); }
    };
}

#endif
