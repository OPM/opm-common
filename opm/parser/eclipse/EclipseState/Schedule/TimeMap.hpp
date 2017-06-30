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


#ifndef TIMEMAP_HPP_
#define TIMEMAP_HPP_

#include <vector>
#include <ctime>
#include <map>

namespace Opm {

    class Deck;
    class DeckKeyword;
    class DeckRecord;

    class TimeMap {
    public:
        explicit TimeMap(std::time_t startTime);
        explicit TimeMap( const Deck& deck);

        void addTime(std::time_t newTime);
        void addTStep(int64_t step);
        void addFromDATESKeyword( const DeckKeyword& DATESKeyword );
        void addFromTSTEPKeyword( const DeckKeyword& TSTEPKeyword );
        size_t size() const;
        size_t last() const;
        size_t numTimesteps() const;
        double getTotalTime() const;

        std::time_t operator[] (size_t index) const;
        /// Return the date and time where a given time step starts.
        std::time_t getStartTime(size_t tStepIdx) const;
        std::time_t getEndTime() const;
        /// Return the period of time in seconds which passed between the start of the simulation and a given point in time.
        double getTimePassedUntil(size_t tLevelIdx) const;
        /// Return the length of a given time step in seconds.
        double getTimeStepLength(size_t tStepIdx) const;

        /// Return true if the given timestep is the first one of a new month or year, or if frequency > 1,
        /// return true for every n'th timestep of every first new month or first new year timesteps,
        /// starting from start_timestep-1.
        bool isTimestepInFirstOfMonthsYearsSequence(size_t timestep, bool years = true, size_t start_timestep = 1, size_t frequency = 1) const;

        static std::time_t timeFromEclipse(const DeckRecord &dateRecord);

        static std::time_t forward(std::time_t t0, int64_t hours, int64_t minutes, int64_t seconds);
        static std::time_t forward(std::time_t t0, int64_t seconds);
        static std::time_t mkdate(int year, int month, int day);
        static std::time_t mkdatetime(int year, int month, int day, int hour, int minute, int second);
    private:
        static const std::map<std::string, int>& eclipseMonthIndices();

        std::vector<std::time_t> m_timeList;

        const std::vector<size_t>& getFirstTimestepMonths() const;
        const std::vector<size_t>& getFirstTimestepYears() const;
        bool isTimestepInFreqSequence (size_t timestep, size_t start_timestep, size_t frequency, bool years) const;
        size_t closest(const std::vector<size_t> & vec, size_t value) const;

        std::vector<size_t> m_first_timestep_years;   // A list of the first timestep of every year
        std::vector<size_t> m_first_timestep_months;  // A list of the first timestep of every month
    };
}



#endif /* TIMEMAP_HPP_ */
