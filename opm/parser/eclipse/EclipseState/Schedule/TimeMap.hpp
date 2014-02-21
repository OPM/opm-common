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


#ifndef TIMEMAP_HPP_
#define TIMEMAP_HPP_

#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {

    class TimeMap {
    public:
        TimeMap(boost::posix_time::ptime startDate);
        void addTime(boost::posix_time::ptime newTime);
        void addTStep(boost::posix_time::time_duration step);
        void addFromDATESKeyword( DeckKeywordConstPtr DATESKeyword );
        void addFromTSTEPKeyword( DeckKeywordConstPtr TSTEPKeyword );
        size_t size() const;
        /// Return the date and time where a given time step starts.
        boost::posix_time::ptime getStartTime(int tStepIdx) const
        { return m_timeList[tStepIdx]; }
        static boost::posix_time::ptime timeFromEclipse(DeckRecordConstPtr dateRecord);
        static boost::posix_time::ptime timeFromEclipse(int day , const std::string& month, int year, const std::string& eclipseTimeString = "00:00:00.000");
        static boost::posix_time::time_duration dayTimeFromEclipse(const std::string& eclipseTimeString);
    private:
        static std::map<std::string , boost::gregorian::greg_month> initEclipseMonthNames();

        std::vector<boost::posix_time::ptime> m_timeList;
    };
    typedef std::shared_ptr<TimeMap> TimeMapPtr;
    typedef std::shared_ptr<const TimeMap> TimeMapConstPtr;
}



#endif /* TIMEMAP_HPP_ */
