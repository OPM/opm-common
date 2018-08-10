/*
  Copyright 2015 Statoil ASA.

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
#include <cstdio>

#define BOOST_TEST_MODULE InputKeywordTests

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <opm/parser/eclipse/Generator/KeywordLoader.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>

inline std::string prefix() {
    return boost::unit_test::framework::master_test_suite().argv[1];
}

BOOST_AUTO_TEST_CASE(EmptyKeywordLoader) {
    Opm::KeywordLoader loader(false);

    BOOST_CHECK_EQUAL( false , loader.hasKeyword("NO"));
    BOOST_CHECK_EQUAL( 0U , loader.size() );
    BOOST_CHECK_THROW( loader.getKeyword("NO") , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(LoadKeyword) {
    Opm::KeywordLoader loader(false);

    BOOST_CHECK_THROW( loader.loadKeyword("does/not/exists") , std::invalid_argument );
    BOOST_CHECK_THROW( loader.loadKeyword(prefix() + "invalid.json") , std::invalid_argument);
    BOOST_CHECK_THROW( loader.loadKeyword(prefix() + "PORO-invalid") , std::invalid_argument);

    loader.loadKeyword(prefix() + "PORO.json");
    loader.loadKeyword(prefix() + "PORO.json");

    BOOST_CHECK_EQUAL( true , loader.hasKeyword("PORO"));
    BOOST_CHECK_EQUAL( 1U , loader.size() );

    loader.getKeyword("PORO");
}



