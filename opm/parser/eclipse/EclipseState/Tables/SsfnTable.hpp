/*
  Copyright (C) 2015 IRIS AS

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
#ifndef OPM_PARSER_SSFN_TABLE_HPP
#define OPM_PARSER_SSFN_TABLE_HPP

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class SsfnTable : public SimpleTable {
    public:
        friend class TableManager;
        SsfnTable() = default;

        /*!
         * \brief Read the SSFN keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckItemConstPtr item)
        {
            SimpleTable::init(item,
                              std::vector<std::string>{
                                  "SolventFraction",
                                      "GasRelPermMultiplier",
                                      "SolventRelPermMultiplier"});

            SimpleTable::checkNonDefaultable("SolventFraction");
            SimpleTable::checkMonotonic("SolventFraction",   /*isAscending=*/true);
            SimpleTable::checkNonDefaultable("GasRelPermMultiplier");
            SimpleTable::checkMonotonic("GasRelPermMultiplier",  /*isAscending=*/true);
            SimpleTable::checkNonDefaultable("SolventRelPermMultiplier");
            SimpleTable::checkMonotonic("SolventRelPermMultiplier", /*isAscending=*/true);
        }

        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        const std::vector<double> &getSolventFractionColumn() const
        { return SimpleTable::getColumn(0); }

        const std::vector<double> &getGasRelPermMultiplierColumn() const
        { return SimpleTable::getColumn(1); }

        const std::vector<double> &getSolventRelPermMultiplierColumn() const
        { return SimpleTable::getColumn(2); }
    };
}

#endif

