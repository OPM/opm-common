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
#include <opm/common/ErrorMacros.hpp>

#include <iostream>
#include <algorithm>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>

bool ECLFilesComparator::keywordValidForComparing(const std::string& keyword) const {
    auto it = std::find(keywords1.begin(), keywords1.end(), keyword);
    if (it == keywords1.end()) {
        std::cout << "Could not find keyword: " << keyword << std::endl;
        OPM_THROW(std::runtime_error, "Keyword does not exist in first file.");
    }
    it = find(keywords2.begin(), keywords2.end(), keyword);
    if (it == keywords2.end()) {
        std::cout << "Could not find keyword: " << keyword << std::endl;
        OPM_THROW(std::runtime_error, "Keyword does not exist in second file.");
    }
    ecl_kw_type* ecl_kw = ecl_file_iget_named_kw(ecl_file1, keyword.c_str(), 0);
    ecl_type_enum ecl_kw_type = ecl_kw_get_type(ecl_kw);
    switch (ecl_kw_type) {
        case ECL_DOUBLE_TYPE:
            return true;
        case ECL_FLOAT_TYPE:
            return true;
        case ECL_CHAR_TYPE:
            std::cout << "\nKeyword " << keyword << " is of type ECL_CHAR_TYPE, "
                << "which is not supported for comparison." << std::endl;
            return false;
        case ECL_INT_TYPE:
            std::cout << "\nKeyword " << keyword << " is of type ECL_INT_TYPE, "
                << "which is not supported for comparison." << std::endl;
            return false;
        case ECL_BOOL_TYPE:
            std::cout << "\nKeyword " << keyword << " is of type ECL_BOOL_TYPE, "
                << "which is not supported for comparison." << std::endl;
            return false;
        case ECL_MESS_TYPE:
            std::cout << "\nKeyword " << keyword << " is of type ECL_MESS_TYPE, "
                << "which is not supported for comparison." << std::endl;
            return false;
        default:
            std::cout << "\nKeyword " << keyword << "has undefined type." << std::endl;
            return false;
    }
}



void ECLFilesComparator::printResultsForKeyword(const std::string& keyword) const {
    const double absDeviationAverage = average(absDeviation);
    const double relDeviationAverage = average(relDeviation);
    std::cout << "Average absolute deviation = " << absDeviationAverage  << std::endl;
    std::cout << "Median absolute deviation  = " << median(absDeviation) << std::endl;
    std::cout << "Average relative deviation = " << relDeviationAverage  << std::endl;
    std::cout << "Median relative deviation  = " << median(relDeviation) << "\n\n";
}



void ECLFilesComparator::deviationsForOccurence(const std::string& keyword, int occurence) {
    ecl_kw_type* ecl_kw1         = ecl_file_iget_named_kw(ecl_file1, keyword.c_str(), occurence);
    ecl_kw_type* ecl_kw2         = ecl_file_iget_named_kw(ecl_file2, keyword.c_str(), occurence);
    const unsigned int numCells1 = ecl_kw_get_size(ecl_kw1);
    const unsigned int numCells2 = ecl_kw_get_size(ecl_kw2);
    if (numCells1 != numCells2) {
        std::cout << "For keyword " << keyword << " and occurence " << occurence << ":\n";
        OPM_THROW(std::runtime_error, "Number of active cells are different.");
    }
    std::vector<double> values1(numCells1), values2(numCells2);
    ecl_kw_get_data_as_double(ecl_kw1, values1.data());
    ecl_kw_get_data_as_double(ecl_kw2, values2.data());

    auto it = std::find(keywordWhitelist.begin(), keywordWhitelist.end(), keyword);
    for (unsigned int cell = 0; cell < values1.size(); cell++) {
        deviationsForCell(values1[cell], values2[cell], keyword, occurence, cell, it == keywordWhitelist.end());
    }
}



void ECLFilesComparator::deviationsForCell(double val1, double val2, const std::string& keyword, int occurence, int cell, bool allowNegativeValues) {
    int i, j, k;
    ecl_grid_get_ijk1(ecl_grid1, cell, &i, &j, &k);
    // Coordinates from this function are zero-based, hence incrementing
    i++, j++, k++;

    if (showValues) {
        std::cout << "Occurence        = "  << occurence << std::endl;
        std::cout << "Grid coordinate  = (" << i << ", " << j << ", " << k << ")\n";
        std::cout << "(value1, value2) = (" << val1 << ", " << val2 << ")\n\n";
    }
    if (!allowNegativeValues) {
        if (val1 < 0) {
            if (std::abs(val1) > absTolerance) {
                std::cout << "For keyword " << keyword << " (which is listed for positive values only), occurence " << occurence
                    << ", and grid coordinate (" << i << ", " << j << ", " << k << "):\n"
                    << "the vector has a value of " << val1 << ", which exceeds the absolute tolerance of " << absTolerance << ".\n";
                OPM_THROW(std::runtime_error, "Negative value in first file.");
            }
            val1 = 0;
        }
        if (val2 < 0) {
            if (std::abs(val2) > absTolerance) {
                std::cout << "For keyword " << keyword << " (which is listed for positive values only), occurence " << occurence
                    << ", and grid coordinate (" << i << ", " << j << ", " << k << "):\n"
                    << "the vector has a value of " << val2 << ", which exceeds the absolute tolerance of " << absTolerance << ".\n";
                OPM_THROW(std::runtime_error, "Negative value in second file.");
            }
            val2 = 0;
        }
    }
    Deviation dev = calculateDeviations(val1, val2);
    if (dev.abs > absTolerance && dev.rel > relTolerance) {
        std::cout << "For keyword " << keyword << ", occurence " << occurence
            << ", and grid coordinate (" << i << ", " << j << ", " << k << "):\n"
            << "(first value, second value) = (" << val1 << ", " << val2 << ")\n";
        std::cout << "The absolute deviation is " << dev.abs << ", and the tolerance limit is " << absTolerance << ".\n";
        std::cout << "The relative deviation is " << dev.rel << ", and the tolerance limit is " << relTolerance << ".\n";
        OPM_THROW(std::runtime_error, "Absolute deviation exceeds tolerance.");
    }
    if (dev.abs != -1) {
        absDeviation.push_back(dev.abs);
    }
    if (dev.rel != -1) {
        relDeviation.push_back(dev.rel);
    }
}



ECLFilesComparator::ECLFilesComparator(eclFileEnum fileType, const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance):
    fileType(fileType), absTolerance(absTolerance), relTolerance(relTolerance) {

    std::string file1, file2;
    if (fileType == RESTARTFILE) {
        file1 = basename1 + ".UNRST";
        file2 = basename2 + ".UNRST";
    }
    else if (fileType == INITFILE) {
        file1 = basename1 + ".INIT";
        file2 = basename2 + ".INIT";
    }

    ecl_file1 = ecl_file_open(file1.c_str(), 0);
    ecl_file2 = ecl_file_open(file2.c_str(), 0);
    ecl_grid1 = ecl_grid_load_case(basename1.c_str());
    ecl_grid2 = ecl_grid_load_case(basename2.c_str());

    if (ecl_file1 == nullptr) {
        std::cout << "Tried to open: " << file1 << std::endl;
        OPM_THROW(std::runtime_error, "Error opening first file.");
    }
    if (ecl_file2 == nullptr) {
        std::cout << "Tried to open: " << file2 << std::endl;
        OPM_THROW(std::runtime_error, "Error opening second file.");
    }
    if (ecl_grid1 == nullptr) {
        std::cout << "Tried to open: " << basename1 + ".EGRID" << std::endl;
        OPM_THROW(std::runtime_error, "Error opening first file.");
    }
    if (ecl_grid2 == nullptr) {
        std::cout << "Tried to open: " << basename2 + ".EGRID" << std::endl;
        OPM_THROW(std::runtime_error, "Error opening second file.");
    }

    unsigned int numKeywords1 = ecl_file_get_num_distinct_kw(ecl_file1);
    unsigned int numKeywords2 = ecl_file_get_num_distinct_kw(ecl_file2);
    for (unsigned int i = 0; i < numKeywords1; ++i) {
        std::string keyword(ecl_file_iget_distinct_kw(ecl_file1, i));
        keywords1.push_back(keyword);
    }
    for (unsigned int i = 0; i < numKeywords2; ++i) {
        std::string keyword(ecl_file_iget_distinct_kw(ecl_file2, i));
        keywords2.push_back(keyword);
    }
}



ECLFilesComparator::~ECLFilesComparator() {
    ecl_file_close(ecl_file1);
    ecl_file_close(ecl_file2);
    ecl_grid_free(ecl_grid1);
    ecl_grid_free(ecl_grid2);
}



void ECLFilesComparator::printKeywords() const {
    std::cout << "\nKeywords for first file:\n";
    for (const auto& it : keywords1) std::cout << it << std::endl;
    std::cout << "\nKeywords for second file:\n";
    for (const auto& it : keywords2) std::cout << it << std::endl;
}



bool ECLFilesComparator::gridCompare() const {
    const int globalGridCount1 = ecl_grid_get_global_size(ecl_grid1);
    const int activeGridCount1 = ecl_grid_get_active_size(ecl_grid1);
    const int globalGridCount2 = ecl_grid_get_global_size(ecl_grid2);
    const int activeGridCount2 = ecl_grid_get_active_size(ecl_grid2);
    if (globalGridCount1 != globalGridCount2) {
        OPM_THROW(std::runtime_error, "Grid file: The total number of cells are different.");
        return false;
    }
    if (activeGridCount1 != activeGridCount2) {
        OPM_THROW(std::runtime_error, "Grid file: The number of active cells are different.");
        return false;
    }
    return true;
}



void ECLFilesComparator::results() {
    if (keywords1.size() != keywords2.size()) {
        OPM_THROW(std::runtime_error, "Number of keywords are not equal.");
    }
    for (const auto& it : keywords1) resultsForKeyword(it);
}



void ECLFilesComparator::resultsForKeyword(const std::string keyword) {
    std::cout << "\nKeyword " << keyword << ":\n\n";
    const unsigned int occurrences1 = ecl_file_get_num_named_kw(ecl_file1, keyword.c_str());
    const unsigned int occurrences2 = ecl_file_get_num_named_kw(ecl_file2, keyword.c_str());
    if (occurrences1 != occurrences2) {
        std::cout << "For keyword " << keyword << ":\n";
        OPM_THROW(std::runtime_error, "Number of keyword occurrences are not equal.");
    }
    if (!keywordValidForComparing(keyword)) {
        return;
    }
    for (unsigned int occurence = 0; occurence < occurrences1; ++occurence) {
        deviationsForOccurence(keyword, occurence);
    }
    printResultsForKeyword(keyword);
    absDeviation.clear();
    relDeviation.clear();
}



Deviation ECLFilesComparator::calculateDeviations(double val1, double val2) {
    val1 = std::abs(val1);
    val2 = std::abs(val2);
    Deviation deviation;
    if (val1 != 0 || val2 != 0) {
        deviation.abs = std::abs(val1 - val2);
        if (val1 != 0 && val2 != 0) {
            deviation.rel = deviation.abs/(std::max(val1, val2));
        }
    }
    return deviation;
}



double ECLFilesComparator::median(std::vector<double> vec) {
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



double ECLFilesComparator::average(const std::vector<double>& vec) {
    if (vec.empty()) {
        return 0;
    }
    double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
    return sum/vec.size();
}
