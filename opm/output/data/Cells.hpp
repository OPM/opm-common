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

#ifndef OPM_OUTPUT_CELLS_HPP
#define OPM_OUTPUT_CELLS_HPP

#include <map>
#include <vector>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

class SimulationDataContainer;

namespace data {

    enum class TargetType {
        RESTART_SOLUTION,
        RESTART_AUXILLARY,
        SUMMARY,
        INIT,
    };

    /**
     * Small struct that keeps track of data for output to restart/summary files.
     */
    struct CellData {
        std::string name;          //< Name of the output field (will end up "verbatim" in output)
        UnitSystem::measure dim;   //< Dimension of the data to write
        std::vector<double> data;  //< The actual data itself
        TargetType target;
    };

}
}

#endif //OPM_OUTPUT_CELLS_HPP
