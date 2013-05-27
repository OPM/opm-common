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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE ParserTestsInternalData
#include <boost/test/unit_test.hpp>


#include "opm/parser/eclipse/Parser/Parser.hpp"
#include "opm/parser/eclipse/Parser/ParserKW.hpp"
#include "opm/parser/eclipse/Parser/ParserRecordSize.hpp"
#include "opm/parser/eclipse/RawDeck/RawDeck.hpp"
using namespace Opm;

//NOTE: needs statoil dataset

BOOST_AUTO_TEST_CASE(ParseFileWithManyKeywords) {
    boost::filesystem::path current = boost::filesystem::current_path();
    std::cout << "Current path: " << current.string() << "\n";
    
    boost::filesystem::path multipleKeywordFile("testdata/statoil/gurbat_trimmed.DATA");
    std::cout << "BOOST path running ParseFullTestFile\n";

    ParserPtr parser(new Parser());

    RawDeckPtr rawDeck = parser->parse(multipleKeywordFile.string());

    //This check is not necessarily correct, 
    //as it depends on that all the fixed recordNum keywords are specified
    BOOST_CHECK_EQUAL((unsigned) 275, rawDeck->getNumberOfKeywords());
}

//NOTE: needs statoil dataset

BOOST_AUTO_TEST_CASE(ParseFullTestFile) {
    boost::filesystem::path multipleKeywordFile("testdata/statoil/ECLIPSE.DATA");

    ParserPtr parser(new Parser());

    RawDeckPtr rawDeck = parser->parse(multipleKeywordFile.string());
    // Note, cannot check the number of keywords, since the number of
    // records are not defined (yet) for all these keywords.
    // But we can check a copule of keywords, and that they have the correct
    // number of records

    RawKeywordConstPtr matchingKeyword = rawDeck->getKeyword("OIL");
    std::list<RawRecordConstPtr> records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL("OIL", matchingKeyword->getKeyword());

    BOOST_CHECK_EQUAL((unsigned) 0, records.size());

    matchingKeyword = rawDeck->getKeyword("VFPPDIMS");
    records = matchingKeyword->getRecords();
    BOOST_CHECK_EQUAL("VFPPDIMS", matchingKeyword->getKeyword());
    BOOST_CHECK_EQUAL((unsigned) 1, records.size());

    const std::string& recordString = records.front()->getRecordString();
    BOOST_CHECK_EQUAL("20  20  15  15  15   50", recordString);
    std::deque<std::string> recordItems = records.front()->getItems();
    BOOST_CHECK_EQUAL((unsigned) 6, recordItems.size());
}
