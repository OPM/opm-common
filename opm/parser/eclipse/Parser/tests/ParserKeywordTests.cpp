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

    ParserKeywordSizePtr recordSize(new ParserKeywordSize(100));
    ParserKeyword parserKeyword(keyword, recordSize);
    BOOST_CHECK_EQUAL(parserKeyword.getName(), keyword);
}

BOOST_AUTO_TEST_CASE(setRecord_validRecord_recordSet) {
    ParserKeywordPtr parserKeyword(new ParserKeyword("JA"));
    ParserRecordConstPtr parserRecord = ParserRecordConstPtr(new ParserRecord());
    parserKeyword->setRecord(parserRecord);
    BOOST_CHECK_EQUAL(parserRecord, parserKeyword->getRecord());
}

BOOST_AUTO_TEST_CASE(NameTooLong) {
    std::string keyword("KEYWORDTOOLONG");
    ParserKeywordSizePtr recordSize(new ParserKeywordSize(100));
    BOOST_CHECK_THROW(ParserKeyword parserKeyword(keyword, recordSize), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(MixedCase) {
    std::string keyword("KeyWord");

    ParserKeywordSizePtr recordSize(new ParserKeywordSize(100));
    BOOST_CHECK_THROW(ParserKeyword parserKeyword(keyword, recordSize), std::invalid_argument);
}
