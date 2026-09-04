/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#include <opm/common/utility/TimeService.hpp>

#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <chrono>
#include <ctime>
#include <utility>
#include <stdexcept>
#include <string>

namespace Opm {
namespace TimeService {

namespace {
    const std::unordered_map<std::string, int> month_indices = {
        {"JAN", 1},
        {"FEB", 2},
        {"MAR", 3},
        {"APR", 4},
        {"MAI", 5},
        {"MAY", 5},
        {"JUN", 6},
        {"JUL", 7},
        {"JLY", 7},
        {"AUG", 8},
        {"SEP", 9},
        {"OCT", 10},
        {"OKT", 10},
        {"NOV", 11},
        {"DEC", 12},
        {"DES", 12}};

    const std::unordered_map<int, std::string> month_names = {
        {1, "JAN"},
        {2, "FEB"},
        {3, "MAR"},
        {4, "APR"},
        {5, "MAY"},
        {6, "JUN"},
        {7, "JUL"},
        {8, "AUG"},
        {9, "SEP"},
        {10, "OCT"},
        {11, "NOV"},
        {12, "DEC"}};



    // The days std::chrono::year spans, -32767-01-01 to 32767-12-31, taken
    // from the types themselves rather than restated, and the seconds those
    // days hold. Both conversions accept exactly this and refuse the rest.
    //
    // long long, not std::time_t: the second counts are near 1e12, and a
    // 32-bit time_t would not merely hold the wrong value - a constant
    // expression that overflows is ill-formed, so the file would not compile.
    // Leaving the arithmetic at the width of long is what the old conversion
    // got wrong; a fixed 64 bits here is what keeps that from recurring.
    // Comparing a time_t against these widens it, which loses nothing.
    namespace calendar_bounds {
        constexpr long long first_day =
            std::chrono::sys_days{std::chrono::year::min() / std::chrono::January / 1}
            .time_since_epoch().count();
        constexpr long long last_day =
            std::chrono::sys_days{std::chrono::year::max() / std::chrono::December / 31}
            .time_since_epoch().count();
        constexpr long long first_second = first_day * 86400;
        constexpr long long last_second  = last_day * 86400 + 86399;
    }

    // The arithmetic above is a fixed 64 bits, but a std::time_t is what
    // both conversions take and return, and a narrow one would silently
    // truncate every date the calendar bounds admit. A schedule runs past
    // 2038 as a matter of course, so require the width rather than lose the
    // dates quietly. Nothing OPM Flow builds on has a narrower time_t.
    static_assert(sizeof(std::time_t) >= 8,
                  "OPM Flow schedules run past 2038; std::time_t must be 64-bit");


} // anonymous namespace



const time_t system_clock_epoch = std::chrono::system_clock::to_time_t({});

time_point from_time_t(std::time_t t) {
    auto diff = std::difftime(t, system_clock_epoch);
    return time_point(std::chrono::seconds(static_cast<std::chrono::seconds::rep>(diff)));
}

std::time_t to_time_t(const time_point& tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count() + system_clock_epoch;
}


time_point now() {
    time_point epoch;
    auto default_now = std::chrono::system_clock::now();
    return epoch + std::chrono::duration_cast<Opm::time_point::duration>(default_now.time_since_epoch());
}

std::time_t advance(const std::time_t tp, const double sec)
{
    const auto t = Opm::TimeService::from_time_t(tp) + std::chrono::duration_cast<Opm::time_point::duration>(std::chrono::duration<double>(sec));
    return Opm::TimeService::to_time_t(t);
}

std::time_t makeUTCTime(std::tm timePoint)
{
    return portable_timegm(&timePoint);
}

const std::unordered_map<std::string , int>& eclipseMonthIndices() {
    return month_indices;
}

int eclipseMonth(const std::string& name) {
    auto iter = month_indices.find(name);
    if (iter != month_indices.end())
        return iter->second;

    return std::stod(name);
}


const std::unordered_map<int, std::string>& eclipseMonthNames() {
    return month_names;
}

bool valid_month(const std::string& month_name)
{
    return month_indices.contains(month_name);
}

std::time_t mkdatetime(int in_year, int in_month, int in_day, int hour, int minute, int second) {
    const auto tp = TimeStampUTC{ TimeStampUTC::YMD { in_year, in_month, in_day } }
        .hour(hour).minutes(minute).seconds(second);

    std::time_t t = asTimeT(tp);
    {
        /*
          The underlying mktime( ) function will happily wrap
          around dates like January 33, this function will check
          that no such wrap-around has taken place.
        */
        const auto check = TimeStampUTC{ t };
        if ((in_day != check.day()) || (in_month != check.month()) || (in_year != check.year()))
            throw std::invalid_argument("Invalid input arguments for date.");
    }
    return t;
}

std::time_t mkdate(int in_year, int in_month, int in_day) {
    return mkdatetime(in_year , in_month , in_day, 0,0,0);
}

// timegm() is POSIX, not C++, and the Windows spelling _mkgmtime() stops at
// year 3000. This is the conversion written with the C++20 <chrono> calendar
// types instead, valid wherever they are: a std::tm's year, month, day and
// time of day to seconds since the epoch, in UTC.
//
// Only the month is normalised into the year, as the earlier version of this
// function did. A day outside the month counts on from its first (33 January
// is 2 February) and the time of day is added as it stands, which is the
// wrap-around mkdatetime() relies on to reject such input.
std::time_t portable_timegm(const std::tm* t)
{
    namespace ch = std::chrono;

    // Everything in long long before any arithmetic: a std::tm's fields are
    // ints, and a caller's absurd value must end in the refusal below, not in
    // an overflow on the way there.
    long long yr    = static_cast<long long>(t->tm_year) + 1900;
    long long month = t->tm_mon;    // 0-11
    if (month > 11) {
        yr += month / 12;
        month %= 12;
    } else if (month < 0) {
        const long long years_diff = (11 - month) / 12;
        yr -= years_diff;
        month += 12 * years_diff;
    }

    // std::chrono::year runs from -32767 to 32767. No schedule is anywhere
    // near either end; refuse the rest rather than hand back a wrong instant.
    if (yr < static_cast<int>(ch::year::min()) || yr > static_cast<int>(ch::year::max())) {
        throw std::out_of_range {
            "Calendar year " + std::to_string(yr) +
            " is outside the range std::chrono::year can represent"
        };
    }

    const ch::sys_days first_of_month =
        ch::year{static_cast<int>(yr)} / ch::month{static_cast<unsigned>(month + 1)} / 1;
    // long long, not int: 86400 * days overflows a 32-bit type for any date
    // outside roughly 1902-2038, and widening the day count promotes the
    // whole expression. Not std::time_t either, for the reason the bounds
    // above are not.
    const long long days_from_1970 =
        static_cast<long long>(first_of_month.time_since_epoch().count()) +
        (static_cast<long long>(t->tm_mday) - 1);
    const long long result =
        60 * (60 * (24 * days_from_1970 + t->tm_hour) + t->tm_min) + t->tm_sec;

    // The day of the month and the time of day may have carried the instant
    // past the calendar's end even though the year was inside it -
    // 32767-12-32, or 24:00 on the last day - and portable_gmtime() would
    // refuse what came back. Refuse it here instead, against the same bounds.
    if (result < calendar_bounds::first_second || result > calendar_bounds::last_second) {
        throw std::out_of_range {
            "Date " + std::to_string(yr) + "-" + std::to_string(month + 1) + "-" +
            std::to_string(t->tm_mday) + " with the time of day added lies outside " +
            "the range std::chrono::year can represent"
        };
    }
    return static_cast<std::time_t>(result);
}

/*
  Break a time_t into UTC civil time without std::gmtime(), the inverse of
  portable_timegm() above.

  std::gmtime() is unsuitable here on two counts. It returns nullptr for
  time points it cannot represent -- some C runtimes refuse dates beyond
  year 3000, which simulation schedules legitimately reach -- and
  dereferencing that is a crash. It also returns a pointer to a static
  buffer, so two threads converting timestamps concurrently overwrite each
  other's result.

  The C++20 <chrono> calendar types have neither problem and are the exact
  inverse of what portable_timegm() does in the other direction, so use
  them unconditionally: every platform then exercises the same code.

  Every field std::gmtime() sets is set here too, so this is a drop-in
  replacement: DoubHEAD's day-of-year calculation reads tm_yday.
  tm_isdst is 0, as it is for UTC.
*/
std::tm portable_gmtime(const std::time_t t)
{
    namespace ch = std::chrono;

    // Floor division, without the overflow that t - 86399 has at the
    // bottom of the range: take the quotient and remainder first, then
    // carry a negative remainder into the day. Neither operation can
    // overflow for any representable time_t.
    auto days = t / 86400;
    auto secs = t % 86400;
    if (secs < 0) {
        secs += 86400;
        --days;
    }

    // A 64-bit time_t exceeds the days std::chrono::year spans by a wide
    // margin in either direction; no schedule holds such an instant, and
    // a day count outside them does not fit the calendar arithmetic, so
    // refuse it - as std::gmtime() refuses, with nullptr, what its
    // runtime cannot represent - rather than hand back a wrong date.
    if ((days < calendar_bounds::first_day) || (days > calendar_bounds::last_day)) {
        throw std::out_of_range {
            "Time point " + std::to_string(static_cast<long long>(t)) +
            " is outside the range of years std::chrono::year can represent"
        };
    }

    const ch::sys_days day{ch::days{static_cast<int>(days)}};
    const ch::year_month_day ymd{day};

    std::tm tm{};
    tm.tm_year = static_cast<int>(ymd.year()) - 1900;
    tm.tm_mon  = static_cast<int>(static_cast<unsigned>(ymd.month())) - 1;
    tm.tm_mday = static_cast<int>(static_cast<unsigned>(ymd.day()));
    tm.tm_hour = static_cast<int>(secs / 3600); secs %= 3600;
    tm.tm_min  = static_cast<int>(secs / 60);
    tm.tm_sec  = static_cast<int>(secs % 60);

    // Day of year, counted from 0 as std::tm wants it, and the weekday
    // with Sunday as 0, as std::tm and <chrono>'s c_encoding() agree.
    tm.tm_yday = static_cast<int>((day - ch::sys_days{ymd.year() / ch::January / 1}).count());
    tm.tm_wday = static_cast<int>(ch::weekday{day}.c_encoding());

    tm.tm_isdst = 0;   // UTC never has one
    return tm;
}

std::time_t timeFromEclipse(const DeckRecord &dateRecord) {
    const auto &dayItem = dateRecord.getItem(0);
    const auto &monthItem = dateRecord.getItem(1);
    const auto &yearItem = dateRecord.getItem(2);
    const auto &timeItem = dateRecord.getItem(3);

    int hour = 0, min = 0, second = 0;
    if (timeItem.hasValue(0)) {
        if (sscanf(timeItem.get<std::string>(0).c_str(), "%d:%d:%d" , &hour,&min,&second) != 3) {
            hour = min = second = 0;
        }
    }

    // Accept lower- and mixed-case month names.
    std::string monthname = uppercase(monthItem.get<std::string>(0));

    std::time_t date = mkdatetime(yearItem.get<int>(0),
                                  TimeService::eclipseMonthIndices().at(monthname),
                                  dayItem.get<int>(0),
                                  hour,
                                  min,
                                  second);
    return date;
}

}
}

namespace {



    std::tm makeTm(const Opm::TimeStampUTC& tp) {
        auto timePoint = std::tm{};

        timePoint.tm_year = tp.year()  - 1900;
        timePoint.tm_mon  = tp.month() -    1;
        timePoint.tm_mday = tp.day();
        timePoint.tm_hour = tp.hour();
        timePoint.tm_min  = tp.minutes();
        timePoint.tm_sec  = tp.seconds();

        return timePoint;
    }

}

Opm::TimeStampUTC::TimeStampUTC(const std::time_t tp)
{
    const auto tm = TimeService::portable_gmtime(tp);

    this->ymd_ = YMD { tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday };

    this->hour(tm.tm_hour).minutes(tm.tm_min).seconds(tm.tm_sec);
}

Opm::TimeStampUTC::TimeStampUTC(const Opm::TimeStampUTC::YMD& ymd,
                                int hour, int minutes, int seconds, int usec)
    : ymd_(ymd)
    , hour_(hour)
    , minutes_(minutes)
    , seconds_(seconds)
    , usec_(usec)
{}

Opm::TimeStampUTC& Opm::TimeStampUTC::operator=(const std::time_t tp)
{
    const auto tm = TimeService::portable_gmtime(tp);

    this->ymd_ = YMD { tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday };

    this->hour(tm.tm_hour).minutes(tm.tm_min).seconds(tm.tm_sec);

    return *this;
}

bool Opm::TimeStampUTC::operator==(const TimeStampUTC& data) const
{
    return ymd_ == data.ymd_ &&
           hour_ == data.hour_ &&
           minutes_ == data.minutes_ &&
           seconds_ == data.seconds_ &&
           usec_ == data.usec_;
}

Opm::TimeStampUTC::TimeStampUTC(const YMD& ymd)
    : ymd_{ std::move(ymd) }
{}

Opm::TimeStampUTC::TimeStampUTC(int year, int month, int day)
    : ymd_{ year, month, day }
{}

Opm::TimeStampUTC& Opm::TimeStampUTC::hour(const int h)
{
    this->hour_ = h;
    return *this;
}

Opm::TimeStampUTC& Opm::TimeStampUTC::minutes(const int m)
{
    this->minutes_ = m;
    return *this;
}

Opm::TimeStampUTC& Opm::TimeStampUTC::seconds(const int s)
{
    this->seconds_ = s;
    return *this;
}

Opm::TimeStampUTC& Opm::TimeStampUTC::microseconds(const int us)
{
    this->usec_ = us;
    return *this;
}


std::time_t Opm::asTimeT(const TimeStampUTC& tp)
{
    return Opm::TimeService::makeUTCTime(makeTm(tp));
}

std::time_t Opm::asLocalTimeT(const TimeStampUTC& tp)
{
    auto tm = makeTm(tp);
    return std::mktime(&tm);
}

Opm::TimeStampUTC Opm::operator+(const Opm::TimeStampUTC& lhs, std::chrono::duration<double> delta) {
    return Opm::TimeStampUTC( Opm::TimeService::advance(Opm::asTimeT(lhs) , delta.count()) );
}

Opm::time_point Opm::asTimePoint(const TimeStampUTC& ts)
{
    return Opm::TimeService::from_time_t( Opm::asTimeT(ts) );
}
