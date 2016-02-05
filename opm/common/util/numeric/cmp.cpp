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

#include <math.h>
#include <string.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <opm/common/util/numeric/cmp.hpp>


namespace Opm {
    namespace cmp {

        bool double_equal(double value1 , double value2 , double abs_eps , double rel_eps) {
            bool equal = true;
            double diff = fabs(value1 - value2);
            if (diff > abs_eps) {
                double scale = std::max(fabs(value1), fabs(value2));

                if (diff > scale * rel_eps) {
                    equal = false;
                }
            }
            return equal;
        }


        bool double_equal(double value1 , double value2) {
            return double_equal( value1 , value2 , default_abs_epsilon , default_rel_epsilon );
        }

        /*****************************************************************/

        bool double_vector_equal(const std::vector<double>& v1, const std::vector<double>& v2, double abs_eps, double rel_eps) {
            if (v1.size() != v2.size()) {
                return false;
            }

            for (size_t i = 0; i < v1.size(); i++) {
                if (!double_equal( v1[i] , v2[i]))
                    return false;
            }

            return true;
        }

        bool double_vector_equal(const std::vector<double>& v1, const std::vector<double>& v2) {
            return double_vector_equal( v1 , v2 , default_abs_epsilon , default_rel_epsilon);
        }


        bool double_ptr_equal(const double * p1, const double *p2, size_t num_elements, double abs_eps , double rel_eps) {
            if (memcmp(p1 , p2 , num_elements * sizeof * p1) == 0)
                return true;
            else {
                size_t index;
                for (index = 0; index < num_elements; index++) {
                    if (!double_equal( p1[index] , p2[index] , abs_eps , rel_eps)) {
                        return false;
                    }
                }
            }
            return true;
        }


        bool double_ptr_equal(const double * p1, const double *p2, size_t num_elements) {
            return double_ptr_equal( p1 , p2  ,num_elements , default_abs_epsilon , default_rel_epsilon );
        }

    }
}
