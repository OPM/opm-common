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

#include <stdexcept>
#include <string>
#include <vector>

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

inline Opm::Schedule loadSchedule(const std::string& deck_file, const std::string& inputSkipMode)
{
    Opm::ParseContext parseContext({{Opm::ParseContext::PARSE_RANDOM_SLASH, Opm::InputErrorAction::IGNORE},
                                    {Opm::ParseContext::PARSE_MISSING_DIMS_KEYWORD, Opm::InputErrorAction::WARN},
                                    {Opm::ParseContext::SUMMARY_UNKNOWN_WELL, Opm::InputErrorAction::WARN},
                                    {Opm::ParseContext::SUMMARY_UNKNOWN_GROUP, Opm::InputErrorAction::WARN}});
    parseContext.setInputSkipMode(inputSkipMode);
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

void print_help_and_exit()
{
    const char *help_text = R"(Usage: plot_ms_wells [--input-skip-mode=<mode>] <deck_file> [deck_file ...]

Description:
  Reads reservoir simulation deck(s), parses Multi-Segment Well (MSW) structures,
  and generates Graphviz (.gv) files for each multi-segment well for visualization.
  Each .gv file can be converted to PDF or PNG using Graphviz tools (e.g. dot).

Options:
  -h, --help    Display this help message and exit.
  --input-skip-mode=<mode>
                Which of SKIP100/ENDSKIP and SKIP300/ENDSKIP blocks to ignore
                when parsing.  One of '100' (default), '300' (e.g. for
                compositional runs), or 'all'.

Example:
  plot_ms_wells MSW.DATA
  plot_ms_wells --input-skip-mode=300 COMPOSITIONAL_MSW.DATA
)";
    std::cerr << help_text;
}


int main(int argc, char** argv)
{
    std::string inputSkipMode{"100"};
    std::vector<std::string> deck_files;

    try {
        for (int iarg = 1; iarg < argc; ++iarg) {
            const std::string arg = argv[iarg];
            if (arg == "-h" || arg == "--help") {
                print_help_and_exit();
                std::exit(EXIT_SUCCESS);
            }
            else if (arg.starts_with("--input-skip-mode=")) {
                inputSkipMode = arg.substr(arg.find('=') + 1);
            }
            else if (arg == "--input-skip-mode") {
                if (++iarg == argc) {
                    throw std::invalid_argument { "Missing argument for --input-skip-mode" };
                }
                inputSkipMode = argv[iarg];
            }
            else {
                deck_files.push_back(arg);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << '\n';
        std::exit(EXIT_FAILURE);
    }

    if (deck_files.empty()) {
        print_help_and_exit();
        std::exit(EXIT_FAILURE);
    }

    std::ostringstream os;
    std::shared_ptr<Opm::StreamLog> string_log = std::make_shared<Opm::StreamLog>(os, Opm::Log::DefaultMessageTypes);
    Opm::OpmLog::addBackend( "STRING" , string_log);
    try {
        for (const auto& filename : deck_files) {
            const auto sched = loadSchedule(filename, inputSkipMode);
            createDot(sched);
        }
    } catch (const std::exception& e) {
        std::cerr << "\n\n***** Caught an exception: " << e.what() << std::endl;
        std::cerr << "\n\n***** Printing log: "<< std::endl;
        std::cerr << os.str();
        std::cerr << "\n\n***** Exiting due to errors." << std::endl;
        return EXIT_FAILURE;
    }
}
