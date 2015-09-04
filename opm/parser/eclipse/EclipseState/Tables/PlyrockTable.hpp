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
#ifndef OPM_PARSER_PLYROCK_TABLE_HPP
#define	OPM_PARSER_PLYROCK_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class PlyrockTable : protected SingleRecordTable {

        friend class TableManager;
        PlyrockTable() = default;

        /*!
         * \brief Read the PLYROCK keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SingleRecordTable::init(record,
                             std::vector<std::string>{
                                 "DeadPoreVolume",
                                 "ResidualResistanceFactor",
                                 "RockDensityFactor",
                                 "AdsorbtionIndex",
                                 "MaxAdsorbtion"
                             },
                             /*firstEntityOffset=*/0);

            // the entries of this keyword cannot be defaulted except for the
            // forth. ensure this.
            int nRows = numRows();
            int nCols = numColumns();
            for (int rowIdx = 0; rowIdx < nRows; ++rowIdx) {
                for (int colIdx = 0; colIdx < nCols; ++colIdx) {
                    if (m_valueDefaulted[colIdx][rowIdx]) {
                        if (colIdx == 3) {
                            m_valueDefaulted[colIdx][rowIdx] = false;
                            m_columns[colIdx][rowIdx] = 1.0;
                        }
                        else
                            throw std::invalid_argument("The values of the PLYROCK table cannot be defaulted");
                    }
                }
            }
        }

    public:
        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;

        // since this keyword is not necessarily monotonic, it cannot be evaluated!
        //using SingleRecordTable::evaluate;

        const std::vector<double> &getDeadPoreVolumeColumn() const
        { return SingleRecordTable::getColumn(0); }

        const std::vector<double> &getResidualResistanceFactorColumn() const
        { return SingleRecordTable::getColumn(1); }

        const std::vector<double> &getRockDensityFactorColumn() const
        { return SingleRecordTable::getColumn(2); }

        // is column is actually an integer, but this is not yet
        // supported by opm-parser (yet?) as it would require quite a
        // few changes in the table support classes (read: it would
        // probably require a lot of template vodoo) and some in the
        // JSON-to-C conversion code. In the meantime, the index is
        // just a double which can be converted to an integer in the
        // calling code. (Make sure, that you don't interpolate
        // indices, though!)
        const std::vector<double> &getAdsorbtionIndexColumn() const
        { return SingleRecordTable::getColumn(3); }

        const std::vector<double> &getMaxAdsorbtionColumn() const
        { return SingleRecordTable::getColumn(4); }
    };
}

#endif
