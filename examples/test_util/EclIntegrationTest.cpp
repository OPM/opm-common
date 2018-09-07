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

#include "EclIntegrationTest.hpp"
#include <opm/common/ErrorMacros.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

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


void ECLIntegrationTest::setCellVolumes() {
    double absTolerance = getAbsTolerance();
    double relTolerance = getRelTolerance();
    const unsigned int globalGridCount1 = ecl_grid_get_global_size(ecl_grid1);
    const unsigned int activeGridCount1 = ecl_grid_get_active_size(ecl_grid1);
    const unsigned int globalGridCount2 = ecl_grid_get_global_size(ecl_grid2);
    const unsigned int activeGridCount2 = ecl_grid_get_active_size(ecl_grid2);
    if (globalGridCount1 != globalGridCount2) {
        OPM_THROW(std::runtime_error, "In grid file:"
                << "\nCells in first file: "  << globalGridCount1
                << "\nCells in second file: " << globalGridCount2
                << "\nThe number of global cells differ.");
    }
    if (activeGridCount1 != activeGridCount2) {
        OPM_THROW(std::runtime_error, "In grid file:"
                << "\nCells in first file: "  << activeGridCount1
                << "\nCells in second file: " << activeGridCount2
                << "\nThe number of active cells differ.");
    }
    for (unsigned int cell = 0; cell < globalGridCount1; ++cell) {
        const double cellVolume1 = getCellVolume(ecl_grid1, cell);
        const double cellVolume2 = getCellVolume(ecl_grid2, cell);
        Deviation dev = calculateDeviations(cellVolume1, cellVolume2);
        if (dev.abs > absTolerance && dev.rel > relTolerance) {
            int i, j, k;
            ecl_grid_get_ijk1(ecl_grid1, cell, &i, &j, &k);
            // Coordinates from this function are zero-based, hence incrementing
            i++, j++, k++;
            OPM_THROW(std::runtime_error, "In grid file: Deviations of cell volume exceed tolerances. "
                    << "\nFor cell with coordinate (" << i << ", " << j << ", " << k << "):"
                    << "\nCell volume in first file: "  << cellVolume1
                    << "\nCell volume in second file: " << cellVolume2
                    << "\nThe absolute deviation is " << dev.abs << ", and the tolerance limit is " << absTolerance << "."
                    << "\nThe relative deviation is " << dev.rel << ", and the tolerance limit is " << relTolerance << ".");
        } // The second input case is used as reference.
        cellVolumes.push_back(cellVolume2);
    }
}



void ECLIntegrationTest::initialOccurrenceCompare(const std::string& keyword) {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, 0, 0);
    std::vector<double> values1(numCells);
    initialCellValues.resize(numCells);

    ecl_kw_get_data_as_double(ecl_kw1, values1.data());
    ecl_kw_get_data_as_double(ecl_kw2, initialCellValues.data());

    // This variable sums up the difference between the keyword value for the first case and the keyword value for the second case, for each cell. The sum is weighted with respect to the cell volume of each cell.
    double weightedDifference = 0;
    // This variable sums up the keyword value for the first case for each cell. The sum is weighted with respect to the cell volume of each cell.
    double weightedTotal = 0;
    for (size_t cell = 0; cell < initialCellValues.size(); ++cell) {
        weightedTotal += initialCellValues[cell]*cellVolumes[cell];
        weightedDifference += std::abs(values1[cell] - initialCellValues[cell])*cellVolumes[cell];
    }
    if (weightedTotal != 0) {
        double ratioValue = weightedDifference/weightedTotal;
        if ((ratioValue) > getRelTolerance()) {
            OPM_THROW(std::runtime_error, "\nFor keyword " << keyword << " and occurrence 0:"
                    << "\nThe ratio of the deviation and the total value is " << ratioValue
                    << ", which exceeds the relative tolerance of " << getRelTolerance() << "."
                    << "\nSee the docs for more information about how the ratio is computed.");
        }
    }
}



void ECLIntegrationTest::occurrenceCompare(const std::string& keyword, int occurrence) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence, occurrence);
    std::vector<double> values1(numCells), values2(numCells);
    ecl_kw_get_data_as_double(ecl_kw1, values1.data());
    ecl_kw_get_data_as_double(ecl_kw2, values2.data());

    // This variable sums up the difference between the keyword value for the first case and the keyword value for the second case, for each cell. The sum is weighted with respect to the cell volume of each cell.
    double weightedDifference = 0;
    // This variable sums up the difference between the keyword value for the occurrence and the initial keyword value for each cell. The sum is weighted with respect to the cell volume of each cell.
    double relativeWeightedTotal = 0;

    for (size_t cell = 0; cell < values1.size(); ++cell) {
        relativeWeightedTotal += std::abs(values1[cell] - initialCellValues[cell])*cellVolumes[cell];
        weightedDifference += std::abs(values1[cell] - values2[cell])*cellVolumes[cell];
    }

    if (relativeWeightedTotal != 0) {
        double ratioValue = weightedDifference/relativeWeightedTotal;
        if ((ratioValue) > getRelTolerance()) {
            OPM_THROW(std::runtime_error, "\nFor keyword " << keyword << " and occurrence " << occurrence << ":"
                    << "\nThe ratio of the deviation and the total value is " << ratioValue
                    << ", which exceeds the relative tolerance of " << getRelTolerance() << "."
                    << "\nSee the docs for more information about how the ratio is computed.");
        }
    }
}



ECLIntegrationTest::ECLIntegrationTest(const std::string& basename1,
                                       const std::string& basename2,
                                       double absTolerance, double relTolerance) :
    ECLFilesComparator(ECL_UNIFIED_RESTART_FILE, basename1, basename2, absTolerance, relTolerance) {
    std::cout << "\nUsing cell volumes and keyword values from case " << basename2
              << " as reference." << std::endl << std::endl;
    setCellVolumes();
}



bool ECLIntegrationTest::elementInWhitelist(const std::string& keyword) const {
    auto it = std::find(keywordWhitelist.begin(), keywordWhitelist.end(), keyword);
    return it != keywordWhitelist.end();
}



void ECLIntegrationTest::equalNumKeywords() const {
    if (keywords1.size() != keywords2.size()) {
        OPM_THROW(std::runtime_error, "\nKeywords in first file: " << keywords1.size()
                << "\nKeywords in second file: " << keywords2.size()
                << "\nThe number of keywords differ.");
    }
}



void ECLIntegrationTest::results() {
    for (const auto& it : keywordWhitelist)
        resultsForKeyword(it);
}



void ECLIntegrationTest::resultsForKeyword(const std::string& keyword) {
    std::cout << "Comparing " << keyword << "...";
    keywordValidForComparing(keyword);
    const unsigned int occurrences1 = ecl_file_get_num_named_kw(ecl_file1, keyword.c_str());
    const unsigned int occurrences2 = ecl_file_get_num_named_kw(ecl_file2, keyword.c_str());
    if (occurrences1 != occurrences2) {
        OPM_THROW(std::runtime_error, "For keyword " << keyword << ":"
                << "\nKeyword occurrences in first file: "  << occurrences1
                << "\nKeyword occurrences in second file: " << occurrences2
                << "\nThe number of occurrences differ.");
    }
    initialOccurrenceCompare(keyword);
    for (unsigned int occurrence = 1; occurrence < occurrences1; ++occurrence) {
        occurrenceCompare(keyword, occurrence);
    }
    std::cout << "done." << std::endl;
}
