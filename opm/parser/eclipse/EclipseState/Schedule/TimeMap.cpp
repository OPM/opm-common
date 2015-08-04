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

#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>


namespace Opm {
    TimeMap::TimeMap(boost::posix_time::ptime startDate) {
        if (startDate.is_not_a_date_time())
          throw std::invalid_argument("Input argument not properly initialized.");

        m_timeList.push_back( boost::posix_time::ptime(startDate) );
    }

    TimeMap::TimeMap(Opm::DeckConstPtr deck) {
        // The default start date is not specified in the Eclipse
        // reference manual. We hence just assume it is same as for
        // the START keyword for Eclipse R100, i.e., January 1st,
        // 1983...
        boost::posix_time::ptime startTime(boost::gregorian::date(1983, 1, 1));

        // use the 'START' keyword to find out the start date (if the
        // keyword was specified)
        if (deck->hasKeyword("START")) {
            Opm::DeckKeywordConstPtr keyword = deck->getKeyword("START");
            startTime = timeFromEclipse(keyword->getRecord(/*index=*/0));
        }

        m_timeList.push_back( startTime );

        // find all "TSTEP" and "DATES" keywords in the deck and deal
        // with them one after another
        size_t numKeywords = deck->size();
        for (size_t keywordIdx = 0; keywordIdx < numKeywords; ++keywordIdx) {
            Opm::DeckKeywordConstPtr keyword = deck->getKeyword(keywordIdx);

            // We're only interested in "TSTEP" and "DATES" keywords,
            // so we ignore everything else here...
            if (keyword->name() != "TSTEP" &&
                keyword->name() != "DATES")
            {
                continue;
            }

            if (keyword->name() == "TSTEP")
                addFromTSTEPKeyword(keyword);
            else if (keyword->name() == "DATES")
                addFromDATESKeyword(keyword);
        }
    }

    size_t TimeMap::numTimesteps() const {
        return m_timeList.size() - 1;
    }


    boost::posix_time::ptime TimeMap::getStartTime(size_t tStepIdx) const {
        return m_timeList[tStepIdx];
    }


    double TimeMap::getTotalTime() const
    {
        if (m_timeList.size() < 2)
            return 0.0;
        boost::posix_time::time_duration deltaT = m_timeList.back() - m_timeList.front();
        return static_cast<double>(deltaT.total_milliseconds())/1000.0;
    }

    void TimeMap::addTime(boost::posix_time::ptime newTime) {
        boost::posix_time::ptime lastTime = m_timeList.back();
        if (newTime > lastTime)
            m_timeList.push_back( newTime );
        else
            throw std::invalid_argument("Times added must be in strictly increasing order.");
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

    const std::map<std::string , boost::gregorian::greg_month>& TimeMap::eclipseMonthNames() {
        static std::map<std::string , boost::gregorian::greg_month> monthNames;

        if (monthNames.size() == 0) {
            monthNames.insert( std::make_pair( "JAN" , boost::gregorian::Jan ));
            monthNames.insert( std::make_pair( "FEB" , boost::gregorian::Feb ));
            monthNames.insert( std::make_pair( "MAR" , boost::gregorian::Mar ));
            monthNames.insert( std::make_pair( "APR" , boost::gregorian::Apr ));
            monthNames.insert( std::make_pair( "MAI" , boost::gregorian::May ));
            monthNames.insert( std::make_pair( "MAY" , boost::gregorian::May ));
            monthNames.insert( std::make_pair( "JUN" , boost::gregorian::Jun ));
            monthNames.insert( std::make_pair( "JUL" , boost::gregorian::Jul ));
            monthNames.insert( std::make_pair( "JLY" , boost::gregorian::Jul ));
            monthNames.insert( std::make_pair( "AUG" , boost::gregorian::Aug ));
            monthNames.insert( std::make_pair( "SEP" , boost::gregorian::Sep ));
            monthNames.insert( std::make_pair( "OCT" , boost::gregorian::Oct ));
            monthNames.insert( std::make_pair( "OKT" , boost::gregorian::Oct ));
            monthNames.insert( std::make_pair( "NOV" , boost::gregorian::Nov ));
            monthNames.insert( std::make_pair( "DEC" , boost::gregorian::Dec ));
            monthNames.insert( std::make_pair( "DES" , boost::gregorian::Dec ));
        }

        return monthNames;
    }

    boost::posix_time::ptime TimeMap::timeFromEclipse(int day,
                                                      const std::string& eclipseMonthName,
                                                      int year,
                                                      const std::string& eclipseTimeString) {
        boost::gregorian::greg_month month = eclipseMonthNames().at( eclipseMonthName );
        boost::gregorian::date date( year , month , day );
        boost::posix_time::time_duration dayTime = dayTimeFromEclipse(eclipseTimeString);
        return boost::posix_time::ptime(date, dayTime);
    }

    boost::posix_time::time_duration TimeMap::dayTimeFromEclipse(const std::string& eclipseTimeString) {
        return boost::posix_time::duration_from_string(eclipseTimeString);
    }

    boost::posix_time::ptime TimeMap::timeFromEclipse(DeckRecordConstPtr dateRecord) {
        static const std::string errorMsg("The datarecord must consist of the for values "
                                          "\"DAY(int), MONTH(string), YEAR(int), TIME(string)\".\n");
        if (dateRecord->size() != 4) {
            throw std::invalid_argument( errorMsg);
        }

        DeckItemConstPtr dayItem = dateRecord->getItem( 0 );
        DeckItemConstPtr monthItem = dateRecord->getItem( 1 );
        DeckItemConstPtr yearItem = dateRecord->getItem( 2 );
        DeckItemConstPtr timeItem = dateRecord->getItem( 3 );

        try {
            int day = dayItem->getInt(0);
            const std::string& month = monthItem->getString(0);
            int year = yearItem->getInt(0);
            std::string eclipseTimeString = timeItem->getString(0);

            return TimeMap::timeFromEclipse(day, month, year, eclipseTimeString);
        } catch (...) {
            throw std::invalid_argument( errorMsg );
        }
    }

    void TimeMap::addFromDATESKeyword( DeckKeywordConstPtr DATESKeyword ) {
        if (DATESKeyword->name() != "DATES")
            throw std::invalid_argument("Method requires DATES keyword input.");

        for (size_t recordIndex = 0; recordIndex < DATESKeyword->size(); recordIndex++) {
            DeckRecordConstPtr record = DATESKeyword->getRecord( recordIndex );
            boost::posix_time::ptime nextTime = TimeMap::timeFromEclipse( record );
            addTime( nextTime );
        }
    }

    void TimeMap::addFromTSTEPKeyword( DeckKeywordConstPtr TSTEPKeyword ) {
        if (TSTEPKeyword->name() != "TSTEP")
            throw std::invalid_argument("Method requires TSTEP keyword input.");
        {
            DeckRecordConstPtr record = TSTEPKeyword->getRecord( 0 );
            DeckItemConstPtr item = record->getItem( 0 );

            for (size_t itemIndex = 0; itemIndex < item->size(); itemIndex++) {
                double days = item->getRawDouble( itemIndex );
                long int wholeSeconds = static_cast<long int>(days * 24*60*60);
                long int milliSeconds = static_cast<long int>((days * 24*60*60 - wholeSeconds)*1000);
                boost::posix_time::time_duration step =
                    boost::posix_time::seconds(wholeSeconds) +
                    boost::posix_time::milliseconds(milliSeconds);
                addTStep( step );
            }
        }
    }

    double TimeMap::getTimeStepLength(size_t tStepIdx) const
    {
        assert(tStepIdx < numTimesteps());
        const boost::posix_time::ptime &t1
            = m_timeList[tStepIdx];
        const boost::posix_time::ptime &t2
            = m_timeList[tStepIdx + 1];
        const boost::posix_time::time_duration &deltaT
            = t2 - t1;
        return static_cast<double>(deltaT.total_milliseconds())/1000.0;
    }

    double TimeMap::getTimePassedUntil(size_t tLevelIdx) const
    {
        assert(tLevelIdx < m_timeList.size());
        const boost::posix_time::ptime &t1
            = m_timeList.front();
        const boost::posix_time::ptime &t2
            = m_timeList[tLevelIdx];
        const boost::posix_time::time_duration &deltaT
            = t2 - t1;
        return static_cast<double>(deltaT.total_milliseconds())/1000.0;
    }


     void TimeMap::initFirstTimestepsMonths(std::vector<size_t>& timesteps, size_t from_timestep) const {
        timesteps.clear();

        const boost::posix_time::ptime& ptime_prev = getStartTime(from_timestep-1);
        boost::gregorian::date prev_date = ptime_prev.date();

        for (size_t timestepIndex = from_timestep; timestepIndex < m_timeList.size(); ++timestepIndex) {
            const boost::posix_time::ptime& ptime_cur = getStartTime(timestepIndex);
            boost::gregorian::date cur_date = ptime_cur.date();

            if (cur_date.month() != prev_date.month()) {
                timesteps.push_back(timestepIndex);
                prev_date = cur_date;
            }
        }
    }



    void TimeMap::initFirstTimestepsYears(std::vector<size_t>& timesteps, size_t from_timestep) const {
        timesteps.clear();

        const boost::posix_time::ptime& ptime_prev = getStartTime(from_timestep-1);
        boost::gregorian::date prev_date = ptime_prev.date();

        for (size_t timestepIndex = from_timestep; timestepIndex < m_timeList.size(); ++timestepIndex) {
            const boost::posix_time::ptime& ptime_cur = getStartTime(timestepIndex);
            boost::gregorian::date cur_date = ptime_cur.date();

            if (cur_date.year() != prev_date.year()) {
                timesteps.push_back(timestepIndex);
                prev_date = cur_date;
            }
        }
    }


}


