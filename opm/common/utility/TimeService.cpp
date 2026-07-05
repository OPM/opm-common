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
#include <limits>
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



    // The days_from_civil() function is from Howard Hinnant, http://howardhinnant.github.io/date_algorithms.html
    // The website states: "Consider these donated to the public domain."

    // Returns number of days since civil 1970-01-01.  Negative values indicate
    //    days prior to 1970-01-01.
    // Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
    //                 m is in [1, 12]
    //                 d is in [1, last_day_of_month(y, m)]
    //                 y is "approximately" in
    //                   [numeric_limits<Int>::min()/366, numeric_limits<Int>::max()/366]
    //                 Exact range of validity is:
    //                 [civil_from_days(numeric_limits<Int>::min()),
    //                  civil_from_days(numeric_limits<Int>::max()-719468)]
    template <class Int>
    constexpr
    Int
    days_from_civil(Int y, unsigned m, unsigned d) noexcept
    {
        static_assert(std::numeric_limits<unsigned>::digits >= 18,
                      "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<Int>::digits >= 20,
                      "This algorithm has not been ported to a 16 bit signed integer");
        y -= m <= 2;
        const Int era = (y >= 0 ? y : y-399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);      // [0, 399]
        const unsigned doy = (153*(m > 2 ? m-3 : m+9) + 2)/5 + d-1;  // [0, 365]
        const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
        return era * 146097 + static_cast<Int>(doe) - 719468;
    }

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

bool valid_month(const std::string& month_name) {
    return (month_indices.count(month_name) != 0);
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

// The portable_timegm() function is based on
// https://stackoverflow.com/questions/16647819/timegm-cross-platform
// answer by Sergey D.
std::time_t portable_timegm(const std::tm* t)
{
    int year = t->tm_year + 1900;
    int month = t->tm_mon;          // 0-11
    if (month > 11) {
        year += month / 12;
        month %= 12;
    } else if (month < 0) {
        int years_diff = (11 - month) / 12;
        year -= years_diff;
        month += 12 * years_diff;
    }
    int days_from_1970 = days_from_civil(year, month + 1, t->tm_mday);
    return 60 * (60 * (24L * days_from_1970 + t->tm_hour) + t->tm_min) + t->tm_sec;
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

    /*
      std::gmtime() returns nullptr for time points it cannot represent --
      notably on Windows, where the CRT rejects dates after year 3000 (glibc
      handles these fine). Dereferencing that nullptr crashes, and simulation
      schedules can legitimately extend that far. Fall back to an explicit
      civil-from-days conversion (Howard Hinnant's algorithm, exact over the
      full 64 bit time_t range).
    */
    std::tm gmtime_or_civil(const std::time_t t)
    {
        {
            auto tcopy = t;
            if (const auto* tm_ptr = std::gmtime(&tcopy)) {
                return *tm_ptr;
            }
        }

        const auto days = (t >= 0 ? t : t - 86399) / 86400;   // floor division
        auto secs = t - days * 86400;

        const auto z   = days + 719468;
        const auto era = (z >= 0 ? z : z - 146096) / 146097;
        const auto doe = static_cast<unsigned long long>(z - era * 146097);
        const auto yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
        const auto y   = static_cast<long long>(yoe) + era * 400;
        const auto doy = doe - (365*yoe + yoe/4 - yoe/100);
        const auto mp  = (5*doy + 2) / 153;
        const auto d   = doy - (153*mp + 2)/5 + 1;
        const auto m   = mp < 10 ? mp + 3 : mp - 9;

        std::tm tm{};
        tm.tm_year = static_cast<int>(y + (m <= 2)) - 1900;
        tm.tm_mon  = static_cast<int>(m) - 1;
        tm.tm_mday = static_cast<int>(d);
        tm.tm_hour = static_cast<int>(secs / 3600); secs %= 3600;
        tm.tm_min  = static_cast<int>(secs / 60);
        tm.tm_sec  = static_cast<int>(secs % 60);
        return tm;
    }

}

Opm::TimeStampUTC::TimeStampUTC(const std::time_t tp)
{
    const auto tm = gmtime_or_civil(tp);

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
    const auto tm = gmtime_or_civil(tp);

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
