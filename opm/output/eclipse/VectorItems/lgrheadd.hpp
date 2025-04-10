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

#ifndef OPM_OUTPUT_ECLIPSE_VECTOR_LGRHEADD_HPP
#define OPM_OUTPUT_ECLIPSE_VECTOR_LGRHEADD_HPP

#include <vector>

namespace Opm { namespace RestartIO { namespace Helpers { namespace VectorItems {

    // This is a subset of the items in src/opm/output/eclipse/LgrHeadQ.cpp .
    // Promote items from that list to this in order to make them public.
    enum lgrheadd : std::vector<int>::size_type {
        FIRST  = 0,   // First element: 0
        SECOND = 1,   // Second element: -0.10000000000000D+21
        THIRD  = 2,   // Third element: -0.10000000000000D+21
        FOURTH = 3,   // Fourth element: -0.10000000000000D+21
        FIFTH  = 4    // Fifth element: -0.10000000000000D+21

    };

}}}} // Opm::RestartIO::Helpers::VectorItems

#endif // OPM_OUTPUT_ECLIPSE_VECTOR_LGRHEADD_HPP
