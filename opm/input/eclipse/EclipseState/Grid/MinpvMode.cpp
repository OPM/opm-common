/*
  Copyright 2015 Statoil ASA.

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

#include <config.h>
#include <opm/input/eclipse/EclipseState/Grid/MinpvMode.hpp>

#include <ostream>
#include <stdexcept>

namespace Opm {

std::ostream& operator<<(std::ostream& os, const MinpvMode& mm)
{
    if (mm == MinpvMode::EclSTD)
        os << "EclStd";
    else if (mm == MinpvMode::Inactive)
        os << "Inactive";
    else
        throw std::runtime_error("Unsupported MinpvMode type in operator<<()");

    return os;
}

}
