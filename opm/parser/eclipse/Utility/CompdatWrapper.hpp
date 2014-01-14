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
#ifndef OPM_PARSER_COMPDAT_WRAPPER_HPP
#define	OPM_PARSER_COMPDAT_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class CompdatWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'COMPDAT' keyword.
         */
        CompdatWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of injection wells
         */
        int numWells() const
        { return m_keyword->size(); }

        /*!
         * \brief Return the human-readable name of the well with a given index
         */
        std::string wellName(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(0)->getString(0); }

        /*!
         * \brief Return the I-coordinate of the well
         */
        int coordinateI(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(1)->getInt(0); }

        /*!
         * \brief Return the J-coordinate of the well
         */
        int coordinateJ(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(2)->getInt(0); }

        /*!
         * \brief Return the upper K-coordinate of the well
         */
        int coordinateKUpper(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(3)->getInt(0); }

        /*!
         * \brief Return the lower K-coordinate of the well
         */
        int coordinateKLower(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(4)->getInt(0); }

        /*!
         * \brief Return whether a well is open or closed
         *
         * This is one of:
         * - OPEN: Well injects
         * - SHUT: Well does not influence the reservoir
         * - AUTO: Simulation selects one of the above depending in the
         *         well parameters and reservoir conditions at the well.
         */
        std::string wellStatus(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(5)->getString(0); }

        /*!
         * \brief Return the index of the saturation table to
         *        calculate the relative permebilities of the connection
         */
        int saturationTableIndex(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(6)->getInt(0); }

        /*!
         * \brief Return the transmiscibility factor to be used for the connection
         */
        double transmiscibilityFactor(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(7)->getSIDouble(0); }

        /*!
         * \brief Return the diameter of the well
         */
        double wellDiameter(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(8)->getSIDouble(0); }

        /*!
         * \brief Return the effective intrinisic permeability to be used for the well
         */
        double intrinsicPermeability(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(9)->getSIDouble(0); }

        /*!
         * \brief Return the skin factor to be used for the well
         */
        double skinFactor(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(10)->getSIDouble(0); }

        /*!
         * \brief Return the "D-factor" (for non-Darcy flow regimes) to be used for the well
         */
        double dFactor(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(11)->getSIDouble(0); }

        /*!
         * \brief Return the direction into which the cells are penetrated by the well
         *
         * This is one of:
         * - X
         * - Y
         * - Z
         * - FX (fracture in X direction)
         * - FY (fracture in Y direction)
         */
        std::string penetrationDirection(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(12)->getString(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_COMPDAT_WRAPPER_HPP

