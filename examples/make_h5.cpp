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

#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/eclipse/EclUtil.hpp>
#include <opm/common/utility/FileSystem.hpp>


static void printHelp() {

    std::cout << "\nThis program create one or more lodsmry files, designed for effective load on the demand.   \n"
              << "These files are created with input from the smspec and unsmry file. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-f if LODSMRY file exist, this will be replaced. Default behaviour is that existing file is kept.\n"
              << "-n Maximum number of threads to be used if mulitple files should be created.\n"
              << "-h Print help and exit.\n\n";
}


/*
    std::cout << "H5_VERS_MAJOR     :  " << H5_VERS_MAJOR << std::endl;
    std::cout << "H5_VERS_MINOR     :  " << H5_VERS_MINOR << std::endl;
    std::cout << "H5_VERS_RELEASE   :  " << H5_VERS_RELEASE << std::endl;
    std::cout << "H5_VERS_SUBRELEASE:  " << H5_VERS_SUBRELEASE << std::endl;
    std::cout << "H5_VERS_INFO      :  " << H5_VERS_INFO << std::endl;
*/


int main(int argc, char **argv) {

    int c                          = 0;
    int max_threads                = -1;
    bool force                     = false;
    bool eclrun_layout             = false;
    bool info                      = false;

    while ((c = getopt(argc, argv, "fn:hei")) != -1) {
        switch (c) {
        case 'f':
            force = true;
            break;
        case 'h':
            printHelp();
            return 0;
        case 'e':
            eclrun_layout=true;
            break;
        case 'n':
            max_threads = atoi(optarg);
            break;
        case 'i':
            info = true;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    auto lap0 = std::chrono::system_clock::now();

    int argOffset = optind;

    Opm::EclIO::ESmry smryFile(argv[optind]);

    if (info){

        std::cout << "\nNumber of vectors  : " << smryFile.numberOfVectors() << std::endl;
        std::cout << "Number of timesteps: " << smryFile.numberOfTimeSteps() << std::endl;
        std::cout << "\n";
        exit(0);
    }

    Opm::filesystem::path inputFileName = argv[optind];

    Opm::filesystem::path h5FileName = inputFileName.parent_path() / inputFileName.stem();

    if (eclrun_layout)
        h5FileName = h5FileName += ".h5";
    else
        h5FileName = h5FileName += ".H5SMRY";

    if (Opm::EclIO::fileExists(h5FileName) && (force))
        remove (h5FileName);


    if (eclrun_layout){
        if (!smryFile.make_h5_eclrun_file())
            std::cout << "\n! Warning, smspec already have one h5 file, existing kept use option -f to replace this" << std::endl;
    } else {
        if (!smryFile.make_h5smry_file())
            std::cout << "\n! Warning, smspec already have one H5SMRY file, existing kept use option -f to replace this" << std::endl;
    }

    auto lap1 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds1 = lap1-lap0;

    if (eclrun_layout)
        std::cout << "\nruntime creating h5 from smspec/unsmry : " << elapsed_seconds1.count() << " seconds" << std::endl;
    else
        std::cout << "\nruntime creating h5smry from smspec/unsmry : " << elapsed_seconds1.count() << " seconds" << std::endl;

    return 0;
}
