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
#ifndef OPM_PARSER_PVTG_OUTER_TABLE_HPP
#define	OPM_PARSER_PVTG_OUTER_TABLE_HPP

#include "MultiRecordTable.hpp"

namespace Opm {
    // forward declarations
    template <class OuterTable, class InnerTable>
    class FullTable;
    class PvtgTable;
    class PvtgOuterTable;
    class PvtgInnerTable;

    class PvtgOuterTable : protected MultiRecordTable {
        friend class PvtgTable;
        friend class FullTable<PvtgOuterTable, PvtgInnerTable>;
        PvtgOuterTable() = default;

        /*!
         * \brief Read the per record table of the PVTG keyword and
         *        provide some convenience methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword, size_t tableIdx)
        {
            MultiRecordTable::init(keyword,
                                   std::vector<std::string>{"P", "RV", "BG", "MUG"},
                                   tableIdx);

            MultiRecordTable::checkNonDefaultable("P");
            MultiRecordTable::checkMonotonic("P", /*isAscending=*/true);
            MultiRecordTable::applyDefaultsLinear("RV");
            MultiRecordTable::applyDefaultsLinear("BG");
            MultiRecordTable::applyDefaultsLinear("MUG");
        }

    public:
        using MultiRecordTable::numTables;
        using MultiRecordTable::numRows;
        using MultiRecordTable::numColumns;
        using MultiRecordTable::evaluate;
        using MultiRecordTable::firstRecordIndex;
        using MultiRecordTable::numRecords;

        const std::vector<double> &getPressureColumn() const
        { return MultiRecordTable::getColumn(0); }

        const std::vector<double> &getOilSolubilityColumn() const
        { return MultiRecordTable::getColumn(1); }

        const std::vector<double> &getGasFormationFactorColumn() const
        { return MultiRecordTable::getColumn(2); }

        const std::vector<double> &getGasViscosityColumn() const
        { return MultiRecordTable::getColumn(3); }
    };
}

#endif
