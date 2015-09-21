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

#include "SimpleTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class ImkrvdTable : public SimpleTable {
    public:
        friend class TableManager;
        ImkrvdTable() = default;

        /*!
         * \brief Read the IMKRVD keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckItemConstPtr item)
        {
            SimpleTable::init(item,
                             std::vector<std::string>{"DEPTH",
                                     "KRWMAX",
                                     "KRGMAX",
                                     "KROMAX",
                                     "KRWCRIT",
                                     "KRGCRIT",
                                     "KROCRITG",
                                      "KROCRITW" });


            SimpleTable::checkNonDefaultable("DEPTH");
            SimpleTable::checkMonotonic("DEPTH", /*isAscending=*/true);
            SimpleTable::applyDefaultsLinear("KRWMAX");
            SimpleTable::applyDefaultsLinear("KRGMAX");
            SimpleTable::applyDefaultsLinear("KROMAX");
            SimpleTable::applyDefaultsLinear("KRWCRIT");
            SimpleTable::applyDefaultsLinear("KRGCRIT");
            SimpleTable::applyDefaultsLinear("KROCRITG");
            SimpleTable::applyDefaultsLinear("KROCRITW");
        }

        using SimpleTable::numTables;
        using SimpleTable::numRows;
        using SimpleTable::numColumns;
        using SimpleTable::evaluate;

        /*!
         * \brief The datum depth for the remaining columns
         */
        const std::vector<double> &getDepthColumn() const
        { return SimpleTable::getColumn(0); }

        /*!
         * \brief Maximum relative permeability of water
         */
        const std::vector<double> &getKrwmaxColumn() const
        { return SimpleTable::getColumn(1); }

        /*!
         * \brief Maximum relative permeability of gas
         */
        const std::vector<double> &getKrgmaxColumn() const
        { return SimpleTable::getColumn(2); }

        /*!
         * \brief Maximum relative permeability of oil
         */
        const std::vector<double> &getKromaxColumn() const
        { return SimpleTable::getColumn(3); }

        /*!
         * \brief Relative permeability of water at the critical oil (or gas) saturation
         */
        const std::vector<double> &getKrwcritColumn() const
        { return SimpleTable::getColumn(4); }

        /*!
         * \brief Relative permeability of gas at the critical oil (or water) saturation
         */
        const std::vector<double> &getKrgcritColumn() const
        { return SimpleTable::getColumn(5); }

        /*!
         * \brief Oil relative permeability of oil at the critical gas saturation
         */
        const std::vector<double> &getKrocritgColumn() const
        { return SimpleTable::getColumn(6); }

        /*!
         * \brief Oil relative permeability of oil at the critical water saturation
         */
        const std::vector<double> &getKrocritwColumn() const
        { return SimpleTable::getColumn(7); }
    };
}

#endif

