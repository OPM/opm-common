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
#ifndef OPM_PARSER_ENPTVD_TABLE_HPP
#define	OPM_PARSER_ENPTVD_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class EnptvdTable : protected SingleRecordTable {


        friend class TableManager;
        EnptvdTable() = default;

        /*!
         * \brief Read the ENPTVD keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SingleRecordTable::init(record,
                             std::vector<std::string>{"DEPTH",
                                     "SWCO",
                                     "SWCRIT",
                                     "SWMAX",
                                     "SGCO",
                                     "SGCRIT",
                                     "SGMAX",
                                     "SOWCRIT",
                                     "SOGCRIT"},
                             /*firstEntityOffset=*/0);

            SingleRecordTable::checkNonDefaultable("DEPTH");
            SingleRecordTable::checkMonotonic("DEPTH", /*isAscending=*/true);
            SingleRecordTable::applyDefaultsLinear("SWCO");
            SingleRecordTable::applyDefaultsLinear("SWCRIT");
            SingleRecordTable::applyDefaultsLinear("SWMAX");
            SingleRecordTable::applyDefaultsLinear("SGCO");
            SingleRecordTable::applyDefaultsLinear("SGCRIT");
            SingleRecordTable::applyDefaultsLinear("SGMAX");
            SingleRecordTable::applyDefaultsLinear("SOWCRIT");
            SingleRecordTable::applyDefaultsLinear("SOGCRIT");
        }

    public:
        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        // using this method is strongly discouraged but the current endpoint scaling
        // code makes it hard to avoid
        using SingleRecordTable::getColumn;

        const std::vector<double> &getDepthColumn() const
        { return SingleRecordTable::getColumn(0); }

        /*!
         * \brief Connate water saturation
         */
        const std::vector<double> &getSwcoColumn() const
        { return SingleRecordTable::getColumn(1); }

        /*!
         * \brief Critical water saturation
         */
        const std::vector<double> &getSwcritColumn() const
        { return SingleRecordTable::getColumn(2); }

        /*!
         * \brief Maximum water saturation
         */
        const std::vector<double> &getSwmaxColumn() const
        { return SingleRecordTable::getColumn(3); }

        /*!
         * \brief Connate gas saturation
         */
        const std::vector<double> &getSgcoColumn() const
        { return SingleRecordTable::getColumn(4); }

        /*!
         * \brief Critical gas saturation
         */
        const std::vector<double> &getSgcritColumn() const
        { return SingleRecordTable::getColumn(5); }

        /*!
         * \brief Maximum gas saturation
         */
        const std::vector<double> &getSgmaxColumn() const
        { return SingleRecordTable::getColumn(6); }

        /*!
         * \brief Critical oil-in-water saturation
         */
        const std::vector<double> &getSowcritColumn() const
        { return SingleRecordTable::getColumn(7); }

        /*!
         * \brief Critical oil-in-gas saturation
         */
        const std::vector<double> &getSogcritColumn() const
        { return SingleRecordTable::getColumn(8); }
    };
}

#endif

