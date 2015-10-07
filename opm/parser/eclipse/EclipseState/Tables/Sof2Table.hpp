/*
  Copyright (C) 2012 IRIS AS

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
#ifndef OPM_PARSER_SOF2_TABLE_HPP
#define OPM_PARSER_SOF2_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class Sof2Table : public SimpleTable {
        friend class TableManager;

        /*!
         * \brief Read the SOF2 keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckItemConstPtr item)
        {
            SimpleTable::init(item,
                              std::vector<std::string>{"SO", "KRO" });

            SimpleTable::checkNonDefaultable("SO");
            SimpleTable::checkNonDefaultable("KRO");
            SimpleTable::checkMonotonic("SO", /*isAscending=*/true);
            SimpleTable::checkMonotonic("KRO", /*isAscending=*/true, /*strict*/false);
        }

    public:
        Sof2Table() = default;

#ifdef BOOST_TEST_MODULE
        // DO NOT TRY TO CALL THIS METHOD! it is only for the unit tests!
        void initFORUNITTESTONLY(Opm::DeckItemConstPtr item)
        { init(item); }
#endif

        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        const std::vector<double> &getSoColumn() const
        { return SimpleTable::getColumn(0); }

        const std::vector<double> &getKroColumn() const
        { return SimpleTable::getColumn(1); }
    };
}

#endif

