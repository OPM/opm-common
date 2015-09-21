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

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class EnptvdTable : public SimpleTable {
    public:

        friend class TableManager;
        EnptvdTable() = default;

        /*!
         * \brief Read the ENPTVD keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckItemConstPtr item)
        {
            SimpleTable::init(item,
                             std::vector<std::string>{"DEPTH",
                                      "SWCO",
                                      "SWCRIT",
                                      "SWMAX",
                                      "SGCO",
                                      "SGCRIT",
                                      "SGMAX",
                                      "SOWCRIT",
                                      "SOGCRIT"});

            SimpleTable::checkNonDefaultable("DEPTH");
            SimpleTable::checkMonotonic("DEPTH", /*isAscending=*/true);
            SimpleTable::applyDefaultsLinear("SWCO");
            SimpleTable::applyDefaultsLinear("SWCRIT");
            SimpleTable::applyDefaultsLinear("SWMAX");
            SimpleTable::applyDefaultsLinear("SGCO");
            SimpleTable::applyDefaultsLinear("SGCRIT");
            SimpleTable::applyDefaultsLinear("SGMAX");
            SimpleTable::applyDefaultsLinear("SOWCRIT");
            SimpleTable::applyDefaultsLinear("SOGCRIT");
        }

        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        // using this method is strongly discouraged but the current endpoint scaling
        // code makes it hard to avoid
        using SimpleTable::getColumn;

        const std::vector<double> &getDepthColumn() const
        { return SimpleTable::getColumn(0); }

        /*!
         * \brief Connate water saturation
         */
        const std::vector<double> &getSwcoColumn() const
        { return SimpleTable::getColumn(1); }

        /*!
         * \brief Critical water saturation
         */
        const std::vector<double> &getSwcritColumn() const
        { return SimpleTable::getColumn(2); }

        /*!
         * \brief Maximum water saturation
         */
        const std::vector<double> &getSwmaxColumn() const
        { return SimpleTable::getColumn(3); }

        /*!
         * \brief Connate gas saturation
         */
        const std::vector<double> &getSgcoColumn() const
        { return SimpleTable::getColumn(4); }

        /*!
         * \brief Critical gas saturation
         */
        const std::vector<double> &getSgcritColumn() const
        { return SimpleTable::getColumn(5); }

        /*!
         * \brief Maximum gas saturation
         */
        const std::vector<double> &getSgmaxColumn() const
        { return SimpleTable::getColumn(6); }

        /*!
         * \brief Critical oil-in-water saturation
         */
        const std::vector<double> &getSowcritColumn() const
        { return SimpleTable::getColumn(7); }

        /*!
         * \brief Critical oil-in-gas saturation
         */
        const std::vector<double> &getSogcritColumn() const
        { return SimpleTable::getColumn(8); }
    };
}

#endif

