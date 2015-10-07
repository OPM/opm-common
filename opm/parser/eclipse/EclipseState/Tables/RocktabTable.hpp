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

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class RocktabTable : public  SimpleTable {
    public:
        friend class TableManager;
        RocktabTable() = default;

        /*!
         * \brief Read the ROCKTAB keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckItemConstPtr item,
                  bool isDirectional,
                  bool hasStressOption)
        {
            SimpleTable::init(item,
                              isDirectional
                              ? std::vector<std::string>{"PO", "PV_MULT", "TRANSMIS_MULT_X", "TRANSMIS_MULT_Y", "TRANSMIS_MULT_Z"}
                              : std::vector<std::string>{"PO", "PV_MULT", "TRANSMIS_MULT"});
            m_isDirectional = isDirectional;

            SimpleTable::checkNonDefaultable("PO");
            SimpleTable::checkMonotonic("PO", /*isAscending=*/hasStressOption);
            SimpleTable::applyDefaultsLinear("PV_MULT");
            if (isDirectional) {
                SimpleTable::applyDefaultsLinear("TRANSMIS_MULT");
            } else {
                SimpleTable::applyDefaultsLinear("TRANSMIS_MULT_X");
                SimpleTable::applyDefaultsLinear("TRANSMIS_MULT_Y");
                SimpleTable::applyDefaultsLinear("TRANSMIS_MULT_Z");
            }
        }

        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        const std::vector<double> &getPressureColumn() const
        { return SimpleTable::getColumn(0); }

        const std::vector<double> &getPoreVolumeMultiplierColumn() const
        { return SimpleTable::getColumn(1); }

        const std::vector<double> &getTransmissibilityMultiplierColumn() const
        { return SimpleTable::getColumn(2); }

        const std::vector<double> &getTransmissibilityMultiplierXColumn() const
        { return SimpleTable::getColumn(2); }

        const std::vector<double> &getTransmissibilityMultiplierYColumn() const
        {
            if (!m_isDirectional)
                return SimpleTable::getColumn(2);
            return SimpleTable::getColumn(3);
        }

        const std::vector<double> &getTransmissibilityMultiplierZColumn() const
        {
            if (!m_isDirectional)
                return SimpleTable::getColumn(2);
            return SimpleTable::getColumn(4);
        }

    private:
        bool m_isDirectional;
    };
}

#endif
