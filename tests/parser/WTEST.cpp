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
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE WTEST
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

using namespace Opm;


BOOST_AUTO_TEST_CASE(CreateWellTestConfig) {
    WellTestConfig wc;

    BOOST_CHECK_EQUAL(wc.size() , 0);


    wc.add_well("NAME", WellTestConfig::Reason::PHYSICAL, 10, 10, 10);
    BOOST_CHECK_EQUAL(wc.size(), 1);
    BOOST_CHECK_THROW(wc.add_well("NAME2", "", 10.0,10,10.0), std::invalid_argument);
    BOOST_CHECK_THROW(wc.add_well("NAME3", "X", 1,2,3), std::invalid_argument);

    wc.add_well("NAME", "PEGDC", 10, 10, 10);
    BOOST_CHECK_EQUAL(wc.size(), 6);
    wc.add_well("NAMEX", "PGDC", 10, 10, 10);
    BOOST_CHECK_EQUAL(wc.size(), 10);
    wc.drop_well("NAME");
    BOOST_CHECK_EQUAL(wc.size(), 4);
    BOOST_CHECK(wc.has("NAMEX"));
    BOOST_CHECK(wc.has("NAMEX", WellTestConfig::Reason::PHYSICAL));
    BOOST_CHECK(!wc.has("NAMEX", WellTestConfig::Reason::ECONOMIC));
    BOOST_CHECK(!wc.has("NAME"));


    BOOST_CHECK_THROW(wc.get("NAMEX", WellTestConfig::Reason::ECONOMIC), std::invalid_argument);
    BOOST_CHECK_THROW(wc.get("NO_NAME", WellTestConfig::Reason::ECONOMIC), std::invalid_argument);
    const auto& wt = wc.get("NAMEX", WellTestConfig::Reason::PHYSICAL);
    BOOST_CHECK_EQUAL(wt.name, "NAMEX");
}

