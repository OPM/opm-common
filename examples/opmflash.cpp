/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*
  opmflash: the isothermal (P-T) or the isenthalpic (P-H) flash of a mixture,
  from the command line, with the fluid system and the flash solvers of the
  compositional simulator. The mixture is made of built-in components or is
  the fluid of a compositional deck. For quick checks, curves and maps without
  a simulator run.

  The options, their units and examples: opmflash --help (printUsage() in
  opmflash.hpp, which holds the program apart from main()). The tests:
  tests/test_opmflash.cpp.
*/

#include "config.h"

#include "opmflash.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace Opm::OpmFlash;

// The flash for the number of components of the run. opmflash is built for 2
// to 10 components; each count instantiates PTFlash and PHFlash once more.
// This is the program's own list: the compositional simulator has its own
// (OPM_COMPILE_COMPONENTS in opm-simulators).
int dispatch(const Run& run)
{
    switch (const auto n = run.mixture.components.size(); n) {
    case 2: return runCases<2>(std::cout, run);
    case 3: return runCases<3>(std::cout, run);
    case 4: return runCases<4>(std::cout, run);
    case 5: return runCases<5>(std::cout, run);
    case 6: return runCases<6>(std::cout, run);
    case 7: return runCases<7>(std::cout, run);
    case 8: return runCases<8>(std::cout, run);
    case 9: return runCases<9>(std::cout, run);
    case 10: return runCases<10>(std::cout, run);
    default:
        throw std::runtime_error("opmflash is built for 2 to 10 components, got " +
                                 std::to_string(n));
    }
}

} // anonymous namespace

int main(int argc, char** argv)
{
    try {
        if (argc <= 1) {
            printShortUsage(std::cerr);
            return EXIT_FAILURE;
        }
        const Command command = parseCli(std::vector<std::string>(argv + 1, argv + argc));
        switch (command.action) {
        case Action::Help:
            printUsage(std::cout);
            return EXIT_SUCCESS;
        case Action::ListComponents:
            listComponents(std::cout);
            return EXIT_SUCCESS;
        case Action::Run:
            break;
        }
        const Run run = prepare(command.cli);
        return dispatch(run);
    }
    catch (const std::exception& e) {
        std::cerr << "opmflash: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
