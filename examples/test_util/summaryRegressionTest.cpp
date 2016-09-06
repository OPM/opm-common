/*
   Copyright 2016 Statoil ASA.
   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   char* unit = getUnit(keyword);
   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/test_util/summaryRegressionTest.hpp>
#include <string>
#include <getopt.h>

void printHelp(); //< Declare global function which prints help text
void printHelp(){
    std::cout << "The program takes four arguments"          << std::endl;
    std::cout << "1) <path to file1>/<base_name>"            << std::endl;
    std::cout << "2) <path to file2>/<base_name>"            << std::endl;
    std::cout << "the basename should be without ectension." << std::endl;
    std::cout << "3) relative tolerance (between 0 and 1)"   << std::endl;
    std::cout << "4) absolute tolerance"                     << std::endl;
}

//---------------------------------------------------


int main (int argc, char ** argv){
    bool spesificKeyword = false;
    const char* keyword  = nullptr;
    int c = 0;

    while ((c = getopt(argc, argv, "hk:")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                return 0;
            case 'k':
                spesificKeyword = true;
                keyword = optarg;
                break;
            case '?':
                if (optopt == 'k') {
                    std::cout << "Option k requires an keyword." << std::endl;
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
    const char * basename1   = argv[argOffset];
    const char * basename2   = argv[argOffset+1];
    double absoluteTolerance = atof(argv[argOffset+2]);
    double relativeTolerance = atof(argv[argOffset+3]);
    try {
        RegressionTest read(basename1,basename2, absoluteTolerance, relativeTolerance);
        if(spesificKeyword){
            read.getRegressionTest(keyword);
        }
        else{
            read.getRegressionTest();
        }
    }
    catch(const std::exception& e) {
        std::cerr << "Program threw an exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
