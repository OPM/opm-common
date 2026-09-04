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

#ifndef OPM_TIMESERVICE_HEADER_INCLUDED
#define OPM_TIMESERVICE_HEADER_INCLUDED

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <unordered_map>

namespace Opm {

    class DeckRecord;

    using time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<std::int64_t, std::ratio<1,1000>>>;

    namespace TimeService {
    std::time_t to_time_t(const time_point& tp);
    time_point from_time_t(std::time_t t);
    time_point now();

    std::time_t advance(const std::time_t tp, const double sec);
    std::time_t makeUTCTime(std::tm timePoint);
    const std::unordered_map<std::string , int>& eclipseMonthIndices();
    const std::unordered_map<int, std::string>& eclipseMonthNames();
    int eclipseMonth(const std::string& name);
    bool valid_month(const std::string& month_name);

    std::time_t mkdatetime(int in_year, int in_month, int in_day, int hour, int minute, int second);
    std::time_t mkdate(int in_year, int in_month, int in_day);
    /// Seconds since the epoch for a std::tm read as UTC civil time: POSIX
    /// timegm(), written with the C++20 <chrono> calendar types so it is the
    /// same everywhere (Windows' _mkgmtime() stops at year 3000).
    ///
    /// The month is normalised into the year; a day beyond the month and the
    /// time of day are added as they stand, so 33 January is 2 February.
    ///
    /// Throws std::out_of_range for an instant outside what std::chrono::year
    /// can represent, -32767-01-01T00:00:00Z to 32767-12-31T23:59:59Z. That
    /// is a year past either end, and also an in-range year whose day of the
    /// month or time of day carries the instant past it -- 32767-12-32, or
    /// 24:00 on the last day. What this returns, portable_gmtime() always
    /// takes back.
    std::time_t portable_timegm(const std::tm* t);

    /// Break a time_t into UTC civil time.
    ///
    /// The inverse of portable_timegm(), and a replacement for std::gmtime():
    /// that returns nullptr for time points its C runtime cannot represent --
    /// MSVC's refuses everything before 1970 and after year 3000, both of
    /// which simulation schedules legitimately reach -- and hands back a
    /// pointer to a static buffer that concurrent callers overwrite.
    ///
    /// Throws std::out_of_range for an instant whose calendar year lies
    /// outside std::chrono::year, -32767 to 32767; within it, every field
    /// std::gmtime() fills is filled, tm_yday and tm_wday included.
    std::tm portable_gmtime(std::time_t t);
    std::time_t timeFromEclipse(const DeckRecord &dateRecord);
    }

    class TimeStampUTC
    {
    public:
        struct YMD {
            int year{0};
            int month{0};
            int day{0};

            bool operator==(const YMD& data) const
            {
                return year == data.year &&
                       month == data.month &&
                       day == data.day;
            }

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(year);
                serializer(month);
                serializer(day);
            }
        };

        TimeStampUTC() = default;

        explicit TimeStampUTC(const std::time_t tp);
        explicit TimeStampUTC(const YMD& ymd);
        TimeStampUTC(int year, int month, int day);
        TimeStampUTC(const YMD& ymd,
                     int hour,
                     int minutes,
                     int seconds,
                     int usec);

        TimeStampUTC& operator=(const std::time_t tp);
        bool operator==(const TimeStampUTC& data) const;

        TimeStampUTC& hour(const int h);
        TimeStampUTC& minutes(const int m);
        TimeStampUTC& seconds(const int s);
        TimeStampUTC& microseconds(const int us);

        const YMD& ymd() const { return ymd_; }
        int year()         const { return this->ymd_.year;  }
        int month()        const { return this->ymd_.month; }
        int day()          const { return this->ymd_.day;   }
        int hour()         const { return this->hour_;      }
        int minutes()      const { return this->minutes_;   }
        int seconds()      const { return this->seconds_;   }
        int microseconds() const { return this->usec_;      }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(ymd_);
            serializer(hour_);
            serializer(minutes_);
            serializer(seconds_);
            serializer(usec_);
        }

    private:

        YMD ymd_{};
        int hour_{0};
        int minutes_{0};
        int seconds_{0};
        int usec_{0};
    };

    TimeStampUTC operator+(const TimeStampUTC& lhs, std::chrono::duration<double> delta);
    std::time_t asTimeT(const TimeStampUTC& tp);
    std::time_t asLocalTimeT(const TimeStampUTC& tp);
    time_point asTimePoint(const TimeStampUTC& tp);


} // namespace Opm

#endif // OPM_TIMESERVICE_HEADER_INCLUDED
