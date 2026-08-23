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

#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>

#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/input/eclipse/EclipseState/InitConfig/InitConfig.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/utility/GroupStructureViz.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <fmt/format.h>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    struct DataFile
    {
        std::filesystem::path path{};
    };

    struct Arguments
    {
        std::vector<DataFile> data_files{};
        bool separateWellGroups{false};
        bool helpoption{false};
    };

    Opm::RestartIO::RstState
    loadRstState(std::shared_ptr<Opm::EclIO::ERst> rst_file,
                 const int                         report_step,
                 const Opm::Parser&                parser,
                 std::optional<int>                numPVTTables = std::nullopt)
    {
        auto rst_view = std::make_shared<Opm::EclIO::RestartFileView>
            (std::move(rst_file), report_step);

        // Note: We ignore load()'s grid argument here.  For the purposes
        // of "wellgraph", we don't need to care about aquifers.
        return Opm::RestartIO::RstState::
            load(std::move(rst_view), parser, numPVTTables);
    }

    Opm::RestartIO::RstState
    loadRstState(const std::string&  rst_filename,
                 const int           report_step,
                 const Opm::Parser&  parser,
                 std::optional<int>  numPVTTables = std::nullopt)
    {
        return loadRstState(std::make_shared<Opm::EclIO::ERst>(rst_filename),
                            report_step, parser, numPVTTables);
    }

    Opm::Schedule loadSchedule(const Opm::Deck&             deck,
                               const Opm::EclipseState&     state,
                               std::shared_ptr<Opm::Python> python,
                               const Opm::Parser&           parser,
                               const Opm::ParseContext&     parseContext,
                               Opm::ErrorGuard&             errorGuard)
    {
        const auto& initConfig = state.cfg().init();

        if (! initConfig.restartRequested()) {
            // Not a restarted run.  Common case.
            return { deck, state, parseContext, errorGuard, std::move(python) };
        }

        // If we get here, we're processing a restarted run.  Load schedule objects
        // (wells, groups, &c) from the restart file in addition to the .DATA file.

        const auto report_step = initConfig.getRestartStep();
        const auto rst_filename = state.getIOConfig()
            .getRestartFileName(initConfig.getRestartRootName(),
                                report_step, /* output = */ false);

        const auto rst_state =
            loadRstState(rst_filename, report_step, parser,
                         state.runspec().tabdims().getNumPVTTables());

        return {
            deck, state, parseContext, errorGuard, std::move(python),
            /* lowActionParsingStrictness = */ true,
            /* slave_mode = */                 false,
            /* keepKeywords = */               false,
            /* outputInterval = */             std::nullopt,
            &rst_state
        };
    }

    Opm::Schedule loadSchedule(const std::filesystem::path& deck_file,
                               const Opm::Parser&           parser,
                               std::shared_ptr<Opm::Python> python)
    {
        std::cout << "Loading and parsing deck: " << deck_file.generic_string() << " ..... ";
        std::cout.flush();

        Opm::ParseContext parseContext{};
        Opm::ErrorGuard errors{};

        parseContext.update(Opm::InputErrorAction::WARN);

        const auto deck = parser.parseFile(deck_file.generic_string(), parseContext, errors);
        std::cout << "complete.\n";

        std::cout << "Creating EclipseState .... ";
        std::cout.flush();
        const auto state = Opm::EclipseState{deck};
        std::cout << "complete.\n";

        std::cout << "Creating Schedule .... ";
        std::cout.flush();
        auto sched = loadSchedule(deck, state, std::move(python),
                                  parser, parseContext, errors);
        std::cout << "complete." << std::endl;

        errors.clear();

        return sched;
    }

    Arguments parseArguments(const int argc, char** argv)
    {
        Arguments args{};

        auto iarg = 1;

        while (iarg < argc) {
            const std::string_view arg = argv[iarg];

            if (arg == "--separate-well-groups") {
                args.separateWellGroups = true;
            }
            else if (arg == "-h" || arg == "--help") {
                args.helpoption = true;
            }
            else if (!arg.empty() && (arg.front() != '-')) {
                args.data_files.push_back({ arg });
            }
            else {
                throw std::invalid_argument {
                    fmt::format("Unrecognized argument: '{0}'", arg)
                };
            }

            ++iarg;
        }

        return args;
    }

    void print_help()
    {
        const char *const help_text = R"(Usage: wellgraph [--separate-well-groups] <deck_file> [<deck_file> ...]

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

    void processDataFiles(const Arguments&             args,
                          const Opm::Parser&           parser,
                          std::shared_ptr<Opm::Python> python)
    {
        for (const auto& data_file : args.data_files) {
            const auto casename = data_file.path.stem().generic_string();
            const auto sched = loadSchedule(data_file.path, parser, python);

            Opm::writeWellGroupGraph(sched, casename, args.separateWellGroups);
        }
    }

} // Anonymous namespace

int main(int argc, char** argv)
{
    const auto args = [argc, argv]() -> Arguments {
        try {
            return parseArguments(argc, argv);
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing arguments: " << e.what() << '\n';
            return {};
        }
    }();

    if (args.helpoption || args.data_files.empty()) {
        print_help();
        return args.helpoption ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    std::ostringstream os;
    Opm::OpmLog::addBackend("STRING", std::make_shared<Opm::StreamLog>(os, Opm::Log::DefaultMessageTypes));

    try {
        auto python = std::make_shared<Opm::Python>();
        auto parser = Opm::Parser{python};

        processDataFiles(args, parser, std::move(python));
    }
    catch (const std::exception& e) {
        std::cerr << "\n\n***** Caught an exception: " << e.what() << '\n'
                  << "\n\n***** Printing log: " << '\n'
                  << os.str()
                  << "\n\n***** Exiting due to errors.\n";

        return EXIT_FAILURE;
    }
}
