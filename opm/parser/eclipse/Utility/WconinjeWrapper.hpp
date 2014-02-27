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
#ifndef OPM_PARSER_WCONINJE_WRAPPER_HPP
#define	OPM_PARSER_WCONINJE_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class WconinjeWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'WCONINJE' keyword.
         */
        WconinjeWrapper(Opm::DeckKeywordConstPtr keyword)
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
         * \brief Return the injector type of a well.
         *
         * This is one of:
         * - OIL
         * - WATER
         * - STEAM-GAS
         * - GAS
         * - MULTI
         */
        std::string wellType(int wellIdx) const
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
        std::string wellStatus(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(2)->getString(0); }

        /*!
         * \brief Return the what should be controlled for a given well
         *
         * This is one of:
         * - RATE: Control for the surface volume rate of the fluid
         * - RESV: Control for the reservoir volume rate of the fluid
         * - BHP: Control for the bottom hole pressure
         * - THP: Control for the top hole pressure
         * - GRUP: Use the control mode which applies for the group of the well
         */
        std::string controlMode(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(3)->getString(0); }

        /*!
         * \brief Return the target for the volumetric surface rate of a well
         *
         * If the control mode does not use the volumetric surface
         * rate, this is the upper limit.
         */
        double rawSurfaceTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(4)->getRawDouble(0); }

        /*!
         * \brief Return the target for the volumetric reservoir rate of a well
         *
         * If the control mode does not use the volumetric reservoir
         * rate, this is the upper limit.
         */
        double rawReservoirTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(5)->getRawDouble(0); }

        /*!
         * \brief The target of the bottom hole pressure
         *
         * If the control mode does not use the bottom hole pressure,
         * this specifies the upper limit.
         */
        double bottomHoleTargetPressure(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(6)->getSIDouble(0); }

        /*!
         * \brief The target of the top hole pressure
         *
         * If the control mode does not use the bottom hole pressure,
         * this specifies the upper limit.
         */
        double topHoleTargetPressure(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(7)->getSIDouble(0); }

        /*!
         * \brief The index of the PVT table used for the injected fluid
         */
        int vfpTableIndex(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(8)->getInt(0); }

        /*!
         * \brief The vaporized oil concentration in the injected gas (if the well injects gas)
         */
        double vaporizedOilConcentration(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(9)->getSIDouble(0); }

        /*!
         * \brief The gas to steam ratio (at reservoir conditions?) for GAS-STEAM injectors.
         */
        double gasSteamRatio(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(10)->getSIDouble(0); }

        /*!
         * \brief The proportion of oil at the surface for multi-phase injector wells.
         */
        double surfaceOilRatio(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(11)->getSIDouble(0); }

        /*!
         * \brief The proportion water oil at the surface for multi-phase injector wells.
         */
        double surfaceWaterRatio(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(12)->getSIDouble(0); }

        /*!
         * \brief The proportion water oil at the surface for multi-phase injector wells.
         */
        double surfaceGasRatio(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(13)->getSIDouble(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_WCONINJE_WRAPPER_HPP

