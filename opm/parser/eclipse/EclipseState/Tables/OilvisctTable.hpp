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
#ifndef OPM_PARSER_OILVISCT_TABLE_HPP
#define	OPM_PARSER_OILVISCT_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class EclipseState;

    class OilvisctTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class EclipseState;
        OilvisctTable() = default;

        /*!
         * \brief Read the OILVISCT keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword, int recordIdx)
        {
            ParentType::init(keyword,
                             std::vector<std::string>{
                                 "Temperature",
                                 "Viscosity"
                             },
                             recordIdx,
                             /*firstEntityOffset=*/0);

            ParentType::checkNonDefaultable("Temperature");
            ParentType::checkMonotonic("Temperature", /*isAscending=*/true);

            ParentType::checkNonDefaultable("Viscosity");
            ParentType::checkMonotonic("Viscosity", /*isAscending=*/true, /*strictlyMonotonic=*/false);
        }

    public:
        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        const std::vector<double> &getTemperatureColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getOilViscosityColumn() const
        { return ParentType::getColumn(1); }
    };
}

#endif
