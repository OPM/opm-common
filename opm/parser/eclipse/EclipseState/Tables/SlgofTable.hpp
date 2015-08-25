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
    class EclipseState;

    class SlgofTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class EclipseState;

        /*!
         * \brief Read the SGOF keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx)
        {
            ParentType::init(keyword,
                             std::vector<std::string>{"SL", "KRG", "KROG", "PCOG"},
                             recordIdx,
                             /*firstEntityOffset=*/0);

            ParentType::checkNonDefaultable("SL");
            ParentType::checkMonotonic("SL", /*isAscending=*/true);
            ParentType::checkMonotonic("KRG", /*isAscending=*/false, /*strictlyMonotonic=*/false);
            ParentType::checkMonotonic("KROG", /*isAscending=*/true, /*strictlyMonotonic=*/false);
            ParentType::checkMonotonic("PCOG", /*isAscending=*/false, /*strictlyMonotonic=*/false);
            ParentType::applyDefaultsLinear("KRG");
            ParentType::applyDefaultsLinear("KROG");
            ParentType::applyDefaultsLinear("PCOG");

            if (getSlColumn().back() != 1.0) {
                throw std::invalid_argument("The last saturation of the SLGOF keyword must be 1!");
            }
        }

    public:
        SlgofTable() = default;

        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        const std::vector<double> &getSlColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getKrgColumn() const
        { return ParentType::getColumn(1); }

        const std::vector<double> &getKrogColumn() const
        { return ParentType::getColumn(2); }

        // this column is p_g - p_o (non-wetting phase pressure minus
        // wetting phase pressure for a given gas saturation. the name
        // is inconsistent, but it is the one used in the Eclipse
        // manual...)
        const std::vector<double> &getPcogColumn() const
        { return ParentType::getColumn(3); }
    };
}

#endif
