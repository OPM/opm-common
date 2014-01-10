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
#ifndef OPM_PARSER_WELSPECS_WRAPPER_HPP
#define	OPM_PARSER_WELSPECS_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class WelspecsWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of an individual well as exposed by the
         *        'WELSPECS' keyword.
         */
        WelspecsWrapper(Opm::DeckRecordConstPtr record)
            : m_record(record)
        {
        }

        /*!
         * \brief Return the name of the well
         */
        const std::string wellName() const
        { return m_record->getItem(0)->getString(0); }

        /*!
         * \brief Return the name of the group this well belongs to
         */
        const std::string groupName() const
        { return m_record->getItem(1)->getString(0); }

        /*!
         * \brief Return east-west grid coordinate of the well
         */
        int coordinateI() const
        { return m_record->getItem(2)->getInt(0); }

        /*!
         * \brief Return north-south grid coordinate of the well
         */
        int coordinateJ() const
        { return m_record->getItem(3)->getInt(0); }

        /*!
         * \brief Return reference depth to which the bottom hole pressure of the well applies
         */
        double referenceDepth() const
        { return m_record->getItem(4)->getSIDouble(0); }

        /*!
         * \brief Return the preferred fluid phase of this well
         *
         * (whatever this means.) This method returns a string that
         * contains one of "OIL", "WATER", "GAS", or "LIQ".
         */
        const std::string preferredPhase() const
        { return m_record->getItem(5)->getString(0); }

        /*!
         * \brief Return effective drainage radius of the well
         */
        double drainageRadius() const
        { return m_record->getItem(6)->getSIDouble(0); }

        /*!
         * \brief Return the inflow equation to be used for the well
         *
         * This is one of "STD", "NO", "R-G", "YES", "P-P" or "GPP".
         */
        const std::string inflowEquation() const
        { return m_record->getItem(7)->getString(0); }

        /*!
         * \brief Returns true if the well is closed for fluids
         */
        bool isShut() const
        { return m_record->getItem(8)->getString(0) == "SHUT"; }

        /*!
         * \brief Returns true if crossflow should be allowed
         */
        bool allowCrossflow() const
        { return m_record->getItem(9)->getString(0) == "YES"; }

        /*!
         * \brief Returns the pressure number to be used for the wellbore fluids
         */
        int pressureTableNumber() const
        { return m_record->getItem(10)->getInt(0); }

        /*!
         * \brief Indicates the type of the calculation to be used for hydrostatic pressure
         *
         * This is one of:
         *  - "SEG": segmented density calculation
         *  - "AVG": averaged density calculation
         */
        const std::string hydrostaticCalculation() const
        { return m_record->getItem(11)->getString(0); }

        /*!
         * \brief Indicates the "fluid in place" region table number
         *        used to calculate volumetric rates at reservoir
         *        conditions.
         */
        int inPlaceRegionNumber() const
        { return m_record->getItem(12)->getInt(0); }

        // items 14 and 15 are "reserved for FrontSim"

        /*!
         * \brief Indicates well model to be used
         *
         * This is one of:
         *  - "STD": The standard well model (Peaceman??)
         *  - "HMIW": High Mobility Injection Wells
         */
        const std::string wellModel() const
        { return m_record->getItem(15)->getString(0); }

    private:
        Opm::DeckRecordConstPtr m_record;
    };
}

#endif	// OPM_PARSER_WELSPECS_WRAPPER_HPP

