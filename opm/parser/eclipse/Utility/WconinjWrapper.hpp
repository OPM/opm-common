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
#ifndef OPM_PARSER_WCONINJ_WRAPPER_HPP
#define	OPM_PARSER_WCONINJ_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class WconinjWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'WCONINJ' keyword.
         */
        WconinjWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of water injection wells
         */
        int numWells() const
        { return m_keyword->size() }

        /*!
         * \brief Return the human-readable name of the well with a given index
         */
        std::string wellName(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(0)->getString(0); }

        /*!
         * \brief Return the injector type of a well.
         *
         * This is one of:
         * - OIL
         * - WATER
         * - GAS
         */
        std::string wellType(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(1)->getString(0); }

        /*!
         * \brief Return whether a well is open or closed
         *
         * This is one of:
         * - OPEN: Well injects
         * - STOP: Well does not reach the reservoir, but it
         *         injects. (and some of this fluid reaches the reservoir
         *         via crossflow)
         * - SHUT: Well does not influence the reservoir
         * - AUTO: Simulation selects one of the above depending in the
         *         well parameters and reservoir conditions at the well.
         */
        std::string wellType(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(2)->getString(0); }

        /*!
         * \brief Return the what should be controlled for a given well
         *
         * This is one of:
         * - RATE: Control for the surface volume rate of the fluid
         * - RESV: Control for the reservoir volume rate of the fluid
         * - BHP: Control for the bottom hole pressure
         * - THP: Control for the top hole pressure
         */
        std::string controlMode(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(3)->getString(0); }

        /*!
         * \brief Return the target for the volumetric surface rate of a well
         *
         * If the control mode does not use the volumetric surface
         * rate, this is the upper limit.
         */
        double surfaceTargetRate(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(4)->getSIDouble(0); }

        /*!
         * \brief Return the target for the volumetric reservoir rate of a well
         *
         * If the control mode does not use the volumetric reservoir
         * rate, this is the upper limit.
         */
        double reservoirTargetRate(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(5)->getSIDouble(0); }

        /*!
         * \brief Return the reinjection replacement percentage of well
         */
        double reinjectionReplacementRatio(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(6)->getSIDouble(0); }

        /*!
         * \brief Return how reinjection should be handled
         *
         * This is one of:
         * - NONE
         * - GPRD
         * - FPRD
         */
        std::string reinjectionReplacementType(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(7)->getString(0); }

        /*!
         * \brief The target of the bottom hole pressure
         *
         * If the control mode does not use the bottom hole pressure,
         * this specifies the upper limit.
         */
        double bottomHolePressureTarget(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(9)->getSIDouble(0); }

        /*!
         * \brief The target of the top hole pressure
         *
         * If the control mode does not use the bottom hole pressure,
         * this specifies the upper limit.
         */
        double topHolePressureTarget(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(10)->getSIDouble(0); }

        /*!
         * \brief The index of the PVT table used for the injected fluid
         */
        int vfpTableIndex(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(11)->getInt(0); }

        /*!
         * \brief The vaporized oil concentration in the injected gas (if the well injects gas)
         */
        double vaporizedOilConcentration(ind wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(12)->getSIDouble(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_WCONINJ_WRAPPER_HPP

