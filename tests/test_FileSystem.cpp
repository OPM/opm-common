/*
   +   Copyright 2021 Equinor ASA.
   +
   +   This file is part of the Open Porous Media project (OPM).
   +
   +   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   +   the Free Software Foundation, either version 3 of the License, or
   +   (at your option) any later version.
   +
   +   OPM is distributed in the hope that it will be useful,
   +   but WITHOUT ANY WARRANTY; without even the implied warranty of
   +   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   +   GNU General Public License for more details.
   +
   +   You should have received a copy of the GNU General Public License
   +   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   +   */

#include "config.h"

#include <opm/common/utility/FileSystem.hpp>

#define BOOST_TEST_MODULE FileSystem
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(TestProximate) {

    Opm::filesystem::path rel1("/a/b");
    Opm::filesystem::path input1("/a/b/c");
    auto test1 = Opm::proximate(input1, rel1);
    BOOST_CHECK_EQUAL(test1.string(), "c");

    Opm::filesystem::path input2("/a/c");
    auto test2 = Opm::proximate(input2, rel1);
    BOOST_CHECK_EQUAL(test2.string(), "../c");

    Opm::filesystem::path input3("c");
    auto test3 = Opm::proximate(input3, rel1);
    BOOST_CHECK_EQUAL(test3.string(), "c");

    auto test4 = Opm::proximate(rel1, input3);
    BOOST_CHECK_EQUAL(test4.string(), "/a/b");
}
