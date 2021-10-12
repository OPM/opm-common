/*
  Copyright 2021 Statoil ASA.

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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <getopt.h>
#include <fmt/format.h>
#include <unordered_set>

#include <opm/parser/eclipse/Deck/DeckValue.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/common/utility/FileSystem.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/I.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/FileDeck.hpp>

namespace fs = Opm::filesystem;

const std::unordered_set<std::string> remove_from_solution = {"EQUIL", "PRESSURE", "SWAT", "SGAS"};
const std::unordered_set<std::string> keep_in_solution = {"ABC"};

void print_help_and_exit(const std::optional<std::string> error_msg = {}) {

    if (error_msg.has_value()) {
        std::cerr << "Error:" << std::endl;
        std::cerr << error_msg.value() << std::endl;
        std::cerr << "------------------------------------------------------" << std::endl;
    }

    std::string keep_keywords;
    for (const auto& kw : keep_in_solution)
        keep_keywords += kw + " ";

    const std::string help_text = fmt::format(R"(

The rst_deck program will load a simulation deck and parameters for a restart
and reformat the deck to become a restart deck. Before the updated deck is
output the program will update the SOLUTION and SCHEDULE sections. All keywords
from the SOLUTION section will be cleared out(1) and a RESTART keyword will be
inserted. In the SCHEDULE section the program can either remove all keywords up
until the restart date, or alternatively insert SKIPREST immediately following
the SCHEDULE keyword(2).

When creating the updated restart deck the program can either link to unmodified
include files with INCLUDE statements, create a copy of deck structure in an
alternative location or create one large file with all keywords in the same
file. Apart from the alterations to support restart the output deck will be
equivalent to the input deck, but formatting is not retained and comments have
been stripped away.

Arguments:

1. The data file we are starting with.

2. The basename of the restart file - with an optional path prefix and a :N to
   restart from step N(3). A restart step value of 0 is interpreted as a dry run
   - a deck which has not been set up for restart will be written out.

3. Basename of the restart deck we create, can optionally contain a path prefix;
   the path will be created if it does not already exist. This argument is
   optional, if it is not provided the program will dump a restart deck on
   stdout. If the argument corresponds to an existing directory the restart case
   will get the same name as the base case.

Options:

-s: Manipulate the SCHEDULE section by inserting a SKIPREST keyword immediately
    following the SCHEDULE keyword. If the -s option is not used the SCHEDULE
    section will be modified by removing all keywords until we reach the restart
    date. NB: Currently the -s option is required

-m: [share|inline|copy] The restart deck can reuse the unmodified include files
    from the base case, this is mode 'share' and is the default. With mode
    'inline' the restart deck will be one long file and with mode 'copy' the
    file structure of the base case will be retained. The default if no -m
    option is given is the 'share' mode.

    In the case of 'share' and 'copy' the correct path to include files will be
    negotiated based on the path given to the output case in the third argument.
    If the restart deck is passed to stdout the include files will be resolved
    based on output in cwd. 

Example:

   rst_deck /path/to/history/HISTORY.DATA rst/HISTORY:30 /path/to/rst/RESTART -s

1: The program has a compiled list of keywords which will be retained in the
   SOLUTION section. The current value of that list is: {}

2: Current version of the program *only* supports the SKIPREST option, and the
   -s option is required.

3: The second argument is treated purely as a string and inserted verbatim into
   the updated restart deck. In a future version we might interpret the second
   argument as a file path and check the content and also do filesystem
   manipulations from it.

)", keep_keywords);

    std::cerr << help_text << std::endl;
    if (error_msg.has_value())
        std::exit(EXIT_FAILURE);

    std::exit(EXIT_SUCCESS);
}



struct options {
    std::string input_deck;
    std::pair<std::string, int> restart;
    std::optional<std::string> target;

    Opm::FileDeck::OutputMode mode{Opm::FileDeck::OutputMode::SHARE};
    bool skiprest{false};
};


Opm::FileDeck load_deck(const options& opt) {
    Opm::ParseContext parseContext(Opm::InputError::WARN);
    Opm::ErrorGuard errors;
    Opm::Parser parser;

    /* Use the same default ParseContext as flow. */
    parseContext.update(Opm::ParseContext::PARSE_RANDOM_SLASH, Opm::InputError::IGNORE);
    parseContext.update(Opm::ParseContext::PARSE_MISSING_DIMS_KEYWORD, Opm::InputError::WARN);
    parseContext.update(Opm::ParseContext::SUMMARY_UNKNOWN_WELL, Opm::InputError::WARN);
    parseContext.update(Opm::ParseContext::SUMMARY_UNKNOWN_GROUP, Opm::InputError::WARN);
    auto deck = parser.parseFile(opt.input_deck, parseContext, errors);
    return Opm::FileDeck{ deck };
}


Opm::FileDeck::OutputMode mode(const std::string& mode_arg) {
    if (mode_arg == "inline")
        return Opm::FileDeck::OutputMode::INLINE;

    if (mode_arg == "share")
        return Opm::FileDeck::OutputMode::SHARE;

    if (mode_arg == "copy")
        return Opm::FileDeck::OutputMode::COPY;

    print_help_and_exit(fmt::format("Mode argument: \'{}\' not recognized. Valid options are inline|share|copy", mode_arg));
    return Opm::FileDeck::OutputMode::INLINE;
}

std::pair<std::string, std::size_t> split_restart(const std::string& restart_base) {
    auto sep_pos = restart_base.rfind(':');
    if (sep_pos == std::string::npos)
        print_help_and_exit(fmt::format("Expected restart argument on the form: BASE:NUMBER - e.g. HISTORY:60"));

    return std::make_pair(restart_base.substr(0, sep_pos), std::stoi(restart_base.substr(sep_pos + 1)));
}


options load_options(int argc, char **argv) {
    options opt;
    while (true) {
        int c;
        c = getopt(argc, argv, "hm:s");
        if (c == -1)
            break;

        switch(c) {
        case 'm':
            opt.mode = mode(optarg);
            break;
        case 's':
            opt.skiprest = true;
            break;
        case 'h':
            print_help_and_exit();
            break;
        }
    }

    auto arg_offset = optind;
    if (arg_offset >= argc)
        print_help_and_exit();

    opt.input_deck = argv[arg_offset];
    opt.restart = split_restart(argv[arg_offset + 1]);
    if ((argc - arg_offset) >= 3) {
        opt.target = argv[arg_offset + 2];
        if (opt.mode == Opm::FileDeck::OutputMode::COPY) {
            auto target = fs::path(opt.target.value()).parent_path();
            if (fs::exists(target)) {
                auto input = fs::path(opt.input_deck).parent_path();
                if (fs::equivalent(target, input))
                    opt.mode = Opm::FileDeck::OutputMode::SHARE;
            }
        }
    } else {
        if (opt.mode == Opm::FileDeck::OutputMode::COPY)
            print_help_and_exit("When writing output to stdout you must use inline|share mode");
    }

    return opt;
}


void update_solution(const options& opt, Opm::FileDeck& file_deck)
{
    if (opt.restart.second == 0)
        return;

    const auto solution = file_deck.find("SOLUTION");
    if (!solution.has_value())
        print_help_and_exit(fmt::format("Could not find SOLUTION section in input deck: {}", opt.input_deck));

    auto summary = file_deck.find("SUMMARY");
    if (!summary.has_value())
        print_help_and_exit(fmt::format("Could not find SUMMARY section in input deck: {}", opt.input_deck));

    auto index = solution.value();
    index++;
    {
        while (true) {
            const auto& keyword = file_deck[index];
            if (keep_in_solution.count(keyword.name()) == 0) {
                file_deck.erase(index);
                summary.value()--;
            } else
                index++;

            if (index == summary)
                break;
        }
    }

    {
        Opm::UnitSystem units;
        std::vector<Opm::DeckValue> values{ Opm::DeckValue{opt.restart.first}, Opm::DeckValue{opt.restart.second} };
        Opm::DeckKeyword restart(Opm::ParserKeywords::RESTART(), std::vector<std::vector<Opm::DeckValue>>{ values }, units, units);

        auto schedule = file_deck.find("SCHEDULE");
        file_deck.insert(++schedule.value(), restart);
    }
}


void update_schedule(const options& opt, Opm::FileDeck& file_deck)
{
    if (opt.restart.second == 0)
        return;

    const auto schedule = file_deck.find("SCHEDULE");
    if (opt.skiprest) {
        Opm::DeckKeyword skiprest( Opm::ParserKeywords::SKIPREST{} );

        auto index = schedule.value();
        file_deck.insert(++index, skiprest);
    }
}


int main(int argc, char** argv) {
    auto options = load_options(argc, argv);
    auto file_deck = load_deck(options);
    update_solution(options, file_deck);
    update_schedule(options, file_deck);
    if (!options.target.has_value())
        file_deck.dump_stdout(fs::current_path(), options.mode);
    else {
        std::string target = options.target.value();
        if (fs::is_directory(target))
            file_deck.dump( fs::absolute(target), fs::path(options.input_deck).filename(), options.mode );
        else {
            auto target_path = fs::path( fs::absolute(options.target.value()) );
            file_deck.dump( fs::absolute(target_path.parent_path()), target_path.filename(), options.mode );
        }
    }
}

