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
#ifndef HANDLER_CONTEXT_HPP
#define HANDLER_CONTEXT_HPP

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <cstddef>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

class DeckKeyword;
class ErrorGuard;
class ParseContext;
class ScheduleBlock;
class ScheduleGrid;
class ScheduleState;
struct SimulatorUpdate;
class WelSegsSet;

class HandlerContext
{
public:
    /// \param welsegs_wells All wells with a WELSEGS entry for checks.
    /// \param compegs_wells All wells with a COMPSEGS entry for checks.
    HandlerContext(const ScheduleBlock& block_,
                   const DeckKeyword& keyword_,
                   const ScheduleGrid& grid_,
                   const std::size_t currentStep_,
                   const std::vector<std::string>& matching_wells_,
                   bool actionx_mode_,
                   const ParseContext& parseContext_,
                   ErrorGuard& errors_,
                   SimulatorUpdate* sim_update_,
                   const std::unordered_map<std::string, double>* target_wellpi_,
                   std::unordered_map<std::string, double>* wpimult_global_factor_,
                   WelSegsSet* welsegs_wells_,
                   std::set<std::string>* compsegs_wells_)
        : block(block_)
        , keyword(keyword_)
        , currentStep(currentStep_)
        , matching_wells(matching_wells_)
        , actionx_mode(actionx_mode_)
        , parseContext(parseContext_)
        , errors(errors_)
        , sim_update(sim_update_)
        , target_wellpi(target_wellpi_)
        , wpimult_global_factor(wpimult_global_factor_)
        , welsegs_wells(welsegs_wells_)
        , compsegs_wells(compsegs_wells_)
        , grid(grid_)
    {}

    //! \brief Mark that a well has changed.
    void affected_well(const std::string& well_name);

    //! \brief Mark that well structure has changed.
    void record_well_structure_change();

    /// \brief Mark that the well occured in a WELSEGS keyword.
    void welsegs_handled(const std::string& well_name);

    /// \brief Mark that the well occured in a COMPSEGS keyword.
    void compsegs_handled(const std::string& well_name);

    const ScheduleBlock& block;
    const DeckKeyword& keyword;
    const std::size_t currentStep;
    const std::vector<std::string>& matching_wells;
    const bool actionx_mode;
    const ParseContext& parseContext;
    ErrorGuard& errors;
    SimulatorUpdate* sim_update{nullptr};
    const std::unordered_map<std::string, double>* target_wellpi{nullptr};
    std::unordered_map<std::string, double>* wpimult_global_factor{nullptr};
    WelSegsSet* welsegs_wells{nullptr};
    std::set<std::string>* compsegs_wells{nullptr};
    const ScheduleGrid& grid;
};

} // end namespace Opm

#endif // HANDLER_CONTEXT_HPP
