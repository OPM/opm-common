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
#ifndef OPM_PARSER_WGRUPCON_WRAPPER_HPP
#define	OPM_PARSER_WGRUPCON_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class WgrupconWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'WGRUPCON' keyword.
         */
        WgrupconWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of well groups to which this keyword applies
         */
        int numGroups() const
        { return m_keyword->size(); }

        /*!
         * \brief Return the human-readable name of a well group with
         *        a given index
         */
        std::string groupName(int groupIdx) const
        { return m_keyword->getRecord(groupIdx)->getItem(0)->getString(0); }

        /*!
         * \brief Return whether the group is available for higher
         *        level groups to control.
         */
        bool isUnconstraint(int groupIdx) const
        { return m_keyword->getRecord(groupIdx)->getItem(1)->getString(0) == "YES"; }

        /*!
         * \brief Return the guide rate for the group
         */
        double guideRate(int groupIdx) const
        { return m_keyword->getRecord(groupIdx)->getItem(2)->getSIDouble(0); }

        /*!
         * \brief Return the type of a well group.
         *
         * This is one of:
         * - OIL
         * - WAT
         * - GAS
         * - LIQ
         * - COMB
         * - WGA
         * - CVAL
         * - RAT
         * - RES
         */
        std::string controlPhase(int groupIdx) const
        { return m_keyword->getRecord(groupIdx)->getItem(3)->getString(0); }

        /*!
         * \brief Return the scaling factor for the guide rate of a group
         */
        double guideRateScalingFactor(int groupIdx) const
        { return m_keyword->getRecord(groupIdx)->getItem(4)->getSIDouble(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_WGRUPCON_WRAPPER_HPP

