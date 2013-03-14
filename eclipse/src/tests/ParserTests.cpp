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
    ASSERT_EQ(0, deck.GetKeywords().size());
    delete parser;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
