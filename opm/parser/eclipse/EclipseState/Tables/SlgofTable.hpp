/*
  Copyright (C) 2015 by Andreas Lauser

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
#ifndef OPM_PARSER_SLGOF_TABLE_HPP
#define	OPM_PARSER_SLGOF_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class SlgofTable : protected SingleRecordTable {
        

        friend class TableManager;

        /*!
         * \brief Read the SGOF keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx)
        {
            SingleRecordTable::init(keyword,
                             std::vector<std::string>{"SL", "KRG", "KROG", "PCOG"},
                             recordIdx,
                             /*firstEntityOffset=*/0);

            SingleRecordTable::checkNonDefaultable("SL");
            SingleRecordTable::checkMonotonic("SL", /*isAscending=*/true);
            SingleRecordTable::checkMonotonic("KRG", /*isAscending=*/false, /*strictlyMonotonic=*/false);
            SingleRecordTable::checkMonotonic("KROG", /*isAscending=*/true, /*strictlyMonotonic=*/false);
            SingleRecordTable::checkMonotonic("PCOG", /*isAscending=*/false, /*strictlyMonotonic=*/false);
            SingleRecordTable::applyDefaultsLinear("KRG");
            SingleRecordTable::applyDefaultsLinear("KROG");
            SingleRecordTable::applyDefaultsLinear("PCOG");

            if (getSlColumn().back() != 1.0) {
                throw std::invalid_argument("The last saturation of the SLGOF keyword must be 1!");
            }
        }

    public:
        SlgofTable() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckKeywordConstPtr keyword, size_t tableIdx)
        { init(keyword, tableIdx); }
#endif

        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        const std::vector<double> &getSlColumn() const
        { return SingleRecordTable::getColumn(0); }

        const std::vector<double> &getKrgColumn() const
        { return SingleRecordTable::getColumn(1); }

        const std::vector<double> &getKrogColumn() const
        { return SingleRecordTable::getColumn(2); }

        // this column is p_g - p_o (non-wetting phase pressure minus
        // wetting phase pressure for a given gas saturation. the name
        // is inconsistent, but it is the one used in the Eclipse
        // manual...)
        const std::vector<double> &getPcogColumn() const
        { return SingleRecordTable::getColumn(3); }
    };
}

#endif
