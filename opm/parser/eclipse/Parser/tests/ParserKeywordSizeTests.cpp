/*
  Copyright 2013 Statoil ASA.

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


#define BOOST_TEST_MODULE ParserTests
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/ParserKeywordSize.hpp>


using namespace Opm;

BOOST_AUTO_TEST_CASE(initialize_defaultConstructor_allGood) {
    BOOST_REQUIRE_NO_THROW(ParserKeywordSize keywordSize);
}

BOOST_AUTO_TEST_CASE(fixedSize_sizeNotFixed_exceptionThrown) {
    ParserKeywordSize keywordSize;
    BOOST_CHECK_THROW(keywordSize.fixedSize(), std::logic_error);
}

BOOST_AUTO_TEST_CASE(fixedSize_sizeIsFixedAndSet_sizeIsReturnedCorrectly) {
    BOOST_REQUIRE_NO_THROW(ParserKeywordSize keywordSize(100));
    ParserKeywordSize keywordSize(100);

    BOOST_CHECK_EQUAL(100U, keywordSize.fixedSize());
}

BOOST_AUTO_TEST_CASE(hasFixedSize_sizeofmanykinds_trueOnlyForFIXED) {
    ParserKeywordSize keywordSizeFixed(100);
    ParserKeywordSize keywordSizeDefault;

    BOOST_CHECK(keywordSizeFixed.hasFixedSize());
    BOOST_CHECK(!keywordSizeDefault.hasFixedSize());

}


