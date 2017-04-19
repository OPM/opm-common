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

#include "test_CompareEclipseRestart.hpp"

//------------------------------------------------//

int main(int argc, char** argv) {
    if (argc != 5) {
        printHelp();
    }
    else {
        try {
            const char* gridFile1 = argv[1];
            const char* gridFile2 = argv[2];
            const char* unrstFile1 = argv[3];
            const char* unrstFile2 = argv[4];
            // Comparing grid sizes from .EGRID-files
            {
                ecl_grid_type* ecl_grid1 = ecl_grid_alloc(gridFile1);
                ecl_grid_type* ecl_grid2 = ecl_grid_alloc(gridFile2); 
                std::cout << "\nName of grid1: " << ecl_grid_get_name(ecl_grid1) << std::endl;
                const int gridCount1 = ecl_grid_get_global_size(ecl_grid1);
                std::cout << "Grid1 count = " << gridCount1 << std::endl;
                std::cout << "Name of grid2: " << ecl_grid_get_name(ecl_grid2) << std::endl;
                const int gridCount2 = ecl_grid_get_global_size(ecl_grid2);
                std::cout << "Grid2 count = " << gridCount2 << std::endl;

                ecl_grid_free(ecl_grid1);
                ecl_grid_free(ecl_grid2);
            }

            //Comparing keyword values from .UNRST-files:
            UNRSTReader read(unrstFile1, unrstFile2);
            for (const char* keyword : {"SGAS", "SWAT", "PRESSURE"}) {
                std::cout << "\nKeyword " << keyword << ":\n\n";

                std::vector<double> absDeviation, relDeviation;
                read.results(keyword, absDeviation, relDeviation);
                std::cout << "Average absolute deviation = " << UNRSTReader::average(absDeviation) << std::endl;
                std::cout << "Median absolute deviation = " << UNRSTReader::median(absDeviation) << std::endl;
                std::cout << "Average relative deviation = " << UNRSTReader::average(relDeviation) << std::endl;
                std::cout << "Median relative deviation = " << UNRSTReader::median(relDeviation) << std::endl;
            }
            std::cout << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Program threw an exception: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        return 0;
    }
}

//------------------------------------------------//

void printHelp() {
    std::cout << "The program takes four arguments:\n"
        << "1. .EGRID-file number 1\n" 
        << "2. .EGRID-file number 2\n" 
        << "2. .UNRST-file number 1\n" 
        << "3. .UNRST-file number 2\n";
}

//------------------------------------------------//


UNRSTReader::UNRSTReader(const char* unrstFile1, const char* unrstFile2) {
    ecl_file1 = ecl_file_open(unrstFile1, ECL_FILE_CLOSE_STREAM);
    ecl_file2 = ecl_file_open(unrstFile2, ECL_FILE_CLOSE_STREAM);

    if (ecl_file1 == nullptr) {
        OPM_THROW(std::runtime_error, "Error opening first .UNRST-file.");
    } 
    if (ecl_file2 == nullptr) {
        OPM_THROW(std::runtime_error, "Error opening second .UNRST-file.");
    }
}



UNRSTReader::~UNRSTReader() {
    ecl_file_close(ecl_file1);
    ecl_file_close(ecl_file2);
}



bool UNRSTReader::results(const char* keyword, std::vector<double>& absDeviation, std::vector<double>& relDeviation) const {
    if (!ecl_file_has_kw(ecl_file1, keyword) || !ecl_file_has_kw(ecl_file2, keyword)) {
        OPM_THROW(std::runtime_error, "The file does not have this keyword.");
        return false;
    }
    const unsigned int occurrences1 = ecl_file_get_num_named_kw(ecl_file1, keyword);
    const unsigned int occurrences2 = ecl_file_get_num_named_kw(ecl_file2, keyword);
    if (occurrences1 != occurrences2) {
        OPM_THROW(std::runtime_error, "Number of keyword occurrences are not equal.");
        return false;
    }
    for (unsigned int index = 0; index < occurrences1; ++index) {
        ecl_kw_type* ecl_kw1 = ecl_file_iget_named_kw(ecl_file1, keyword, index);
        const unsigned int numActiveCells1 = ecl_kw_get_size(ecl_kw1);
        ecl_kw_type* ecl_kw2 = ecl_file_iget_named_kw(ecl_file2, keyword, index);
        const unsigned int numActiveCells2 = ecl_kw_get_size(ecl_kw2);
        if (numActiveCells1 != numActiveCells2) {
            OPM_THROW(std::runtime_error, "Number of active cells are different.");
            return false;
        }
        std::vector<double> values1, values2; // Elements in the vector corresponds to active cells
        values1.resize(numActiveCells1);
        values2.resize(numActiveCells2);
        ecl_kw_get_data_as_double(ecl_kw1, values1.data());
        ecl_kw_get_data_as_double(ecl_kw2, values2.data());

        for (unsigned int i = 0; i < values1.size(); i++) {
            Deviation dev = calculateDeviations(values1[i], values2[i]);
            if (dev.abs != -1) {
                absDeviation.push_back(dev.abs);
            }
            if (dev.rel != -1) {
                relDeviation.push_back(dev.rel);
            }
        }
    }
    return true;
}



Deviation UNRSTReader::calculateDeviations(double val1, double val2) const {
    Deviation deviation;
    if (val1 < 0) {
        val1 = 0;
        //std::cout << "Value 1 has a negative quantity." << std::endl; 
    }
    if (val2 < 0) {
        val2 = 0;
        //std::cout << "Value 2 has a negative quantity." << std::endl; 
    }

    if (!(val1 == 0 && val2 == 0)) {
        deviation.abs = std::max(val1, val2) - std::min(val1, val2);
        if (val1 != 0 && val2 != 0) {
            deviation.rel = deviation.abs/(std::max(val1, val2));
        }
    }
    return deviation;
}



double UNRSTReader::median(std::vector<double> vec) {
    if (vec.empty()) {
        return 0;
    } 
    else {
        size_t n = vec.size()/2;
        nth_element(vec.begin(), vec.begin() + n, vec.end());
        if (vec.size() % 2 == 0) {
            return 0.5*(vec[n-1]+vec[n]);
        }
        else {
            return vec[n];
        }
    }
}



double UNRSTReader::average(const std::vector<double>& vec) {
    double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
    return sum/vec.size();
}
