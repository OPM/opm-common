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

#include <opm/input/eclipse/Schedule/ScheduleBlock.hpp>

#include <opm/input/eclipse/Deck/DeckOutput.hpp>

#include <fmt/format.h>

namespace Opm {

ScheduleBlock::ScheduleBlock(const KeywordLocation& location,
                             ScheduleTimeType time_type,
                             const time_point& start_time)
    : m_time_type(time_type)
    , m_start_time(start_time)
    , m_location(location)
{
}

std::size_t ScheduleBlock::size() const
{
    return this->m_keywords.size();
}

void ScheduleBlock::push_back(const DeckKeyword& keyword)
{
    this->m_keywords.push_back(keyword);
}

std::vector<DeckKeyword>::const_iterator ScheduleBlock::begin() const
{
    return this->m_keywords.begin();
}

std::vector<DeckKeyword>::const_iterator ScheduleBlock::end() const
{
    return this->m_keywords.end();
}

const DeckKeyword& ScheduleBlock::operator[](const std::size_t index) const
{
    return this->m_keywords.at(index);
}

const time_point& ScheduleBlock::start_time() const
{
    return this->m_start_time;
}

const std::optional<time_point>& ScheduleBlock::end_time() const
{
    return this->m_end_time;
}

ScheduleTimeType ScheduleBlock::time_type() const
{
    return this->m_time_type;
}

void ScheduleBlock::end_time(const time_point& t)
{
    this->m_end_time = t;
}

bool ScheduleBlock::operator==(const ScheduleBlock& other) const
{
    return this->m_start_time == other.m_start_time &&
           this->m_end_time == other.m_end_time &&
           this->m_location == other.m_location &&
           this->m_time_type == other.m_time_type &&
           this->m_keywords == other.m_keywords;
}

void ScheduleBlock::dump_time(time_point current_time, DeckOutput& output) const
{
    if (this->m_time_type == ScheduleTimeType::START)
        return;

    if (this->m_time_type == ScheduleTimeType::DATES) {
        TimeStampUTC ts(TimeService::to_time_t(this->start_time()));
        auto ecl_month = TimeService::eclipseMonthNames().at(ts.month());
        std::string dates_string = fmt::format(R"(
DATES
   {} '{}' {} /
/
)", ts.day(), ecl_month, ts.year());
        output.write_string(dates_string);
    } else {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(this->start_time() - current_time);
        double days = seconds.count() / 86400.0;
        std::string tstep_string = fmt::format(R"(
TSTEP
   {} /
)", days);
        output.write_string(tstep_string);
    }
}


void ScheduleBlock::dump_deck(DeckOutput& output, time_point& current_time) const
{
    this->dump_time(current_time, output);
    if (!this->end_time().has_value())
        return;

    for (const auto& keyword : this->m_keywords)
        keyword.write(output);

    current_time = this->end_time().value();
}


const KeywordLocation& ScheduleBlock::location() const
{
    return this->m_location;
}


ScheduleBlock ScheduleBlock::serializationTestObject()
{
    ScheduleBlock block;
    block.m_time_type = ScheduleTimeType::TSTEP;
    block.m_start_time = TimeService::from_time_t( asTimeT( TimeStampUTC( 2003, 10, 10 )));
    block.m_end_time = TimeService::from_time_t( asTimeT( TimeStampUTC( 1993, 07, 06 )));
    block.m_location = KeywordLocation::serializationTestObject();
    block.m_keywords = {DeckKeyword::serializationTestObject()};
    return block;
}

std::optional<DeckKeyword> ScheduleBlock::get(const std::string& kw) const
{
    for (const auto& keyword : this->m_keywords) {
        if (keyword.name() == kw)
            return keyword;
    }
    return {};
}

}
