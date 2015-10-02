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

#include <iostream>
#include <memory>

#include <opm/parser/eclipse/OpmLog/CounterLog.hpp>
#include <opm/parser/eclipse/OpmLog/StreamLog.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>
#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>


void loadDeck( const char * deck_file) {
    Opm::ParseMode parseMode;
    Opm::ParserPtr parser(new Opm::Parser());
    std::shared_ptr<const Opm::Deck> deck;
    std::shared_ptr<Opm::EclipseState> state;

    std::cout << "Loading deck: " << deck_file << " ..... "; std::cout.flush();
    deck = parser->parseFile(deck_file, parseMode);
    std::cout << "parse complete - creating EclipseState .... ";  std::cout.flush();
    state = std::make_shared<Opm::EclipseState>( deck , parseMode );
    std::cout << "complete." << std::endl;
}


int main(int argc, char** argv) {
    for (int iarg = 1; iarg < argc; iarg++)
        loadDeck( argv[iarg] );

    return 0;
}

