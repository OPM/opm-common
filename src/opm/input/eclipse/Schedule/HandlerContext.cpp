/*
  Copyright 2013 Statoil ASA.

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

#include "HandlerContext.hpp"

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Schedule/Action/SimulatorUpdate.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include "MSW/WelSegsSet.hpp"

#include <stdexcept>

namespace Opm {

void HandlerContext::affected_well(const std::string& well_name)
{
    if (sim_update) {
        sim_update->affected_wells.insert(well_name);
    }
}

void HandlerContext::record_tran_change()
{
    if (sim_update) {
        sim_update->tran_update = true;
    }
}

void HandlerContext::record_well_structure_change()
{
    if (sim_update) {
        sim_update->well_structure_changed = true;
    }
}

void HandlerContext::welsegs_handled(const std::string& well_name)
{
    if (welsegs_wells) {
        welsegs_wells->insert(well_name, keyword.location());
    }
}

void HandlerContext::compsegs_handled(const std::string& well_name)
{
    if (compsegs_wells) {
        compsegs_wells->insert(well_name);
    }
}

ScheduleState& HandlerContext::state()
{
    return schedule_.snapshots[currentStep];
}

double HandlerContext::getWellPI(const std::string& well_name) const
{
    if (!target_wellpi) {
        throw std::logic_error("Lookup of well PI with no map available");
    }

    auto wellpi_iter = target_wellpi->find(well_name);
    if (wellpi_iter == target_wellpi->end()) {
        throw std::logic_error("Missing current PI for well " + well_name);
    }

    return wellpi_iter->second;
}

}
