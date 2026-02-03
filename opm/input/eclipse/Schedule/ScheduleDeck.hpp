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

} // namespace Opm

namespace Opm {

    /// All SCHEDULE section keywords in a simulation run.
    ///
    /// Knows how to partition the schedule section into report steps.  In
    /// turn, we form the Schedule object by iterating over the contents of
    /// the ScheduleDeck.  Finally, class ScheduleDeck provides indexed
    /// access through operator[] whose argument is a report step.
    /// Internally the ScheduleDeck class is a vector of ScheduleBlock
    /// instances, one for each report step.
    class ScheduleDeck
    {
    public:
        /// Default constructor.
        ///
        /// Forms an object that's mostly usable as the target of a
        /// deserialisation operation.
        ScheduleDeck();

        /// Constructor.
        ///
        /// \param[in] start_time Simulation start time inferred from the
        /// START keyword.
        ///
        /// \param[in] deck Simulation model description.
        ///
        /// \param[in] rst_info Restart step and restart time in restarted
        /// simulation runs, and whether or not the SKIPREST keyword is
        /// active in the run.
        explicit ScheduleDeck(const time_point&          start_time,
                              const Deck&                deck,
                              const ScheduleRestartInfo& rst_info);

        /// Model input associated to a single report step.
        ///
        /// Bounds-checked, mutable version.
        ///
        /// \param[in] index Report step.  Must be in the range 0..size()-1.
        /// This function will throw an exception if the bound is violated.
        ///
        /// \return Mutable collection of input keywords associated to
        /// report step \p index.
        ScheduleBlock& mutableKeywordBlock(const std::size_t index);

        /// Model input associated to a single report step.
        ///
        /// Bounds-checked, immutable version.
        ///
        /// \param[in] index Report step.  Must be in the range 0..size()-1.
        /// This function will throw an exception if the bound is violated.
        ///
        /// \return Input keywords associated to report step \p index.
        const ScheduleBlock& operator[](const std::size_t index) const;

        /// Start of report step sequence.
        ///
        /// Provided mostly to enable using standard algorithms and
        /// range-for for iterating over the ScheduleDeck.
        auto begin() const { return this->m_blocks.begin(); }

        /// One past the end of report step sequence.
        ///
        /// Provided mostly to enable using standard algorithms and
        /// range-for for iterating over the ScheduleDeck.
        auto end() const { return this->m_blocks.end(); }

        /// Number of report steps in SCHEDULE section.
        auto size() const { return this->m_blocks.size(); }

        /// Report step index of restarted simulation's restart step.
        std::size_t restart_offset() const;

        /// Location of simulation run's SCHEDULE section keyword.
        const KeywordLocation& location() const;

        /// Simulated time, in seconds, since start of simulation.
        ///
        /// \param[in] timeStep Report step.
        ///
        /// \return Simulated time in seconds, at start of report step \p
        /// timeStep, since beginning of the simulation.
        double seconds(std::size_t timeStep) const;

        /// Equality predicate.
        ///
        /// \param[in] other Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// other.
        bool operator==(const ScheduleDeck& other) const;

        /// Create a serialisation test object.
        static ScheduleDeck serializationTestObject();

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_restart_time);
            serializer(m_restart_offset);
            serializer(skiprest);
            serializer(m_blocks);
            serializer(m_location);
        }

        /// Write schedule section keywords to output stream.
        ///
        /// Mostly for debugging support and the opmpack utility.
        ///
        /// \param[in,out] os Output stream.
        ///
        /// \param[in] usys Run's unit system for converting quantities in
        /// internal (SI) units to run's input units.
        void dump_deck(std::ostream& os, const UnitSystem& usys) const;

        /// Discard input keywords for a single report step.
        ///
        /// This provides memory savings for runs that don't need dynamic
        /// ACTION* processing.
        ///
        /// \param[in] idx Report step for which to discard input keywords
        /// that have already been internalised into a \c Schedule object.
        void clearKeywords(const std::size_t idx);

    private:
        /// Simulation restart time (for restarted runs).
        time_point m_restart_time{};

        /// Simulation restart step (for restarted runs).
        std::size_t m_restart_offset{};

        /// Whether or not SKIPREST is active in a restarted run.
        bool skiprest{false};

        /// Location of run's SCHEDULE section keyword.
        KeywordLocation m_location{};

        /// Input keyword blocks.
        ///
        /// One block for each report step.
        std::vector<ScheduleBlock> m_blocks{};

        /// Process DATES keyword.
        ///
        /// Creates one entry in m_blocks for each record in DATES.
        ///
        /// \param[in] keyword DATES keyword.
        ///
        /// \param[in] restart_time Simulation restart time.  Needed for
        /// diagnostic purposes.
        ///
        /// \param[in,out] context Tracking of current simulated time.
        void handleDATES(const DeckKeyword&   keyword,
                         const std::time_t    restart_time,
                         ScheduleDeckContext& context);

        /// Create a ScheduleBlock for a new report step.
        ///
        /// Increases size of \c m_blocks by one element.
        ///
        /// \param[in] time_type Kind of time stepping keyword.
        ///
        /// \param[in] t Time point at which this time stepping was entered.
        ///
        /// \param[in] location Location of this time stepping keyword.
        ///
        /// \param[in,out] context Tracking of current simulated time.
        void add_block(ScheduleTimeType       time_type,
                       const time_point&      t,
                       const KeywordLocation& location,
                       ScheduleDeckContext&   context);

        /// Process TSTEP keyword.
        ///
        /// Creates one entry in m_blocks for each element in TSTEP.
        ///
        /// \param[in] TSTEPKeyword TSTEP keyword.
        ///
        /// \param[in,out] context Tracking of current simulated time.
        void add_TSTEP(const DeckKeyword&   TSTEPKeyword,
                       ScheduleDeckContext& context);
    };
}

#endif
