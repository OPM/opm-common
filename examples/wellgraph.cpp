/*
  Copyright 2013, 2020 Equinor ASA.

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

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <chrono>
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

#include <opm/utility/GroupStructureViz.hpp>


inline void createDot(const Opm::Schedule& schedule, const std::string& casename, const bool separateWellGroups)
{
    Opm::writeWellGroupGraph(schedule, casename, separateWellGroups);
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

    std::cout << "Loading and parsing deck: " << deck_file << " ..... "; std::cout.flush();
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


void print_help()
{
    const char *help_text = R"(Usage: wellgraph [--separate-well-groups] <deck_file> [deck_file ...]

Description:
  Reads reservoir simulation deck(s), parsing the group and well hierarchy structures,
  and generates Graphviz (.gv) files to visualize the relationships between groups and also wells.
  The .gv file can be converted to PDF or PNG using Graphviz tools (e.g. dot).
  For the cases with many groups and wells, the generated graph can be very large,
  and it is recommended to visualize the group relations and group-wells relations separately
  for better readability. This can be achieved by using --separate-well-groups option, which will
  generate two .gv files for each deck: <casename>_group_structure.gv and <casename>_well_groups.gv.

Options:
  -h, --help             Display this help message and exit.
  --separate-well-groups Generate separate graphs for group relationships and
                         group-well relationships for better readability.

Example:
  wellgraph --separate-well-groups GROUPWELL.DATA
)";
    std::cerr << help_text;
}


int main(int argc, char** argv)
{

    if (argc < 2) {
        print_help();
        std::exit(EXIT_FAILURE);
    }

    const std::string arg1 = argv[1];
    if (arg1 == "-h" || arg1 == "--help") {
        print_help();
        std::exit(EXIT_SUCCESS);
    }

    bool separateWellGroups = false;
    std::vector<std::string> files;

    for (int iarg = 1; iarg < argc; iarg++) {
        const std::string arg = argv[iarg];
        if (arg == "--separate-well-groups") {
            separateWellGroups = true;
        } else {
            files.push_back(arg);
        }
    }

    std::ostringstream os;
    std::shared_ptr<Opm::StreamLog> string_log = std::make_shared<Opm::StreamLog>(os, Opm::Log::DefaultMessageTypes);
    Opm::OpmLog::addBackend( "STRING" , string_log);
    try {
        for (const auto& filename : files) {
            const auto sched = loadSchedule(filename);
            const auto casename = std::filesystem::path(filename).stem();
            createDot(sched, casename, separateWellGroups);
        }
    } catch (const std::exception& e) {
        std::cout << "\n\n***** Caught an exception: " << e.what() << std::endl;
        std::cout << "\n\n***** Printing log: "<< std::endl;
        std::cout << os.str();
        std::cout << "\n\n***** Exiting due to errors." << std::endl;
    }
}
