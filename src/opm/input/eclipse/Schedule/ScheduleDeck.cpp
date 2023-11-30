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

#include <opm/input/eclipse/Schedule/ScheduleDeck.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/String.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckOutput.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Schedule/ScheduleRestartInfo.hpp>

#include <chrono>
#include <ctime>
#include <unordered_set>

#include <fmt/format.h>
#include <fmt/chrono.h>

namespace Opm {

/*****************************************************************************/

struct ScheduleDeckContext {
    bool rst_skip;
    time_point last_time;

    ScheduleDeckContext(bool skip, time_point t) :
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


ScheduleDeck::ScheduleDeck(time_point start_time, const Deck& deck, const ScheduleRestartInfo& rst_info) {
    const std::unordered_set<std::string> skiprest_include = {"VFPPROD", "VFPINJ", "RPTSCHED", "RPTRST", "TUNING", "MESSAGES"};

    this->m_restart_time = TimeService::from_time_t(rst_info.time);
    this->m_restart_offset = rst_info.report_step;
    this->skiprest = rst_info.skiprest;
    if (this->m_restart_offset > 0) {
        for (std::size_t it = 0; it < this->m_restart_offset; it++) {
            if (it == 0)
                this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::START, start_time);
            else
                this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::RESTART, start_time);
            this->m_blocks.back().end_time(start_time);
        }
        if (!this->skiprest) {
            this->m_blocks.back().end_time(this->m_restart_time);
            this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::RESTART, this->m_restart_time);
        }
    } else
        this->m_blocks.emplace_back(KeywordLocation{}, ScheduleTimeType::START, start_time);

    ScheduleDeckContext context(this->skiprest, this->m_blocks.back().start_time());
    for( const auto& keyword : SCHEDULESection(deck)) {
        if (keyword.name() == "DATES") {
            for (size_t recordIndex = 0; recordIndex < keyword.size(); recordIndex++) {
                const auto &record = keyword.getRecord(recordIndex);
                std::time_t nextTime;

                try {
                    nextTime = TimeService::timeFromEclipse(record);
                } catch (const std::exception& e) {
                    const OpmInputError opm_error { e, keyword.location() } ;
                    OpmLog::error(opm_error.what());
                    std::throw_with_nested(opm_error);
                }

                const auto currenTime = std::chrono::system_clock::to_time_t(context.last_time);
                if (nextTime < currenTime) {
                    auto msg = fmt::format(
                               "Keyword DATES specifies a time {:%d-%b-%Y %H:%M:%S} earlier than the end time of previous report step {:%d-%b-%Y %H:%M:%S}",
                               fmt::gmtime(nextTime), fmt::gmtime(currenTime));
                    if (rst_info.time > 0 && !this->skiprest) { // the situation with SKIPREST is handled in function add_block
                        msg += "\nin a RESTARTing simulation, Please check whether SKIPREST is supposed to be used for this circumstance";
                    }
                    throw OpmInputError(msg, keyword.location());
                }
                this->add_block(ScheduleTimeType::DATES, TimeService::from_time_t( nextTime ), context, keyword.location());
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

namespace {

    std::string
    format_skiprest_error(const ScheduleTimeType time_type,
                          const time_point&      restart_time,
                          const time_point&      t)
    {
        const auto rst = TimeStampUTC {
            TimeService::to_time_t(restart_time)
        };

        const auto current = TimeStampUTC {
            TimeService::to_time_t(t)
        };

        auto rst_tm = std::tm{};
        rst_tm.tm_year = rst.year()  - 1900;
        rst_tm.tm_mon  = rst.month() -    1;
        rst_tm.tm_mday = rst.day();

        rst_tm.tm_hour = rst.hour();
        rst_tm.tm_min  = rst.minutes();
        rst_tm.tm_sec  = rst.seconds();

        auto current_tm = std::tm{};
        current_tm.tm_year = current.year()  - 1900;
        current_tm.tm_mon  = current.month() -    1;
        current_tm.tm_mday = current.day();

        current_tm.tm_hour = current.hour();
        current_tm.tm_min  = current.minutes();
        current_tm.tm_sec  = current.seconds();

        const auto* keyword = (time_type == ScheduleTimeType::DATES)
            ? "DATES" : "TSTEP";
        const auto* record = (time_type == ScheduleTimeType::DATES)
            ? "record" : "report step";

        return fmt::format("In a restarted simulation using SKIPREST, the {0} keyword must have\n"
                           "a {1} corresponding to the RESTART time {2:%d-%b-%Y %H:%M:%S}.\n"
                           "Reached time {3:%d-%b-%Y %H:%M:%S} without an intervening {1}.",
                           keyword, record, rst_tm, current_tm);
    }
}

void ScheduleDeck::add_block(ScheduleTimeType time_type, const time_point& t, ScheduleDeckContext& context, const KeywordLocation& location) {
    context.last_time = t;
    if (context.rst_skip) {
        if (t < this->m_restart_time)
            return;

        if (t == this->m_restart_time)
            context.rst_skip = false;

        if (t > this->m_restart_time) {
            if (this->skiprest) {
                const auto reason =
                    format_skiprest_error(time_type, this->m_restart_time, t);

                throw OpmInputError(reason, location);
            }
            context.rst_skip = false;
        }
    }
    this->m_blocks.back().end_time(t);
    this->m_blocks.emplace_back( location, time_type, t );
}


void ScheduleDeck::add_TSTEP(const DeckKeyword& TSTEPKeyword, ScheduleDeckContext& context) {
    const auto &item = TSTEPKeyword.getRecord(0).getItem(0);
    for (size_t itemIndex = 0; itemIndex < item.data_size(); itemIndex++) {
        {
            const auto tstep = item.get<double>(itemIndex);
            if (tstep < 0) {
                const auto msg = fmt::format("a negative TSTEP value {} is input", tstep);
                throw OpmInputError(msg, TSTEPKeyword.location());
            }
        }
        auto next_time = context.last_time + std::chrono::duration_cast<time_point::duration>(std::chrono::duration<double>(item.getSIDouble(itemIndex)));
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
    time_point start_time;
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

ScheduleDeck ScheduleDeck::serializationTestObject() {
    ScheduleDeck deck;
    deck.m_restart_time = TimeService::from_time_t( asTimeT( TimeStampUTC( 2013, 12, 12 )));
    deck.m_restart_offset = 123;
    deck.m_location = KeywordLocation::serializationTestObject();
    deck.m_blocks = { ScheduleBlock::serializationTestObject(), ScheduleBlock::serializationTestObject() };
    return deck;
}

void ScheduleDeck::dump_deck(std::ostream& os) const {
    DeckOutput output(os);

    output.write_string("SCHEDULE\n");
    auto current_time = this->m_blocks[0].start_time();
    for (const auto& block : this->m_blocks)
        block.dump_deck(output, current_time);
}


}
