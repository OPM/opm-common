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

#include "EclRegressionTest.hpp"
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


void ECLRegressionTest::printResultsForKeyword(const std::string& keyword) const {
    std::cout << "Deviation results for keyword " << keyword << " of type "
        << ecl_type_get_name(ecl_file_iget_named_data_type(ecl_file1, keyword.c_str(), 0))
        << ":\n";
    const double absDeviationAverage = average(absDeviation);
    const double relDeviationAverage = average(relDeviation);
    std::cout << "Average absolute deviation = " << absDeviationAverage  << std::endl;
    std::cout << "Median absolute deviation  = " << median(absDeviation) << std::endl;
    std::cout << "Average relative deviation = " << relDeviationAverage  << std::endl;
    std::cout << "Median relative deviation  = " << median(relDeviation) << "\n\n";
}



void ECLRegressionTest::boolComparisonForOccurrence(const std::string& keyword,
                                                    int occurrence1, int occurrence2) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    for (size_t cell = 0; cell < numCells; cell++) {
        bool data1 = ecl_kw_iget_bool(ecl_kw1, cell);
        bool data2 = ecl_kw_iget_bool(ecl_kw2, cell);
        if (data1 != data2) {
            printValuesForCell(keyword, occurrence1, occurrence2, numCells, cell, data1, data2);
            HANDLE_ERROR(std::runtime_error, "Values of bool type differ.");
        }
    }
}



void ECLRegressionTest::charComparisonForOccurrence(const std::string& keyword,
                                                    int occurrence1, int occurrence2) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    for (size_t cell = 0; cell < numCells; cell++) {
        std::string data1(ecl_kw_iget_char_ptr(ecl_kw1, cell));
        std::string data2(ecl_kw_iget_char_ptr(ecl_kw2, cell));
        if (data1.compare(data2) != 0) {
            printValuesForCell(keyword, occurrence1, occurrence2, numCells, cell, data1, data2);
            HANDLE_ERROR(std::runtime_error, "Values of char type differ.");
        }
    }
}



void ECLRegressionTest::intComparisonForOccurrence(const std::string& keyword,
                                                   int occurrence1, int occurrence2) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    std::vector<int> values1(numCells), values2(numCells);
    ecl_kw_get_memcpy_int_data(ecl_kw1, values1.data());
    ecl_kw_get_memcpy_int_data(ecl_kw2, values2.data());
    for (size_t cell = 0; cell < values1.size(); cell++) {
        if (values1[cell] != values2[cell]) {
            printValuesForCell(keyword, occurrence1, occurrence2, values1.size(), cell, values1[cell], values2[cell]);
            HANDLE_ERROR(std::runtime_error, "Values of int type differ.");
        }
    }
}



void ECLRegressionTest::doubleComparisonForOccurrence(const std::string& keyword,
                                                      int occurrence1, int occurrence2) {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    std::vector<double> values1(numCells), values2(numCells);
    ecl_kw_get_data_as_double(ecl_kw1, values1.data());
    ecl_kw_get_data_as_double(ecl_kw2, values2.data());

    auto it = std::find(keywordDisallowNegatives.begin(), keywordDisallowNegatives.end(), keyword);
    for (size_t cell = 0; cell < values1.size(); cell++) {
        deviationsForCell(values1[cell], values2[cell], keyword, occurrence1, occurrence2, cell, it == keywordDisallowNegatives.end());
    }
}



void ECLRegressionTest::deviationsForCell(double val1, double val2,
                                          const std::string& keyword,
                                          int occurrence1, int occurrence2,
                                          size_t kw_size, size_t cell,
                                          bool allowNegativeValues) {
    double absTolerance = getAbsTolerance();
    double relTolerance = getRelTolerance();
    if (!allowNegativeValues) {
        if (val1 < 0) {
            if (std::abs(val1) > absTolerance) {
                printValuesForCell(keyword, occurrence1, occurrence2, kw_size, cell, val1, val2);
                HANDLE_ERROR(std::runtime_error, "Negative value in first file, "
                        << "which in absolute value exceeds the absolute tolerance of " << absTolerance << ".");
            }
            val1 = 0;
        }
        if (val2 < 0) {
            if (std::abs(val2) > absTolerance) {
                printValuesForCell(keyword, occurrence1, occurrence2, kw_size, cell, val1, val2);
                HANDLE_ERROR(std::runtime_error, "Negative value in second file, "
                        << "which in absolute value exceeds the absolute tolerance of " << absTolerance << ".");
            }
            val2 = 0;
        }
    }
    Deviation dev = calculateDeviations(val1, val2);
    if (dev.abs > absTolerance && dev.rel > relTolerance) {
        if (analysis) {
          deviations[keyword].push_back(dev);
        } else {
            printValuesForCell(keyword, occurrence1, occurrence2, kw_size, cell, val1, val2);
            HANDLE_ERROR(std::runtime_error, "Deviations exceed tolerances."
                    << "\nThe absolute deviation is " << dev.abs << ", and the tolerance limit is " << absTolerance << "."
                    << "\nThe relative deviation is " << dev.rel << ", and the tolerance limit is " << relTolerance << ".");
        }
    }
    if (dev.abs != -1) {
        absDeviation.push_back(dev.abs);
    }
    if (dev.rel != -1) {
        relDeviation.push_back(dev.rel);
    }
}



void ECLRegressionTest::gridCompare(const bool volumecheck) const {
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

    if (!volumecheck) {
        return;
    }

    for (unsigned int cell = 0; cell < globalGridCount1; ++cell) {
        const bool active1 = ecl_grid_cell_active1(ecl_grid1, cell);
        const bool active2 = ecl_grid_cell_active1(ecl_grid2, cell);
        if (active1 != active2) {
            int i, j, k;
            ecl_grid_get_ijk1(ecl_grid1, cell, &i, &j, &k);
            // Coordinates from this function are zero-based, hence incrementing
            i++, j++, k++;
            HANDLE_ERROR(std::runtime_error, "Grid cell with one-based indices ( "
                         << i << ", " << j << ", " << k << " ) is "
                         << (active1 ? "active" : "inactive") << " in first grid, but "
                         << (active2 ? "active" : "inactive") << " in second grid.");
        }
        const double cellVolume1 = getCellVolume(ecl_grid1, cell);
        const double cellVolume2 = getCellVolume(ecl_grid2, cell);
        Deviation dev = calculateDeviations(cellVolume1, cellVolume2);
        if (dev.abs > absTolerance && dev.rel > relTolerance) {
            int i, j, k;
            ecl_grid_get_ijk1(ecl_grid1, cell, &i, &j, &k);
            // Coordinates from this function are zero-based, hence incrementing
            i++, j++, k++;
            HANDLE_ERROR(std::runtime_error, "In grid file: Deviations of cell volume exceed tolerances. "
                    << "\nFor cell with one-based indices (" << i << ", " << j << ", " << k << "):"
                    << "\nCell volume in first file: "  << cellVolume1
                    << "\nCell volume in second file: " << cellVolume2
                    << "\nThe absolute deviation is " << dev.abs << ", and the tolerance limit is " << absTolerance << "."
                    << "\nThe relative deviation is " << dev.rel << ", and the tolerance limit is " << relTolerance << "."
                    << "\nCell 1 active: " << active1
                    << "\nCell 2 active: " << active2);
        }
    }
}



void ECLRegressionTest::results() {
    if (!this->acceptExtraKeywords) {
        if (keywords1.size() != keywords2.size()) {
            std::set<std::string> keys(keywords1.begin() , keywords1.end());
            for (const auto& key2: keywords2)
                keys.insert( key2 );

            for (const auto& key : keys)
                fprintf(stderr," %8s:%3d     %8s:%3d \n",key.c_str() , ecl_file_get_num_named_kw( ecl_file1 , key.c_str()),
                                                         key.c_str() , ecl_file_get_num_named_kw( ecl_file2 , key.c_str()));


            OPM_THROW(std::runtime_error, "\nKeywords in first file: " << keywords1.size()
                      << "\nKeywords in second file: " << keywords2.size()
                      << "\nThe number of keywords differ.");
        }
    }

    for (const auto& it : keywords1)
        resultsForKeyword(it);

    if (analysis) {
        std::cout << deviations.size() << " keyword"
                  << (deviations.size() > 1 ? "s":"") << " exhibit failures" << std::endl;
        for (const auto& iter : deviations) {
            std::cout << "\t" << iter.first << std::endl;
            std::cout << "\t\tFails for " << iter.second.size() << " entries" << std::endl;
            std::cout.precision(7);
            double absErr = std::max_element(iter.second.begin(), iter.second.end(),
                                          [](const Deviation& a, const Deviation& b)
                                          {
                                            return a.abs < b.abs;
                                          })->abs;
            double relErr = std::max_element(iter.second.begin(), iter.second.end(),
                                          [](const Deviation& a, const Deviation& b)
                                          {
                                            return a.rel < b.rel;
                                          })->rel;
            std::cout << "\t\tLargest absolute error: "
                      <<  std::scientific << absErr << std::endl;
            std::cout << "\t\tLargest relative error: "
                      <<  std::scientific << relErr << std::endl;
        }
    }
}



void ECLRegressionTest::resultsForKeyword(const std::string& keyword) {
    keywordValidForComparing(keyword);
    const unsigned int occurrences1 = ecl_file_get_num_named_kw(ecl_file1, keyword.c_str());
    const unsigned int occurrences2 = ecl_file_get_num_named_kw(ecl_file2, keyword.c_str());
    if (!onlyLastOccurrence && occurrences1 != occurrences2) {
        OPM_THROW(std::runtime_error, "For keyword " << keyword << ":"
                << "\nKeyword occurrences in first file: "  << occurrences1
                << "\nKeyword occurrences in second file: " << occurrences2
                << "\nThe number of occurrences differ.");
    }
    // Assuming keyword type is constant for every occurrence:
    const ecl_type_enum kw_type = ecl_type_get_type( ecl_file_iget_named_data_type(ecl_file1, keyword.c_str(), 0) );
    switch(kw_type) {
        case ECL_DOUBLE_TYPE:
        case ECL_FLOAT_TYPE:
            std::cout << "Comparing " << keyword << "...";
            if (onlyLastOccurrence) {
                doubleComparisonForOccurrence(keyword, occurrences1 - 1, occurrences2 - 1);
            }
            else {
                for (unsigned int occurrence = 0; occurrence < occurrences1; ++occurrence) {
                    doubleComparisonForOccurrence(keyword, occurrence, occurrence);
                }
            }
            std::cout << "done." << std::endl;
            printResultsForKeyword(keyword);
            absDeviation.clear();
            relDeviation.clear();
            return;
        case ECL_INT_TYPE:
            std::cout << "Comparing " << keyword << "...";
            if (onlyLastOccurrence) {
                intComparisonForOccurrence(keyword, occurrences1 - 1, occurrences2 - 1);
            }
            else {
                for (unsigned int occurrence = 0; occurrence < occurrences1; ++occurrence) {
                    intComparisonForOccurrence(keyword, occurrence, occurrence);
                }
            }
            break;
        case ECL_CHAR_TYPE:
            std::cout << "Comparing " << keyword << "...";
            if (onlyLastOccurrence) {
                charComparisonForOccurrence(keyword, occurrences1 - 1, occurrences2 - 1);
            }
            else {
                for (unsigned int occurrence = 0; occurrence < occurrences1; ++occurrence) {
                    charComparisonForOccurrence(keyword, occurrence, occurrence);
                }
            }
            break;
        case ECL_BOOL_TYPE:
            std::cout << "Comparing " << keyword << "...";
            if (onlyLastOccurrence) {
                boolComparisonForOccurrence(keyword, occurrences1 - 1, occurrences2 - 1);
            }
            else {
                for (unsigned int occurrence = 0; occurrence < occurrences1; ++occurrence) {
                    boolComparisonForOccurrence(keyword, occurrence, occurrence);
                }
            }
            break;
        case ECL_MESS_TYPE:
            std::cout << "\nKeyword " << keyword << " is of type MESS"
                << ", which is not supported in regression test." << "\n\n";
            return;
        default:
            std::cout << "\nKeyword " << keyword << "has undefined type." << std::endl;
            return;
    }
    std::cout << "done." << std::endl;
}
