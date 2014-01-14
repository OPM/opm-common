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
#ifndef OPM_PARSER_WCONPROD_WRAPPER_HPP
#define	OPM_PARSER_WCONPROD_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class WconprodWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'WCONPROD' keyword.
         */
        WconprodWrapper(Opm::DeckKeywordConstPtr keyword)
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
         * \brief Return the connection type of a well.
         *
         * This is one of:
         * - OPEN
         * - STOP
         * - SHUT
         * - AUTO
         */
        std::string wellStatus(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(1)->getString(0); }

        /*!
         * \brief Return the control mode of a well.
         *
         * This is one of:
         * - ORAT (surface oil rate)
         * - WRAT (surface water rate)
         * - GRAT (surface gas rate)
         * - LRAT (surface liquid rate)
         * - CRAT (linear combination of the surface rates)
         * - RESV (reservoir fluid volume rate)
         * - BHP (bottom-hole pressure)
         * - THP (top-hole pressure)
         * - WGRA (wet-gas rate)
         * - TMRA (total molar rate)
         * - STRA (steam volume rate)
         * - SATP (water saturation pressure)
         * - SATT (water temperature)
         * - CVAL (caloric rate)
         * - NGL (NGL rate, WTF?)
         * - GRUP (group of the well controls the regime)
         * - ' ' (undefined, default)
         */
        std::string controlMode(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(2)->getString(0); }

        /*!
         * \brief The upper limit of the surface oil rate.
         */
        double oilTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(3)->getSIDouble(0); }

        /*!
         * \brief The upper limit of the surface water rate.
         */
        double waterTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(4)->getSIDouble(0); }

        /*!
         * \brief The upper limit of the surface gas rate.
         */
        double gasTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(5)->getSIDouble(0); }

        /*!
         * \brief The upper rate limit of the liquids at the surface.
         */
        double liquidTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(6)->getSIDouble(0); }

        /*!
         * \brief The upper limit of the fluid volumes at the reservoir.
         */
        double reservoirTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(7)->getSIDouble(0); }

        /*!
         * \brief The lower limit of the bottom hole pressure.
         */
        double bottomHoleTargetPressure(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(8)->getSIDouble(0); }

        /*!
         * \brief The lower limit of the top hole pressure.
         */
        double topHoleTargetPressure(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(9)->getSIDouble(0); }

        /*!
         * \brief The index of the well's VFP table
         */
        int vfpTableIndex(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(10)->getInt(0); }

        /*!
         * \brief The artificial lift quantity of the well.
         */
        double artificialLiftQuantity(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(11)->getSIDouble(0); }

        /*!
         * \brief The upper limit of the produced wet gas
         */
        double wetGasTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(12)->getSIDouble(0); }

        /*!
         * \brief The upper limit of the produced molar rate
         */
        double molarTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(13)->getSIDouble(0); }

        /*!
         * \brief The upper limit for the rate of the produced steam
         */
        double steamTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(14)->getSIDouble(0); }

        /*!
         * \brief The pressure offset used for saturation pressure control
         */
        double pressureOffset(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(15)->getSIDouble(0); }

        /*!
         * \brief The temperature offset used for saturation temperature control
         */
        double temperatureOffset(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(16)->getSIDouble(0); }

        /*!
         * \brief The upper limit of the caloric rate
         */
        double caloricTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(17)->getSIDouble(0); }

        /*!
         * \brief The index of the linearly combined rate specified by the LINCOM keyword
         */
        int lincomIndex(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(18)->getInt(0); }

        /*!
         * \brief The upper limit of the NGL rate
         */
        double nglTargetRate(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(19)->getSIDouble(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_WCONPROD_WRAPPER_HPP

