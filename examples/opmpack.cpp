/*
  Copyright 2018 Statoil ASA.

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
#include <getopt.h>
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>


inline void pack_deck( const char * deck_file, std::ostream& os) {
    Opm::ParseContext parseContext(Opm::InputError::WARN);
    Opm::Parser parser;

    auto deck = parser.parseFile(deck_file, parseContext);
    os << deck;

}


void print_help_and_exit() {
    const char * help_text = R"(
The opmpack program will load a deck, resolve all include
files and then print it out again on stdout. All comments
will be stripped and the value types will be validated.

By passing the option -o you can redirect the output to a file
or a directory.

Print on stdout:

   opmpack  /path/to/case/CASE.DATA


Print MY_CASE.DATA in /tmp:

    opmpack -o /tmp /path/to/MY_CASE.DATA


Print NEW_CASE in cwd:

    opmpack -o NEW_CASE.DATA path/to/MY_CASE.DATA


)";
    std::cerr << help_text << std::endl;
    exit(1);
}


int main(int argc, char** argv) {
    int arg_offset = 1;
    bool stdout_output = true;
    const char * coutput_arg;

    while (true) {
        int c;
        c = getopt(argc, argv, "o:");
        if (c == -1)
            break;

        switch(c) {
        case 'o':
            stdout_output = false;
            coutput_arg = optarg;
            break;
        }
    }
    arg_offset = optind;
    if (arg_offset >= argc)
        print_help_and_exit();

    if (stdout_output)
        pack_deck(argv[arg_offset], std::cout);
    else {
        std::ofstream os;
        using path = boost::filesystem::path;
        path input_arg(argv[arg_offset]);
        path output_arg(coutput_arg);
        if (boost::filesystem::is_directory(output_arg)) {
            path output_path = output_arg / input_arg.filename();
            os.open(output_path.string());
        } else
            os.open(output_arg.string());

        pack_deck(argv[arg_offset], os);
    }
}

