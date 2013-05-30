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

#include <map>
#include <string>
#include <iostream>
#define BOOST_TEST_MODULE RawDeckTests
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/RawDeck.hpp>
#include <opm/parser/eclipse/RawDeck/RawParserKWs.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <boost/test/test_tools.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(Initialize_NoThrow) {
    RawParserKWsConstPtr fixedKeywords(new RawParserKWs());
    BOOST_CHECK_NO_THROW( RawDeck rawDeck( fixedKeywords ));
}


BOOST_AUTO_TEST_CASE(GetNumberOfKeywords_EmptyDeck_RetunsZero) {
    RawParserKWsConstPtr fixedKeywords(new RawParserKWs());
    RawDeckPtr rawDeck(new RawDeck(fixedKeywords));
    BOOST_CHECK_EQUAL((unsigned) 0, rawDeck->getNumberOfKeywords());
}

BOOST_AUTO_TEST_CASE(HasKeyword_NotExisting_RetunsFalse) {
    RawParserKWsConstPtr fixedKeywords(new RawParserKWs());
    RawDeckPtr rawDeck(new RawDeck(fixedKeywords));
    BOOST_CHECK_EQUAL(false, rawDeck->hasKeyword("TEST"));
}

BOOST_AUTO_TEST_CASE(GetKeyword_EmptyDeck_ThrowsExeption) {
    RawParserKWsConstPtr fixedKeywords(new RawParserKWs());
    RawDeckPtr rawDeck(new RawDeck(fixedKeywords));
    BOOST_CHECK_THROW(rawDeck->getKeyword("TEST"), std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(PrintToOStream_noThrow) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");


    RawDeckPtr rawDeck(new RawDeck(RawParserKWsConstPtr(new RawParserKWs())));
    rawDeck->parse(singleKeywordFile.string());
    std::cout << *rawDeck << "\n";
}


BOOST_AUTO_TEST_CASE(Parse_InvalidInputFile_Throws) {
    RawDeckPtr rawDeck(new RawDeck(RawParserKWsConstPtr(new RawParserKWs())));
    BOOST_CHECK_THROW(rawDeck->parse("nonexistingfile.asdf"), std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(Parse_ValidInputFile_NoThrow) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");
    RawDeckPtr rawDeck(new RawDeck(RawParserKWsConstPtr(new RawParserKWs())));
    BOOST_CHECK_NO_THROW(rawDeck->parse(singleKeywordFile.string()));
}



BOOST_AUTO_TEST_CASE(ParseFileWithOneKeyword) {
    boost::filesystem::path singleKeywordFile("testdata/mini.data");

    RawDeckPtr rawDeck(new RawDeck(RawParserKWsConstPtr(new RawParserKWs())));
    rawDeck->parse(singleKeywordFile.string());

    BOOST_CHECK_EQUAL((unsigned) 1, rawDeck->getNumberOfKeywords());
    RawKeywordConstPtr rawKeyword = rawDeck->getKeyword("ENDSCALE");
    const std::list<RawRecordConstPtr>& records = rawKeyword->getRecords();
    BOOST_CHECK_EQUAL((unsigned) 1, records.size());
    RawRecordConstPtr record = records.back();

    const std::string& recordString = record->getRecordString();
    BOOST_CHECK_EQUAL("'NODIR'  'REVERS'  1  20", recordString);

    const std::deque<std::string>& recordElements = record->getItems();
    BOOST_CHECK_EQUAL((unsigned) 4, recordElements.size());

    BOOST_CHECK_EQUAL("NODIR", recordElements[0]);
    BOOST_CHECK_EQUAL("REVERS", recordElements[1]);
    BOOST_CHECK_EQUAL("1", recordElements[2]);
    BOOST_CHECK_EQUAL("20", recordElements[3]);
}

BOOST_AUTO_TEST_CASE(ParseFileWithFewKeywords) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");

    RawDeckPtr rawDeck(new RawDeck(RawParserKWsConstPtr(new RawParserKWs())));
    rawDeck->parse(singleKeywordFile.string());

    BOOST_CHECK_EQUAL((unsigned) 7, rawDeck->getNumberOfKeywords());

    RawKeywordConstPtr matchingKeyword = rawDeck->getKeyword("OIL");
    std::list<RawRecordConstPtr> records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL("OIL", matchingKeyword->getKeyword());
    BOOST_CHECK_EQUAL((unsigned) 0, records.size());

    // The two next come in via the include of the include path/readthis.sch file
    matchingKeyword = rawDeck->getKeyword("GRUPTREE");
    BOOST_CHECK_EQUAL("GRUPTREE", matchingKeyword->getKeyword());
    records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL((unsigned) 2, records.size());

    matchingKeyword = rawDeck->getKeyword("WHISTCTL");
    BOOST_CHECK_EQUAL("WHISTCTL", matchingKeyword->getKeyword());
    records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL((unsigned) 1, records.size());

    matchingKeyword = rawDeck->getKeyword("METRIC");
    BOOST_CHECK_EQUAL("METRIC", matchingKeyword->getKeyword());
    records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL((unsigned) 0, records.size());

    matchingKeyword = rawDeck->getKeyword("GRIDUNIT");
    BOOST_CHECK_EQUAL("GRIDUNIT", matchingKeyword->getKeyword());
    records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL((unsigned) 1, records.size());

    matchingKeyword = rawDeck->getKeyword("RADFIN4");
    records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL("RADFIN4", matchingKeyword->getKeyword());
    BOOST_CHECK_EQUAL((unsigned) 1, records.size());

    matchingKeyword = rawDeck->getKeyword("ABCDAD");
    BOOST_CHECK_EQUAL("ABCDAD", matchingKeyword->getKeyword());
    records = matchingKeyword->getRecords();

    BOOST_CHECK_EQUAL((unsigned) 2, records.size());
}
