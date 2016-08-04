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
#include <iomanip>
#include <algorithm>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>
#include <cmath>
#include <numeric>


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
void ECLFilesComparator::printValuesForCell(const std::string& keyword, int occurrence1, int occurrence2, size_t cell, const T& value1, const T& value2) const {
    int i, j, k;
    ecl_grid_get_ijk1(ecl_grid1, cell, &i, &j, &k);
    // Coordinates from this function are zero-based, hence incrementing
    i++, j++, k++;
    std::cout << std::endl
              << "Occurrence in first file    = "  << occurrence1 << "\n"
              << "Occurrence in second file   = "  << occurrence2 << "\n"
              << "Grid coordinate             = (" << i << ", " << j << ", " << k << ")" << "\n"
              << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";
}
template void ECLFilesComparator::printValuesForCell<bool>       (const std::string& keyword, int occurrence1, int occurrence2, size_t cell, const bool&        value1, const bool&        value2) const;
template void ECLFilesComparator::printValuesForCell<int>        (const std::string& keyword, int occurrence1, int occurrence2, size_t cell, const int&         value1, const int&         value2) const;
template void ECLFilesComparator::printValuesForCell<double>     (const std::string& keyword, int occurrence1, int occurrence2, size_t cell, const double&      value1, const double&      value2) const;
template void ECLFilesComparator::printValuesForCell<std::string>(const std::string& keyword, int occurrence1, int occurrence2, size_t cell, const std::string& value1, const std::string& value2) const;



ECLFilesComparator::ECLFilesComparator(int file_type, const std::string& basename1,
                                       const std::string& basename2,
                                       double absTolerance, double relTolerance) :
 file_type(file_type), absTolerance(absTolerance), relTolerance(relTolerance) {

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
        std::cout << std::setw(15) << std::left << it << " of type " << ecl_util_get_type_name(ecl_file_iget_named_type(ecl_file1, it.c_str(), 0)) << std::endl;
    }
    std::cout << "\nKeywords in second file:\n";
    for (const auto& it : keywords2) {
        std::cout << std::setw(15) << std::left << it << " of type " << ecl_util_get_type_name(ecl_file_iget_named_type(ecl_file2, it.c_str(), 0)) << std::endl;
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



void RegressionTest::printResultsForKeyword(const std::string& keyword) const {
    std::cout << "Deviation results for keyword " << keyword << " of type "
        << ecl_util_get_type_name(ecl_file_iget_named_type(ecl_file1, keyword.c_str(), 0))
        << ":\n";
    const double absDeviationAverage = average(absDeviation);
    const double relDeviationAverage = average(relDeviation);
    std::cout << "Average absolute deviation = " << absDeviationAverage  << std::endl;
    std::cout << "Median absolute deviation  = " << median(absDeviation) << std::endl;
    std::cout << "Average relative deviation = " << relDeviationAverage  << std::endl;
    std::cout << "Median relative deviation  = " << median(relDeviation) << "\n\n";
}



void RegressionTest::boolComparisonForOccurrence(const std::string& keyword,
                                                 int occurrence1, int occurrence2) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    for (size_t cell = 0; cell < numCells; cell++) {
        bool data1 = ecl_kw_iget_bool(ecl_kw1, cell);
        bool data2 = ecl_kw_iget_bool(ecl_kw2, cell);
        if (data1 != data2) {
            printValuesForCell(keyword, occurrence1, occurrence2, cell, data1, data2);
            OPM_THROW(std::runtime_error, "Values of bool type differ.");
        }
    }
}



void RegressionTest::charComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    for (size_t cell = 0; cell < numCells; cell++) {
        std::string data1(ecl_kw_iget_char_ptr(ecl_kw1, cell));
        std::string data2(ecl_kw_iget_char_ptr(ecl_kw2, cell));
        if (data1.compare(data2) != 0) {
            printValuesForCell(keyword, occurrence1, occurrence2, cell, data1, data2);
            OPM_THROW(std::runtime_error, "Values of char type differ.");
        }
    }
}



void RegressionTest::intComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2) const {
    ecl_kw_type* ecl_kw1 = nullptr;
    ecl_kw_type* ecl_kw2 = nullptr;
    const unsigned int numCells = getEclKeywordData(ecl_kw1, ecl_kw2, keyword, occurrence1, occurrence2);
    std::vector<int> values1(numCells), values2(numCells);
    ecl_kw_get_memcpy_int_data(ecl_kw1, values1.data());
    ecl_kw_get_memcpy_int_data(ecl_kw2, values2.data());
    for (size_t cell = 0; cell < values1.size(); cell++) {
        if (values1[cell] != values2[cell]) {
            printValuesForCell(keyword, occurrence1, occurrence2, cell, values1[cell], values2[cell]);
            OPM_THROW(std::runtime_error, "Values of int type differ.");
        }
    }
}



void RegressionTest::doubleComparisonForOccurrence(const std::string& keyword, int occurrence1, int occurrence2) {
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



void RegressionTest::deviationsForCell(double val1, double val2, const std::string& keyword, int occurrence1, int occurrence2, size_t cell, bool allowNegativeValues) {
    double absTolerance = getAbsTolerance();
    double relTolerance = getRelTolerance();
    if (!allowNegativeValues) {
        if (val1 < 0) {
            if (std::abs(val1) > absTolerance) {
                printValuesForCell(keyword, occurrence1, occurrence2, cell, val1, val2);
                OPM_THROW(std::runtime_error, "Negative value in first file, "
                        << "which in absolute value exceeds the absolute tolerance of " << absTolerance << ".");
            }
            val1 = 0;
        }
        if (val2 < 0) {
            if (std::abs(val2) > absTolerance) {
                printValuesForCell(keyword, occurrence1, occurrence2, cell, val1, val2);
                OPM_THROW(std::runtime_error, "Negative value in second file, "
                        << "which in absolute value exceeds the absolute tolerance of " << absTolerance << ".");
            }
            val2 = 0;
        }
    }
    Deviation dev = calculateDeviations(val1, val2);
    if (dev.abs > absTolerance && dev.rel > relTolerance) {
        printValuesForCell(keyword, occurrence1, occurrence2, cell, val1, val2);
        OPM_THROW(std::runtime_error, "Deviations exceed tolerances."
                << "\nThe absolute deviation is " << dev.abs << ", and the tolerance limit is " << absTolerance << "."
                << "\nThe relative deviation is " << dev.rel << ", and the tolerance limit is " << relTolerance << ".");
    }
    if (dev.abs != -1) {
        absDeviation.push_back(dev.abs);
    }
    if (dev.rel != -1) {
        relDeviation.push_back(dev.rel);
    }
}



void RegressionTest::gridCompare() const {
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
                << "\nThe number of cells differ.");
    }
    if (activeGridCount1 != activeGridCount2) {
        OPM_THROW(std::runtime_error, "In grid file:"
                << "\nCells in first file: "  << activeGridCount1
                << "\nCells in second file: " << activeGridCount2
                << "\nThe number of cells differ.");
    }
    for (unsigned int cell = 0; cell < globalGridCount1; ++cell) {
        const double cellVolume1 = ecl_grid_get_cell_volume1(ecl_grid1, cell);
        const double cellVolume2 = ecl_grid_get_cell_volume1(ecl_grid2, cell);
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
        }
    }
}



void RegressionTest::results() {
    if (keywords1.size() != keywords2.size()) {
        OPM_THROW(std::runtime_error, "\nKeywords in first file: " << keywords1.size()
                << "\nKeywords in second file: " << keywords2.size()
                << "\nThe number of keywords differ.");
    }
    for (const auto& it : keywords1)
        resultsForKeyword(it);
}



void RegressionTest::resultsForKeyword(const std::string keyword) {
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
    const ecl_type_enum kw_type = ecl_file_iget_named_type(ecl_file1, keyword.c_str(), 0);
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
            std::cout << "\nKeyword " << keyword << " is of type "
                << ecl_util_get_type_name(kw_type)
                << ", which is not supported in regression test." << "\n\n";
            return;
        default:
            std::cout << "\nKeyword " << keyword << "has undefined type." << std::endl;
            return;
    }
    std::cout << "done." << std::endl;
}



void IntegrationTest::setCellVolumes() {
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
        const double cellVolume1 = ecl_grid_get_cell_volume1(ecl_grid1, cell);
        const double cellVolume2 = ecl_grid_get_cell_volume1(ecl_grid2, cell);
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



void IntegrationTest::initialOccurrenceCompare(const std::string& keyword) {
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



void IntegrationTest::occurrenceCompare(const std::string& keyword, int occurrence) const {
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



IntegrationTest::IntegrationTest(const std::string& basename1, const std::string& basename2, double absTolerance, double relTolerance):
    ECLFilesComparator(ECL_UNIFIED_RESTART_FILE, basename1, basename2, absTolerance, relTolerance) {
    std::cout << "\nUsing cell volumes and keyword values from case " << basename2
              << " as reference." << std::endl << std::endl;
    setCellVolumes();
}



bool IntegrationTest::elementInWhitelist(const std::string& keyword) const {
    auto it = std::find(keywordWhitelist.begin(), keywordWhitelist.end(), keyword);
    return it != keywordWhitelist.end();
}



void IntegrationTest::equalNumKeywords() const {
    if (keywords1.size() != keywords2.size()) {
        OPM_THROW(std::runtime_error, "\nKeywords in first file: " << keywords1.size()
                << "\nKeywords in second file: " << keywords2.size()
                << "\nThe number of keywords differ.");
    }
}



void IntegrationTest::results() {
    for (const auto& it : keywordWhitelist)
        resultsForKeyword(it);
}



void IntegrationTest::resultsForKeyword(const std::string keyword) {
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
