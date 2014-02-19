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


    boost::gregorian::date TimeMap::getStartDate() const {
        return m_startDate;
    }


    std::map<std::string , boost::gregorian::greg_month> TimeMap::initEclipseMonthNames() {
        std::map<std::string , boost::gregorian::greg_month> monthNames;

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
        
        return monthNames;
    }
    

    boost::gregorian::date TimeMap::dateFromEclipse(int day , const std::string& eclipseMonthName, int year) {
        static const std::map<std::string,boost::gregorian::greg_month> monthNames = initEclipseMonthNames();
        boost::gregorian::greg_month month = monthNames.at( eclipseMonthName );
        return boost::gregorian::date( year , month , day );
    }
    


    boost::gregorian::date TimeMap::dateFromEclipse(DeckRecordConstPtr dateRecord) {
        static const std::string errorMsg("The datarecord must consist of the three values "
                                          "\"DAY(int)  MONTH(string)  YEAR(int)\" plus the optional value \"TIME(string)\".\n");
        if (dateRecord->size() < 3 || dateRecord->size() > 4)
            throw std::invalid_argument( errorMsg);

        DeckItemConstPtr dayItem = dateRecord->getItem( 0 );
        DeckItemConstPtr monthItem = dateRecord->getItem( 1 );
        DeckItemConstPtr yearItem = dateRecord->getItem( 2 );
        //DeckItemConstPtr timeItem = dateRecord->getItem( 3 );

        try {
            int day = dayItem->getInt(0);
            const std::string& month = monthItem->getString(0);
            int year = yearItem->getInt(0);

            return TimeMap::dateFromEclipse( day , month , year);
        } catch (...) {
            throw std::invalid_argument( errorMsg );
        }
    }

    
    
    void TimeMap::addFromDATESKeyword( DeckKeywordConstPtr DATESKeyword ) {
        if (DATESKeyword->name() != "DATES")
            throw std::invalid_argument("Method requires DATES keyword input.");

        for (size_t recordIndex = 0; recordIndex < DATESKeyword->size(); recordIndex++) {
            DeckRecordConstPtr record = DATESKeyword->getRecord( recordIndex );
            boost::gregorian::date date = TimeMap::dateFromEclipse( record );
            addDate( date );
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
                boost::posix_time::time_duration step = boost::posix_time::seconds( static_cast<long int>(days * 24 * 3600) );
                addTStep( step );
            }
        }
    } 
    
}


