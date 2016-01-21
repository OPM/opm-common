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
#ifndef OPM_PARSER_GRUPTREE_WRAPPER_HPP
#define	OPM_PARSER_GRUPTREE_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {
    class GruptreeWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'GRUPTREE' keyword.
         */
        GruptreeWrapper(std::shared_ptr< const DeckKeyword > keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return the number of nodes in the well group tree
         */
        int numNodes() const
        { return m_keyword->size(); }

        /*!
         * \brief Return the human-readable name of the child group of
         *        a well group with a given index
         */
        std::string childName(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(0)->getString(0); }

        /*!
         * \brief Return the human-readable name of the parent group
         *        of a well group with a given index
         */
        std::string parentName(int wellIdx) const
        { return m_keyword->getRecord(wellIdx)->getItem(1)->getString(0); }

    private:
        std::shared_ptr< const DeckKeyword > m_keyword;
    };
}

#endif	// OPM_PARSER_GRUPTREE_WRAPPER_HPP

