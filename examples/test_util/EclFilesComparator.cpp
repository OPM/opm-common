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

#include "EclFilesComparator.hpp"
#include <opm/common/ErrorMacros.hpp>
#include <opm/common/utility/numeric/calculateCellVol.hpp>

#include <stdio.h>

#include <set>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <numeric>

#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_type.h>

#include <ert/ecl_well/well_info.h>


// helper macro to handle error throws or not
#define HANDLE_ERROR(type, message) \
  { \
    if (throwOnError) \
      OPM_THROW(type, message); \
    else { \
      std::cerr << message << std::endl; \
      ++num_errors; \
    } \
  }


namespace {
    /*
      This is just a basic survival test; we verify that the ERT well
      loader which is used in Resinsight can load the well description
      from the restart file.
    */

    void loadWells( const ecl_grid_type * grid , ecl_file_type * rst_file ) {
        well_info_type * well_info = well_info_alloc( grid );
        well_info_add_UNRST_wells2( well_info , ecl_file_get_global_view( rst_file ), true );
        well_info_free( well_info );
    }

}


void ECLFilesComparator::keywordValidForComparing(const std::string& keyword) const {
    auto it = std::find(keywords1.begin(), keywords1.end(), keyword);
    if (it == keywords1.end()) {
        OPM_THROW(std::runtime_error, "Keyword " << keyword << " does not exist in first file.");
    }
    it = find(keywords2.begin(), keywords2.end(), keyword);
    if (it == keywords2.end()) {
        OPM_THROW(std::runtime_error, "Keyword " << keyword << " does not exist in second file.");
    }
}


unsigned int ECLFilesComparator::getEclKeywordData(ecl_kw_type*& ecl_kw1, ecl_kw_type*& ecl_kw2, const std::string& keyword, int occurrence1, int occurrence2) const {
    ecl_kw1 = ecl_file_iget_named_kw(ecl_file1, keyword.c_str(), occurrence1);
    ecl_kw2 = ecl_file_iget_named_kw(ecl_file2, keyword.c_str(), occurrence2);
    const unsigned int numCells1 = ecl_kw_get_size(ecl_kw1);
    const unsigned int numCells2 = ecl_kw_get_size(ecl_kw2);
    if (numCells1 != numCells2) {
        OPM_THROW(std::runtime_error, "For keyword " << keyword << ":"
                << "\nOccurrence in first file " << occurrence1
                << "\nOccurrence in second file " << occurrence2
                << "\nCells in first file: " << numCells1
                << "\nCells in second file: " << numCells2
                << "\nThe number of cells differ.");
    }
    return numCells1;
}



template <typename T>
void ECLFilesComparator::printValuesForCell(const std::string& /*keyword*/, int occurrence1, int occurrence2, size_t kw_size, size_t cell, const T& value1, const T& value2) const {
    if (kw_size == static_cast<size_t>(ecl_grid_get_active_size(ecl_grid1))) {
        int i, j, k;
        ecl_grid_get_ijk1A(ecl_grid1, cell, &i, &j, &k);
        // Coordinates from this function are zero-based, hence incrementing
        i++, j++, k++;
        std::cout << std::endl
                  << "Occurrence in first file    = "  << occurrence1 << "\n"
                  << "Occurrence in second file   = "  << occurrence2 << "\n"
                  << "Value index                 = "  << cell << "\n"
                  << "Grid coordinate             = (" << i << ", " << j << ", " << k << ")" << "\n"
                  << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";

        return;
    }

    if (kw_size == static_cast<size_t>(ecl_grid_get_global_size(ecl_grid1))) {
        int i, j, k;
        ecl_grid_get_ijk1(ecl_grid1, cell, &i, &j, &k);
        // Coordinates from this function are zero-based, hence incrementing
        i++, j++, k++;
        std::cout << std::endl
                  << "Occurrence in first file    = "  << occurrence1 << "\n"
                  << "Occurrence in second file   = "  << occurrence2 << "\n"
                  << "Value index                 = "  << cell << "\n"
                  << "Grid coordinate             = (" << i << ", " << j << ", " << k << ")" << "\n"
                  << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";
        return;
    }

    std::cout << std::endl
              << "Occurrence in first file    = "  << occurrence1 << "\n"
              << "Occurrence in second file   = "  << occurrence2 << "\n"
              << "Value index                 = "  << cell << "\n"
              << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";
}

template void ECLFilesComparator::printValuesForCell<bool>       (const std::string& keyword, int occurrence1, int occurrence2, size_t kw_size, size_t cell, const bool&        value1, const bool&        value2) const;
template void ECLFilesComparator::printValuesForCell<int>        (const std::string& keyword, int occurrence1, int occurrence2, size_t kw_size, size_t cell, const int&         value1, const int&         value2) const;
template void ECLFilesComparator::printValuesForCell<double>     (const std::string& keyword, int occurrence1, int occurrence2, size_t kw_size, size_t cell, const double&      value1, const double&      value2) const;
template void ECLFilesComparator::printValuesForCell<std::string>(const std::string& keyword, int occurrence1, int occurrence2, size_t kw_size, size_t cell, const std::string& value1, const std::string& value2) const;


ECLFilesComparator::ECLFilesComparator(int file_type_arg, const std::string& basename1,
                                       const std::string& basename2,
                                       double absToleranceArg, double relToleranceArg) :
 file_type(file_type_arg), absTolerance(absToleranceArg), relTolerance(relToleranceArg) {

    std::string file1, file2;
    if (file_type == ECL_UNIFIED_RESTART_FILE) {
        file1 = basename1 + ".UNRST";
        file2 = basename2 + ".UNRST";
    }
    else if (file_type == ECL_INIT_FILE) {
        file1 = basename1 + ".INIT";
        file2 = basename2 + ".INIT";
    }
    else if (file_type == ECL_RFT_FILE) {
        file1 = basename1 + ".RFT";
        file2 = basename2 + ".RFT";
    }
    else {
        OPM_THROW(std::invalid_argument, "Unsupported filetype sent to ECLFilesComparator's constructor."
                << "Only unified restart (.UNRST), initial (.INIT) and .RFT files are supported.");
    }
    ecl_file1 = ecl_file_open(file1.c_str(), 0);
    ecl_file2 = ecl_file_open(file2.c_str(), 0);
    ecl_grid1 = ecl_grid_load_case(basename1.c_str());
    ecl_grid2 = ecl_grid_load_case(basename2.c_str());
    if (ecl_file1 == nullptr) {
        OPM_THROW(std::invalid_argument, "Error opening first file: " << file1);
    }
    if (ecl_file2 == nullptr) {
        OPM_THROW(std::invalid_argument, "Error opening second file: " << file2);
    }
    if (ecl_grid1 == nullptr) {
        OPM_THROW(std::invalid_argument, "Error opening first grid file: " << basename1);
    }
    if (ecl_grid2 == nullptr) {
        OPM_THROW(std::invalid_argument, "Error opening second grid file. " << basename2);
    }
    unsigned int numKeywords1 = ecl_file_get_num_distinct_kw(ecl_file1);
    unsigned int numKeywords2 = ecl_file_get_num_distinct_kw(ecl_file2);
    keywords1.reserve(numKeywords1);
    keywords2.reserve(numKeywords2);
    for (unsigned int i = 0; i < numKeywords1; ++i) {
        std::string keyword(ecl_file_iget_distinct_kw(ecl_file1, i));
        keywords1.push_back(keyword);
    }
    for (unsigned int i = 0; i < numKeywords2; ++i) {
        std::string keyword(ecl_file_iget_distinct_kw(ecl_file2, i));
        keywords2.push_back(keyword);
    }

    if (file_type == ECL_UNIFIED_RESTART_FILE) {
        loadWells( ecl_grid1 , ecl_file1 );
        loadWells( ecl_grid2 , ecl_file2 );
    }
}



ECLFilesComparator::~ECLFilesComparator() {
    ecl_file_close(ecl_file1);
    ecl_file_close(ecl_file2);
    ecl_grid_free(ecl_grid1);
    ecl_grid_free(ecl_grid2);
}



void ECLFilesComparator::printKeywords() const {
    std::cout << "\nKeywords in the first file:\n";
    for (const auto& it : keywords1) {
        std::cout << std::setw(15) << std::left << it << " of type " << ecl_type_get_name( ecl_file_iget_named_data_type(ecl_file1, it.c_str(), 0)) << std::endl;
    }
    std::cout << "\nKeywords in second file:\n";
    for (const auto& it : keywords2) {
        std::cout << std::setw(15) << std::left << it << " of type " << ecl_type_get_name( ecl_file_iget_named_data_type(ecl_file2, it.c_str(), 0)) << std::endl;
    }
}



void ECLFilesComparator::printKeywordsDifference() const {
    std::vector<std::string> common;
    std::vector<std::string> uncommon;
    const std::vector<std::string>* keywordsShort = &keywords1;
    const std::vector<std::string>* keywordsLong = &keywords2;
    if (keywords1.size() > keywords2.size()) {
        keywordsLong = &keywords1;
        keywordsShort = &keywords2;
    }
    for (const auto& it : *keywordsLong) {
        const auto position = std::find(keywordsShort->begin(), keywordsShort->end(), it);
        if (position != keywordsShort->end()) {
            common.push_back(*position);
        }
        else {
            uncommon.push_back(it);
        }
    }
    std::cout << "\nCommon keywords for the two cases:\n";
    for (const auto& it : common) std::cout << it << std::endl;
    std::cout << "\nUncommon keywords for the two cases:\n";
    for (const auto& it : uncommon) std::cout << it << std::endl;
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



double ECLFilesComparator::getCellVolume(const ecl_grid_type* ecl_grid,
                                         const int globalIndex) {
    std::vector<double> x(8, 0.0);
    std::vector<double> y(8, 0.0);
    std::vector<double> z(8, 0.0);
    for (int i = 0; i < 8; i++) {
        ecl_grid_get_cell_corner_xyz1(ecl_grid, globalIndex, i, &x.data()[i], &y.data()[i], &z.data()[i]);
    }
    return calculateCellVol(x,y,z);
}
