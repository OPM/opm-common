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
#ifndef OPM_PARSER_TSTEP_WRAPPER_HPP
#define	OPM_PARSER_TSTEP_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <vector>
#include <algorithm>

namespace Opm {
    class TstepWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data exposed by the 'TSTEP' keyword.
         */
        TstepWrapper(Opm::DeckKeywordConstPtr keyword)
        {
            m_timesteps = keyword->getSIDoubleData();
            m_totalTime = std::accumulate(m_timesteps.begin(), m_timesteps.end(), 0.0);
        }

        /*!
         * \brief Return the simulated timestep sizes in seconds.
         */
        const std::vector<double> &timestepVector() const
        { return m_timesteps; }

        /*!
         * \brief Return the total simulation time in seconds.
         */
        double totalTime() const
        { return m_totalTime; }

    private:
        std::vector<double> m_timesteps;
        double m_totalTime;
    };
}

#endif	// OPM_PARSER_TSTEP_KEYWORD_HPP

