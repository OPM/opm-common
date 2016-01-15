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
#ifndef OPM_PARSER_ENDSCALE_WRAPPER_HPP
#define	OPM_PARSER_ENDSCALE_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class EndscaleWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'ENDSCALE' keyword.
         */
        EndscaleWrapper(std::shared_ptr< const DeckKeyword > keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the directional switch for endpoint scaling
         *
         * This is one of:
         * - DIRECT
         * - NODIR
         */
        std::string directionSwitch() const
        { return m_keyword->getRecord(0)->getItem(0)->getString(0); }

        /*!
         * \brief Return whether the enpoint scaling should be reversible or not
         */
        bool isReversible() const
        { return m_keyword->getRecord(0)->getItem(1)->getString(0) == "REVERS"; }

        /*!
         * \brief Return the number of endpoint (depending on depth) tables
         */
        int numEndscaleTables() const
        { return m_keyword->getRecord(0)->getItem(2)->getInt(0); }

        /*!
         * \brief Return the maximum number of nodes in endpoint tables
         */
        int numMaxNodes() const
        { return m_keyword->getRecord(0)->getItem(3)->getInt(0); }

        /*!
         * \brief Return the options for combining temperature endpoint data.
         */
        int combiningOptions() const
        { return m_keyword->getRecord(0)->getItem(4)->getInt(0); }

    private:
        std::shared_ptr< const DeckKeyword > m_keyword;
    };
}

#endif	// OPM_PARSER_ENDSCALE_WRAPPER_HPP

