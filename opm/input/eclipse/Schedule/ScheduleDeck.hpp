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

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Schedule/ScheduleBlock.hpp>

#include <cstddef>
#include <iosfwd>
#include <vector>


namespace Opm {

    struct ScheduleRestartInfo;

    class Deck;
    class DeckOutput;
    struct ScheduleDeckContext;
    class Runspec;
    class UnitSystem;

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
        explicit ScheduleDeck(time_point start_time, const Deck& deck, const ScheduleRestartInfo& rst_info);
        ScheduleDeck();
        void add_block(ScheduleTimeType time_type, const time_point& t,
                       ScheduleDeckContext& context, const KeywordLocation& location);
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
        static ScheduleDeck serializationTestObject();
        template<class Serializer>
        void serializeOp(Serializer& serializer) {
            serializer(m_restart_time);
            serializer(m_restart_offset);
            serializer(skiprest);
            serializer(m_blocks);
            serializer(m_location);
        }

        void dump_deck(std::ostream& os, const UnitSystem& usys) const;

        void clearKeywords(const std::size_t idx);

    private:
        time_point m_restart_time{};
        std::size_t m_restart_offset{};
        bool skiprest{false};
        KeywordLocation m_location{};
        std::vector<ScheduleBlock> m_blocks{};
    };
}

#endif
