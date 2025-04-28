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
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/input/eclipse/Schedule/Action/SimulatorUpdate.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include "MSW/WelSegsSet.hpp"

#include <fmt/format.h>

#include <stdexcept>
#include <iostream>

namespace Opm {

void HandlerContext::affected_well(const std::string& well_name)
{
    if (sim_update) {
        sim_update->affected_wells.insert(well_name);
    }
}

void HandlerContext::welpi_well(const std::string& well_name)
{
    if (sim_update != nullptr) {
        sim_update->welpi_wells.insert(well_name);
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

void HandlerContext::setExitCode(int code)
{
    schedule_.exit_status = code;
}

bool HandlerContext::updateWellStatus(const std::string& well,
                                      WellStatus status,
                                      const std::optional<KeywordLocation>& location)
{
    return schedule_.updateWellStatus(well, currentStep,
                                      status, location);
}

WellStatus HandlerContext::getWellStatus(const std::string& well) const
{
    return schedule_.getWell(well, currentStep).getStatus();
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
        // In particular when an ACTIONX keyword is called via PYACTION
        // coming in here with an empty list of matching wells is not
        // entirely unheard of. It is probably not what the user wanted and
        // we give a warning, but the simulation continues.
        auto msg = OpmInputError::format("No matching wells for ACTIONX keyword '{keyword}' "
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

void HandlerContext::addGroup(const std::string& groupName)
{
    schedule_.addGroup(groupName, currentStep);
}

void HandlerContext::addGroupToGroup(const std::string& parent_group,
                                     const std::string& child_group)
{
    schedule_.addGroupToGroup(parent_group, child_group);
}

void HandlerContext::welspecsCreateNewWell(const DeckRecord&  record,
                                           const std::string& wellName,
                                           const std::string& groupName)
{
    auto wellConnectionOrder = Connection::Order::TRACK;

    if (const auto& compord = block.get("COMPORD"); compord.has_value())
    {
        const auto nrec = compord->size();

        for (auto compordRecordNr = 0*nrec; compordRecordNr < nrec; ++compordRecordNr) {
            const auto& compordRecord = compord->getRecord(compordRecordNr);

            const std::string& wellNamePattern = compordRecord.getItem(0).getTrimmedString(0);

            if (Well::wellNameInWellNamePattern(wellName, wellNamePattern)) {
                const std::string& compordString = compordRecord.getItem(1).getTrimmedString(0);
                wellConnectionOrder = Connection::OrderFromString(compordString);
            }
        }
    }

    schedule_.addWell(wellName, record, currentStep, wellConnectionOrder);
    schedule_.addWellToGroup(groupName, wellName, currentStep);

    this->affected_well(wellName);
}

void HandlerContext::
welspecsUpdateExistingWells(const DeckRecord&               record,
                            const std::vector<std::string>& wellNames,
                            const std::string&              groupName)
{
    using Kw = ParserKeywords::WELSPECS;

    const auto& headI = record.getItem<Kw::HEAD_I>();
    const auto& headJ = record.getItem<Kw::HEAD_J>();
    const auto& pvt   = record.getItem<Kw::P_TABLE>();
    const auto& drad  = record.getItem<Kw::D_RADIUS>();
    const auto& ref_d = record.getItem<Kw::REF_DEPTH>();

    const auto I = headI.defaultApplied(0) ? std::nullopt : std::optional<int> {headI.get<int>(0) - 1};
    const auto J = headJ.defaultApplied(0) ? std::nullopt : std::optional<int> {headJ.get<int>(0) - 1};

    const auto pvt_table = pvt.defaultApplied(0) ? std::nullopt : std::optional<int> { pvt.get<int>(0) };
    const auto drainageRadius = drad.defaultApplied(0) ? std::nullopt : std::optional<double> { drad.getSIDouble(0) };

    auto ref_depth = std::optional<double>{};
    if (! ref_d.defaultApplied(0) && ref_d.hasValue(0)) {
        ref_depth.emplace(ref_d.getSIDouble(0));
    }

    const std::string& cross_flow_str = record.getItem<Kw::CROSSFLOW>().getTrimmedString(0);
    const bool allow_crossflow = cross_flow_str != "NO";

    const std::string& shutin_str = record.getItem<Kw::AUTO_SHUTIN>().getTrimmedString(0);
    const bool auto_shutin = shutin_str != "STOP";

    for (const auto& wellName : wellNames) {
        auto well = state().wells.get(wellName);

        const auto updateHead = well.updateHead(I, J);
        const auto updateRefD = well.updateRefDepth(ref_depth);
        const auto updateDRad = well.updateDrainageRadius(drainageRadius);
        const auto updatePVT  = well.updatePVTTable(pvt_table);
        const auto updateCrossflow = well.updateCrossFlow(allow_crossflow);
        const auto updateShutin = well.updateAutoShutin(auto_shutin);

        if (updateHead || updateRefD || updateDRad || updatePVT || updateCrossflow || updateShutin) {
            well.updateRefDepth();

            state().wellgroup_events()
                .addEvent(wellName, ScheduleEvents::WELL_WELSPECS_UPDATE);

            state().wells.update(std::move(well));

            this->affected_well(wellName);
        }

        this->schedule_.addWellToGroup(groupName, wellName, currentStep);
    }
}

}
