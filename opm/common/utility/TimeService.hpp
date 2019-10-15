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

#include <ctime>

namespace Opm {

    class TimeStampUTC
    {
    public:
        struct YMD {
            int year{0};
            int month{0};
            int day{0};
        };

        TimeStampUTC() = default;

        explicit TimeStampUTC(const std::time_t tp);
        explicit TimeStampUTC(const YMD& ymd);

        TimeStampUTC& operator=(const std::time_t tp);

        TimeStampUTC& hour(const int h);
        TimeStampUTC& minutes(const int m);
        TimeStampUTC& seconds(const int s);
        TimeStampUTC& microseconds(const int us);

        int year()         const { return this->ymd_.year;  }
        int month()        const { return this->ymd_.month; }
        int day()          const { return this->ymd_.day;   }
        int hour()         const { return this->hour_;      }
        int minutes()      const { return this->minutes_;   }
        int seconds()      const { return this->seconds_;   }
        int microseconds() const { return this->usec_;      }

    private:
        YMD ymd_{};
        int hour_{0};
        int minutes_{0};
        int seconds_{0};
        int usec_{0};
    };

    std::time_t asTimeT(const TimeStampUTC& tp);

} // namespace Opm

#endif // OPM_TIMESERVICE_HEADER_INCLUDED
