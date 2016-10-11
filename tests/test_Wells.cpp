/*
  Copyright 2016 Statoil ASA.

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

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE Wells
#include <boost/test/unit_test.hpp>

#include <stdexcept>

#include <opm/output/data/Wells.hpp>

using namespace Opm;
using rt = data::Rates::opt;

BOOST_AUTO_TEST_CASE(has) {
    data::Rates rates;

    rates.set( rt::wat, 10 );
    BOOST_CHECK( rates.has( rt::wat ) );
    BOOST_CHECK( !rates.has( rt::gas ) );
    BOOST_CHECK( !rates.has( rt::oil ) );

    rates.set( rt::gas, 0 );
    BOOST_CHECK( rates.has( rt::gas ) );
    BOOST_CHECK( !rates.has( rt::oil ) );
}

BOOST_AUTO_TEST_CASE(set_and_get) {
    data::Rates rates;

    const double wat = 10;
    const double gas = 10;

    rates.set( rt::wat, wat );
    rates.set( rt::gas, gas );

    BOOST_CHECK_EQUAL( wat, rates.get( rt::wat ) );
    BOOST_CHECK_EQUAL( gas, rates.get( rt::gas ) );
}

BOOST_AUTO_TEST_CASE(get_wrong) {
    data::Rates rates;
    const double wat = 10;
    const double gas = 10;

    rates.set( rt::wat, wat );
    rates.set( rt::gas, gas );

    const double def = 1;

    BOOST_CHECK_EQUAL( def, rates.get( rt::oil, def ) );
    BOOST_CHECK_THROW( rates.get( rt::oil ), std::invalid_argument );
}


