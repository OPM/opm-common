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

#define BOOST_TEST_MODULE EventTests
#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Events.hpp>

#include <stdexcept>

BOOST_AUTO_TEST_CASE(CreateEmpty)
{
    Opm::Events events;

    BOOST_CHECK_EQUAL( false , events.hasEvent(Opm::ScheduleEvents::NEW_WELL));

    events.addEvent( Opm::ScheduleEvents::NEW_WELL);
    BOOST_CHECK_EQUAL( true  , events.hasEvent(Opm::ScheduleEvents::NEW_WELL));

    events.addEvent( Opm::ScheduleEvents::WELL_STATUS_CHANGE);
    BOOST_CHECK_EQUAL( true  , events.hasEvent(Opm::ScheduleEvents::NEW_WELL));
    BOOST_CHECK_EQUAL( true  , events.hasEvent(Opm::ScheduleEvents::WELL_STATUS_CHANGE));
    BOOST_CHECK_EQUAL( true  , events.hasEvent(Opm::ScheduleEvents::WELL_STATUS_CHANGE | Opm::ScheduleEvents::NEW_WELL));


    events.clearEvent(Opm::ScheduleEvents::NEW_WELL);
    BOOST_CHECK_EQUAL( false , events.hasEvent(Opm::ScheduleEvents::NEW_WELL));

    events.addEvent( Opm::ScheduleEvents::NEW_WELL);
    BOOST_CHECK_EQUAL( true , events.hasEvent(Opm::ScheduleEvents::NEW_WELL));

    events.clearEvent(Opm::ScheduleEvents::NEW_WELL | Opm::ScheduleEvents::WELL_STATUS_CHANGE | Opm::ScheduleEvents::NEW_GROUP);
    BOOST_CHECK_EQUAL( false , events.hasEvent(Opm::ScheduleEvents::NEW_WELL));
    BOOST_CHECK_EQUAL( false , events.hasEvent(Opm::ScheduleEvents::WELL_STATUS_CHANGE));



    Opm::WellGroupEvents wg_events;
    wg_events.addWell("W1");
    wg_events.addEvent("W1", Opm::ScheduleEvents::WELL_STATUS_CHANGE );

    const auto& ev = wg_events.at("W1");
    BOOST_CHECK_EQUAL( false , ev.hasEvent(Opm::ScheduleEvents::NEW_GROUP));
    BOOST_CHECK_EQUAL( true , ev.hasEvent(Opm::ScheduleEvents::WELL_STATUS_CHANGE));

    BOOST_CHECK_THROW(wg_events.at("NO_SUCH_WELL"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(MergeEvents)
{
    using SchEvt = Opm::ScheduleEvents::Events;

    auto ev1 = Opm::Events{};
    ev1.addEvent(SchEvt::NEW_WELL);
    ev1.addEvent(SchEvt::GROUP_CHANGE);
    ev1.addEvent(SchEvt::COMPLETION_CHANGE);

    {
        auto ev2 = Opm::Events{};
        ev2.addEvent(SchEvt::NEW_GROUP);
        ev2.addEvent(SchEvt::GROUP_PRODUCTION_UPDATE);

        ev1.merge(ev2);
    }

    // -----------------------------------------------------------------------

    BOOST_CHECK_MESSAGE(ev1.hasEvent(SchEvt::NEW_WELL),
                        R"(Merged collection must have NEW_WELL)");
    BOOST_CHECK_MESSAGE(ev1.hasEvent(SchEvt::GROUP_CHANGE),
                        R"(Merged collection must have GROUP_CHANGE)");
    BOOST_CHECK_MESSAGE(ev1.hasEvent(SchEvt::COMPLETION_CHANGE),
                        R"(Merged collection must have COMPLETION_CHANGE)");

    // -----------------------------------------------------------------------

    BOOST_CHECK_MESSAGE(ev1.hasEvent(SchEvt::NEW_GROUP),
                        R"(Merged collection must have NEW_GROUP)");
    BOOST_CHECK_MESSAGE(ev1.hasEvent(SchEvt::GROUP_PRODUCTION_UPDATE),
                        R"(Merged collection must have GROUP_PRODUCTION_UPDATE)");

    // -----------------------------------------------------------------------

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::WELL_WELSPECS_UPDATE),
                        R"(Merged collection must NOT have WELL_WELSPECS_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::PRODUCTION_UPDATE),
                        R"(Merged collection must NOT have PRODUCTION_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::INJECTION_UPDATE),
                        R"(Merged collection must NOT have INJECTION_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::WELL_STATUS_CHANGE),
                        R"(Merged collection must NOT have WELL_STATUS_CHANGE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::GEO_MODIFIER),
                        R"(Merged collection must NOT have GEO_MODIFIER)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::TUNING_CHANGE),
                        R"(Merged collection must NOT have TUNING_CHANGE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::VFPINJ_UPDATE),
                        R"(Merged collection must NOT have VFPINJ_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::VFPPROD_UPDATE),
                        R"(Merged collection must NOT have VFPROD_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::GROUP_INJECTION_UPDATE),
                        R"(Merged collection must NOT have GROUP_INJECTION_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::WELL_PRODUCTIVITY_INDEX),
                        R"(Merged collection must NOT have WELL_PRODUCTIVITY_INDEX)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::WELLGROUP_EFFICIENCY_UPDATE),
                        R"(Merged collection must NOT have WELLGROUP_EFFICIENCY_UPDATE)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::INJECTION_TYPE_CHANGED),
                        R"(Merged collection must NOT have INJECTION_TYPE_CHANGED)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::WELL_SWITCHED_INJECTOR_PRODUCER),
                        R"(Merged collection must NOT have WELL_SWITCHED_INJECTOR_PRODUCER)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::ACTIONX_WELL_EVENT),
                        R"(Merged collection must NOT have ACTIONX_WELL_EVENT)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::REQUEST_OPEN_WELL),
                        R"(Merged collection must NOT have REQUEST_OPEN_WELL)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::REQUEST_SHUT_WELL),
                        R"(Merged collection must NOT have REQUEST_SHUT_WELL)");

    BOOST_CHECK_MESSAGE(! ev1.hasEvent(SchEvt::TUNINGDP_CHANGE),
                        R"(Merged collection must NOT have TUNINGDP_CHANGE)");
}

BOOST_AUTO_TEST_CASE(MergeEventCollections)
{
    using SchEvt = Opm::ScheduleEvents::Events;

    auto c1 = Opm::WellGroupEvents{};
    c1.addWell("P1");
    c1.addEvent("P1", SchEvt::GROUP_CHANGE);
    c1.addEvent("P1", SchEvt::COMPLETION_CHANGE);
    c1.addGroup("G1");

    {
        auto c2 = Opm::WellGroupEvents{};
        c2.addGroup("G1");
        c2.addEvent("G1", SchEvt::GROUP_PRODUCTION_UPDATE);

        c2.addGroup("G2");

        c1.merge(c2);
    }

    BOOST_CHECK_MESSAGE(c1.has("P1"), R"(Well "P1" must exist in merged collection)");
    BOOST_CHECK_MESSAGE(c1.has("G1"), R"(Group "G1" must exist in merged collection)");
    BOOST_CHECK_MESSAGE(c1.has("G2"), R"(Group "G2" must exist in merged collection)");

    // -----------------------------------------------------------------------

    BOOST_CHECK_MESSAGE(c1.hasEvent("P1", SchEvt::NEW_WELL),
                        R"(Merged collection must have NEW_WELL for "P1")");
    BOOST_CHECK_MESSAGE(c1.hasEvent("P1", SchEvt::GROUP_CHANGE),
                        R"(Merged collection must have GROUP_CHANGE for "P1")");
    BOOST_CHECK_MESSAGE(c1.hasEvent("P1", SchEvt::COMPLETION_CHANGE),
                        R"(Merged collection must have COMPLETION_CHANGE for "P1")");

    // -----------------------------------------------------------------------

    BOOST_CHECK_MESSAGE(c1.hasEvent("G1", SchEvt::NEW_GROUP),
                        R"(Merged collection must have NEW_GROUP for "G1")");
    BOOST_CHECK_MESSAGE(c1.hasEvent("G1", SchEvt::GROUP_PRODUCTION_UPDATE),
                        R"(Merged collection must have GROUP_PRODUCTION_UPDATE for "G1")");

    // -----------------------------------------------------------------------

    BOOST_CHECK_MESSAGE(c1.hasEvent("G2", SchEvt::NEW_GROUP),
                        R"(Merged collection must have NEW_GROUP for "G2")");
}
