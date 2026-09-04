/*
  Copyright 2026 Equinor ASA.

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

#include "config.h"

#define BOOST_TEST_MODULE Test TimeService
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <chrono>
#include <ctime>
#include <limits>
#include <stdexcept>

// TimeStampUTC(std::time_t) breaks a time_t into civil time itself rather than
// calling std::gmtime, so these check the range std::gmtime would not have
// covered: dates a C runtime may refuse, and instants before the epoch.

BOOST_AUTO_TEST_CASE(FromTimeT_Epoch)
{
    const auto ts = Opm::TimeStampUTC{ std::time_t{0} };

    BOOST_CHECK_EQUAL(ts.year(),    1970);
    BOOST_CHECK_EQUAL(ts.month(),      1);
    BOOST_CHECK_EQUAL(ts.day(),        1);
    BOOST_CHECK_EQUAL(ts.hour(),       0);
    BOOST_CHECK_EQUAL(ts.minutes(),    0);
    BOOST_CHECK_EQUAL(ts.seconds(),    0);
}

BOOST_AUTO_TEST_CASE(FromTimeT_BeforeEpoch)
{
    // One second before the epoch. Needs floor division of a negative time_t:
    // truncation towards zero would land on 1970-01-01.
    const auto ts = Opm::TimeStampUTC{ std::time_t{-1} };

    BOOST_CHECK_EQUAL(ts.year(),    1969);
    BOOST_CHECK_EQUAL(ts.month(),     12);
    BOOST_CHECK_EQUAL(ts.day(),       31);
    BOOST_CHECK_EQUAL(ts.hour(),      23);
    BOOST_CHECK_EQUAL(ts.minutes(),   59);
    BOOST_CHECK_EQUAL(ts.seconds(),   59);
}

BOOST_AUTO_TEST_CASE(FromTimeT_LeapDay)
{
    // 2000-02-29, the century leap year the 400-year rule keeps.
    const auto ts = Opm::TimeStampUTC{ std::time_t{951'782'400} };

    BOOST_CHECK_EQUAL(ts.year(),    2000);
    BOOST_CHECK_EQUAL(ts.month(),      2);
    BOOST_CHECK_EQUAL(ts.day(),       29);
}

BOOST_AUTO_TEST_CASE(FromTimeT_BeyondYear3000)
{
    // 3001-01-01T00:00:00Z: the first instant past MSVC's _gmtime64 range,
    // which ends with year 3000. That refusal is why the conversion no longer
    // goes through std::gmtime. Schedules do reach here.
    const auto ts = Opm::TimeStampUTC{ std::time_t{32'535'216'000} };

    BOOST_CHECK_EQUAL(ts.year(),    3001);
    BOOST_CHECK_EQUAL(ts.month(),      1);
    BOOST_CHECK_EQUAL(ts.day(),        1);
    BOOST_CHECK_EQUAL(ts.hour(),       0);
}

BOOST_AUTO_TEST_CASE(RoundTripThroughTimeT)
{
    // asTimeT() and TimeStampUTC(time_t) go through portable_timegm() and
    // portable_gmtime() respectively; they must be exact inverses.
    for (const auto& ymd : { Opm::TimeStampUTC::YMD{1901,  1,  1},
                             Opm::TimeStampUTC::YMD{1969, 12, 31},
                             Opm::TimeStampUTC::YMD{1970,  1,  1},
                             Opm::TimeStampUTC::YMD{2000,  2, 29},
                             Opm::TimeStampUTC::YMD{2026,  8,  5},
                             Opm::TimeStampUTC::YMD{2100,  3,  1},
                             Opm::TimeStampUTC::YMD{3000,  1,  1} })
    {
        const auto stamp = Opm::TimeStampUTC{ ymd }.hour(13).minutes(37).seconds(7);
        const auto back  = Opm::TimeStampUTC{ Opm::asTimeT(stamp) };

        BOOST_CHECK_EQUAL(back.year(),    ymd.year);
        BOOST_CHECK_EQUAL(back.month(),   ymd.month);
        BOOST_CHECK_EQUAL(back.day(),     ymd.day);
        BOOST_CHECK_EQUAL(back.hour(),    13);
        BOOST_CHECK_EQUAL(back.minutes(), 37);
        BOOST_CHECK_EQUAL(back.seconds(),  7);
    }
}

// mkdatetime() rejects an impossible date by converting it and converting it
// back, and that check only works because portable_timegm() lets a day beyond
// the month carry into the next one instead of normalising it away or
// refusing it outright. Nothing else in the suite covers that, so a later
// rewrite of portable_timegm() -- to year_month_day_last, say, or to an ok()
// check on the date -- would quietly make OPM accept 30 FEB in a DATES record
// with every other test still passing.

BOOST_AUTO_TEST_CASE(MkDate_RejectsImpossibleDates)
{
    // 30 February 1983 counts on to 2 March, 33 January 2026 to 2 February,
    // and month 13 of 2026 to January 2027: in each case mkdate() is handed
    // back a date it did not ask for, and says so.
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(1983,  2, 30), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026,  1, 33), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026, 13,  1), std::invalid_argument);

    // 29 February is not wrap-around in a leap year, and is in every other.
    BOOST_CHECK_NO_THROW(Opm::TimeService::mkdate(2000, 2, 29));
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(1900, 2, 29), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026, 2, 29), std::invalid_argument);
}

// portable_gmtime() is the std::gmtime() replacement on the restart output
// path, and that path reads more than the date: DoubHEAD takes tm_yday, and
// a drop-in replacement has to agree on tm_wday too. Dates with a known
// weekday and day of year, on both sides of the epoch and beyond year 3000.

BOOST_AUTO_TEST_CASE(PortableGmtime_DayOfYearAndWeekday)
{
    struct Case { std::time_t t; int year; int mon; int mday; int yday; int wday; };
    for (const auto& c : { Case{               0, 1970,  1,  1,   0, 4 },   // Thursday
                           Case{              -1, 1969, 12, 31, 364, 3 },   // Wednesday
                           Case{     951'782'400, 2000,  2, 29,  59, 2 },   // Tuesday
                           // Sunday, and a leap year's last day
                           Case{     978'220'800, 2000, 12, 31, 365, 0 },
                           // Thursday, past _gmtime64's range
                           Case{  32'535'216'000, 3001,  1,  1,   0, 4 } })
    {
        const auto tm = Opm::TimeService::portable_gmtime(c.t);

        BOOST_CHECK_EQUAL(tm.tm_year + 1900, c.year);
        BOOST_CHECK_EQUAL(tm.tm_mon + 1,     c.mon);
        BOOST_CHECK_EQUAL(tm.tm_mday,        c.mday);
        BOOST_CHECK_EQUAL(tm.tm_yday,        c.yday);
        BOOST_CHECK_EQUAL(tm.tm_wday,        c.wday);
        BOOST_CHECK_EQUAL(tm.tm_isdst,       0);
    }
}

BOOST_AUTO_TEST_CASE(PortableGmtime_FarFromEpoch)
{
    // 0001-01-01T00:00:00Z, a Monday: 62 billion seconds before the epoch,
    // so the floor division of a negative time_t is exercised well away
    // from anything a C runtime handles, with a checkable answer.
    const auto tm = Opm::TimeService::portable_gmtime(std::time_t{-62'135'596'800});

    BOOST_CHECK_EQUAL(tm.tm_year + 1900, 1);
    BOOST_CHECK_EQUAL(tm.tm_mon + 1,     1);
    BOOST_CHECK_EQUAL(tm.tm_mday,        1);
    BOOST_CHECK_EQUAL(tm.tm_hour,        0);
    BOOST_CHECK_EQUAL(tm.tm_yday,        0);
    BOOST_CHECK_EQUAL(tm.tm_wday,        1);
}

BOOST_AUTO_TEST_CASE(PortableGmtime_EndsOfTheCalendar)
{
    // The last and the first instant std::chrono::year can represent:
    // 32767-12-31T23:59:59Z and -32767-01-01T00:00:00Z. Both convert, with
    // every field, and what one direction hands back the other takes back.
    {
        const auto t = std::time_t{971'890'963'199};
        auto tm = Opm::TimeService::portable_gmtime(t);

        BOOST_CHECK_EQUAL(tm.tm_year + 1900, 32767);
        BOOST_CHECK_EQUAL(tm.tm_mon + 1,     12);
        BOOST_CHECK_EQUAL(tm.tm_mday,        31);
        BOOST_CHECK_EQUAL(tm.tm_hour,        23);
        BOOST_CHECK_EQUAL(tm.tm_min,         59);
        BOOST_CHECK_EQUAL(tm.tm_sec,         59);
        BOOST_CHECK_EQUAL(tm.tm_yday,        364);
        BOOST_CHECK_EQUAL(Opm::TimeService::portable_timegm(&tm), t);
    }
    {
        // Taken from <chrono> itself, and checked against the arithmetic:
        // 12'687'428 days before the epoch.
        namespace ch = std::chrono;
        const auto first_day = ch::sys_days{ch::year::min() / ch::January / 1}
            .time_since_epoch().count();
        const auto t = std::time_t{first_day} * 86400;
        BOOST_CHECK_EQUAL(t, std::time_t{-1'096'193'779'200});
        auto tm = Opm::TimeService::portable_gmtime(t);

        BOOST_CHECK_EQUAL(tm.tm_year + 1900, -32767);
        BOOST_CHECK_EQUAL(tm.tm_mon + 1,     1);
        BOOST_CHECK_EQUAL(tm.tm_mday,        1);
        BOOST_CHECK_EQUAL(tm.tm_hour,        0);
        BOOST_CHECK_EQUAL(tm.tm_yday,        0);
        BOOST_CHECK_EQUAL(Opm::TimeService::portable_timegm(&tm), t);
    }
}

BOOST_AUTO_TEST_CASE(PortableGmtime_YearOutsideTm)
{
    // The ends of the 64-bit range. The floor division there used to
    // overflow (t - 86399 at the minimum); now it does not, and what is
    // refused is only what comes after it: a year outside std::chrono::year.
    // That refusal is the defined behaviour, in place of a silently wrong
    // date, and it starts one second past either end of the calendar.
    BOOST_CHECK_THROW(Opm::TimeService::portable_gmtime(std::numeric_limits<std::time_t>::min()),
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeService::portable_gmtime(std::numeric_limits<std::time_t>::max()),
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeService::portable_gmtime(std::time_t{971'890'963'199} + 1),
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeService::portable_gmtime(std::time_t{-1'096'193'779'200} - 1),
                      std::out_of_range);

    // And the other direction refuses a std::tm whose year is past the end,
    // after month normalisation: December of 32767 plus one month is 32768.
    std::tm past{};
    past.tm_year = 32767 - 1900;
    past.tm_mon  = 12;
    past.tm_mday = 1;
    BOOST_CHECK_THROW(Opm::TimeService::portable_timegm(&past), std::out_of_range);

    // Or whose year is inside it but whose day of the month or time of day
    // carries the instant past the end: 32767-12-32, 24:00 on 32767-12-31,
    // and an hour before -32767-01-01. The last valid instants beside them
    // convert.
    std::tm edge{};
    edge.tm_year = 32767 - 1900;
    edge.tm_mon  = 11;
    edge.tm_mday = 31;
    edge.tm_hour = 23; edge.tm_min = 59; edge.tm_sec = 59;
    BOOST_CHECK_EQUAL(Opm::TimeService::portable_timegm(&edge), std::time_t{971'890'963'199});
    edge.tm_mday = 32; edge.tm_hour = 0; edge.tm_min = 0; edge.tm_sec = 0;
    BOOST_CHECK_THROW(Opm::TimeService::portable_timegm(&edge), std::out_of_range);
    edge.tm_mday = 31; edge.tm_hour = 24;
    BOOST_CHECK_THROW(Opm::TimeService::portable_timegm(&edge), std::out_of_range);

    edge = std::tm{};
    edge.tm_year = -32767 - 1900;
    edge.tm_mon  = 0;
    edge.tm_mday = 1;
    BOOST_CHECK_EQUAL(Opm::TimeService::portable_timegm(&edge), std::time_t{-1'096'193'779'200});
    edge.tm_hour = -1;
    BOOST_CHECK_THROW(Opm::TimeService::portable_timegm(&edge), std::out_of_range);

    // An absurd field is refused too, rather than overflowing on the way.
    edge = std::tm{};
    edge.tm_year = 2026 - 1900;
    edge.tm_mon  = std::numeric_limits<int>::min();
    edge.tm_mday = 1;
    BOOST_CHECK_THROW(Opm::TimeService::portable_timegm(&edge), std::out_of_range);
    edge.tm_mon  = 0;
    edge.tm_mday = std::numeric_limits<int>::max();
    BOOST_CHECK_THROW(Opm::TimeService::portable_timegm(&edge), std::out_of_range);
}
