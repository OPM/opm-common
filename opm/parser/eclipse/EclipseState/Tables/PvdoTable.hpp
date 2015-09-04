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
#ifndef OPM_PARSER_PVDO_TABLE_HPP
#define	OPM_PARSER_PVDO_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class PvdoTable : protected SingleRecordTable {
        

        friend class TableManager;
        PvdoTable() = default;

        /*!
         * \brief Read the PVDO keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SingleRecordTable::init(record,
                             std::vector<std::string>{"P", "BO", "MUO"},
                             /*firstEntityOffset=*/0);

            SingleRecordTable::checkNonDefaultable("P");
            SingleRecordTable::checkMonotonic("P", /*isAscending=*/true);

            SingleRecordTable::applyDefaultsLinear("BO");
            SingleRecordTable::checkMonotonic("BO", /*isAscending=*/false);

            SingleRecordTable::applyDefaultsLinear("MUO");
            SingleRecordTable::checkMonotonic("MUO", /*isAscending=*/true, /*strictlyMonotonic=*/false);
        }

    public:
        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        const std::vector<double> &getPressureColumn() const
        { return SingleRecordTable::getColumn(0); }

        const std::vector<double> &getFormationFactorColumn() const
        { return SingleRecordTable::getColumn(1); }

        const std::vector<double> &getViscosityColumn() const
        { return SingleRecordTable::getColumn(2); }
    };
}

#endif
