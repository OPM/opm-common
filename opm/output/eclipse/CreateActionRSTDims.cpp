/*
  Copyright (c) 2018 Equinor ASA
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

#include <opm/output/eclipse/VectorItems/action.hpp>

#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Schedule/Action/Actdims.hpp>
#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>

#include <cstddef>
#include <vector>

// ---------------------------------------------------------------------
// Public interface for restart file action dimensioning information.
// ---------------------------------------------------------------------

std::size_t
Opm::RestartIO::Helpers::entriesPerLine(const Actdims& actdims)
{
    return actdims.line_size();
}

std::size_t
Opm::RestartIO::Helpers::entriesPerZLACT(const Actdims& actdims,
                                         const Action::Actions& acts)
{
    return entriesPerLine(actdims)
        *  static_cast<std::size_t>(acts.max_input_lines());
}

std::size_t Opm::RestartIO::Helpers::entriesPerIACN(const Actdims& actdims)
{
    return VectorItems::IACN::ConditionSize * actdims.max_conditions();
}

std::size_t Opm::RestartIO::Helpers::entriesPerSACN(const Actdims& actdims)
{
    return VectorItems::SACN::ConditionSize * actdims.max_conditions();
}

std::size_t Opm::RestartIO::Helpers::entriesPerZACN(const Actdims& actdims)
{
    return VectorItems::ZACN::ConditionSize * actdims.max_conditions();
}



std::vector<int>
Opm::RestartIO::Helpers::
createActionRSTDims(const Schedule&     sched,
                    const std::size_t   simStep)
{
    const auto& acts = sched[simStep].actions();
    const auto& actdims = sched.runspec().actdims();
    std::vector<int> action_rst_dims(9);

    //No of Actionx keywords
    action_rst_dims[0] = acts.ecl_size();
    action_rst_dims[1] = entriesPerIACT();
    action_rst_dims[2] = entriesPerSACT();
    action_rst_dims[3] = entriesPerZACT();
    action_rst_dims[4] = entriesPerZLACT(actdims, acts);
    action_rst_dims[5] = entriesPerZACN(actdims);
    action_rst_dims[6] = entriesPerIACN(actdims);
    action_rst_dims[7] = entriesPerSACN(actdims);
    action_rst_dims[8] = entriesPerLine(actdims);

    return action_rst_dims;
}
