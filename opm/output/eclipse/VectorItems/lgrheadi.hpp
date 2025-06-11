/*
  Copyright (c) 2025 Equinor ASA

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

#ifndef OPM_OUTPUT_ECLIPSE_VECTOR_LGRHEADI_HPP
#define OPM_OUTPUT_ECLIPSE_VECTOR_LGRHEADI_HPP

#include <vector>

namespace Opm { namespace RestartIO { namespace Helpers { namespace VectorItems {

    // This is a subset of the items in src/opm/output/eclipse/LgrHeadI.cpp .
    // Promote items from that list to this in order to make them public.
    enum lgrheadi : std::vector<int>::size_type {
        LGR_INDEX = 0, //  An integer corresponding to number of a given
                       //  LGR cell.
        IS_ACTIVE = 1, //  If LGR Cell not in use set it to  < 0

    };

}}}} // Opm::RestartIO::Helpers::VectorItems

#endif // OPM_OUTPUT_ECLIPSE_VECTOR_LGRHEADI_HPP
