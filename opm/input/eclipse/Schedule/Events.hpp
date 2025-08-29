/*
  Copyright 2015 Statoil ASA.

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
#ifndef SCHEDULE_EVENTS_HPP
#define SCHEDULE_EVENTS_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Opm
{
    namespace ScheduleEvents {
        // These values are used as bitmask - 2^n structure is essential.
        enum Events : std::uint64_t {
            /// The NEW_WELL event is triggered by the WELSPECS keyword.
            /// For wells the event is triggered the first time the well is
            /// mentioned in the WELSPECS keyword, for the Schedule object
            /// the NEW_WELL event is triggered every time a WELSPECS
            /// keyword is encountered.
            NEW_WELL = (UINT64_C(1) << 0),

            /// When the well data is updated with the WELSPECS keyword this
            /// event is triggered. Only applies to individual wells, and
            /// not the global Schedule object.
            WELL_WELSPECS_UPDATE = (UINT64_C(1) << 1),

            // WELL_POLYMER_UPDATE = (UINT64_C(1) << 2),

            /// The NEW_GROUP event is triggered by the WELSPECS and
            /// GRUPTREE keywords.
            NEW_GROUP = (UINT64_C(1) << 3),

            /// The PRODUCTION_UPDATE event is triggered by the WCONPROD,
            /// WCONHIST, WELTARG, WEFAC keywords.  The event will be
            /// triggered if *any* of the elements in one of keywords is
            /// changed. Quite simlar for INJECTION_UPDATE and
            /// POLYMER_UPDATE.
            PRODUCTION_UPDATE = (UINT64_C(1) << 4),
            INJECTION_UPDATE = (UINT64_C(1) << 5),
            //POLYMER_UPDATES = (UINT64_C(1) << 6),

            /// This event is triggered if the well status is changed
            /// between {OPEN,SHUT,STOP,AUTO}.  There are many keywords
            /// which can trigger a well status change.
            WELL_STATUS_CHANGE = (UINT64_C(1) << 7),

            /// COMPDAT and WELOPEN
            COMPLETION_CHANGE = (UINT64_C(1) << 8),

            /// The well group topology has changed.
            GROUP_CHANGE = (UINT64_C(1) << 9),

            /// Geology modifier.
            GEO_MODIFIER = (UINT64_C(1) << 10),

            /// TUNING has changed
            TUNING_CHANGE = (UINT64_C(1) << 11),

            /// The VFP tables have changed
            VFPINJ_UPDATE = (UINT64_C(1) << 12),
            VFPPROD_UPDATE = (UINT64_C(1) << 13),

            /// GROUP production or injection targets has changed
            GROUP_PRODUCTION_UPDATE = (UINT64_C(1) << 14),
            GROUP_INJECTION_UPDATE = (UINT64_C(1) << 15),

            /// New explicit well productivity/injectivity assignment.
            WELL_PRODUCTIVITY_INDEX = (UINT64_C(1) << 16),

            /// Well/group efficiency factor has changed
            WELLGROUP_EFFICIENCY_UPDATE = (UINT64_C(1) << 17),

            /// Injection type changed
            INJECTION_TYPE_CHANGED = (UINT64_C(1) << 18),

            /// Well switched between injector and producer
            WELL_SWITCHED_INJECTOR_PRODUCER = (UINT64_C(1) << 19),

            /// The well has been affected in an ACTIONX keyword.
            ACTIONX_WELL_EVENT = (UINT64_C(1) << 20),

            /// Some SCHEDULE keywords can set a well to be OPEN to open a
            /// previously STOPped or SHUT well.  The well is SHUT/STOP due
            /// to various causes (SCHEDULE, economical, physical, etc.).
            /// For now, the WELOPEN, WCONPROD and WCONINJE keywords are
            /// considered with this event.
            REQUEST_OPEN_WELL = (UINT64_C(1) << 21),

            /// Analogue to above but when SCHEDULE set it to SHUT
            REQUEST_SHUT_WELL = (UINT64_C(1) << 22),
        };
    } // namespace ScheduleEvents

    /// Events tied to a time and applicable to the simulation or an
    /// individual well or group.
    ///
    /// The event time typically coincides with the start of a report step,
    /// although could be different if the event is triggered in an ACTION
    /// block.
    ///
    /// This class implements a simple system for recording when various
    /// events happen in the Schedule file.  The purpose of the class is
    /// that downstream code can query this system whether a certain a event
    /// has taken place, and then perform potentially expensive calculations
    /// conditionally:
    ///
    ///   auto events = schedule->getEvents();
    ///   if (events.hasEvent(SchedulEvents::NEW_WELL, reportStep))
    ///      // Perform expensive calculation which must be performed
    ///      // when a new well is introduced.
    ///      ...
    class Events
    {
    public:
        /// Create a serialisation test object.
        static Events serializationTestObject();

        /// Incorporate a new event into collection.
        ///
        /// \param[in] event Single event, such as a new well being introduced.
        void addEvent(ScheduleEvents::Events event);

        /// Remove one or more events from collection.
        ///
        /// \param[in] eventMask Bit mask of events to clear from current
        /// collection.
        void clearEvent(std::uint64_t eventMask);

        /// Remove all events from collection.
        void reset();

        /// Merge current event collection with other.
        ///
        /// Resulting collection (\c *this) has the union of the events in
        /// both collections.
        void merge(const Events& events);

        /// Event existence predicate.
        ///
        /// \param[in] eventMask Bit mask of events for which to check
        /// existence.
        ///
        /// \return Whether not at least one of the events represented in \p
        /// eventMask is active in the current collection.
        bool hasEvent(std::uint64_t eventMask) const;

        /// Equality predicate.
        ///
        /// \param[in] data Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// data.
        bool operator==(const Events& data) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_events);
        }

    private:
        /// Event collection.
        std::uint64_t m_events = 0;
    };

    /// Collection of events tied to a time and associated to specific,
    /// named wells or groups.
    class WellGroupEvents
    {
    public:
        /// Create a serialisation test object.
        static WellGroupEvents serializationTestObject();

        /// Include a named well into the events collection.
        ///
        /// Typically used in the handler for the 'WELSPECS' keyword.
        void addWell(const std::string& wname);

        /// Include a named group in the events collection.
        ///
        /// Typically used in the handlers for the 'WELSPECS' and GRUPTREE
        /// keywords
        void addGroup(const std::string& gname);

        /// Add a single event for a named well or group.
        ///
        /// \param[in] wgname Well or group name.
        ///
        /// \param[in] event Single named event.  If \p event already exists
        /// for \p wgname, then this function does nothing.
        void addEvent(const std::string& wgname, ScheduleEvents::Events event);

        /// Remove one or more individual events from the collection tied to
        /// a single named well or group.
        ///
        /// \param[in] wgname Well or group name.
        ///
        /// \param[in] eventMask One or more events combined using bitwise
        /// 'or' ('|').  All events for \p wgname that are set in \p
        /// eventMask will be cleared.
        void clearEvent(const std::string& wgname, std::uint64_t eventMask);

        /// Remove all events for all known wells and groups
        ///
        /// Typically used only when preparing the events system for a new
        /// report step as part of Schedule object initialisation.
        void reset();

        /// Merge current event collection with other.
        ///
        /// Resulting collection (\c *this) has the union of the events for
        /// all wells and groups in both collections.
        void merge(const WellGroupEvents& events);

        /// Check if any events have ever been registered for a named well
        /// or group.
        ///
        /// \param[in] wgname Well or group name.
        ///
        /// \return Whether or not the well or group \wgname has been
        /// registered in this collection.
        bool has(const std::string& wgname) const;

        /// Query current collection for one or more specific events
        /// associated to a specific well or group.
        ///
        /// \param[in] wgname Well or group name.
        ///
        /// \param[in] eventMask Bit mask of events for which to check
        /// existence.
        ///
        /// \return Whether not at least one of the events represented in \p
        /// eventMask is active in the current collection.
        bool hasEvent(const std::string& wgname, std::uint64_t eventMask) const;

        /// Look up collection of events for named well or group.
        ///
        /// Throws an exception of type \code std::invalid_argument \endcode
        /// if the named well or group has not been previously registered
        /// through addWell() or addGroup().
        ///
        /// \param[in] wgname Well or group name.
        ///
        /// \return Event collection for \p wgname.
        const Events& at(const std::string& wgname) const;

        /// Equality predicate.
        ///
        /// \param[in] data Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// data.
        bool operator==(const WellGroupEvents& data) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_wellgroup_events);
        }

    private:
        /// Event collection for all registered wells and groups.
        std::unordered_map<std::string, Events> m_wellgroup_events{};
    };

} // namespace Opm

#endif // SCHEDULE_EVENTS_HPP
