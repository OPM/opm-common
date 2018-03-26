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

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>


inline void loadDeck( const char * deck_file) {
    Opm::ParseContext parseContext;
    Opm::Parser parser;

    std::cout << "Loading deck: " << deck_file << " ..... "; std::cout.flush();
    auto deck = parser.parseFile(deck_file, parseContext);
    std::cout << "parse complete - creating EclipseState .... ";  std::cout.flush();
    Opm::EclipseState state( deck, parseContext );
    Opm::Schedule schedule( deck, state.getInputGrid(), state.get3DProperties(), state.runspec().phases(), parseContext);
    Opm::SummaryConfig summary( deck, schedule, state.getTableManager( ), parseContext );
    std::cout << "complete." << std::endl;

}


int main(int argc, char** argv) {
    for (int iarg = 1; iarg < argc; iarg++)
        loadDeck( argv[iarg] );
}

