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
#include <iostream>
#include <typeinfo>

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/SCHEDULESection.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(SectionTest) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword("TEST0") );
    deck->addKeyword( DeckKeyword("RUNSPEC") );
    deck->addKeyword( DeckKeyword("TEST1") );
    deck->addKeyword( DeckKeyword("GRID") );
    deck->addKeyword( DeckKeyword("TEST2") );
    deck->addKeyword( DeckKeyword("SCHEDULE") );
    deck->addKeyword( DeckKeyword("TEST3") );

    Section runspecSection(*deck, "RUNSPEC");
    Section gridSection(*deck, "GRID");
    BOOST_CHECK(runspecSection.hasKeyword("TEST1"));
    BOOST_CHECK(gridSection.hasKeyword("TEST2"));

    BOOST_CHECK(!runspecSection.hasKeyword("TEST0"));
    BOOST_CHECK(!gridSection.hasKeyword("TEST0"));
    BOOST_CHECK(!runspecSection.hasKeyword("TEST3"));
    BOOST_CHECK(!gridSection.hasKeyword("TEST3"));
    BOOST_CHECK(!runspecSection.hasKeyword("TEST2"));
    BOOST_CHECK(!gridSection.hasKeyword("TEST1"));
}

BOOST_AUTO_TEST_CASE(IteratorTest) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword("TEST2") );
    deck->addKeyword( DeckKeyword( "TEST3" ) );
    deck->addKeyword( DeckKeyword( "GRID" ) );
    Section section(*deck, "RUNSPEC");

    int numberOfItems = 0;
    for (auto iter=section.begin(); iter != section.end(); ++iter) {
        std::cout << iter->name() << std::endl;
        numberOfItems++;
    }

    // the keywords expected here are RUNSPEC, TEST2 and TEST3...
    BOOST_CHECK_EQUAL(3, numberOfItems);
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_EmptyDeck) {
    DeckPtr deck(new Deck());
    BOOST_REQUIRE_THROW(RUNSPECSection section(*deck), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSimpleDeck) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "TEST1") );
    deck->addKeyword( DeckKeyword( "RUNSPEC") );
    deck->addKeyword( DeckKeyword( "TEST2") );
    deck->addKeyword( DeckKeyword( "TEST3") );
    deck->addKeyword( DeckKeyword( "GRID") );
    deck->addKeyword( DeckKeyword( "TEST4") );

    RUNSPECSection section(*deck);
    BOOST_CHECK(!section.hasKeyword("TEST1"));
    BOOST_CHECK(section.hasKeyword("RUNSPEC"));
    BOOST_CHECK(section.hasKeyword("TEST2"));
    BOOST_CHECK(section.hasKeyword("TEST3"));
    BOOST_CHECK(!section.hasKeyword("GRID"));
    BOOST_CHECK(!section.hasKeyword("TEST4"));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSmallestPossibleDeck) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "GRID") );
    RUNSPECSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("RUNSPEC"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("GRID"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByEDITKeyword) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "EDIT" ) );
    GRIDSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("EDIT"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByPROPSKeyword) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "PROPS" ) );
    GRIDSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(EDITSection_TerminatedByPROPSKeyword) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "EDIT" ) );
    deck->addKeyword( DeckKeyword( "PROPS" ) );
    EDITSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("EDIT"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedByREGIONSKeyword) {
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "REGIONS" ) );
    PROPSSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("REGIONS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedBySOLUTIONKeyword) {
    DeckPtr deck(new Deck());

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "SOLUTION" ) );

    PROPSSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(REGIONSSection_TerminatedBySOLUTIONKeyword) {
    DeckPtr deck(new Deck());

    deck->addKeyword( DeckKeyword( "REGIONS" ) );
    deck->addKeyword( DeckKeyword( "SOLUTION" ) );

    REGIONSSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("REGIONS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySUMMARYKeyword) {
    DeckPtr deck(new Deck());

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "SUMMARY" ) );

    SOLUTIONSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SUMMARY"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySCHEDULEKeyword) {
    DeckPtr deck(new Deck());

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );

    SOLUTIONSection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SCHEDULE"));
}

BOOST_AUTO_TEST_CASE(SCHEDULESection_NotTerminated) {
    DeckPtr deck(new Deck());

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    SCHEDULESection section(*deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SCHEDULE"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST3"));

    BOOST_CHECK( Section::hasSCHEDULE(*deck ));
    BOOST_CHECK( !Section::hasREGIONS(*deck ));
}

BOOST_AUTO_TEST_CASE(Section_ValidDecks) {
    // minimal deck
    DeckPtr deck(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    BOOST_CHECK(Opm::Section::checkSectionTopology(*deck));

    // deck with all optional sections
    deck.reset(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "EDIT" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "REGIONS" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST6" ) );

    deck->addKeyword( DeckKeyword( "SUMMARY" ) );
    deck->addKeyword( DeckKeyword( "TEST7" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST8" ) );

    BOOST_CHECK(Opm::Section::checkSectionTopology(*deck));
}

BOOST_AUTO_TEST_CASE(Section_InvalidDecks) {
    // keyword before RUNSPEC
    DeckPtr deck(new Deck());

    deck->addKeyword( DeckKeyword( "TEST0" ) );
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // wrong section order
    deck.reset(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "EDIT" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "REGIONS" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST6" ) );

    deck->addKeyword( DeckKeyword( "SUMMARY" ) );
    deck->addKeyword( DeckKeyword( "TEST7" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST8" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // duplicate section
    deck.reset(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST21" ) );

    deck->addKeyword( DeckKeyword( "EDIT" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "REGIONS" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST6" ) );

    deck->addKeyword( DeckKeyword( "SUMMARY" ) );
    deck->addKeyword( DeckKeyword( "TEST7" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST8" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // section after SCHEDULE
    deck.reset(new Deck());
    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "REGIONS" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST6" ) );

    deck->addKeyword( DeckKeyword( "SUMMARY" ) );
    deck->addKeyword( DeckKeyword( "TEST7" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST8" ) );

    deck->addKeyword( DeckKeyword( "EDIT" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // missing RUNSPEC
    deck.reset(new Deck());

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // missing GRID
    deck.reset(new Deck());

    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // missing PROPS
    deck.reset(new Deck());

    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // missing SOLUTION
    deck.reset(new Deck());

    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck->addKeyword( DeckKeyword( "TEST5" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));

    // missing SCHEDULE
    deck.reset(new Deck());

    deck->addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck->addKeyword( DeckKeyword( "TEST1" ) );

    deck->addKeyword( DeckKeyword( "GRID" ) );
    deck->addKeyword( DeckKeyword( "TEST2" ) );

    deck->addKeyword( DeckKeyword( "PROPS" ) );
    deck->addKeyword( DeckKeyword( "TEST3" ) );

    deck->addKeyword( DeckKeyword( "SOLUTION" ) );
    deck->addKeyword( DeckKeyword( "TEST4" ) );

    BOOST_CHECK(!Opm::Section::checkSectionTopology(*deck));
}
