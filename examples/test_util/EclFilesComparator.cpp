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

#include <stdio.h>

#include <set>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <numeric>


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


template <typename T>
void ECLFilesComparator::printValuesForCell(const std::string& keyword, const std::string reference, size_t kw_size, size_t cell, EGrid *grid, const T& value1, const T& value2) const {

  int nActive=-1;
  int nTot=-1;

  if (grid!=nullptr){
      nActive=grid->activeCells();
      nTot=grid->totalNumberOfCells();
  }

  if (static_cast<int>(kw_size) == nActive) {
	
	int i, j, k;
	grid->ijk_from_active_index(cell, i, j, k);

        i++, j++, k++;

	std::cout << std::endl
                  << "\nKeyword: " << keyword << ", origin "  << reference << "\n"
                  << "Global index (zero based)   = "  << cell << "\n"
                  << "Grid coordinate             = (" << i << ", " << j << ", " << k << ")" << "\n"
                  << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";
        return;
    }

    if (static_cast<int>(kw_size) == nTot) {

	int i, j, k;
	grid->ijk_from_global_index(cell, i, j, k);

        i++, j++, k++;

	std::cout << std::endl
                  << "\nKeyword: " << keyword << ", origin "  << reference << "\n\n"
                  << "Global index (zero based)   = "  << cell << "\n"
                  << "Grid coordinate             = (" << i << ", " << j << ", " << k << ")" << "\n"
                  << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";
	return;
    }

    std::cout << std::endl
              << "\nKeyword: " << keyword << ", origin "  << reference << "\n\n"
              << "Value index                 = "  << cell << "\n"
              << "(first value, second value) = (" << value1 << ", " << value2 << ")\n\n";
}

template void ECLFilesComparator::printValuesForCell<bool>       (const std::string& keyword, const std::string reference, size_t kw_size, size_t cell, EGrid *grid, const bool&        value1, const bool&        value2) const;
template void ECLFilesComparator::printValuesForCell<int>        (const std::string& keyword, const std::string reference, size_t kw_size, size_t cell, EGrid *grid, const int&         value1, const int&         value2) const;
template void ECLFilesComparator::printValuesForCell<double>     (const std::string& keyword, const std::string reference, size_t kw_size, size_t cell, EGrid *grid, const double&      value1, const double&      value2) const;
template void ECLFilesComparator::printValuesForCell<std::string>(const std::string& keyword, const std::string reference, size_t kw_size, size_t cell, EGrid *grid, const std::string& value1, const std::string& value2) const;


ECLFilesComparator::ECLFilesComparator(const std::string& basename1,
                                       const std::string& basename2,
                                       double absToleranceArg, double relToleranceArg) :
    absTolerance(absToleranceArg), relTolerance(relToleranceArg) {

    rootName1=basename1;
    rootName2=basename2;
}


Deviation ECLFilesComparator::calculateDeviations(double val1, double val2) {
    val1 = std::abs(val1);
    val2 = std::abs(val2);


    Deviation deviation;
    if (val1 != 0 || val2 != 0) {
        deviation.abs = std::fabs(val1 - val2);
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


