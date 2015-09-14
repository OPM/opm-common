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
#ifndef OPM_PARSER_PVTG_INNER_TABLE_HPP
#define	OPM_PARSER_PVTG_INNER_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {
    // forward declarations
    template <class OuterTable, class InnerTable>
    class FullTable;
    class PvtgTable;
    class PvtgOuterTable;
    class PvtgInnerTable;

    class PvtgInnerTable : protected MultiRecordTable {


        friend class PvtgTable;
        friend class FullTable<PvtgOuterTable, PvtgInnerTable>;
        PvtgInnerTable() = default;

        /*!
         * \brief Read the per record table of the PVTG keyword and
         *        provide some convenience methods for it.
         *
         * The first value of the record (-> Rv) is skipped.
         */
        void init(Opm::DeckItemConstPtr item)
        {
            SimpleTable::init(item,
                              std::vector<std::string>{"RV", "BG", "MUG"});

            SimpleTable::checkNonDefaultable("RV");
            SimpleTable::checkMonotonic("RV", /*isAscending=*/false);
            SimpleTable::applyDefaultsLinear("BG");
            SimpleTable::applyDefaultsLinear("MUG");
        }

    public:
        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        const std::vector<double> &getOilSolubilityColumn() const
        { return SimpleTable::getColumn(0); }

        const std::vector<double> &getGasFormationFactorColumn() const
        { return SimpleTable::getColumn(1); }

        const std::vector<double> &getGasViscosityColumn() const
        { return SimpleTable::getColumn(2); }
    };
}

#endif
