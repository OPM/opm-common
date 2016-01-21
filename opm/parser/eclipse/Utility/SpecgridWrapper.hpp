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
#ifndef OPM_PARSER_SPECGRID_WRAPPER_HPP
#define	OPM_PARSER_SPECGRID_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <vector>

namespace Opm {
    class SpecgridWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'SPECGRID' keyword.
         */
        SpecgridWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of grid blocks in I direction
         */
        int numGridBlocksI() const
        { return m_keyword->getRecord(0)->getItem(0)->getInt(0); }

        /*!
         * \brief Return the number of grid blocks in J direction
         */
        int numGridBlocksJ() const
        { return m_keyword->getRecord(0)->getItem(1)->getInt(0); }

        /*!
         * \brief Return the number of grid blocks in K direction
         */
        int numGridBlocksK() const
        { return m_keyword->getRecord(0)->getItem(2)->getInt(0); }

        /*!
         * \brief The number of IJK grid blocks as a vector
         */
        std::vector<int> numBlocksVector() const
        {
            std::vector<int> dim(3);
            dim[0] = numGridBlocksI();
            dim[1] = numGridBlocksJ();
            dim[2] = numGridBlocksK();
            return dim;
        }

        /*!
         * \brief Return the number of reservoirs
         */
        int numReservoirs() const
        { return m_keyword->getRecord(0)->getItem(3)->getInt(0); }

        /*!
         * \brief Type of the coordinate system
         *
         * This is one of:
         * - "T": cylindrical coordinates
         * - "F": cartesianCoordinates
         */
        std::string coordSystemType() const
        { return m_keyword->getRecord(0)->getItem(4)->getString(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_SPECGRID_WRAPPER_HPP

