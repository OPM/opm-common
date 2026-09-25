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

#include <fmt/format.h>

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

time_point mkdatetime(int in_year, int in_month, int in_day, int hour, int minute, int second) {
    const auto tp = TimeStampUTC{ TimeStampUTC::YMD { in_year, in_month, in_day } }
        .hour(hour).minutes(minute).seconds(second);

    const auto t = asTimePoint(tp);
    {
        /*
          asTimePoint() will happily wrap around dates like January
          33, this function will check that no such wrap-around
          has taken place.
        */
        const auto check = TimeStampUTC{ t };
        if ((in_day != check.day()) || (in_month != check.month()) || (in_year != check.year()))
            throw std::invalid_argument("Invalid input arguments for date.");
    }
    return t;
}

time_point mkdate(int in_year, int in_month, int in_day) {
    return mkdatetime(in_year , in_month , in_day, 0,0,0);
}

time_point timeFromEclipse(const DeckRecord &dateRecord) {
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

    return mkdatetime(yearItem.get<int>(0),
                      TimeService::eclipseMonthIndices().at(monthname),
                      dayItem.get<int>(0),
                      hour,
                      min,
                      second);
}

}
}

namespace {

    // Conversions are defined for the years std::chrono::year represents,
    // [calendarBegin, calendarEnd), and throw std::out_of_range elsewhere.
    constexpr auto calendarBegin = std::chrono::sys_seconds {
        std::chrono::sys_days { std::chrono::year::min() / std::chrono::January / 1 }
    };

    constexpr auto calendarEnd = std::chrono::sys_seconds {
        std::chrono::sys_days { std::chrono::year::max() / std::chrono::December / 31 }
        + std::chrono::days { 1 }
    };

    std::string outsideCalendar(const std::string& what)
    {
        return fmt::format("{} is outside the representable years {} to {}", what,
                           static_cast<int>(std::chrono::year::min()),
                           static_cast<int>(std::chrono::year::max()));
    }

    // The month carries into the year, and the day and time of day count on
    // from the first of the month, so January 33 is February 2.
    std::chrono::sys_seconds toSysSeconds(const Opm::TimeStampUTC& ts)
    {
        namespace ch = std::chrono;

        // 64-bit arithmetic so that absurd fields end in the range check
        // rather than in overflow.
        const auto month = static_cast<long long>(ts.month()) - 1;
        const auto carry = (month < 0) ? (month - 11) / 12 : month / 12;
        const auto year  = ts.year() + carry;

        if ((year >= static_cast<int>(ch::year::min())) &&
            (year <= static_cast<int>(ch::year::max())))
        {
            const auto firstOfMonth = ch::sys_days {
                ch::year  { static_cast<int>(year) } /
                ch::month { static_cast<unsigned>(month - 12*carry + 1) } / 1
            };

            const auto t = ch::sys_seconds { firstOfMonth } + ch::seconds {
                ((static_cast<long long>(ts.day()) - 1) * 24 + ts.hour()) * 3600
                + static_cast<long long>(ts.minutes()) * 60 + ts.seconds()
            };

            if ((t >= calendarBegin) && (t < calendarEnd)) {
                return t;
            }
        }

        throw std::out_of_range {
            outsideCalendar(fmt::format("Date {}-{:02}-{:02} {:02}:{:02}:{:02}",
                                        ts.year(), ts.month(), ts.day(),
                                        ts.hour(), ts.minutes(), ts.seconds()))
        };
    }

    Opm::TimeStampUTC toTimeStampUTC(const std::chrono::sys_seconds t)
    {
        namespace ch = std::chrono;

        // Check before flooring to days, whose count may be 32 bits.
        if ((t < calendarBegin) || (t >= calendarEnd)) {
            throw std::out_of_range {
                outsideCalendar(fmt::format("Time point {}", t.time_since_epoch().count()))
            };
        }

        const auto day = ch::floor<ch::days>(t);
        const auto ymd = ch::year_month_day { day };
        const auto hms = ch::hh_mm_ss { t - day };

        return {
            Opm::TimeStampUTC::YMD {
                static_cast<int>(ymd.year()),
                static_cast<int>(static_cast<unsigned>(ymd.month())),
                static_cast<int>(static_cast<unsigned>(ymd.day()))
            },
            static_cast<int>(hms.hours().count()),
            static_cast<int>(hms.minutes().count()),
            static_cast<int>(hms.seconds().count()),
            0
        };
    }

}

Opm::TimeStampUTC::TimeStampUTC(const std::time_t tp)
    : TimeStampUTC { toTimeStampUTC(std::chrono::sys_seconds { std::chrono::seconds { tp } }) }
{}

Opm::TimeStampUTC::TimeStampUTC(const time_point& tp)
    : TimeStampUTC { toTimeStampUTC(std::chrono::floor<std::chrono::seconds>(tp)) }
{}

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
    return *this = TimeStampUTC { tp };
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


std::tm Opm::asTm(const TimeStampUTC& tp)
{
    auto timePoint = std::tm{};

    timePoint.tm_year = tp.year()  - 1900;
    timePoint.tm_mon  = tp.month() -    1;
    timePoint.tm_mday = tp.day();
    timePoint.tm_hour = tp.hour();
    timePoint.tm_min  = tp.minutes();
    timePoint.tm_sec  = tp.seconds();

    return timePoint;
}

std::time_t Opm::asTimeT(const TimeStampUTC& tp)
{
    return toSysSeconds(tp).time_since_epoch().count();
}

std::time_t Opm::asLocalTimeT(const TimeStampUTC& tp)
{
    auto tm = asTm(tp);
    return std::mktime(&tm);
}

Opm::TimeStampUTC Opm::operator+(const Opm::TimeStampUTC& lhs, std::chrono::duration<double> delta) {
    return Opm::TimeStampUTC( Opm::TimeService::advance(Opm::asTimeT(lhs) , delta.count()) );
}

Opm::time_point Opm::asTimePoint(const TimeStampUTC& ts)
{
    return toSysSeconds(ts);
}
