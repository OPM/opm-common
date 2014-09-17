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
#ifndef OPM_PARSER_ROCKTAB_TABLE_HPP
#define	OPM_PARSER_ROCKTAB_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class EclipseState;

    class RocktabTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class EclipseState;
        RocktabTable() = default;

        /*!
         * \brief Read the ROCKTAB keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  bool isDirectional,
                  bool hasStressOption,
                  int recordIdx)
        {
            ParentType::init(keyword,
                             isDirectional
                             ? std::vector<std::string>{"PO", "PV_MULT", "TRANSMIS_MULT_X", "TRANSMIS_MULT_Y", "TRANSMIS_MULT_Z"}
                             : std::vector<std::string>{"PO", "PV_MULT", "TRANSMIS_MULT"},
                             recordIdx,
                             /*firstEntityOffset=*/0);
            m_isDirectional = isDirectional;

            ParentType::checkNonDefaultable("PO");
            ParentType::checkMonotonic("PO", /*isAscending=*/hasStressOption);
            ParentType::applyDefaultsLinear("PV_MULT");
            if (isDirectional) {
                ParentType::applyDefaultsLinear("TRANSMIS_MULT");
            } else {
                ParentType::applyDefaultsLinear("TRANSMIS_MULT_X");
                ParentType::applyDefaultsLinear("TRANSMIS_MULT_Y");
                ParentType::applyDefaultsLinear("TRANSMIS_MULT_Z");
            }
        }

    public:
        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        const std::vector<double> &getPressureColumn() const
        { return ParentType::getColumn(0); }

        const std::vector<double> &getPoreVolumeMultiplierColumn() const
        { return ParentType::getColumn(1); }

        const std::vector<double> &getTransmissibilityMultiplierColumn() const
        { return ParentType::getColumn(2); }

        const std::vector<double> &getTransmissibilityMultiplierXColumn() const
        { return ParentType::getColumn(2); }

        const std::vector<double> &getTransmissibilityMultiplierYColumn() const
        {
            if (!m_isDirectional)
                return ParentType::getColumn(2);
            return ParentType::getColumn(3);
        }

        const std::vector<double> &getTransmissibilityMultiplierZColumn() const
        {
            if (!m_isDirectional)
                return ParentType::getColumn(2);
            return ParentType::getColumn(4);
        }

    private:
        bool m_isDirectional;
    };
}

#endif
