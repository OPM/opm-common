/*
  Copyright (C) 2014 IRIS AS

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
#ifndef OPM_PARSER_SWFN_TABLE_HPP
#define OPM_PARSER_SWFN_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class SwfnTable : protected SingleRecordTable {


        friend class TableManager;

        /*!
         * \brief Read the SWFN keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SingleRecordTable::init(record,
                             std::vector<std::string>{"SW", "KRW", "PCOW"},
                             /*firstEntityOffset=*/0);

            SingleRecordTable::checkNonDefaultable("SW");
            SingleRecordTable::checkMonotonic("SW",   /*isAscending=*/true);
            SingleRecordTable::applyDefaultsLinear("KRW");
            SingleRecordTable::applyDefaultsLinear("PCOW");
            SingleRecordTable::checkMonotonic("KRW",  /*isAscending=*/true,  /*strict=*/false);
            SingleRecordTable::checkMonotonic("PCOW", /*isAscending=*/false, /*strict=*/false);
        }

    public:
        SwfnTable() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckRecordConstPtr record)
        { init(record); }
#endif

        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        const std::vector<double> &getSwColumn() const
        { return SingleRecordTable::getColumn(0); }

        const std::vector<double> &getKrwColumn() const
        { return SingleRecordTable::getColumn(1); }

        // this column is p_o - p_w (non-wetting phase pressure minus
        // wetting phase pressure for a given water saturation)
        const std::vector<double> &getPcowColumn() const
        { return SingleRecordTable::getColumn(2); }
    };
}

#endif

