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

#include <opm/output/eclipse/LgrHEADI.hpp>

#include <opm/output/eclipse/VectorItems/lgrheadi.hpp>

#include <cmath>
#include <ctime>
#include <vector>

// Public LGRHEADI items are recorded in the common header file
//
//     opm/output/eclipse/VectorItems/lgrheadi.hpp
//
// Promote items from 'index' to that list to make them public.
// The 'index' list always uses public items where available.
namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

enum index : std::vector<int>::size_type {
    LGR_INDEX    =       VI::lgrheadi::LGR_INDEX,//       0       0
    IS_ACTIVE    =       VI::lgrheadi::IS_ACTIVE,//       0       0
    ih_003       =       3       ,               //       0       0
    ih_004       =       4       ,               //       0       0
    ih_005       =       5       ,               //       0       0
    ih_006       =       6       ,               //       0       0
    ih_007       =       7       ,               //       0       0
    ih_008       =       8       ,               //       0       0
    ih_009       =       9       ,               //       0       0
    ih_010       =      10       ,               //       0       0
    ih_011       =      11       ,               //       0       0
    ih_012       =      12       ,               //       0       0
    ih_013       =      13       ,               //       0       0
    ih_014       =      14       ,               //       0       0
    ih_015       =      15       ,               //       0       0
    ih_016       =      16       ,               //       0       0
    ih_017       =      17       ,               //       0       0
    ih_018       =      18       ,               //       0       0
    ih_019       =      19       ,               //       0       0
    ih_020       =      20       ,               //       0       0
    ih_021       =      21       ,               //       0       0
    ih_022       =      22       ,               //       0       0
    ih_023       =      23       ,               //       0       0
    ih_024       =      24       ,               //       0       0
    ih_025       =      25       ,               //       0       0
    ih_026       =      26       ,               //       0       0
    ih_027       =      27       ,               //       0       0
    ih_028       =      28       ,               //       0       0
    ih_029       =      29       ,               //       0       0
    ih_030       =      30       ,               //       0       0
    ih_031       =      31       ,               //       0       0
    ih_032       =      32       ,               //       0       0
    ih_033       =      33       ,               //       0       0
    ih_034       =      34       ,               //       0       0
    ih_035       =      35       ,               //       0       0
    ih_036       =      36       ,               //       0       0
    ih_037       =      37       ,               //       0       0
    ih_038       =      38       ,               //       0       0
    ih_039       =      39       ,               //       0       0
    ih_040       =      40       ,               //       0       0
    ih_041       =      41       ,               //       0       0
    ih_042       =      42       ,               //       0       0
    ih_043       =      43       ,               //       0       0
    ih_044       =      44       ,               //       0       0

 // ---------------------------------------------------------------------
 // ---------------------------------------------------------------------
  LGRHEADI_NUMBER_OF_ITEMS        // MUST be last element of enum.
};

// =====================================================================
// Public Interface Below Separator
// =====================================================================

Opm::RestartIO::LgrHEADI::LgrHEADI()
    : data_(LGRHEADI_NUMBER_OF_ITEMS, 0)
{}

Opm::RestartIO::LgrHEADI&
Opm::RestartIO::LgrHEADI::
toggleLGRCell(const bool isLgrCell)
{
    if (isLgrCell)
    {
        this -> data_[IS_ACTIVE] = 100;
    }
    else
    {
        this -> data_[IS_ACTIVE] = 0;
    }
    return *this;
}

Opm::RestartIO::LgrHEADI&
Opm::RestartIO::LgrHEADI::numberoOfLGRCell(const int nactive)
{
    this->data_[LGR_INDEX] = nactive;

    return *this;
}
