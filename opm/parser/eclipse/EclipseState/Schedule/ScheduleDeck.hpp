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
#ifndef SCHEDULE_DECK_HPP
#define SCHEDULE_DECK_HPP

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>


namespace Opm {

    enum class ScheduleTimeType {
        START = 0,
        DATES = 1,
        TSTEP = 2,
        RESTART = 3
    };

    class Deck;
    struct ScheduleDeckContext;


    /*
      The ScheduleBlock is collection of all the Schedule keywords from one
      report step.
    */

    class ScheduleBlock {
    public:
        ScheduleBlock() = default;
        ScheduleBlock(const KeywordLocation& location, ScheduleTimeType time_type, const std::chrono::system_clock::time_point& start_time);
        std::size_t size() const;
        void push_back(const DeckKeyword& keyword);
        std::optional<DeckKeyword> get(const std::string& kw) const;
        const std::chrono::system_clock::time_point& start_time() const;
        const std::optional<std::chrono::system_clock::time_point>& end_time() const;
        void end_time(const std::chrono::system_clock::time_point& t);
        ScheduleTimeType time_type() const;
        const KeywordLocation& location() const;
        const DeckKeyword& operator[](const std::size_t index) const;
        std::vector<DeckKeyword>::const_iterator begin() const;
        std::vector<DeckKeyword>::const_iterator end() const;

        bool operator==(const ScheduleBlock& other) const;
        static ScheduleBlock serializeObject();
        template<class Serializer>
        void serializeOp(Serializer& serializer) {
            serializer(m_time_type);
            serializer(m_start_time);
            serializer(m_end_time);
            serializer.vector(m_keywords);
            m_location.serializeOp(serializer);
        }
    private:
        ScheduleTimeType m_time_type;
        std::chrono::system_clock::time_point m_start_time;
        std::optional<std::chrono::system_clock::time_point> m_end_time;
        KeywordLocation m_location;
        std::vector<DeckKeyword> m_keywords;
    };



    /*
      The purpose of the ScheduleDeck class is to serve as a container holding
      all the keywords of the SCHEDULE section, when the Schedule class is
      assembled that is done by iterating over the contents of the ScheduleDeck.
      The ScheduleDeck class can be indexed with report step through operator[].
      Internally the ScheduleDeck class is a vector of ScheduleBlock instances -
      one for each report step.
    */

    class ScheduleDeck {
    public:
        explicit ScheduleDeck(const Deck& deck, const std::pair<std::time_t, std::size_t>& restart);
        ScheduleDeck();
        void add_block(ScheduleTimeType time_type, const std::chrono::system_clock::time_point& t, ScheduleDeckContext& context, const KeywordLocation& location);
        void add_TSTEP(const DeckKeyword& TSTEPKeyword, ScheduleDeckContext& context);
        ScheduleBlock& operator[](const std::size_t index);
        const ScheduleBlock& operator[](const std::size_t index) const;
        std::vector<ScheduleBlock>::const_iterator begin() const;
        std::vector<ScheduleBlock>::const_iterator end() const;
        std::size_t size() const;
        std::size_t restart_offset() const;
        const KeywordLocation& location() const;
        double seconds(std::size_t timeStep) const;

        bool operator==(const ScheduleDeck& other) const;
        static ScheduleDeck serializeObject();
        template<class Serializer>
        void serializeOp(Serializer& serializer) {
            serializer(m_restart_time);
            serializer(m_restart_offset);
            serializer.vector(m_blocks);
            m_location.serializeOp(serializer);
        }

    private:
        std::chrono::system_clock::time_point m_restart_time;
        std::size_t m_restart_offset;
        KeywordLocation m_location;
        std::vector<ScheduleBlock> m_blocks;
    };
}

#endif
