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
#ifndef OPM_PARSER_ENKRVD_TABLE_HPP
#define	OPM_PARSER_ENKRVD_TABLE_HPP

#include "SingleRecordTable.hpp"

namespace Opm {
    // forward declaration
    class TableManager;

    class EnkrvdTable : protected SingleRecordTable {
        typedef SingleRecordTable ParentType;

        friend class TableManager;
        EnkrvdTable() = default;

        /*!
         * \brief Read the ENKRVD keyword and provide some convenience
         *        methods for it.
         */
        void init(Opm::DeckKeywordConstPtr keyword,
                  int recordIdx)
        {
            ParentType::init(keyword,
                             std::vector<std::string>{"DEPTH",
                                     "KRWMAX",
                                     "KRGMAX",
                                     "KROMAX",
                                     "KRWCRIT",
                                     "KRGCRIT",
                                     "KROCRITG",
                                     "KROCRITW" },
                             recordIdx,
                             /*firstEntityOffset=*/0);

            ParentType::checkNonDefaultable("DEPTH");
            ParentType::checkMonotonic("DEPTH", /*isAscending=*/true);
            ParentType::applyDefaultsLinear("KRWMAX");
            ParentType::applyDefaultsLinear("KRGMAX");
            ParentType::applyDefaultsLinear("KROMAX");
            ParentType::applyDefaultsLinear("KRWCRIT");
            ParentType::applyDefaultsLinear("KRGCRIT");
            ParentType::applyDefaultsLinear("KROCRITG");
            ParentType::applyDefaultsLinear("KROCRITW");
        }

    public:
        using ParentType::numTables;
        using ParentType::numRows;
        using ParentType::numColumns;
        using ParentType::evaluate;

        // using this method is strongly discouraged but the current endpoint scaling
        // code makes it hard to avoid
        using ParentType::getColumn;

        /*!
         * \brief The datum depth for the remaining columns
         */
        const std::vector<double> &getDepthColumn() const
        { return ParentType::getColumn(0); }

        /*!
         * \brief Maximum relative permeability of water
         */
        const std::vector<double> &getKrwmaxColumn() const
        { return ParentType::getColumn(1); }

        /*!
         * \brief Maximum relative permeability of gas
         */
        const std::vector<double> &getKrgmaxColumn() const
        { return ParentType::getColumn(2); }

        /*!
         * \brief Maximum relative permeability of oil
         */
        const std::vector<double> &getKromaxColumn() const
        { return ParentType::getColumn(3); }

        /*!
         * \brief Relative permeability of water at the critical oil (or gas) saturation
         */
        const std::vector<double> &getKrwcritColumn() const
        { return ParentType::getColumn(4); }

        /*!
         * \brief Relative permeability of gas at the critical oil (or water) saturation
         */
        const std::vector<double> &getKrgcritColumn() const
        { return ParentType::getColumn(5); }

        /*!
         * \brief Oil relative permeability of oil at the critical gas saturation
         */
        const std::vector<double> &getKrocritgColumn() const
        { return ParentType::getColumn(6); }

        /*!
         * \brief Oil relative permeability of oil at the critical water saturation
         */
        const std::vector<double> &getKrocritwColumn() const
        { return ParentType::getColumn(7); }
    };
}

#endif

