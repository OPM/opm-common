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
#ifndef OPM_PARSER_GCONINJE_WRAPPER_HPP
#define	OPM_PARSER_GCONINJE_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class GconinjeWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'GCONINJE' keyword.
         */
        GconinjeWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of injection well groups
         */
        int numGroups() const
        { return m_keyword->size(); }

        /*!
         * \brief Return the human-readable name of the well group with a given index
         */
        std::string groupName(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(0)->getString(0); }

        /*!
         * \brief Return the injector type of a well group.
         *
         * This is one of:
         * - OIL
         * - WATER
         * - GAS
         */
        std::string groupType(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(1)->getString(0); }

        /*!
         * \brief Return the what should be controlled for a given well
         *
         * This is one of:
         * - NONE: The individual wells specify how they are controlled
         * - RATE: Control for the surface volume rate of the fluid
         * - RESV: Control for the reservoir volume rate of the fluid
         * - REIN: Injection rate is production rate times a re-injection ratio
         * - VREP: Control the total surface injection rate of all wells
         * - WGRA: Control for the wet-gas injection rate
         * - FLD: Higher level groups specify this group's control
         */
        std::string controlMode(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(2)->getString(0); }

        /*!
         * \brief Return the target for the volumetric surface rate of a well group
         *
         * If the control mode does not use the volumetric surface
         * rate, this is the upper limit.
         */
        double surfaceTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(3)->getSIDouble(0); }

        /*!
         * \brief Return the target for the volumetric reservoir rate of a well group
         *
         * If the control mode does not use the volumetric reservoir
         * rate, this is the upper limit.
         */
        double reservoirTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(4)->getSIDouble(0); }

        /*!
         * \brief The target fraction for reinjection
         */
        double reinjectTargetRatio(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(5)->getSIDouble(0); }

        /*!
         * \brief The target fraction of the voidage replacement fraction
         */
        double voidageReplacementFractionTarget(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(6)->getSIDouble(0); }

        /*!
         * \brief Returns whether a group is unconstraint so that it
         *        be used to hit the target of a higher-level group
         */
        bool isUnconstraint(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(7)->getString(0) == "YES"; }

        /*!
         * \brief The target for the group's share of the next
         *        higher-level group's total injection rate.
         */
        double injectionShareTarget(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(8)->getSIDouble(0); }

        /*!
         * \brief The kind of control which the next higher-level group wants
         *
         * This is one of:
         * - RATE
         * - RESV
         * - VOID
         * - NETV
         * - '    '
         */
        std::string injectionShareType(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(9)->getString(0); }

        /*!
         * \brief The name of the production group which should be partially reinjected
         */
        std::string reinjectGroupName(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(10)->getString(0); }

        /*!
         * \brief The name of the group to which the voidage
         *        replacement volume fraction applies and which's
         *        production rates should be used.
         */
        std::string voidageGroupName(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(11)->getString(0); }

        /*!
         * \brief The target rate for wet gas injection
         */
        double wetGasTargetRate(int wellGroupIdx) const
        { return m_keyword->getRecord(wellGroupIdx)->getItem(12)->getSIDouble(0); }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_GCONINJE_WRAPPER_HPP

