/*
  Copyright 2025 Equinor ASA.
  Copyright 2016, 2017, 2018 Statoil ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#include <opm/output/eclipse/VectorItems/lgrheadd.hpp>
#include <opm/output/eclipse/LgrHEADD.hpp>

#include <cmath>
#include <ctime>
#include <vector>

// Public LGRHEADD items are recorded in the common header file
//
//     opm/output/eclipse/VectorItems/lgrheadd.hpp
//
// Promote items from 'index' to that list to make them public.
// The 'index' list always uses public items where available.
namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

enum index : std::vector<int>::size_type {
    ih_001      =       VI::lgrheadd::FIRST,               //       0       0
    ih_002      =       VI::lgrheadd::SECOND,              //       0       0
    ih_003      =       VI::lgrheadd::THIRD,               //       0       0
    ih_004      =       VI::lgrheadd::FOURTH,              //       0       0
    ih_005      =       VI::lgrheadd::FIFTH,              //       0       0

    
 // ---------------------------------------------------------------------
 // ---------------------------------------------------------------------
  LGRHEADD_NUMBER_OF_ITEMS        // MUST be last element of enum.
};

// =====================================================================
// Public Interface Below Separator
// =====================================================================

Opm::RestartIO::LgrHEADD::LgrHEADD()
    : data_(LGRHEADD_NUMBER_OF_ITEMS, -0.1E+21)
{
    this->data_[ih_001] = 0.0;
}
