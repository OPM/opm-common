/*
  Copyright 2018 Statoil ASA.

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

#include <stdexcept>
#include <iostream>

#define BOOST_TEST_MODULE WTEST
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleTypes.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include "tests/MessageBuffer.cpp"

using namespace Opm;


BOOST_AUTO_TEST_CASE(CreateWellTestConfig) {
    WellTestConfig wc;

    BOOST_CHECK(wc.empty());


    wc.add_well("NAME", "P", 10, 10, 10, 1);
    BOOST_CHECK(!wc.empty());
    BOOST_CHECK_THROW(wc.add_well("NAME2", "", 10.0,10,10.0, 1), std::exception);
    BOOST_CHECK_THROW(wc.add_well("NAME3", "X", 1,2,3, 1), std::exception);

    wc.add_well("NAME", "PEGDC", 10, 10, 10, 1);
    wc.add_well("NAMEX", "PGDC", 10, 10, 10, 1);
    wc.drop_well("NAME");
    BOOST_CHECK(wc.has("NAMEX"));
    BOOST_CHECK(wc.has("NAMEX", WellTestConfig::Reason::PHYSICAL));
    BOOST_CHECK(!wc.has("NAMEX", WellTestConfig::Reason::ECONOMIC));
    BOOST_CHECK(!wc.has("NAME"));

    BOOST_CHECK_THROW(wc.get("NAMEX", WellTestConfig::Reason::ECONOMIC), std::exception);
    BOOST_CHECK_THROW(wc.get("NO_NAME", WellTestConfig::Reason::ECONOMIC), std::exception);
    const auto& wt = wc.get("NAMEX", WellTestConfig::Reason::PHYSICAL);
    BOOST_CHECK_EQUAL(wt.name, "NAMEX");
}


BOOST_AUTO_TEST_CASE(WTEST_STATE2) {
    WellTestConfig wc;
    WellTestState st;
    wc.add_well("WELL_NAME", "P", 0, 0, 0, 0);
    st.close_well("WELL_NAME", WellTestConfig::Reason::PHYSICAL, 100);
    BOOST_CHECK_EQUAL(st.num_closed_wells(), 1U);

    const UnitSystem us{};
    std::vector<Well> wells;
    wells.emplace_back("WELL_NAME", "A", 0, 0, 1, 1, 200., WellType(Phase::OIL), Well::ProducerCMode::NONE, Connection::Order::TRACK, us, 0., 1.0, true, true, 0, Well::GasInflowEquation::STD);
    {
        wells[0].updateStatus(Well::Status::SHUT);
        auto shut_wells = st.updateWells(wc, wells, 5000);
        BOOST_CHECK_EQUAL(shut_wells.size(), 0U);
    }

    {
        wells[0].updateStatus(Well::Status::OPEN);
        auto shut_wells = st.updateWells(wc, wells, 5000);
        BOOST_CHECK_EQUAL( shut_wells.size(), 1U);
    }
}

BOOST_AUTO_TEST_CASE(WTEST_STATE) {
    const double day = 86400.;
    WellTestState st;
    st.close_well("WELL_NAME", WellTestConfig::Reason::ECONOMIC, 100. * day);
    BOOST_CHECK_EQUAL(st.num_closed_wells(), 1U);

    st.openWell("WELL_NAME", WellTestConfig::Reason::ECONOMIC);
    BOOST_CHECK_EQUAL(st.num_closed_wells(), 0);

    st.close_well("WELL_NAME", WellTestConfig::Reason::ECONOMIC, 100. * day);
    BOOST_CHECK_EQUAL(st.num_closed_wells(), 1U);

    st.close_well("WELL_NAME", WellTestConfig::Reason::PHYSICAL, 100. * day);
    BOOST_CHECK_EQUAL(st.num_closed_wells(), 2U);

    st.close_well("WELLX", WellTestConfig::Reason::PHYSICAL, 100. * day);
    BOOST_CHECK_EQUAL(st.num_closed_wells(), 3U);

    const UnitSystem us{};
    std::vector<Well> wells;
    wells.emplace_back("WELL_NAME", "A", 0, 0, 1, 1, 200., WellType(Phase::OIL), Well::ProducerCMode::NONE, Connection::Order::TRACK, us, 0., 1.0, true, true, 0, Well::GasInflowEquation::STD);
    wells.emplace_back("WELLX", "A", 0, 0, 2, 2, 200.,     WellType(Phase::OIL), Well::ProducerCMode::NONE, Connection::Order::TRACK, us, 0., 1.0, true, true, 0, Well::GasInflowEquation::STD);

    WellTestConfig wc;
    {
        wells[0].updateStatus(Well::Status::SHUT);
        auto shut_wells = st.updateWells(wc, wells, 110. * day);
        BOOST_CHECK_EQUAL(shut_wells.size(), 0U);
    }
    {
        wells[0].updateStatus(Well::Status::OPEN);
        auto shut_wells = st.updateWells(wc, wells, 110. * day);
        BOOST_CHECK_EQUAL(shut_wells.size(), 0U);
    }

    wc.add_well("WELL_NAME", "P", 1000. * day, 2, 0, 1);
    // Not sufficient time has passed.
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 200. * day).size(), 0U);

    // We should test it:
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 1200. * day).size(), 1U);

    // Not sufficient time has passed.
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 1700. * day).size(), 0U);

    st.openWell("WELL_NAME", WellTestConfig::Reason::PHYSICAL);

    st.close_well("WELL_NAME", WellTestConfig::Reason::PHYSICAL, 1900. * day);

    // We should not test it:
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 2400. * day).size(), 0U);

    // We should test it now:
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 3000. * day).size(), 1U);

    // Too many attempts:
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 4000. * day).size(), 0U);

    wc.add_well("WELL_NAME", "P", 1000. * day, 3, 0, 5);


    wells[0].updateStatus(Well::Status::SHUT);
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 4100. * day).size(), 0U);

    wells[0].updateStatus(Well::Status::OPEN);
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 4100. * day).size(), 1U);

    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 5200. * day).size(), 1U);

    wc.drop_well("WELL_NAME");
    BOOST_CHECK_EQUAL( st.updateWells(wc, wells, 6300. * day).size(), 0U);
}


BOOST_AUTO_TEST_CASE(WTEST_STATE_COMPLETIONS) {
    WellTestConfig wc;
    WellTestState st;
    st.close_completion("WELL_NAME", 2, 100);
    BOOST_CHECK_EQUAL(st.num_closed_completions(), 1U);

    st.close_completion("WELL_NAME", 2, 100);
    BOOST_CHECK_EQUAL(st.num_closed_completions(), 1U);

    st.close_completion("WELL_NAME", 3, 100);
    BOOST_CHECK_EQUAL(st.num_closed_completions(), 2U);

    st.close_completion("WELLX", 3, 100);
    BOOST_CHECK_EQUAL(st.num_closed_completions(), 3U);

    const UnitSystem us{};
    std::vector<Well> wells;
    wells.emplace_back("WELL_NAME", "A", 0, 0, 1, 1, 200., WellType(Phase::OIL), Well::ProducerCMode::NONE, Connection::Order::TRACK, us, 0., 1.0, true, true, 0, Well::GasInflowEquation::STD);
    wells[0].updateStatus(Well::Status::OPEN);
    wells.emplace_back("WELLX", "A", 0, 0, 2, 2, 200., WellType(Phase::OIL), Well::ProducerCMode::NONE, Connection::Order::TRACK, us, 0., 1.0, true, true, 0, Well::GasInflowEquation::STD);
    wells[1].updateStatus(Well::Status::OPEN);

    auto num_closed_completions = st.updateWells(wc, wells, 5000);
    BOOST_CHECK_EQUAL( num_closed_completions.size(), 0U);

    st.dropCompletion("WELL_NAME", 2);
    st.dropCompletion("WELLX", 3);
    BOOST_CHECK_EQUAL(st.num_closed_completions(), 1U);
}




BOOST_AUTO_TEST_CASE(WTEST_PACK_UNPACK) {
    WellTestState st, st2;
    st.close_completion("WELL_NAME", 2, 100);
    st.close_completion("WELL_NAME", 2, 100);
    st.close_completion("WELL_NAME", 3, 100);
    st.close_completion("WELLX", 3, 100);

    st.close_well("WELL_NAME", WellTestConfig::Reason::ECONOMIC, 100);
    st.close_well("WELL_NAME", WellTestConfig::Reason::PHYSICAL, 100);
    st.close_well("WELLX", WellTestConfig::Reason::PHYSICAL, 100);

    BOOST_CHECK(!(st == st2));

    MessageBuffer buffer;
    st.pack(buffer);

    st2.unpack(buffer);
    BOOST_CHECK(st == st2);
}


