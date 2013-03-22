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
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define BOOST_TEST_MODULE ParserTests
#include <boost/test/unit_test.hpp>

#include "opm/parser/eclipse/Parser.hpp"
#include "opm/parser/eclipse/data/RawDeck.hpp"
using namespace Opm;

BOOST_AUTO_TEST_CASE(Initializing) {
    BOOST_REQUIRE_NO_THROW(Parser parser);
}

BOOST_AUTO_TEST_CASE(ParseWithInvalidInputFileThrows) {
    Parser parser;
    RawDeck rawDeck;
    BOOST_REQUIRE_THROW(parser.parse("nonexistingfile.asdf", rawDeck), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ParseWithValidFileSetOnParseCallNoThrow) {

    boost::filesystem::path singleKeywordFile("testdata/small.data");
    Parser parser;
    RawDeck rawDeck;

    BOOST_REQUIRE_NO_THROW(parser.parse(singleKeywordFile.string(), rawDeck));
}

BOOST_AUTO_TEST_CASE(ParseWithInValidFileSetOnParseCallThrows) {
    boost::filesystem::path singleKeywordFile("testdata/nosuchfile.data");
    Parser parser;
    RawDeck rawDeck;

    BOOST_REQUIRE_THROW(parser.parse(singleKeywordFile.string(), rawDeck), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ParseFileWithFewKeywords) {
    boost::filesystem::path singleKeywordFile("testdata/small.data");

    Parser parser;
    RawDeck rawDeck;

    parser.parse(singleKeywordFile.string(), rawDeck);
    BOOST_REQUIRE_EQUAL((unsigned)4, rawDeck.getNumberOfKeywords());
}

//NOTE: needs statoil dataset

BOOST_AUTO_TEST_CASE(ParseFileWithManyKeywords) {
    boost::filesystem::path multipleKeywordFile("testdata/gurbat_trimmed.DATA");

    Parser parser;
    RawDeck rawDeck;

    parser.parse(multipleKeywordFile.string(), rawDeck);
    BOOST_REQUIRE_EQUAL((unsigned)18, rawDeck.getNumberOfKeywords());
}

//NOTE: needs statoil dataset

BOOST_AUTO_TEST_CASE(ParseFullTestFile) {
    boost::filesystem::path multipleKeywordFile("testdata/ECLIPSE.DATA");

    Parser parser;
    RawDeck rawDeck;

    parser.parse(multipleKeywordFile.string(), rawDeck);
    BOOST_REQUIRE_EQUAL((unsigned)73, rawDeck.getNumberOfKeywords());
}
