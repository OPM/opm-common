/*
  Copyright (c) 2025 Equinor ASA
  Copyright (c) 2018 Statoil ASA

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

#include <opm/output/eclipse/WriteRestartHelpers.hpp>
#include <opm/output/eclipse/LgrHEADD.hpp>
#include <opm/output/eclipse/VectorItems/lgrheadd.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>


#include <vector>



// #####################################################################
// Public Interface (createInteHead()) Below Separator
// ---------------------------------------------------------------------

std::vector<double>
Opm::RestartIO::Helpers::
createLgrHeadd()
{    
    const auto ih = LgrHEADD()
        ;

    return ih.data();
}
