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
    class TableManager;

    class RocktabTable : protected SingleRecordTable {
        

        friend class TableManager;
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
            SingleRecordTable::init(keyword,
                             isDirectional
                             ? std::vector<std::string>{"PO", "PV_MULT", "TRANSMIS_MULT_X", "TRANSMIS_MULT_Y", "TRANSMIS_MULT_Z"}
                             : std::vector<std::string>{"PO", "PV_MULT", "TRANSMIS_MULT"},
                             recordIdx,
                             /*firstEntityOffset=*/0);
            m_isDirectional = isDirectional;

            SingleRecordTable::checkNonDefaultable("PO");
            SingleRecordTable::checkMonotonic("PO", /*isAscending=*/hasStressOption);
            SingleRecordTable::applyDefaultsLinear("PV_MULT");
            if (isDirectional) {
                SingleRecordTable::applyDefaultsLinear("TRANSMIS_MULT");
            } else {
                SingleRecordTable::applyDefaultsLinear("TRANSMIS_MULT_X");
                SingleRecordTable::applyDefaultsLinear("TRANSMIS_MULT_Y");
                SingleRecordTable::applyDefaultsLinear("TRANSMIS_MULT_Z");
            }
        }

    public:
        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        const std::vector<double> &getPressureColumn() const
        { return SingleRecordTable::getColumn(0); }

        const std::vector<double> &getPoreVolumeMultiplierColumn() const
        { return SingleRecordTable::getColumn(1); }

        const std::vector<double> &getTransmissibilityMultiplierColumn() const
        { return SingleRecordTable::getColumn(2); }

        const std::vector<double> &getTransmissibilityMultiplierXColumn() const
        { return SingleRecordTable::getColumn(2); }

        const std::vector<double> &getTransmissibilityMultiplierYColumn() const
        {
            if (!m_isDirectional)
                return SingleRecordTable::getColumn(2);
            return SingleRecordTable::getColumn(3);
        }

        const std::vector<double> &getTransmissibilityMultiplierZColumn() const
        {
            if (!m_isDirectional)
                return SingleRecordTable::getColumn(2);
            return SingleRecordTable::getColumn(4);
        }

    private:
        bool m_isDirectional;
    };
}

#endif
