/*
  Copyright 2021 Equinor ASA.

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
#ifndef SCHEDULE_BLOCK_HPP
#define SCHEDULE_BLOCK_HPP

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace Opm {
    class UnitSystem;
}

namespace Opm {

class DeckOutput;

enum class ScheduleTimeType {
    START = 0,
    DATES = 1,
    TSTEP = 2,
    RESTART = 3,
};

/*
  The ScheduleBlock is collection of all the Schedule keywords from one
  report step.
*/

class ScheduleBlock
{
public:
    ScheduleBlock() = default;
    ScheduleBlock(const KeywordLocation& location,
                  ScheduleTimeType time_type,
                  const time_point& start_time);
    std::size_t size() const;
    void push_back(const DeckKeyword& keyword);
    std::optional<DeckKeyword> get(const std::string& kw) const;
    const time_point& start_time() const;
    const std::optional<time_point>& end_time() const;
    void end_time(const time_point& t);
    ScheduleTimeType time_type() const;
    const KeywordLocation& location() const;
    const DeckKeyword& operator[](const std::size_t index) const;
    std::vector<DeckKeyword>::const_iterator begin() const;
    std::vector<DeckKeyword>::const_iterator end() const;

    bool operator==(const ScheduleBlock& other) const;
    static ScheduleBlock serializationTestObject();

    void clearKeywords();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_time_type);
        serializer(m_start_time);
        serializer(m_end_time);
        serializer(m_keywords);
        serializer(m_location);
    }

    void dump_deck(const UnitSystem& usys,
                   DeckOutput&       output,
                   time_point&       current_time) const;

private:
    ScheduleTimeType m_time_type{ScheduleTimeType::START};
    time_point m_start_time{};
    std::optional<time_point> m_end_time{};
    KeywordLocation m_location{};
    std::vector<DeckKeyword> m_keywords{};

    void dump_time(const UnitSystem& usys,
                   time_point        current_time,
                   DeckOutput&       output) const;

    void writeDates(DeckOutput& output) const;

    void writeTStep(const UnitSystem& usys,
                    time_point        current_time,
                    DeckOutput&       output) const;
};

} // end namespace Opm

#endif // SCHEDULE_BLOCK_HPP
