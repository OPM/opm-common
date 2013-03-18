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


#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "gtest/gtest.h"
#include "Parser.hpp"
#include "EclipseDeck.hpp"



TEST(ParserTest, Initializing) {
    Parser parser;
    ASSERT_TRUE(&parser != NULL);
}

TEST(ParserTest, ParseEmptyFileKeywordVectorEmpty) {
    Parser parser;
    EclipseDeck deck = parser.parse();
    ASSERT_EQ(0, deck.getNumberOfKeywords());
    ASSERT_EQ((unsigned int)0, deck.getKeywords().size());
}

TEST(ParserTest, ParseFileWithOneKeyword) {
    boost::filesystem::path singleKeywordFile("testdata/single.data");

    Parser parser(singleKeywordFile.string());
    EclipseDeck deck = parser.parse();

    ASSERT_EQ(1, deck.getNumberOfKeywords());
    ASSERT_EQ((unsigned int)1, deck.getKeywords().size());
}

TEST(ParserTest, ParseFileWithManyKeywords) {
    boost::filesystem::path multipleKeywordFile("testdata/gurbat_trimmed.DATA");

    Parser parser(multipleKeywordFile.string());
    EclipseDeck deck = parser.parse();

    ASSERT_EQ(18, deck.getNumberOfKeywords());
    ASSERT_EQ((unsigned int)18, deck.getKeywords().size());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
