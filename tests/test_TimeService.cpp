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

#include <ctime>
#include <limits>
#include <stdexcept>

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace {

    // Last and first instants of the years std::chrono::year represents:
    // 32767-12-31T23:59:59Z and -32767-01-01T00:00:00Z.
    constexpr auto lastInstant  = std::time_t{   971'890'963'199};
    constexpr auto firstInstant = std::time_t{-1'096'193'779'200};

    std::time_t makeTimeT(const int year, const int month, const int day,
                          const int hour = 0, const int minute = 0, const int second = 0)
    {
        return Opm::asTimeT(Opm::TimeStampUTC { Opm::TimeStampUTC::YMD { year, month, day } }
                            .hour(hour).minutes(minute).seconds(second));
    }

    void checkTimeStamp(const Opm::TimeStampUTC& ts,
                        const int year, const int month, const int day,
                        const int hour, const int minute, const int second)
    {
        BOOST_CHECK_EQUAL(ts.year(),    year);
        BOOST_CHECK_EQUAL(ts.month(),   month);
        BOOST_CHECK_EQUAL(ts.day(),     day);
        BOOST_CHECK_EQUAL(ts.hour(),    hour);
        BOOST_CHECK_EQUAL(ts.minutes(), minute);
        BOOST_CHECK_EQUAL(ts.seconds(), second);
    }

} // Anonymous namespace

// TimeStampUTC(std::time_t) used to call std::gmtime(), which may return
// nullptr outside 1970-3000.  These cover dates on both sides of that range.

BOOST_AUTO_TEST_CASE(FromTimeT_Epoch)
{
    checkTimeStamp(Opm::TimeStampUTC { std::time_t{0} }, 1970, 1, 1, 0, 0, 0);
}

BOOST_AUTO_TEST_CASE(FromTimeT_BeforeEpoch)
{
    // Negative counts need floor division.  Truncation towards zero would
    // give 1970-01-01.
    checkTimeStamp(Opm::TimeStampUTC { std::time_t{-1} }, 1969, 12, 31, 23, 59, 59);
}

BOOST_AUTO_TEST_CASE(FromTimeT_LeapDay)
{
    // 2000-02-29, the century leap year the 400-year rule keeps.
    checkTimeStamp(Opm::TimeStampUTC { std::time_t{951'782'400} }, 2000, 2, 29, 0, 0, 0);
}

BOOST_AUTO_TEST_CASE(FromTimeT_PastYear3000)
{
    // 3001-01-01, one second past the end of MSVC's std::gmtime() range.
    checkTimeStamp(Opm::TimeStampUTC { std::time_t{32'535'216'000} }, 3001, 1, 1, 0, 0, 0);
}

BOOST_AUTO_TEST_CASE(FromTimeT_YearOne)
{
    checkTimeStamp(Opm::TimeStampUTC { std::time_t{-62'135'596'800} }, 1, 1, 1, 0, 0, 0);
}

BOOST_AUTO_TEST_CASE(AssignFromTimeT)
{
    auto ts = Opm::TimeStampUTC {};
    ts = std::time_t{32'535'216'000};

    BOOST_CHECK(ts == Opm::TimeStampUTC { std::time_t{32'535'216'000} });
}

BOOST_AUTO_TEST_CASE(RoundTrip)
{
    // Includes dates outside 1902-2038, where a 32-bit second count
    // overflows.
    for (const auto& ymd : { Opm::TimeStampUTC::YMD {1901,  1,  1},
                             Opm::TimeStampUTC::YMD {1969, 12, 31},
                             Opm::TimeStampUTC::YMD {1970,  1,  1},
                             Opm::TimeStampUTC::YMD {2000,  2, 29},
                             Opm::TimeStampUTC::YMD {2100,  3,  1},
                             Opm::TimeStampUTC::YMD {3000, 12, 31},
                             Opm::TimeStampUTC::YMD {3001,  1,  1} })
    {
        const auto t = makeTimeT(ymd.year, ymd.month, ymd.day, 13, 37, 7);
        checkTimeStamp(Opm::TimeStampUTC { t }, ymd.year, ymd.month, ymd.day, 13, 37, 7);
    }
}

BOOST_AUTO_TEST_CASE(FieldsCarry)
{
    // asTimeT() lets the month carry into the year and the day and time of
    // day into the month.  mkdatetime() relies on that to detect dates that
    // do not exist.
    BOOST_CHECK_EQUAL(makeTimeT(1983,  2, 30), makeTimeT(1983,  3,  2));
    BOOST_CHECK_EQUAL(makeTimeT(2026,  1, 33), makeTimeT(2026,  2,  2));
    BOOST_CHECK_EQUAL(makeTimeT(2026, 13,  1), makeTimeT(2027,  1,  1));
    BOOST_CHECK_EQUAL(makeTimeT(2026,  0,  1), makeTimeT(2025, 12,  1));
    BOOST_CHECK_EQUAL(makeTimeT(2026, -11, 1), makeTimeT(2025,  1,  1));
    BOOST_CHECK_EQUAL(makeTimeT(2026, -12, 1), makeTimeT(2024, 12,  1));
    BOOST_CHECK_EQUAL(makeTimeT(2026,  3,  0), makeTimeT(2026,  2, 28));
    BOOST_CHECK_EQUAL(makeTimeT(2026, 12, 31, 24), makeTimeT(2027, 1, 1));

    // Without overflow: 2^31 seconds is some 68 years.
    const auto intMin = std::numeric_limits<int>::min();
    BOOST_CHECK_EQUAL(makeTimeT(2026, 1, 1, 0, 0, intMin), makeTimeT(2026, 1, 1) + intMin);
}

BOOST_AUTO_TEST_CASE(MkDate_RejectsNonExistentDates)
{
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(1983,  2, 30), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026,  1, 33), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026, 13,  1), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026,  0,  1), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(1900,  2, 29), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::TimeService::mkdate(2026,  2, 29), std::invalid_argument);

    BOOST_CHECK(Opm::TimeService::mkdate(2000, 2, 29) ==
                Opm::TimeService::from_time_t(951'782'400));
}

BOOST_AUTO_TEST_CASE(EndsOfTheCalendar)
{
    checkTimeStamp(Opm::TimeStampUTC { lastInstant }, 32767, 12, 31, 23, 59, 59);
    checkTimeStamp(Opm::TimeStampUTC { firstInstant }, -32767, 1, 1, 0, 0, 0);

    BOOST_CHECK_EQUAL(makeTimeT(32767, 12, 31, 23, 59, 59), lastInstant);
    BOOST_CHECK_EQUAL(makeTimeT(-32767, 1, 1), firstInstant);
}

BOOST_AUTO_TEST_CASE(OutsideTheCalendar)
{
    // Refused rather than returned as a wrong date.
    BOOST_CHECK_THROW(Opm::TimeStampUTC { lastInstant + 1 }, std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeStampUTC { firstInstant - 1 }, std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeStampUTC { std::numeric_limits<std::time_t>::max() },
                      std::out_of_range);
    BOOST_CHECK_THROW(Opm::TimeStampUTC { std::numeric_limits<std::time_t>::min() },
                      std::out_of_range);

    // A year past either end, and an in-range year that the day of the
    // month or the time of day carries past the end.
    BOOST_CHECK_THROW(makeTimeT( 32768,  1,  1), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT( 32767, 12, 32), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT( 32767, 12, 31, 24), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(-32768,  1,  1), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(-32767,  1,  1, 0, 0, -1), std::out_of_range);

    // Absurd fields end in the same refusal, not in integer overflow.
    constexpr auto intMax = std::numeric_limits<int>::max();
    constexpr auto intMin = std::numeric_limits<int>::min();

    BOOST_CHECK_THROW(makeTimeT(intMax, 1, 1), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(intMin, 1, 1), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(2026, intMax, 1), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(2026, intMin, 1), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(2026, 1, intMax), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(2026, 1, intMin), std::out_of_range);
    BOOST_CHECK_THROW(makeTimeT(2026, 1, 1, intMax), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(FormatPastYear3000)
{
    // fmt::gmtime() throws for the dates its C runtime refuses.  asTm()
    // only copies fields, so formatting its result works for any date.
    const auto ts = Opm::TimeStampUTC { Opm::TimeStampUTC::YMD { 3001, 1, 1 } }
        .hour(13).minutes(37).seconds(7);

    BOOST_CHECK_EQUAL(fmt::format("{:%d-%b-%Y %H:%M:%S}", Opm::asTm(ts)),
                      "01-Jan-3001 13:37:07");
}
