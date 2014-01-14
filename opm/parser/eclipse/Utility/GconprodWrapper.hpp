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
#ifndef OPM_PARSER_GCONPROD_WRAPPER_HPP
#define	OPM_PARSER_GCONPROD_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class GconprodWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'GCONPROD' keyword.
         */
        GconprodWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of prodction well groups
         */
        int numGroups() const
        { return m_keyword->size(); }

        /*!
         * \brief Return the human-readable name of the well group with a given index
         */
        std::string groupName(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(0)->getString(0); }

        /*!
         * \brief Return the what should be controlled for a given well
         *
         * This is one of:
         * - NONE
         * - ORAT
         * - WRAT
         * - GRAT
         * - LRAT
         * - CRAT
         * - RESV
         * - PRBL
         * - WGRA
         * - CVAL
         * - PBGS
         * - PBWS
         * - FLD
         */
        std::string controlMode(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(1)->getString(0); }

        /*!
         * \brief Return the target for the volumetric surface oil rate of a well group
         *
         * If the control mode does not use the volumetric oil surface
         * rate, this is the upper limit.
         */
        double surfaceOilTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(2)->getSIDouble(0); }

        /*!
         * \brief Return the target for the volumetric surface water rate of a well group
         *
         * If the control mode does not use the volumetric water surface
         * rate, this is the upper limit.
         */
        double surfaceWaterTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(3)->getSIDouble(0); }

        /*!
         * \brief Return the target for the volumetric surface gas rate of a well group
         *
         * If the control mode does not use the volumetric gas surface
         * rate, this is the upper limit.
         */
        double surfaceGasTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(4)->getSIDouble(0); }

        /*!
         * \brief Return the target for the volumetric surface liquid rate of a well group
         *
         * If the control mode does not use the volumetric liquid surface
         * rate, this is the upper limit.
         */
        double surfaceLiquidTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(5)->getSIDouble(0); }

        /*!
         * \brief Return the procedure which is taken if the target rates are exceeded
         *
         * This is one of:
         * - NONE
         * - CON
         * - +CON
         * - WELL
         * - PLUG
         * - RATE
         */
        std::string oilExceedanceReaction(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(6)->getString(0); }

        /*!
         * \brief Returns whether a group is unconstraint so that it
         *        be used to hit the target of a higher-level group
         */
        bool isUnconstraint(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(7)->getString(0) == "YES"; }

        /*!
         * \brief The target for the group's share of the next
         *        higher-level group's total production rate.
         */
        double productionShareTarget(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(8)->getSIDouble(0); }

        /*!
         * \brief The kind of control which the next higher-level group wants
         *
         * This is one of:
         * - OIL
         * - WAT
         * - GAS
         * - LIQ
         * - COMB
         * - RES
         * - WGA
         * - CVAL
         * - INJV
         * - POTN
         * - FORM
         * - '    '
         */
        std::string productionShareType(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(9)->getString(0); }

        /*!
         * \brief Return the procedure which is taken if the water target rate is exceeded
         */
        std::string waterExceedanceReaction(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(10)->getString(0); }

        /*!
         * \brief Return the procedure which is taken if the gas target rate is exceeded
         */
        std::string gasExceedanceReaction(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(11)->getString(0); }

        /*!
         * \brief Return the procedure which is taken if the liquid target rate is exceeded
         */
        std::string liquidExceedanceReaction(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(12)->getString(0); }

        /*!
         * \brief Return target rate of all fluids at reservoir conditions
         */
        double reservoirTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(13)->getSIDouble(0); }

        /*!
         * \brief Return balancing fraction of fluids produced at reservoir conditions.
         */
        double reservoirBalanceTarget(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(14)->getSIDouble(0); }

        /*!
         * \brief Return target rate for wet-gas
         */
        double wetGasTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(15)->getSIDouble(0); }

        /*!
         * \brief Return caloric target rate
         */
        double caloricTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(16)->getSIDouble(0); }

        /*!
         * \brief Return balancing fraction of gas at surface conditions.
         */
        double surfaceGasBalanceTarget(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(17)->getSIDouble(0); }

        /*!
         * \brief Return balancing fraction of water at surface conditions.
         */
        double surfaceWaterBalanceTarget(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(18)->getSIDouble(0); }

        /*!
         * \brief Return index of the linearly combined rate specified using the LCOM keyword
         */
        int lcomIndex(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(19)->getInt(0); }

        /*!
         * \brief Return the procedure which is taken if the linearly combined rate is exceeded
         */
        std::string lcomExceedanceReaction(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(20)->getString(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_GCONINJE_WRAPPER_HPP

