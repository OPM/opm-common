#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "gtest/gtest.h"
#include "Parser.hpp"
#include "EclipseDeck.hpp"



TEST(ParserTest, Initializing) {
    Parser * parser = new Parser();
    ASSERT_TRUE(parser != NULL);
    delete parser;
}

TEST(ParserTest, ParseEmptyFileKeywordVectorEmpty) {
    Parser * parser = new Parser();
    EclipseDeck deck = parser->Parse();
    ASSERT_EQ(0, deck.GetNumberOfKeywords());
    ASSERT_EQ((unsigned int)0, deck.GetKeywords().size());
    delete parser;
}

TEST(ParserTest, ParseFileWithOneKeyword) {
    boost::filesystem::path singleKeywordFile("testdata/single.data");

    Parser * parser = new Parser(singleKeywordFile.string());
    EclipseDeck deck = parser->Parse();

    ASSERT_EQ(1, deck.GetNumberOfKeywords());
    ASSERT_EQ((unsigned int)1, deck.GetKeywords().size());

    delete parser;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
