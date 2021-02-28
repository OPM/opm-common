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

#include <chrono>
#include <fmt/format.h>
#include <unordered_set>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/parser/eclipse/Deck/DeckSection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleDeck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

namespace Opm {

ScheduleBlock::ScheduleBlock(const KeywordLocation& location, ScheduleTimeType time_type, const std::chrono::system_clock::time_point& start_time) :
    m_time_type(time_type),
    m_start_time(start_time),
    m_location(location)
{
}

std::size_t ScheduleBlock::size() const {
    return this->m_keywords.size();
}

void ScheduleBlock::push_back(const DeckKeyword& keyword) {
    this->m_keywords.push_back(keyword);
}

std::vector<DeckKeyword>::const_iterator ScheduleBlock::begin() const {
    return this->m_keywords.begin();
}

std::vector<DeckKeyword>::const_iterator ScheduleBlock::end() const {
    return this->m_keywords.end();
}

const DeckKeyword& ScheduleBlock::operator[](const std::size_t index) const {
    return this->m_keywords.at(index);
}

const std::chrono::system_clock::time_point& ScheduleBlock::start_time() const {
    return this->m_start_time;
}

const std::optional<std::chrono::system_clock::time_point>& ScheduleBlock::end_time() const {
    return this->m_end_time;
}

ScheduleTimeType ScheduleBlock::time_type() const {
    return this->m_time_type;
}

void ScheduleBlock::end_time(const std::chrono::system_clock::time_point& t) {
    this->m_end_time = t;
}

bool ScheduleBlock::operator==(const ScheduleBlock& other) const {
    return this->m_start_time == other.m_start_time &&
           this->m_end_time == other.m_end_time &&
           this->m_location == other.m_location &&
           this->m_time_type == other.m_time_type &&
           this->m_keywords == other.m_keywords;
}


const KeywordLocation& ScheduleBlock::location() const {
    return this->m_location;
}


ScheduleBlock ScheduleBlock::serializeObject() {
    ScheduleBlock block;
    block.m_start_time = std::chrono::system_clock::from_time_t( asTimeT( TimeStampUTC( 2003, 10, 10 )));
    block.m_end_time = std::chrono::system_clock::from_time_t( asTimeT( TimeStampUTC( 1993, 07, 06 )));
    block.m_location = KeywordLocation{ "Dummy", "File", 123 };
    return block;
}

std::optional<DeckKeyword> ScheduleBlock::get(const std::string& kw) const {
    for (const auto& keyword : this->m_keywords) {
        if (keyword.name() == kw)
            return keyword;
    }
    return {};
}

/*****************************************************************************/

struct ScheduleDeckContext {
    bool rst_skip;
    std::chrono::system_clock::time_point last_time;

    ScheduleDeckContext(bool skip, std::chrono::system_clock::time_point t) :
        rst_skip(skip),
        last_time(t)
    {}
};


const KeywordLocation& ScheduleDeck::location() const {
    return this->m_location;
}


std::size_t ScheduleDeck::restart_offset() const {
    return this->m_restart_offset;
}


ScheduleDeck::ScheduleDeck(const Deck& deck, const std::pair<std::time_t, std::size_t>& restart) {
    const std::unordered_set<std::string> skiprest_include = {"VFPPROD", "VFPINJ", "RPTSCHED", "RPTRST", "TUNING", "MESSAGES"};
    std::chrono::system_clock::time_point start_time;
    if (deck.hasKeyword("START")) {
        // Use the 'START' keyword to find out the start date (if the
        // keyword was specified)
        const auto& keyword = deck.getKeyword("START");
        start_time = std::chrono::system_clock::from_time_t( TimeMap::timeFromEclipse(keyword.getRecord(0)) );
    } else {
        // The default start date is not specified in the Eclipse
        // reference manual. We hence just assume it is same as for
        // the START keyword for Eclipse E100, i.e., January 1st,
        // 1983...
        start_time = std::chrono::system_clock::from_time_t( TimeMap::mkdate(1983, 1, 1) );
    }

    const auto& [restart_time, restart_offset] = restart;
    this->m_restart_time = std::chrono::system_clock::from_time_t(restart_time);
    this->m_restart_offset = restart_offset;
    if (restart_offset > 0) {
        for (std::size_t it = 0; it < this->m_restart_offset; it++) {
            this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::RESTART, start_time);
            if (it < this->m_restart_offset - 1)
                this->m_blocks.back().end_time(start_time);
        }
    } else
        this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::START, start_time);


    ScheduleDeckContext context(this->m_restart_offset > 0, start_time);
    for( const auto& keyword : SCHEDULESection(deck)) {
        if (keyword.name() == "DATES") {
            for (size_t recordIndex = 0; recordIndex < keyword.size(); recordIndex++) {
                const auto &record = keyword.getRecord(recordIndex);
                std::time_t nextTime;

                try {
                    nextTime = TimeMap::timeFromEclipse(record);
                } catch (const std::exception& e) {
                    const OpmInputError opm_error { e, keyword.location() } ;
                    OpmLog::error(opm_error.what());
                    std::throw_with_nested(opm_error);
                }

                this->add_block(ScheduleTimeType::DATES, std::chrono::system_clock::from_time_t( nextTime ), context, keyword.location());
            }
            continue;
        }
        if (keyword.name() == "TSTEP") {
            this->add_TSTEP(keyword, context);
            continue;
        }

        if (keyword.name() == "SCHEDULE") {
            this->m_location = keyword.location();
            continue;
        }

        if (context.rst_skip) {
            if (skiprest_include.count(keyword.name()) != 0)
                this->m_blocks[0].push_back(keyword);
        } else
            this->m_blocks.back().push_back(keyword);
    }
}


void ScheduleDeck::add_block(ScheduleTimeType time_type, const std::chrono::system_clock::time_point& t, ScheduleDeckContext& context, const KeywordLocation& location) {
    context.last_time = t;
    if (context.rst_skip) {
        if (t < this->m_restart_time)
            return;

        if (t == this->m_restart_time)
            context.rst_skip = false;

        if (t > this->m_restart_time) {
            TimeStampUTC ts(std::chrono::system_clock::to_time_t(this->m_restart_time));
            auto reason = fmt::format("Have scanned past restart data: {:4d}-{:02d}-{:02d}", ts.year(), ts.month(), ts.day());
            throw OpmInputError(reason, location);
        }
    }
    this->m_blocks.back().end_time(t);
    this->m_blocks.emplace_back( location, time_type, t );
}


void ScheduleDeck::add_TSTEP(const DeckKeyword& TSTEPKeyword, ScheduleDeckContext& context) {
    const auto &item = TSTEPKeyword.getRecord(0).getItem(0);
    for (size_t itemIndex = 0; itemIndex < item.data_size(); itemIndex++) {
        const int64_t seconds = static_cast<int64_t>(item.getSIDouble(itemIndex));
        auto next_time = std::chrono::system_clock::from_time_t( TimeMap::forward(std::chrono::system_clock::to_time_t(context.last_time), seconds) );
        this->add_block(ScheduleTimeType::TSTEP, next_time, context, TSTEPKeyword.location());
    }
}


double ScheduleDeck::seconds(std::size_t timeStep) const {
    if (this->m_blocks.empty())
        return 0;

    if (timeStep >= this->m_blocks.size())
        throw std::logic_error(fmt::format("seconds({}) - invalid timeStep. Valid range [0,{}>", timeStep, this->m_blocks.size()));

    std::chrono::duration<double> elapsed = this->m_blocks[timeStep].start_time() - this->m_blocks[0].start_time();
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
}


ScheduleDeck::ScheduleDeck() {
    std::chrono::system_clock::time_point start_time;
    this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::START, start_time);
}


ScheduleBlock& ScheduleDeck::operator[](const std::size_t index) {
    return this->m_blocks.at(index);
}

const ScheduleBlock& ScheduleDeck::operator[](const std::size_t index) const {
    return this->m_blocks.at(index);
}

std::size_t ScheduleDeck::size() const {
    return this->m_blocks.size();
}

std::vector<ScheduleBlock>::const_iterator ScheduleDeck::begin() const {
    return this->m_blocks.begin();
}

std::vector<ScheduleBlock>::const_iterator ScheduleDeck::end() const {
    return this->m_blocks.end();
}


bool ScheduleDeck::operator==(const ScheduleDeck& other) const {
    return this->m_restart_time == other.m_restart_time &&
           this->m_restart_offset == other.m_restart_offset &&
           this->m_blocks == other.m_blocks;
}

ScheduleDeck ScheduleDeck::serializeObject() {
    ScheduleDeck deck;
    deck.m_restart_time = std::chrono::system_clock::from_time_t( asTimeT( TimeStampUTC( 2013, 12, 12 )));
    deck.m_restart_offset = 123;
    deck.m_location = KeywordLocation{ "Deck", "DeckFile", 321 };
    deck.m_blocks = { ScheduleBlock::serializeObject(), ScheduleBlock::serializeObject() };
    return deck;
}

}
