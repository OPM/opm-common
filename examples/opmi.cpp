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
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>


void initLogging() {
    std::shared_ptr<Opm::StreamLog> cout_log = std::make_shared<Opm::StreamLog>(std::cout, Opm::Log::DefaultMessageTypes);
    Opm::OpmLog::addBackend( "COUT" , cout_log);
}

inline void loadDeck( const char * deck_file) {
    Opm::ParseContext parseContext;
    Opm::ErrorGuard errors;
    Opm::Parser parser;

    std::cout << "Loading deck: " << deck_file << " ..... "; std::cout.flush();
    auto deck = parser.parseFile(deck_file, parseContext, errors);
    std::cout << "parse complete - creating EclipseState .... ";  std::cout.flush();
    Opm::EclipseState state( deck, parseContext, errors );
    Opm::Schedule schedule( deck, state.getInputGrid(), state.get3DProperties(), state.runspec(), parseContext, errors);
    Opm::SummaryConfig summary( deck, schedule, state.getTableManager( ), parseContext, errors );
    std::cout << "complete." << std::endl;
}


int main(int argc, char** argv) {
    initLogging();
    for (int iarg = 1; iarg < argc; iarg++)
        loadDeck( argv[iarg] );
}

