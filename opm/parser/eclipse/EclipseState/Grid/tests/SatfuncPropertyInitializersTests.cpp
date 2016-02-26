/*
  Copyright 2015 IRIS
  
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

#define BOOST_TEST_MODULE SatfuncPropertyInitializersTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>

using namespace Opm;

void check_property(EclipseState eclState1, EclipseState eclState2,  const std::string& propertyName) {
     const std::vector<double> data1 = eclState1.getDoubleGridProperty(propertyName)->getData();
     const std::vector<double> data2 = eclState2.getDoubleGridProperty(propertyName)->getData();

     BOOST_CHECK_CLOSE(data1[0], data2[0],1e-12);

}

BOOST_AUTO_TEST_CASE(SaturationFunctionFamilyTests) {
    const char * deckdefault =
            "RUNSPEC\n"
            "OIL\n"
            "GAS\n"
            "WATER\n"
            "DIMENS\n"
            " 1 1 1 /\n"
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "GRID\n"
            "DX\n"
            "1*0.25 /\n"
            "DYV\n"
            "1*0.25 /\n"
            "DZ\n"
            "1*0.25 /\n"
            "TOPS\n"
            "1*0.25 /\n"
            "PORO \n"
            "1*0.10 /\n"
            "PERMX \n"
            "10.25 /\n";

    const char *family1 =
        "SWOF\n"
        " .2  .0 1.0 .0\n"
        " .3  .0  .8 .0\n"
        " .5  .5  .5 .0\n"
        " .8  .8  .0 .0\n"
        " 1.0 1.0 .0 .0 /\n"
        "SGOF\n"
        " .0  .0 1.0 .0\n"
        " .1  .0  .3 .0\n"
        " .5  .5  .1 .0\n"
        " .7  .8  .0 .0\n"
        " .8 1.0  .0 .0/\n";

    const char *family2 =
        "SWFN\n"
        " .2  .0  .0\n"
        " .3  .0  .0\n"
        " .5  .5  .0\n"
        " .8  .8  .0\n"
        " 1.0 1.0 .0 /\n"
        "SGFN\n"
        " .0  .0  .0\n"
        " .1  .0  .0\n"
        " .5  .5  .0\n"
        " .7  .8  .0\n"
        " .8 1.0  .0/\n"
        "SOF3\n"
        " .0  .0  .0\n"
        " .2  .0  .0\n"
        " .3  1*  .0\n"
        " .5  .5  .1\n"
        " .7  .8  .3\n"
        " .8 1.0  1.0/\n";

    ParseMode parseMode;
    ParserPtr parser(new Parser());

    char family1Deck[500] = " ";
    strcat(family1Deck , deckdefault);
    strcat(family1Deck , family1);
    DeckPtr deck1 = parser->parseString(family1Deck, parseMode) ;
    EclipseState state1(deck1, parseMode);


    char family2Deck[700] = " ";
    strcat(family2Deck , deckdefault);
    strcat(family2Deck , family2);
    DeckPtr deck2 = parser->parseString(family2Deck, parseMode) ;
    EclipseState state2(deck2, parseMode);

    check_property(state1, state2, "SWL");
    check_property(state1, state2, "SWU");
    check_property(state1, state2, "SWCR");

    check_property(state1, state2, "SGL");
    check_property(state1, state2, "SGU");
    check_property(state1, state2, "SGCR");

    check_property(state1, state2, "SOWCR");
    check_property(state1, state2, "SOGCR");

    check_property(state1, state2, "PCW");
    check_property(state1, state2, "PCG");

    check_property(state1, state2, "KRW");
    check_property(state1, state2, "KRO");
    check_property(state1, state2, "KRG");

    char familyMixDeck[1000] = " ";
    strcat(familyMixDeck , deckdefault);
    strcat(familyMixDeck , family1);
    strcat(familyMixDeck , family2);
    DeckPtr deckMix = parser->parseString(familyMixDeck, parseMode) ;
    EclipseState stateMix(deckMix, parseMode);
    BOOST_CHECK_THROW(stateMix.getDoubleGridProperty("SGCR") , std::invalid_argument);

}


