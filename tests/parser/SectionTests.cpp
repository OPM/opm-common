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
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE(SectionTest) {
    Deck deck;
    deck.addKeyword( DeckKeyword("TEST0") );
    deck.addKeyword( DeckKeyword("RUNSPEC") );
    deck.addKeyword( DeckKeyword("TEST1") );
    deck.addKeyword( DeckKeyword("GRID") );
    deck.addKeyword( DeckKeyword("TEST2") );
    deck.addKeyword( DeckKeyword("SCHEDULE") );
    deck.addKeyword( DeckKeyword("TEST3") );

    Section runspecSection(deck, "RUNSPEC");
    Section gridSection(deck, "GRID");
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
    Deck deck;
    deck.addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck.addKeyword( DeckKeyword("TEST2") );
    deck.addKeyword( DeckKeyword( "TEST3" ) );
    deck.addKeyword( DeckKeyword( "GRID" ) );
    Section section(deck, "RUNSPEC");

    int numberOfItems = 0;
    for (auto iter=section.begin(); iter != section.end(); ++iter) {
        std::cout << iter->name() << std::endl;
        numberOfItems++;
    }

    // the keywords expected here are RUNSPEC, TEST2 and TEST3...
    BOOST_CHECK_EQUAL(3, numberOfItems);
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_EmptyDeck) {
    Deck deck;
    BOOST_REQUIRE_NO_THROW(RUNSPECSection section(deck));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSimpleDeck) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "TEST1") );
    deck.addKeyword( DeckKeyword( "RUNSPEC") );
    deck.addKeyword( DeckKeyword( "TEST2") );
    deck.addKeyword( DeckKeyword( "TEST3") );
    deck.addKeyword( DeckKeyword( "GRID") );
    deck.addKeyword( DeckKeyword( "TEST4") );

    RUNSPECSection section(deck);
    BOOST_CHECK(!section.hasKeyword("TEST1"));
    BOOST_CHECK(section.hasKeyword("RUNSPEC"));
    BOOST_CHECK(section.hasKeyword("TEST2"));
    BOOST_CHECK(section.hasKeyword("TEST3"));
    BOOST_CHECK(!section.hasKeyword("GRID"));
    BOOST_CHECK(!section.hasKeyword("TEST4"));
}

BOOST_AUTO_TEST_CASE(RUNSPECSection_ReadSmallestPossibleDeck) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "RUNSPEC" ) );
    deck.addKeyword( DeckKeyword( "GRID") );
    RUNSPECSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("RUNSPEC"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("GRID"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByEDITKeyword) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "GRID" ) );
    deck.addKeyword( DeckKeyword( "EDIT" ) );
    GRIDSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("EDIT"));
}

BOOST_AUTO_TEST_CASE(GRIDSection_TerminatedByPROPSKeyword) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "GRID" ) );
    deck.addKeyword( DeckKeyword( "PROPS" ) );
    GRIDSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("GRID"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(EDITSection_TerminatedByPROPSKeyword) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "EDIT" ) );
    deck.addKeyword( DeckKeyword( "PROPS" ) );
    EDITSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("EDIT"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("PROPS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedByREGIONSKeyword) {
    Deck deck;
    deck.addKeyword( DeckKeyword( "PROPS" ) );
    deck.addKeyword( DeckKeyword( "REGIONS" ) );
    PROPSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("REGIONS"));
}

BOOST_AUTO_TEST_CASE(PROPSSection_TerminatedBySOLUTIONKeyword) {
    Deck deck;

    deck.addKeyword( DeckKeyword( "PROPS" ) );
    deck.addKeyword( DeckKeyword( "SOLUTION" ) );

    PROPSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("PROPS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(REGIONSSection_TerminatedBySOLUTIONKeyword) {
    Deck deck;

    deck.addKeyword( DeckKeyword( "REGIONS" ) );
    deck.addKeyword( DeckKeyword( "SOLUTION" ) );

    REGIONSSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("REGIONS"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SOLUTION"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySUMMARYKeyword) {
    Deck deck;

    deck.addKeyword( DeckKeyword( "SOLUTION" ) );
    deck.addKeyword( DeckKeyword( "SUMMARY" ) );

    SOLUTIONSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SUMMARY"));
}

BOOST_AUTO_TEST_CASE(SOLUTIONSection_TerminatedBySCHEDULEKeyword) {
    Deck deck;

    deck.addKeyword( DeckKeyword( "SOLUTION" ) );
    deck.addKeyword( DeckKeyword( "SCHEDULE" ) );

    SOLUTIONSection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SOLUTION"));
    BOOST_CHECK_EQUAL(false, section.hasKeyword("SCHEDULE"));
}

BOOST_AUTO_TEST_CASE(SCHEDULESection_NotTerminated) {
    Deck deck;

    deck.addKeyword( DeckKeyword( "SCHEDULE" ) );
    deck.addKeyword( DeckKeyword( "TEST1" ) );
    deck.addKeyword( DeckKeyword( "TEST2" ) );
    deck.addKeyword( DeckKeyword( "TEST3" ) );
    deck.addKeyword( DeckKeyword( "TEST4" ) );

    SCHEDULESection section(deck);
    BOOST_CHECK_EQUAL(true, section.hasKeyword("SCHEDULE"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST1"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST2"));
    BOOST_CHECK_EQUAL(true, section.hasKeyword("TEST3"));

    BOOST_CHECK( Section::hasSCHEDULE(deck ));
    BOOST_CHECK( !Section::hasREGIONS(deck ));
}

BOOST_AUTO_TEST_CASE(Section_ValidDecks) {

    ParseContext mode = { { { ParseContext::PARSE_UNKNOWN_KEYWORD, InputError::IGNORE } } };
    Parser parser;

    const std::string minimal = "RUNSPEC\n"
                                "TEST1\n"
                                "GRID\n"
                                "TEST2\n"
                                "PROPS\n"
                                "TEST3\n"
                                "SOLUTION\n"
                                "TEST4\n"
                                "SCHEDULE\n"
                                "TEST5\n";

    BOOST_CHECK( Opm::Section::checkSectionTopology( parser.parseString( minimal, mode ), parser) );

    const std::string with_opt = "RUNSPEC\n"
                                "TEST1\n"
                                "GRID\n"
                                "TEST2\n"
                                "EDIT\n"
                                "TEST3\n"
                                "PROPS\n"
                                "TEST4\n"
                                "REGIONS\n"
                                "TEST5\n"
                                "SOLUTION\n"
                                "TEST6\n"
                                "SUMMARY\n"
                                "TEST7\n"
                                "SCHEDULE\n"
                                "TEST8\n";

    BOOST_CHECK(Opm::Section::checkSectionTopology( parser.parseString( with_opt, mode ), parser));
}

BOOST_AUTO_TEST_CASE(Section_InvalidDecks) {

    Parser parser;
    ParseContext mode = { { { ParseContext::PARSE_UNKNOWN_KEYWORD, InputError::IGNORE } } };

    const std::string keyword_before_RUNSPEC =
                                "WWCT \n /\n"
                                "RUNSPEC\n"
                                "TEST1\n"
                                "GRID\n"
                                "TEST2\n"
                                "PROPS\n"
                                "TEST3\n"
                                "SOLUTION\n"
                                "TEST4\n"
                                "SCHEDULE\n"
                                "TEST5\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( keyword_before_RUNSPEC, mode ), parser));

    const std::string wrong_order = "RUNSPEC\n"
                                    "TEST1\n"
                                    "EDIT\n"
                                    "TEST3\n"
                                    "GRID\n"
                                    "TEST2\n"
                                    "PROPS\n"
                                    "TEST4\n"
                                    "REGIONS\n"
                                    "TEST5\n"
                                    "SOLUTION\n"
                                    "TEST6\n"
                                    "SUMMARY\n"
                                    "TEST7\n"
                                    "SCHEDULE\n"
                                    "TEST8\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( wrong_order, mode ), parser));

    const std::string duplicate = "RUNSPEC\n"
                                  "TEST1\n"
                                  "GRID\n"
                                  "TEST2\n"
                                  "GRID\n"
                                  "TEST21\n"
                                  "EDIT\n"
                                  "TEST3\n"
                                  "PROPS\n"
                                  "TEST4\n"
                                  "REGIONS\n"
                                  "TEST5\n"
                                  "SOLUTION\n"
                                  "TEST6\n"
                                  "SUMMARY\n"
                                  "TEST7\n"
                                  "SCHEDULE\n"
                                  "TEST8\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( duplicate, mode ), parser));

    const std::string section_after_SCHEDULE = "RUNSPEC\n"
                                               "TEST1\n"
                                               "GRID\n"
                                               "TEST2\n"
                                               "PROPS\n"
                                               "TEST4\n"
                                               "REGIONS\n"
                                               "TEST5\n"
                                               "SOLUTION\n"
                                               "TEST6\n"
                                               "SUMMARY\n"
                                               "TEST7\n"
                                               "SCHEDULE\n"
                                               "TEST8\n"
                                               "EDIT\n"
                                               "TEST3\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( section_after_SCHEDULE, mode ), parser));

    const std::string missing_runspec = "GRID\n"
                                        "TEST2\n"
                                        "PROPS\n"
                                        "TEST3\n"
                                        "SOLUTION\n"
                                        "TEST4\n"
                                        "SCHEDULE\n"
                                        "TEST5\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( missing_runspec, mode ), parser));


    const std::string missing_GRID = "RUNSPEC\n"
                                     "TEST1\n"
                                     "PROPS\n"
                                     "TEST3\n"
                                     "SOLUTION\n"
                                     "TEST4\n"
                                     "SCHEDULE\n"
                                     "TEST5\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( missing_GRID, mode ), parser));

   const std::string missing_PROPS = "RUNSPEC\n"
                                     "TEST1\n"
                                     "GRID\n"
                                     "TEST2\n"
                                     "SOLUTION\n"
                                     "TEST4\n"
                                     "SCHEDULE\n"
                                     "TEST5\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( missing_PROPS, mode ), parser));

    const std::string missing_SOLUTION = "RUNSPEC\n"
                                         "TEST1\n"
                                         "GRID\n"
                                         "TEST2\n"
                                         "PROPS\n"
                                         "TEST3\n"
                                         "SCHEDULE\n"
                                         "TEST5\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( missing_SOLUTION, mode ), parser));

    const std::string missing_SCHEDULE = "RUNSPEC\n"
                                         "TEST1\n"
                                         "GRID\n"
                                         "TEST2\n"
                                         "PROPS\n"
                                         "TEST3\n"
                                         "SOLUTION\n"
                                         "TEST4\n";

    BOOST_CHECK(!Opm::Section::checkSectionTopology( parser.parseString( missing_SCHEDULE, mode ), parser));
}
