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

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm::Action {
    class WGNames;
}

namespace Opm {
class DeckKeyword;
class DeckRecord;
class ErrorGuard;
class ParseContext;
class Schedule;
class ScheduleBlock;
class ScheduleGrid;
class ScheduleState;
struct ScheduleStatic;
struct SimulatorUpdate;
enum class WellStatus;
class WelSegsSet;
}

namespace Opm {
class HandlerContext
{
public:
    /// \param welsegs_wells All wells with a WELSEGS entry for checks.
    /// \param compegs_wells All wells with a COMPSEGS entry for checks.
    HandlerContext(Schedule& schedule,
                   const ScheduleBlock& block_,
                   const DeckKeyword& keyword_,
                   const ScheduleGrid& grid_,
                   const std::size_t currentStep_,
                   const Action::Result::MatchingEntities& matches_,
                   bool action_mode_,
                   const ParseContext& parseContext_,
                   ErrorGuard& errors_,
                   SimulatorUpdate* sim_update_,
                   const std::unordered_map<std::string, double>* target_wellpi_,
                   std::unordered_map<std::string, double>& wpimult_global_factor_,
                   WelSegsSet* welsegs_wells_,
                   std::set<std::string>* compsegs_wells_)
        : block(block_)
        , keyword(keyword_)
        , currentStep(currentStep_)
        , matches(matches_)
        , action_mode(action_mode_)
        , parseContext(parseContext_)
        , errors(errors_)
        , wpimult_global_factor(wpimult_global_factor_)
        , grid(grid_)
        , target_wellpi(target_wellpi_)
        , welsegs_wells(welsegs_wells_)
        , compsegs_wells(compsegs_wells_)
        , sim_update(sim_update_)
        , schedule_(schedule)
    {}

    //! \brief Mark that a well has changed.
    void affected_well(const std::string& well_name);

    //! \brief Mark that a well is affected by WELPI.
    void welpi_well(const std::string& well_name);

    //! \brief Mark that transmissibilities must be recalculated.
    void record_tran_change();

    //! \brief Mark that well structure has changed.
    void record_well_structure_change();

    //! \brief Returns a reference to current state.
    ScheduleState& state();

    //! \brief Returns a const-ref to the static schedule.
    const ScheduleStatic& static_schedule() const;

    /// \brief Mark that the well occured in a WELSEGS keyword.
    void welsegs_handled(const std::string& well_name);

    /// \brief Mark that the well occured in a COMPSEGS keyword.
    void compsegs_handled(const std::string& well_name);

    //! \brief Set exit code.
    void setExitCode(int code);

    //! \brief Update status of a well.
    bool updateWellStatus(const std::string& well,
                          WellStatus status,
                          const std::optional<KeywordLocation>& location = {});

    //! \brief Get status of a well
    WellStatus getWellStatus(const std::string& well) const;

    //! \brief Adds a group to the schedule.
    void addGroup(const std::string& groupName);
    //! \brief Adds a group to a group.
    void addGroupToGroup(const std::string& parent_group,
                         const std::string& child_group);

    //! \brief Create a new Well from a WELSPECS record.
    void welspecsCreateNewWell(const DeckRecord&  record,
                               const std::string& wellName,
                               const std::string& groupName);

    //! \brief Update one or more existing wells from a WELSPECS record.
    void welspecsUpdateExistingWells(const DeckRecord&               record,
                                     const std::vector<std::string>& wellNames,
                                     const std::string&              groupName);

    //! \brief Obtain PI for a well.
    double getWellPI(const std::string& well_name) const;

    //! \brief Returns elapsed time since simulation start in seconds.
    double elapsed_seconds() const;

    //! \brief Adds parse error for an invalid name pattern.
    void invalidNamePattern(const std::string& namePattern) const;

    //! \brief Obtain action well group names.
    const Action::WGNames& action_wgnames() const;

    //! \brief Obtain well group names from a pattern.
    std::vector<std::string> groupNames(const std::string& pattern) const;

    //! \brief Obtain well names from a pattern.
    //! \details Throws if no wells match the pattern and pattern is not a WLIST.
    std::vector<std::string>
    wellNames(const std::string& pattern) const;

    //! \brief Obtain well names from a pattern.
    //! \param pattern Pattern to match
    //! \param allowEmpty If true do not throw if no wells match the pattern
    std::vector<std::string>
    wellNames(const std::string& pattern, bool allowEmpty) const;

    const ScheduleBlock& block;
    const DeckKeyword& keyword;
    const std::size_t currentStep;
    const Action::Result::MatchingEntities& matches;
    const bool action_mode;
    const ParseContext& parseContext;
    ErrorGuard& errors;
    std::unordered_map<std::string, double>& wpimult_global_factor;
    const ScheduleGrid& grid;

private:
    const std::unordered_map<std::string, double>* target_wellpi{nullptr};
    WelSegsSet* welsegs_wells{nullptr};
    std::set<std::string>* compsegs_wells{nullptr};
    SimulatorUpdate* sim_update{nullptr};
    Schedule& schedule_;
};

} // end namespace Opm

#endif // HANDLER_CONTEXT_HPP
