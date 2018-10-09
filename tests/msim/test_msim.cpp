/*
  Copyright 2018 Equinor ASA.

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

#include <ert/util/test_work_area.h>
#include <ert/util/util.h>

#include <opm/msim/msim.hpp>

using namespace Opm;


BOOST_AUTO_TEST_CASE(RUN) {
    msim msim("SPE1CASE1.DATA");
    {
        test_work_area_type * work_area = test_work_area_alloc("test_msim");
        msim.run();

        for (const auto& fname : {"SPE1CASE1.INIT", "SPE1CASE1.UNRST", "SPE1CASE1.EGRID", "SPE1CASE1.SMSPEC"})
            BOOST_CHECK( util_is_file( fname ));

        test_work_area_free( work_area );
    }
}
