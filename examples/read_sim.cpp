/*
  Copyright 2019 Equinor ASA.

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
#include <string.h>
#include <stdio.h>
#include <set>
#include <iomanip>

#include <cstdlib>

#include "hdf5.h"    // C lib
#include <opm/io/hdf5/Hdf5Util.hpp>


#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/hdf5/H5Smry.hpp>

//#include <opm/io/eclipse/ELODSmry.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclUtil.hpp>
#include <opm/common/utility/FileSystem.hpp>

#include <chrono> // std::chrono::microseconds
#include <thread> // std::this_thread::sleep_for

#include <sys/file.h>



using namespace Opm::EclIO;
using EclEntry = EclFile::EclEntry;



static void printHelp() {

    std::cout << "\nxxxxxx.   \n"
              << "xxxxxx. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-x xxxxxx.\n"
              << "-h Print help and exit.\n\n";
}



void print_hdf5_version_info()
{
    std::cout << "H5_VERS_MAJOR     :  " << H5_VERS_MAJOR << std::endl;
    std::cout << "H5_VERS_MINOR     :  " << H5_VERS_MINOR << std::endl;
    std::cout << "H5_VERS_RELEASE   :  " << H5_VERS_RELEASE << std::endl;
    std::cout << "H5_VERS_SUBRELEASE:  " << H5_VERS_SUBRELEASE << std::endl;
    std::cout << "H5_VERS_INFO      :  " << H5_VERS_INFO << std::endl;
}

int main(int argc, char **argv) {

    int c                          = 0;

    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        default:
            return EXIT_FAILURE;
        }
    }

    print_hdf5_version_info();

    int argOffset = optind;

    if ((argc - argOffset) < 2){
        std::cout << "\n!Error, no summary vectors spesified, nothing will be read \n\n";
        std::cout << "example of usage read_swmr NORNE_ATW2013.H5SMRY TIME FOPR \n\n";
        exit(1);
    }

    std::vector<std::string> smryvect;
    std::vector<std::vector<float>> smrydata;

    for (int i=0; i<argc - argOffset-1; i++) {
        smryvect.push_back(argv[i+argOffset+1]);
    }

    Opm::filesystem::path inputFileName = argv[argOffset];

    size_t nTstep;
    std::chrono::duration<double> elapsed_ts;

    if (inputFileName.extension() == ".H5SMRY") {

        std::cout << "open file .. " << std::flush;
        auto lap00 = std::chrono::system_clock::now();

        Opm::Hdf5IO::H5Smry smry1(inputFileName.string());

        smry1.LoadData(smryvect);

        for (auto key : smryvect){
            smrydata.push_back(smry1.get(key));
        }

        auto lap11 = std::chrono::system_clock::now();

        std::cout << " ok  \n" << std::flush;

        elapsed_ts = lap11-lap00;

        nTstep = smry1.numberOfTimeSteps();

    } else if (inputFileName.extension() == ".SMSPEC") {

        auto lap00 = std::chrono::system_clock::now();
        Opm::EclIO::ESmry smry1(inputFileName);

        smry1.LoadData(smryvect);

        for (auto key : smryvect){
            smrydata.push_back(smry1.get(key));
        }
        auto lap01 = std::chrono::system_clock::now();
        elapsed_ts = lap01-lap00;

        nTstep = smry1.numberOfTimeSteps();
/*
    } else if (inputFileName.extension() == ".LODSMRY") {

        auto lap00 = std::chrono::system_clock::now();

        Opm::EclIO::ELODSmry lodsmry1(inputFileName);

        for (auto key : smryvect){
            auto test = lodsmry1.get(key);
            smrydata.push_back(lodsmry1.get(key));
        }

        nTstep = lodsmry1.numberOfTimeSteps();

        auto lap01 = std::chrono::system_clock::now();
        elapsed_ts = lap01-lap00;
*/
    } else {
        throw std::invalid_argument("file type not supported ");
    }


    for (size_t n = 0; n < nTstep; n ++){

        for (size_t m = 0; m < smrydata.size(); m++)
            std::cout << std::setw(12) << std::fixed << std::setprecision(3) << smrydata[m][n];

        std::cout << std::endl;
    }

    std::cout << "number of timesteps is : " << nTstep << '\n';
    std::cout << "runtime opening vectors " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_ts.count();

    std::cout << "\n\n";

    return 0;
}
