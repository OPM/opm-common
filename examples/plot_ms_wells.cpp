/*
  Copyright 2026 Equinor ASA.

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
#include <sstream>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>

#include <opm/utility/WellStructureViz.hpp>

#include <fmt/format.h>

inline void createDot(const Opm::Schedule& schedule)
{
    // the following is up to adjustment to specify the report step or specific wells
    for (const auto& wellname : schedule.wellNames()) {
        const auto& well = schedule.getWellatEnd(wellname);
        if (well.isMultiSegment()) {
            const auto& segments = well.getSegments();
            const auto& connections = well.getConnections();
            Opm::writeWellStructure(wellname, segments, connections);
            std::cout << fmt::format("Wrote well structure for well '{0}' to file '{0}.gv'.\n", wellname);
            std::cout << fmt::format("Convert output to PDF or PNG with 'dot -Tpdf {0}.gv -o {0}.pdf' or 'dot -Tpng {0}.gv -o {0}.png'\n", wellname);
        }
    }
}

inline Opm::Schedule loadSchedule(const std::string& deck_file)
{
    Opm::ParseContext parseContext({{Opm::ParseContext::PARSE_RANDOM_SLASH, Opm::InputErrorAction::IGNORE},
                                    {Opm::ParseContext::PARSE_MISSING_DIMS_KEYWORD, Opm::InputErrorAction::WARN},
                                    {Opm::ParseContext::SUMMARY_UNKNOWN_WELL, Opm::InputErrorAction::WARN},
                                    {Opm::ParseContext::SUMMARY_UNKNOWN_GROUP, Opm::InputErrorAction::WARN}});
    Opm::ErrorGuard errors;
    Opm::Parser parser;
    auto python = std::make_shared<Opm::Python>();

    std::cout << fmt::format("Loading and parsing deck: {} ..... ", deck_file);  std::cout.flush();
    auto deck = parser.parseFile(deck_file, parseContext, errors);
    std::cout << "complete.\n";

    std::cout << "Creating EclipseState .... ";  std::cout.flush();
    Opm::EclipseState state( deck );
    std::cout << "complete.\n";

    std::cout << "Creating Schedule .... ";  std::cout.flush();
    Opm::Schedule schedule( deck, state, python);
    std::cout << "complete." << std::endl;

    return schedule;
}


int main(int argc, char** argv)
{
    std::ostringstream os;
    std::shared_ptr<Opm::StreamLog> string_log = std::make_shared<Opm::StreamLog>(os, Opm::Log::DefaultMessageTypes);
    Opm::OpmLog::addBackend( "STRING" , string_log);
    try {
        for (int iarg = 1; iarg < argc; iarg++) {
            const std::string filename = argv[iarg];
            const auto sched = loadSchedule(filename);
            createDot(sched);
        }
    } catch (const std::exception& e) {
        std::cerr << "\n\n***** Caught an exception: " << e.what() << std::endl;
        std::cerr << "\n\n***** Printing log: "<< std::endl;
        std::cerr << os.str();
        std::cerr << "\n\n***** Exiting due to errors." << std::endl;
    }
}
