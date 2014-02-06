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
#ifndef OPM_PARSER_SCALECRS_WRAPPER_HPP
#define	OPM_PARSER_SCALECRS_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class ScalecrsWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data of the 'SCALECRS' keyword.
         */
        ScalecrsWrapper(Opm::DeckKeywordConstPtr keyword)
            : m_keyword(keyword)
        {
        }

        /*!
         * \brief Return whether the CRS method should be used to scale the endpoints
         */
        bool isEnabled() const
        {
            return m_keyword->getRecord(0)->getItem(0)->getString(0) == "YES"
                || m_keyword->getRecord(0)->getItem(0)->getString(0) == "Y";
        }

    private:
        Opm::DeckKeywordConstPtr m_keyword;
    };
}

#endif	// OPM_PARSER_SCALECRS_WRAPPER_HPP

