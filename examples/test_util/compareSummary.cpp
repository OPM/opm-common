/*
   Copyright 2016 Statoil ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms
   of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */


#include <opm/test_util/summaryRegressionTest.hpp>
#include <opm/test_util/summaryIntegrationTest.hpp>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <getopt.h>

void printHelp(){
    std::cout << "\n\nThe program can handle both unified and non-unified summary files."<< std::endl;
    std::cout <<"In the case of non-unified summary files all the files must be located in the same directory. Only the basename (full path without extension) is needed as input." << std::endl << std::endl;
    std::cout << "\nThe program takes four arguments"          << std::endl;
    std::cout << "1) <path to file1>/<base_name>, basename without extension"            << std::endl;
    std::cout << "2) <path to file2>/<base_name>, basename without extension"            << std::endl;
    std::cout << "3) absolute tolerance"                     << std::endl;
    std::cout << "4) relative tolerance (between 0 and 1)"   << std::endl;
    std::cout << "The program will only throw an exception when both the absolute and relative tolerance are exceeded." << std::endl;
    std::cout << "The program is capable of performing both a regression test and an integration test, \nhowever only one type of test at a time. ";
    std::cout << "By default the program will run a regression test."<< std::endl;
    std::cout << "\nThe program have command line options:" << std::endl;
    std::cout << "-h \t\tPrint help message." << std::endl << std::endl;
    std::cout << "For the regression test: " << std::endl;
    std::cout << "-r \t\tChoosing regression test (this is default)."<< std::endl;
    std::cout << "-k keyword \tSpecify a specific keyword to compare, for example - k WOPR:PRODU1."<< std::endl;
    std::cout << "-p \t\tWill print the keywords of the files." << std::endl;
    std::cout << "-R \t\tWill allow comparison between a restarted simulation and a normal simulation. The files must end at the same time." << std::endl << std::endl;
    std::cout << "For the integration test:"<< std::endl;
    std::cout << "-i \t\tChoosing integration test." << std::endl;
    std::cout << "-d \t\tThe program will not throw an exception when the volume error ratio exceeds the limit." << std::endl;
    std::cout << "-g \t\tWill print the vector with the greatest error ratio." << std::endl;
    std::cout << "-k keyword \tSpecify a specific keyword to compare, for example - k WOPR:PRODU1."<< std::endl;
    std::cout << "-K \t\tWill not allow different amount of keywords in the two files. Throws an exception if the amount are different." << std::endl;
    std::cout << "-m mainVar \tWill calculate the error ratio for one main variable. Valid input is WOPR, WWPR, WGPR or WBHP." << std::endl;
    std::cout << "-p \t\tWill print the keywords of the files." << std::endl;
    std::cout << "-P keyword \tWill print the summary vectors of a specified kewyord, for example -P WOPR:B-3H." << std::endl;
    std::cout << "-s int \t\tSets the number of spikes that are allowed for each keyword, for example: -s 5." << std::endl;
    std::cout << "-v \t\tFor the rate keywords WOPR, WGPR, WWPR and WBHP. Calculates the error volume of \n\t\tthe two summary files. This is printed to screen." << std::endl;
    std::cout << "-V keyword \tWill calculate the error rate for a specific keyword." << std::endl << std::endl;
    std::cout << "Suggested combination of command line options:"<< std::endl;
    std::cout << " -i -g -m mainVariable, will print the vector which have the greatest error ratio of the main variable of interest.\n"<< std::endl;
}

//---------------------------------------------------


int main (int argc, char ** argv){

    //------------------------------------------------
    //Defines some constants
    bool specificKeyword = false;
    bool allowSpikes = false;
    bool findVolumeError = false;
    bool integrationTest = false;
    bool regressionTest = true;
    bool allowDifferentAmountOfKeywords = true;
    bool printKeywords = false;
    bool printSpecificKeyword = false;
    bool findVectorWithGreatestErrorRatio = false;
    bool oneOfTheMainVariables = false;
    bool throwExceptionForTooGreatErrorRatio = true;
    bool isRestartFile = false;
    const char* keyword  = nullptr;
    const char* mainVariable = nullptr;
    int c = 0;
    int limit = -1;
    //------------------------------------------------

    //------------------------------------------------
    //For setting the options selected
    while ((c = getopt(argc, argv, "dghik:Km:pP:rRs:vV:")) != -1) {
        switch (c) {
            case 'd':
                throwExceptionForTooGreatErrorRatio = false;
                break;
            case 'g':
                findVectorWithGreatestErrorRatio = true;
                throwExceptionForTooGreatErrorRatio = false;
                break;
            case 'h':
                printHelp();
                return 0;
            case 'i':
                integrationTest = true;
                regressionTest = false;
                break;
            case 'k':
                specificKeyword = true;
                keyword = optarg;
                break;
            case 'K':
                allowDifferentAmountOfKeywords = false;
                break;
            case 'm':
                oneOfTheMainVariables = true;
                mainVariable = optarg;
                break;
            case 'p':
                printKeywords = true;
                break;
            case 'P':
                specificKeyword = true;
                printSpecificKeyword = true;
                keyword = optarg;
                break;
            case 'r':
                integrationTest = false;
                regressionTest = true;
                break;
            case 'R':
                isRestartFile = true;
                break;
            case 's':
                allowSpikes = true;
                limit = atof(optarg);
                break;
            case 'v':
                findVolumeError = true;
                break;
            case 'V':
                findVolumeError = true;
                specificKeyword = true;
                keyword = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'm' || optopt == 's') {
                    std::cout << "Option requires an keyword." << std::endl;
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
    //------------------------------------------------

    int argOffset = optind;
    if (argc != argOffset + 4) {
        printHelp();
        return EXIT_FAILURE;
    }
    const char * basename1   = argv[argOffset];
    const char * basename2   = argv[argOffset+1];
    double absoluteTolerance = strtod(argv[argOffset+2], nullptr);
    double relativeTolerance = strtod(argv[argOffset+3], nullptr);

    std::cout << "Comparing '" << basename1 << "' to '" << basename2 << "'." << std::endl;

    try {
        if(regressionTest){
            RegressionTest compare(basename1,basename2, absoluteTolerance, relativeTolerance);
            if(printKeywords){compare.setPrintKeywords(true);}
            if(isRestartFile){compare.setIsRestartFile(true);}
            if(specificKeyword){
                compare.getRegressionTest(keyword);
            }
            else{
                if(printKeywords){compare.setPrintKeywords(true);}
                compare.getRegressionTest();
            }
        }
        if(integrationTest){
            IntegrationTest compare(basename1,basename2, absoluteTolerance, relativeTolerance);
            if(findVectorWithGreatestErrorRatio){compare.setFindVectorWithGreatestErrorRatio(true);}
            if(allowSpikes){compare.setAllowSpikes(true);}
            if(oneOfTheMainVariables){
                compare.setOneOfTheMainVariables(true);
                std::string str(mainVariable);
                std::transform(str.begin(), str.end(),str.begin(), ::toupper);
                if(str == "WOPR" ||str=="WWPR" ||str=="WGPR" || str == "WBHP"){
                    compare.setMainVariable(str);
                }else{
                    throw std::invalid_argument("The input is not a main variable. -m option requires a valid main variable.");
                }
            }
            if(findVolumeError){compare.setFindVolumeError(true);}
            if(limit != -1){compare.setSpikeLimit(limit);}
            if(!allowDifferentAmountOfKeywords){compare.setAllowDifferentAmountOfKeywords(false);}
            if(printKeywords){compare.setPrintKeywords(true);}
            if(!throwExceptionForTooGreatErrorRatio){compare.setThrowExceptionForTooGreatErrorRatio(false);}
            if(specificKeyword){
                if(printSpecificKeyword){compare.setPrintSpecificKeyword(true);}
                compare.getIntegrationTest(keyword);
                return 0;
            }
            compare.getIntegrationTest();
        }
    }
    catch(const std::exception& e) {
        std::cerr << "Program threw an exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
