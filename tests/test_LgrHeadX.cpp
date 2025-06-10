/*
  Copyright 2025 Equinor ASA.

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
// #include <boost/test/tools/old/interface.hpp>
// #include <opm/io/eclipse/EInit.hpp>

#include <opm/output/eclipse/LgrHEADD.hpp>
#include <opm/output/eclipse/LgrHEADI.hpp>
#include <opm/output/eclipse/LgrHEADQ.hpp>

#define BOOST_TEST_MODULE LgrHeadX
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(TestLgrHEADD)
{
    Opm::RestartIO ::LgrHEADD lgrheadd;
    BOOST_REQUIRE_EQUAL(lgrheadd.data().size(),5);
    auto& data = lgrheadd.data();
    BOOST_REQUIRE_EQUAL(data[0], 0.0);
    for (std::vector<double>::size_type i = 1; i < data.size(); ++i) {
        BOOST_REQUIRE_EQUAL(data[i], -0.1E+21);
    }
}

BOOST_AUTO_TEST_CASE(TestLgrHEADI)
{
    Opm::RestartIO ::LgrHEADI lgrheadi;
    lgrheadi.toggleLGRCell();
    lgrheadi.numberoOfLGRCell(10);
    auto& data = lgrheadi.data();
    BOOST_REQUIRE_EQUAL(data.size(),45);
    BOOST_REQUIRE_EQUAL(data[0],10);
    BOOST_REQUIRE_EQUAL(data[1],100);
    lgrheadi.toggleLGRCell(false);
    BOOST_REQUIRE_EQUAL(data[1],0);
}

BOOST_AUTO_TEST_CASE(TestLgrHEADQ)
{
    Opm::RestartIO ::LgrHEADQ lgrheadq;
    auto& data = lgrheadq.data();
    BOOST_REQUIRE_EQUAL(data.size(),5);
    bool expected = false;
    for (const auto& val : data) {
        BOOST_CHECK_EQUAL(val, expected);
    }
}
