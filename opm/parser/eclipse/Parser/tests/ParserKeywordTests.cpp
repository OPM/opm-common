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

#include <opm/json/JsonObject.hpp>
#include "opm/parser/eclipse/Parser/ParserKeyword.hpp"
#include "opm/parser/eclipse/Parser/ParserIntItem.hpp"
#include "opm/parser/eclipse/Parser/ParserItem.hpp"



using namespace Opm;

BOOST_AUTO_TEST_CASE(construct_withname_nameSet) {
    ParserKeyword parserKeyword("BPR");
    BOOST_CHECK_EQUAL(parserKeyword.getName(), "BPR");
}

BOOST_AUTO_TEST_CASE(NamedInit) {
    std::string keyword("KEYWORD");

    ParserKeyword parserKeyword(keyword, 100);
    BOOST_CHECK_EQUAL(parserKeyword.getName(), keyword);
}


BOOST_AUTO_TEST_CASE(ConstructFromJsonObject) {
    Json::JsonObject jsonObject("{\"name\": \"BPR\"}");
    ParserKeyword parserKeyword(jsonObject);
    BOOST_CHECK_EQUAL("BPR" , parserKeyword.getName());
    BOOST_CHECK_EQUAL( false , parserKeyword.hasFixedSize() );
}


BOOST_AUTO_TEST_CASE(ConstructFromJsonObject_withSize) {
    Json::JsonObject jsonObject("{\"name\": \"BPR\", \"size\" : 100}");
    ParserKeyword parserKeyword(jsonObject);
    BOOST_CHECK_EQUAL("BPR" , parserKeyword.getName());
    BOOST_CHECK_EQUAL( true , parserKeyword.hasFixedSize() );
    BOOST_CHECK_EQUAL( 100U , parserKeyword.getFixedSize() );
}


BOOST_AUTO_TEST_CASE(setRecord_validRecord_recordSet) {
    ParserKeywordPtr parserKeyword(new ParserKeyword("JA"));
    ParserRecordConstPtr parserRecord = ParserRecordConstPtr(new ParserRecord());
    parserKeyword->setRecord(parserRecord);
    BOOST_CHECK_EQUAL(parserRecord, parserKeyword->getRecord());
}

BOOST_AUTO_TEST_CASE(constructor_nametoolongwithfixedsize_exceptionthrown) {
    std::string keyword("KEYWORDTOOLONG");
    BOOST_CHECK_THROW(ParserKeyword parserKeyword(keyword, 100), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(constructor_nametoolong_exceptionthrown) {
    std::string keyword("KEYWORDTOOLONG");
    BOOST_CHECK_THROW(ParserKeyword parserKeyword(keyword), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(MixedCase) {
    std::string keyword("KeyWord");

    BOOST_CHECK_THROW(ParserKeyword parserKeyword(keyword, 100), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(getFixedSize_sizeObjectHasFixedSize_sizeReturned) {
    ParserKeywordPtr parserKeyword(new ParserKeyword("JA", 3));
    BOOST_CHECK_EQUAL(3U, parserKeyword->getFixedSize());

}

BOOST_AUTO_TEST_CASE(getFixedSize_sizeObjectDoesNotHaveFixedSizeObjectSet_ExceptionThrown) {
    ParserKeywordPtr parserKeyword(new ParserKeyword("JA"));
    BOOST_CHECK_THROW(parserKeyword->getFixedSize(), std::logic_error);
}


BOOST_AUTO_TEST_CASE(hasFixedSize_hasFixedSizeObject_returnstrue) {
    ParserKeywordPtr parserKeyword(new ParserKeyword("JA", 2));
    BOOST_CHECK(parserKeyword->hasFixedSize());
}

BOOST_AUTO_TEST_CASE(hasFixedSize_sizeObjectDoesNotHaveFixedSize_returnsfalse) {
    ParserKeywordPtr parserKeyword(new ParserKeyword("JA"));
    BOOST_CHECK(!parserKeyword->hasFixedSize());
}

