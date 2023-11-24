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

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Schedule/Action/SimulatorUpdate.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include "MSW/WelSegsSet.hpp"

#include <fmt/format.h>

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

const ScheduleStatic& HandlerContext::static_schedule() const
{
    return schedule_.m_static;
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

double HandlerContext::elapsed_seconds() const
{
    return schedule_.seconds(currentStep);
}

void HandlerContext::invalidNamePattern(const std::string& namePattern) const
{
    std::string msg_fmt = fmt::format("No wells/groups match the pattern: \'{}\'", namePattern);
    if (namePattern == "?") {
        /*
          In particular when an ACTIONX keyword is called via PYACTION
          coming in here with an empty list of matching wells is not
          entirely unheard of. It is probably not what the user wanted and
          we give a warning, but the simulation continues.
        */
        auto msg = OpmInputError::format("No matching wells for ACTIONX {keyword}"
                                         "in {file} line {line}.", keyword.location());
        OpmLog::warning(msg);
    } else {
        parseContext.handleError(ParseContext::SCHEDULE_INVALID_NAME,
                                 msg_fmt, keyword.location(), errors);
    }
}

const Action::WGNames& HandlerContext::action_wgnames() const
{
    return schedule_.action_wgnames;
}

std::vector<std::string>
HandlerContext::groupNames(const std::string& pattern) const
{
    return schedule_.groupNames(pattern);
}

std::vector<std::string>
HandlerContext::wellNames(const std::string& pattern, bool allowEmpty) const
{
    return schedule_.wellNames(pattern, *this, allowEmpty);
}

std::vector<std::string>
HandlerContext::wellNames(const std::string& pattern) const
{
    bool allowEmpty = schedule_.isWList(currentStep, pattern);
    return this->wellNames(pattern, allowEmpty);
}

}
