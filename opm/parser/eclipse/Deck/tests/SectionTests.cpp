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
#include <typeinfo>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(SectionTest) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr test1(new DeckKeyword("TEST1"));
    deck->addKeyword(test1);
    DeckKeywordPtr test2(new DeckKeyword("TEST2"));
    deck->addKeyword(test2);
    DeckKeywordPtr test3(new DeckKeyword("TEST3"));
    deck->addKeyword(test3);
    DeckKeywordPtr test4(new DeckKeyword("TEST4"));
    deck->addKeyword(test4);
    Section section(deck, "TEST1", std::vector<std::string>() = {"TEST3", "TEST4"});
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST3"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST4"));
}

BOOST_AUTO_TEST_CASE(IteratorTest) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr test1(new DeckKeyword("TEST1"));
    deck->addKeyword(test1);
    DeckKeywordPtr test2(new DeckKeyword("TEST2"));
    deck->addKeyword(test2);
    DeckKeywordPtr test3(new DeckKeyword("TEST3"));
    deck->addKeyword(test3);
    DeckKeywordPtr test4(new DeckKeyword("TEST4"));
    deck->addKeyword(test4);
    Section section(deck, "TEST1", std::vector<std::string>() = {"TEST3", "TEST4"});

    int numberOfItems = 0;
    for (auto iter=section.begin(); iter != section.end(); ++iter) {
//        std::cout << typeid(iter).name() << std::endl;
//        std::cout << iter->name() << std::endl;
        numberOfItems++;
    }
    BOOST_CHECK_EQUAL(2, numberOfItems);
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_EmptyDeck) {
    DeckPtr deck(new Deck());
    BOOST_REQUIRE_NO_THROW(RUNSPECSection section(deck));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSimpleDeck) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr test1(new DeckKeyword("TEST1"));
    deck->addKeyword(test1);
    DeckKeywordPtr runSpec(new DeckKeyword("RUNSPEC"));
    deck->addKeyword(runSpec);
    DeckKeywordPtr test2(new DeckKeyword("TEST2"));
    deck->addKeyword(test2);
    DeckKeywordPtr test3(new DeckKeyword("TEST3"));
    deck->addKeyword(test3);
    DeckKeywordPtr grid(new DeckKeyword("GRID"));
    deck->addKeyword(grid);
    DeckKeywordPtr test4(new DeckKeyword("TEST4"));
    deck->addKeyword(test4);
    RUNSPECSection section(deck);
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("RUNSPEC"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST3"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("TEST4"));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSmallestPossibleDeck) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr runSpec(new DeckKeyword("RUNSPEC"));
    deck->addKeyword(runSpec);
    DeckKeywordPtr grid(new DeckKeyword("GRID"));
    deck->addKeyword(grid);
    RUNSPECSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("RUNSPEC"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("GRID"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByEDITKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr grid(new DeckKeyword("GRID"));
    deck->addKeyword(grid);
    DeckKeywordPtr edit(new DeckKeyword("EDIT"));
    deck->addKeyword(edit);
    GRIDSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("EDIT"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByPROPSKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr grid(new DeckKeyword("GRID"));
    deck->addKeyword(grid);
    DeckKeywordPtr props(new DeckKeyword("PROPS"));
    deck->addKeyword(props);
    GRIDSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(EDITSection_TerminatedByPROPSKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr edit(new DeckKeyword("EDIT"));
    deck->addKeyword(edit);
    DeckKeywordPtr props(new DeckKeyword("PROPS"));
    deck->addKeyword(props);
    EDITSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("EDIT"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedByREGIONSKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr props(new DeckKeyword("PROPS"));
    deck->addKeyword(props);
    DeckKeywordPtr regions(new DeckKeyword("REGIONS"));
    deck->addKeyword(regions);
    PROPSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("REGIONS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedBySOLUTIONKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr props(new DeckKeyword("PROPS"));
    deck->addKeyword(props);
    DeckKeywordPtr solution(new DeckKeyword("SOLUTION"));
    deck->addKeyword(solution);
    PROPSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(REGIONSSection_TerminatedBySOLUTIONKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr regions(new DeckKeyword("REGIONS"));
    deck->addKeyword(regions);
    DeckKeywordPtr solution(new DeckKeyword("SOLUTION"));
    deck->addKeyword(solution);
    REGIONSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("REGIONS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySUMMARYKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr solution(new DeckKeyword("SOLUTION"));
    deck->addKeyword(solution);
    DeckKeywordPtr summary(new DeckKeyword("SUMMARY"));
    deck->addKeyword(summary);
    SOLUTIONSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SUMMARY"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySCHEDULEKeyword) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr solution(new DeckKeyword("SOLUTION"));
    deck->addKeyword(solution);
    DeckKeywordPtr schedule(new DeckKeyword("SCHEDULE"));
    deck->addKeyword(schedule);
    SOLUTIONSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SCHEDULE"));
}

BOOST_AUTO_TEST_CASE(SCHEDULESection_NotTerminated) {
    DeckPtr deck(new Deck());
    DeckKeywordPtr schedule(new DeckKeyword("SCHEDULE"));
    deck->addKeyword(schedule);
    DeckKeywordPtr test1(new DeckKeyword("TEST1"));
    deck->addKeyword(test1);
    DeckKeywordPtr test2(new DeckKeyword("TEST2"));
    deck->addKeyword(test2);
    DeckKeywordPtr test3(new DeckKeyword("TEST3"));
    deck->addKeyword(test3);
    SCHEDULESection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SCHEDULE"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST3"));
}
