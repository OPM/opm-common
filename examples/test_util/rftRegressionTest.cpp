/*
   Copyright 2016 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/test_util/EclFilesComparator.hpp>

#include <iostream>
#include <string>
#include <getopt.h>

static void printHelp() {
    std::cout << "compareRestart compares restartfiles and gridsizes from two simulations.\n"
        << "The program takes four arguments:\n\n"
        << "1. File number 1 (full path without extension)\n"
        << "2. File number 2 (full path without extension)\n"
        << "3. Absolute tolerance\n"
        << "4. Relative tolerance (between 0 and 1)\n\n"
        << "In addition, the program takes these options (which must be given before the arguments):\n\n"
        << "-h Print help.\n"
        << "-k Specify specific keyword to compare, for example -k PRESSURE.\n"
        << "-s Print all values side by side from the specified files.\n\n";
}

//------------------------------------------------//

int main(int argc, char** argv) {
    bool showValues      = false;
    bool specificKeyword = false;
    char* keyword        = nullptr;
    int c = 0;

    while ((c = getopt(argc, argv, "hk:s")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                return 0;
            case 'k':
                specificKeyword = true;
                keyword = optarg;
                break;
            case 's':
                showValues = true;
                break;
            case '?':
                if (optopt == 'k') {
                    std::cout << "Option k requires an argument." << std::endl;
                    return EXIT_FAILURE;
                }
                else {
                    std::cout << "Unknown option." << std::endl;
                    return EXIT_FAILURE;
                }
            default:
                return EXIT_FAILURE;
        }
    }

    int argOffset = optind;
    if (argc != argOffset + 4) {
        printHelp();
        return EXIT_FAILURE;
    }

    const std::string basename1 = argv[argOffset];
    const std::string basename2 = argv[argOffset + 1];
    const double absTolerance   = atof(argv[argOffset + 2]);
    const double relTolerance   = atof(argv[argOffset + 3]);

    try {
        std::cout << "\nUsing absolute deviation tolerance of " << absTolerance
            << " and relative tolerance of " << relTolerance << ".\n";
        ECLFilesComparator comparator(RFTFILE, basename1, basename2, absTolerance, relTolerance);
        if (showValues) {
            comparator.setShowValues(true);
        }
        if (specificKeyword) {
            comparator.gridCompare();
            comparator.resultsForKeyword(keyword);
        }
        else {
            comparator.gridCompare();
            comparator.results();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Program threw an exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
