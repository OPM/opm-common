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

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class SlgofTable : protected SimpleTable {
        friend class TableManager;

        /*!
         * \brief Read the SGOF keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SimpleTable::init(record,
                             std::vector<std::string>{"SL", "KRG", "KROG", "PCOG"},
                             /*firstEntityOffset=*/0);

            SimpleTable::checkNonDefaultable("SL");
            SimpleTable::checkMonotonic("SL", /*isAscending=*/true);
            SimpleTable::checkMonotonic("KRG", /*isAscending=*/false, /*strictlyMonotonic=*/false);
            SimpleTable::checkMonotonic("KROG", /*isAscending=*/true, /*strictlyMonotonic=*/false);
            SimpleTable::checkMonotonic("PCOG", /*isAscending=*/false, /*strictlyMonotonic=*/false);
            SimpleTable::applyDefaultsLinear("KRG");
            SimpleTable::applyDefaultsLinear("KROG");
            SimpleTable::applyDefaultsLinear("PCOG");

            if (getSlColumn().back() != 1.0) {
                throw std::invalid_argument("The last saturation of the SLGOF keyword must be 1!");
            }
        }

    public:
        SlgofTable() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckRecordConstPtr record)
        { init(record); }
#endif

        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        const std::vector<double> &getSlColumn() const
        { return SimpleTable::getColumn(0); }

        const std::vector<double> &getKrgColumn() const
        { return SimpleTable::getColumn(1); }

        const std::vector<double> &getKrogColumn() const
        { return SimpleTable::getColumn(2); }

        // this column is p_g - p_o (non-wetting phase pressure minus
        // wetting phase pressure for a given gas saturation. the name
        // is inconsistent, but it is the one used in the Eclipse
        // manual...)
        const std::vector<double> &getPcogColumn() const
        { return SimpleTable::getColumn(3); }
    };
}

#endif
