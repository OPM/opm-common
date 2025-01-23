
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

#include "config.h"

#define NVERBOSE  // Suppress own messages when throw()ing

#define BOOST_TEST_MODULE FastSmallVectorTest
#include <boost/test/unit_test.hpp>

#include <opm/material/common/FastSmallVector.hpp>


BOOST_AUTO_TEST_SUITE (FastSmallVectorTests)

BOOST_AUTO_TEST_CASE (BasicOperations)
{
    // Empty
    {
        Opm::FastSmallVector<int, 3> v;
        BOOST_CHECK(v.size() == 0ul);
        BOOST_CHECK(v.capacity() == 3ul);
    }

    // Size smaller than N, expanding to greater than N
    {
        Opm::FastSmallVector<int, 3> v(2);
        BOOST_CHECK(v.size() == 2ul);
        BOOST_CHECK(v.capacity() == 3ul);
        v[0] = 0;
        v[1] = 1;
        v.push_back(2);
        BOOST_REQUIRE(v.size() == 3ul);
        BOOST_CHECK(v.capacity() == 3ul);
        v.push_back(3);
        BOOST_REQUIRE(v.size() == 4ul);
        BOOST_CHECK(v.capacity() >= 4ul);
        for (int ii = 0; ii < 4; ++ii) {
            BOOST_CHECK_EQUAL(v[ii], ii);
        }
    }

    // Size exactly N
    {
        Opm::FastSmallVector<int, 3> v(3, 42);
        BOOST_REQUIRE(v.size() == 3ul);
        BOOST_CHECK(v.capacity() == 3ul);
        for (int ii = 0; ii < 3; ++ii) {
            BOOST_CHECK_EQUAL(v[ii], 42);
        }
        for (int val : v) {
            BOOST_CHECK_EQUAL(val, 42);
        }
        BOOST_CHECK_EQUAL(v.end() - v.begin(), v.size());
    }

    // Size greater than N
    {
        Opm::FastSmallVector<int, 3> v(4, 42);
        BOOST_REQUIRE(v.size() == 4ul);
        BOOST_CHECK(v.capacity() == 4ul);
        for (int ii = 0; ii < 4; ++ii) {
            BOOST_CHECK_EQUAL(v[ii], 42);
        }
        for (int val : v) {
            BOOST_CHECK_EQUAL(val, 42);
        }
        BOOST_CHECK_EQUAL(v.end() - v.begin(), v.size());

        v.push_back(42);
        BOOST_REQUIRE(v.size() == 5ul);
        BOOST_CHECK(v.capacity() >= 5ul);
        for (int ii = 0; ii < 5; ++ii) {
            BOOST_CHECK_EQUAL(v[ii], 42);
        }
        for (int val : v) {
            BOOST_CHECK_EQUAL(val, 42);
        }

        Opm::FastSmallVector<int, 3> v2(v);
        BOOST_REQUIRE(v2.size() == 5ul);
        for (int val : v2) {
            BOOST_CHECK_EQUAL(val, 42);
        }

        Opm::FastSmallVector<int, 3> v3;
        v3 = v;
        BOOST_REQUIRE(v3.size() == 5ul);
        for (int val : v3) {
            BOOST_CHECK_EQUAL(val, 42);
        }

        // Element-wise assignment
        for (int& val : v3) {
            val = 31;
        }
        for (int val : v3) {
            BOOST_CHECK_EQUAL(val, 31);
        }
    }

}

BOOST_AUTO_TEST_SUITE_END()
