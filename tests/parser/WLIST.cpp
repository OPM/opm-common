/*
  Copyright 2019 Equinor ASA.

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
#include <algorithm>

#define BOOST_TEST_MODULE WLIST_TEST

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/WList.hpp>

BOOST_AUTO_TEST_CASE(CreateWLIST) {
    Opm::WList wlist;
    BOOST_CHECK_EQUAL(wlist.size(), 0);
    wlist.add("W1");
    BOOST_CHECK_EQUAL(wlist.size(), 1);


    wlist.del("NO_SUCH_WELL");
    BOOST_CHECK_EQUAL(wlist.size(), 1);

    wlist.del("W1");
    BOOST_CHECK_EQUAL(wlist.size(), 0);

    wlist.add("W1");
    wlist.add("W2");
    wlist.add("W3");

    auto wells = wlist.wells();
    BOOST_CHECK_EQUAL(wells.size(), 3);
    BOOST_CHECK( std::find(wells.begin(), wells.end(), "W1") != wells.end());
    BOOST_CHECK( std::find(wells.begin(), wells.end(), "W2") != wells.end());
    BOOST_CHECK( std::find(wells.begin(), wells.end(), "W3") != wells.end());

    std::vector<std::string> wells2;
    for (const auto& well : wlist)
        wells2.push_back(well);

    BOOST_CHECK_EQUAL(wells2.size(), 3);
    BOOST_CHECK( std::find(wells2.begin(), wells2.end(), "W1") != wells2.end());
    BOOST_CHECK( std::find(wells2.begin(), wells2.end(), "W2") != wells2.end());
    BOOST_CHECK( std::find(wells2.begin(), wells2.end(), "W3") != wells2.end());

}
