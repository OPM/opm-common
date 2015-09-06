/*
  Copyright 2015 IRIS AS


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
#ifndef OPM_PARSER_PVDS_TABLE_HPP
#define OPM_PARSER_PVDS_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class PvdsTable : protected SimpleTable {
        

        friend class TableManager;

        PvdsTable() = default;

        /*!
         * \brief Read the PVDS keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SimpleTable::init(record,
                             std::vector<std::string>{"P", "BG", "MUG"},
                             /*firstEntityOffset=*/0);

            SimpleTable::checkNonDefaultable("P");
            SimpleTable::checkMonotonic("P", /*isAscending=*/true);

            SimpleTable::applyDefaultsLinear("BG");
            SimpleTable::checkMonotonic("BG", /*isAscending=*/false);

            SimpleTable::applyDefaultsLinear("MUG");
            SimpleTable::checkMonotonic("MUG", /*isAscending=*/true, /*strictlyMonotonic=*/false);
        }

    public:
        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        const std::vector<double> &getPressureColumn() const
        { return SimpleTable::getColumn(0); }

        const std::vector<double> &getFormationFactorColumn() const
        { return SimpleTable::getColumn(1); }

        const std::vector<double> &getViscosityColumn() const
        { return SimpleTable::getColumn(2); }
    };
}

#endif
