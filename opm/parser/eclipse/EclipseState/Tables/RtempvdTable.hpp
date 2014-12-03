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
#ifndef OPM_PARSER_RTEMPVD_TABLE_HPP
#define	OPM_PARSER_RTEMPVD_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class EclipseState;

    class RtempvdTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class EclipseState;
        RtempvdTable() = default;

        /*!
         * \brief Read the RTEMPVD keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword, int recordIdx)
        {
            ParentType::init(keyword,
                             std::vector<std::string>{
                                 "Depth",
                                 "Temperature"
                             },
                             recordIdx,
                             /*firstEntityOffset=*/0);

            ParentType::checkNonDefaultable("Depth");
            ParentType::checkMonotonic("Depth", /*isAscending=*/true);

            ParentType::checkNonDefaultable("Temperature");
        }

    public:
        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        const std::vector<double> &getDepthColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getTemperatureColumn() const
        { return ParentType::getColumn(1); }
    };
}

#endif
