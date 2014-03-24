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


#define BOOST_TEST_MODULE SectionTests

#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(SectionTest) {
    Deck deck;
    DeckKeywordPtr test1(new DeckKeyword("TEST1"));
    deck.addKeyword(test1);
    DeckKeywordPtr test2(new DeckKeyword("TEST2"));
    deck.addKeyword(test2);
    DeckKeywordPtr test3(new DeckKeyword("TEST3"));
    deck.addKeyword(test3);
    DeckKeywordPtr test4(new DeckKeyword("TEST4"));
    deck.addKeyword(test4);
    Section section(deck, "TEST1", std::vector<std::string>() = {"TEST3", "TEST4"});
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST3"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST4"));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_EmptyDeck) {
    Deck deck;
    BOOST_REQUIRE_NO_THROW(RUNSPECSection section(deck));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSimpleDeck) {
    Deck deck;
    DeckKeywordPtr test1(new DeckKeyword("TEST1"));
    deck.addKeyword(test1);
    DeckKeywordPtr runSpec(new DeckKeyword("RUNSPEC"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr test2(new DeckKeyword("TEST2"));
    deck.addKeyword(test2);
    DeckKeywordPtr test3(new DeckKeyword("TEST3"));
    deck.addKeyword(test3);
    DeckKeywordPtr grid(new DeckKeyword("GRID"));
    deck.addKeyword(grid);
    DeckKeywordPtr test4(new DeckKeyword("TEST4"));
    deck.addKeyword(test4);
    RUNSPECSection section(deck);
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("RUNSPEC"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST3"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST4"));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSmallestPossibleDeck) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("RUNSPEC"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("GRID"));
    deck.addKeyword(grid);
    RUNSPECSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("RUNSPEC"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("GRID"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByEDITKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("GRID"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("EDIT"));
    deck.addKeyword(grid);
    GRIDSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("EDIT"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByPROPSKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("GRID"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("PROPS"));
    deck.addKeyword(grid);
    GRIDSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(EDITSection_TerminatedByPROPSKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("EDIT"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("PROPS"));
    deck.addKeyword(grid);
    EDITSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("EDIT"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedByREGIONSKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("PROPS"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("REGIONS"));
    deck.addKeyword(grid);
    PROPSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("REGIONS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedBySOLUTIONKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("PROPS"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("SOLUTION"));
    deck.addKeyword(grid);
    PROPSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(REGIONSSection_TerminatedBySOLUTIONKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("REGIONS"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("SOLUTION"));
    deck.addKeyword(grid);
    REGIONSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("REGIONS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySUMMARYKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("SOLUTION"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("SUMMARY"));
    deck.addKeyword(grid);
    SOLUTIONSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SUMMARY"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySCHEDULEKeyword) {
    Deck deck;
    DeckKeywordPtr runSpec(new DeckKeyword("SOLUTION"));
    deck.addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("SCHEDULE"));
    deck.addKeyword(grid);
    SOLUTIONSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SCHEDULE"));
}
