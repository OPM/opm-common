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
#ifndef OPM_PARSER_START_WRAPPER_HPP
#define	OPM_PARSER_START_WRAPPER_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>

#include <stdexcept>

namespace Opm {
    class StartWrapper {
    public:
        /*!
         * \brief A wrapper class to provide convenient access to the
         *        data exposed by the 'START' keyword.
         */
        StartWrapper(Opm::DeckKeywordConstPtr keyword)
        {
            int day = keyword->getRecord(0)->getItem(0)->getInt(0);
            std::string month = keyword->getRecord(0)->getItem(1)->getString(0);
            int year = keyword->getRecord(0)->getItem(2)->getInt(0);

            int monthNum = 0;
            if (month == "JAN")
                monthNum = 1;
            else if (month == "FEB")
                monthNum = 2;
            else if (month == "MAR")
                monthNum = 3;
            else if (month == "APR")
                monthNum = 4;
            else if (month == "MAY")
                monthNum = 5;
            else if (month == "JUN")
                monthNum = 6;
            else if (month == "JUL")
                monthNum = 7;
            else if (month == "AUG")
                monthNum = 8;
            else if (month == "SEP")
                monthNum = 9;
            else if (month == "OCT")
                monthNum = 10;
            else if (month == "NOV")
                monthNum = 11;
            else if (month == "DEC")
                monthNum = 12;
            else
                throw std::runtime_error("Invalid month specified for START keyword");

            m_startDate = boost::gregorian::date(year, monthNum, day);
        }

        /*!
         * \brief Return calendar date at which the simulation starts.
         */
        const boost::gregorian::date &getStartDate() const
        { return m_startDate; }

    private:
        boost::gregorian::date m_startDate;
    };
}

#endif	// OPM_PARSER_START_KEYWORD_HPP

