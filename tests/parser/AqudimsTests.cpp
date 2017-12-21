/*
  Copyright (C) 2017 TNO

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
#define BOOST_TEST_MODULE AQUDIMS_TESTS

#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Aqudims.hpp>

BOOST_AUTO_TEST_CASE(TEST_CREATE) {
    Opm::Aqudims aqudims;

    BOOST_CHECK_EQUAL( aqudims.getNumAqunum() , 1 );
    BOOST_CHECK_EQUAL( aqudims.getNumConnectionNumericalAquifer() , 1 );
    BOOST_CHECK_EQUAL( aqudims.getNumInfluenceTablesCT() , 1 );
    BOOST_CHECK_EQUAL( aqudims.getNumRowsInfluenceTable() , 36 );
    BOOST_CHECK_EQUAL( aqudims.getNumAnalyticAquifers() , 1 );
    BOOST_CHECK_EQUAL( aqudims.getNumRowsAquancon() , 1 );
    BOOST_CHECK_EQUAL( aqudims.getNumAquiferLists() , 0 );
    BOOST_CHECK_EQUAL( aqudims.getNumAnalyticAquifersSingleList() , 0 );
}
