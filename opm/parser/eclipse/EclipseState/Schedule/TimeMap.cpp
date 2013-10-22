/*
  Copyright 2013 Statoil ASA.

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

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>


namespace Opm {

    TimeMap::TimeMap(boost::gregorian::date startDate) {
        if (startDate.is_not_a_date())
          throw std::invalid_argument("Input argument not properly initialized.");

        m_startDate = startDate;
        m_timeList.push_back( boost::posix_time::ptime(startDate) );
    }


    void TimeMap::addDate(boost::gregorian::date newDate) {
        boost::posix_time::ptime lastTime = m_timeList.back();
        if (boost::posix_time::ptime(newDate) > lastTime)
            m_timeList.push_back( boost::posix_time::ptime(newDate) );
        else
            throw std::invalid_argument("Dates added must be in strictly increasing order.");
    }


    void TimeMap::addTStep(boost::posix_time::time_duration step) {
        if (step.total_seconds() > 0) {
          boost::posix_time::ptime newTime = m_timeList.back() + step;
          m_timeList.push_back( newTime );
        } else
          throw std::invalid_argument("Can only add positive steps");
    }


    size_t TimeMap::size() const {
        return m_timeList.size();
    }

}


