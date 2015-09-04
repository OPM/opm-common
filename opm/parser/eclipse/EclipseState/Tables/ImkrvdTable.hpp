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
#ifndef OPM_PARSER_IMKRVD_TABLE_HPP
#define	OPM_PARSER_IMKRVD_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class ImkrvdTable : protected SingleRecordTable {

        friend class TableManager;
        ImkrvdTable() = default;

        /*!
         * \brief Read the IMKRVD keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckRecordConstPtr record)
        {
            SingleRecordTable::init(record,
                             std::vector<std::string>{"DEPTH",
                                     "KRWMAX",
                                     "KRGMAX",
                                     "KROMAX",
                                     "KRWCRIT",
                                     "KRGCRIT",
                                     "KROCRITG",
                                     "KROCRITW" },
                             /*firstEntityOffset=*/0);

            SingleRecordTable::checkNonDefaultable("DEPTH");
            SingleRecordTable::checkMonotonic("DEPTH", /*isAscending=*/true);
            SingleRecordTable::applyDefaultsLinear("KRWMAX");
            SingleRecordTable::applyDefaultsLinear("KRGMAX");
            SingleRecordTable::applyDefaultsLinear("KROMAX");
            SingleRecordTable::applyDefaultsLinear("KRWCRIT");
            SingleRecordTable::applyDefaultsLinear("KRGCRIT");
            SingleRecordTable::applyDefaultsLinear("KROCRITG");
            SingleRecordTable::applyDefaultsLinear("KROCRITW");
        }

    public:
        using SingleRecordTable::numTables;
        using SingleRecordTable::numRows;
        using SingleRecordTable::numColumns;
        using SingleRecordTable::evaluate;

        /*!
         * \brief The datum depth for the remaining columns
         */
        const std::vector<double> &getDepthColumn() const
        { return SingleRecordTable::getColumn(0); }

        /*!
         * \brief Maximum relative permeability of water
         */
        const std::vector<double> &getKrwmaxColumn() const
        { return SingleRecordTable::getColumn(1); }

        /*!
         * \brief Maximum relative permeability of gas
         */
        const std::vector<double> &getKrgmaxColumn() const
        { return SingleRecordTable::getColumn(2); }

        /*!
         * \brief Maximum relative permeability of oil
         */
        const std::vector<double> &getKromaxColumn() const
        { return SingleRecordTable::getColumn(3); }

        /*!
         * \brief Relative permeability of water at the critical oil (or gas) saturation
         */
        const std::vector<double> &getKrwcritColumn() const
        { return SingleRecordTable::getColumn(4); }

        /*!
         * \brief Relative permeability of gas at the critical oil (or water) saturation
         */
        const std::vector<double> &getKrgcritColumn() const
        { return SingleRecordTable::getColumn(5); }

        /*!
         * \brief Oil relative permeability of oil at the critical gas saturation
         */
        const std::vector<double> &getKrocritgColumn() const
        { return SingleRecordTable::getColumn(6); }

        /*!
         * \brief Oil relative permeability of oil at the critical water saturation
         */
        const std::vector<double> &getKrocritwColumn() const
        { return SingleRecordTable::getColumn(7); }
    };
}

#endif

