/*
  Copyright 2016 Statoil ASA.

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

#ifndef COMMON_UTIL_NUMERIC_CMP
#define COMMON_UTIL_NUMERIC_CMP

#include <cstddef>
#include <vector>

namespace Opm {

    /// In the namespace cmp are implemented functions for
    /// approximate comparison of double values based on absolute
    /// and relative difference. There are three functions:
    ///
    ///   double_equal() : Compare two double values.
    ///
    ///   double_ptr_equal(): This compares all the element in the
    ///      two double * pointers.
    ///
    ///   double_vector_equal(): This compares all the elements in
    ///      two std::vector<double> instances.
    ///
    /// For both double_vector_equal() and double_ptr_equal() the
    /// actual comparison is based on the double_equal()
    /// function. All functions exist as two overloads, one which
    /// takes explicit input values for the absolute and relative
    /// epsilon, and one which uses default values.
    ///
    /// For more details of floating point comparison please consult
    /// this reference:
    ///
    ///    https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
    namespace cmp {

        const double default_abs_epsilon = 1e-8;
        const double default_rel_epsilon = 1e-5;


        bool double_equal(double value1, double value2, double abs_eps , double rel_eps);
        bool double_equal(double value1, double value2);

        bool double_vector_equal(const std::vector<double>& v1, const std::vector<double>& v2, double abs_eps, double rel_eps);
        bool double_vector_equal(const std::vector<double>& v1, const std::vector<double>& v2);

        bool double_ptr_equal(const double* p1, const double* p2, size_t num_elements, double abs_eps, double rel_eps);
        bool double_ptr_equal(const double* p1, const double* p2, size_t num_elements);
    }
}

#endif
