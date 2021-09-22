/*
  Copyright 2018 Statoil ASA

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
#include <fmt/format.h>

#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GTNode.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <exception>
#include <string>
#include <stdexcept>

#define ENABLE_GCNTL_DEBUG_OUTPUT 0

#if ENABLE_GCNTL_DEBUG_OUTPUT
#include <iostream>
#endif // ENABLE_GCNTL_DEBUG_OUTPUT

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateGroupData
// ---------------------------------------------------------------------


namespace {

// maximum number of groups
std::size_t ngmaxz(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NGMAXZ];
}

// maximum number of wells in any group
int nwgmax(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NWGMAX];
}
 namespace value = ::Opm::RestartIO::Helpers::VectorItems::IGroup::Value;
 value::GuideRateMode GuideRateModeFromGuideRateProdTarget(Opm::Group::GuideRateProdTarget grpt) {
    switch (grpt) {
        case Opm::Group::GuideRateProdTarget::OIL:
            return value::GuideRateMode::Oil;
        case Opm::Group::GuideRateProdTarget::WAT:
            return value::GuideRateMode::Water;
        case Opm::Group::GuideRateProdTarget::GAS:
            return value::GuideRateMode::Gas;
        case Opm::Group::GuideRateProdTarget::LIQ:
            return value::GuideRateMode::Liquid;
        case Opm::Group::GuideRateProdTarget::RES:
            return value::GuideRateMode::Resv;
        case Opm::Group::GuideRateProdTarget::COMB:
            return value::GuideRateMode::Comb;
        case Opm::Group::GuideRateProdTarget::WGA:
            return value::GuideRateMode::None;
        case Opm::Group::GuideRateProdTarget::CVAL:
            return value::GuideRateMode::None;
        case Opm::Group::GuideRateProdTarget::INJV:
            return value::GuideRateMode::None;
        case Opm::Group::GuideRateProdTarget::POTN:
            return value::GuideRateMode::Potn;
        case Opm::Group::GuideRateProdTarget::FORM:
            return value::GuideRateMode::Form;
        case Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE:
            return value::GuideRateMode::None;

        default:
            throw std::logic_error(fmt::format("Not recognized value: {} for GuideRateProdTarget", grpt));
    }
}


template <typename GroupOp>
void groupLoop(const std::vector<const Opm::Group*>& groups,
               GroupOp&&                             groupOp)
{
    auto groupID = std::size_t {0};
    for (const auto* group : groups) {
        groupID += 1;

        if (group == nullptr) {
            continue;
        }

        groupOp(*group, groupID - 1);
    }
}

template < typename T>
std::pair<bool, int > findInVector(const std::vector<T>  & vecOfElements, const T  & element)
{
    std::pair<bool, int > result;

    // Find given element in vector
    auto it = std::find(vecOfElements.begin(), vecOfElements.end(), element);

    if (it != vecOfElements.end())
    {
        result.second = std::distance(vecOfElements.begin(), it);
        result.first = true;
    }
    else
    {
        result.first = false;
        result.second = -1;
    }
    return result;
}

int currentGroupLevel(const Opm::Schedule& sched, const Opm::Group& group, const size_t simStep)
{
    auto current = group;
    int level = 0;
    while (current.name() != "FIELD") {
        level += 1;
        current = sched.getGroup(current.parent(), simStep);
    }

    return level;
}

void groupCurrentlyProductionControllable(const Opm::Schedule& sched, const Opm::SummaryState& sumState, const Opm::Group& group, const size_t simStep, bool& controllable)
{
    using wellCtrlMode   = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::WellCtrlMode;
    if (controllable)
        return;

    for (const auto& group_name : group.groups()) {
        const auto& sub_group = sched.getGroup(group_name, simStep);
        auto cur_prod_ctrl = (sub_group.name() == "FIELD") ? static_cast<int>(sumState.get("FMCTP", -1)) :
                             static_cast<int>(sumState.get_group_var(sub_group.name(), "GMCTP", -1));
        if (cur_prod_ctrl <= 0) {
            //come here if group is controlled by higher level
            groupCurrentlyProductionControllable(sched, sumState, sched.getGroup(group_name, simStep), simStep, controllable);
        }
    }

    for (const auto& well_name : group.wells()) {
        const auto& well = sched.getWell(well_name, simStep);
        if (well.isProducer()) {
            int cur_prod_ctrl = 0;
            // Find control mode for well
            const std::string sum_key = "WMCTL";
            if (sumState.has_well_var(well_name, sum_key)) {
                cur_prod_ctrl = static_cast<int>(sumState.get_well_var(well_name, sum_key));
            }
            if (cur_prod_ctrl == wellCtrlMode::Group) {
                controllable = true;
                return;
            }
        }
    }
}


bool groupCurrentlyProductionControllable(const Opm::Schedule& sched, const Opm::SummaryState& sumState, const Opm::Group& group, const size_t simStep) {
    bool controllable = false;
    groupCurrentlyProductionControllable(sched, sumState, group, simStep, controllable);
    return controllable;
}


void groupCurrentlyInjectionControllable(const Opm::Schedule& sched, const Opm::SummaryState& sumState, const Opm::Group& group, const Opm::Phase& iPhase, const size_t simStep, bool& controllable)
{
    using wellCtrlMode   = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::WellCtrlMode;
    if (controllable)
        return;

    for (const auto& group_name : group.groups()) {
        const auto& sub_group = sched.getGroup(group_name, simStep);
        int cur_inj_ctrl = 0;
        if (iPhase == Opm::Phase::WATER) {
            cur_inj_ctrl = (sub_group.name() == "FIELD") ? static_cast<int>(sumState.get("FMCTW", -1)) :
                           static_cast<int>(sumState.get_group_var(sub_group.name(), "GMCTW", -1));
        } else if (iPhase == Opm::Phase::GAS) {
            cur_inj_ctrl = (sub_group.name() == "FIELD") ? static_cast<int>(sumState.get("FMCTG", -1)) :
                           static_cast<int>(sumState.get_group_var(sub_group.name(), "GMCTG", -1));
        }
        if (cur_inj_ctrl <= 0) {
            //come here if group is controlled by higher level
            groupCurrentlyInjectionControllable(sched, sumState, sched.getGroup(group_name, simStep), iPhase, simStep, controllable);
        }
    }

    for (const auto& well_name : group.wells()) {
        const auto& well = sched.getWell(well_name, simStep);
        if (well.isInjector() && iPhase == well.wellType().injection_phase()) {
            int cur_inj_ctrl = 0;
            // Find control mode for well
            const std::string sum_key = "WMCTL";
            if (sumState.has_well_var(well_name, sum_key)) {
                cur_inj_ctrl = static_cast<int>(sumState.get_well_var(well_name, sum_key));
            }

            if (cur_inj_ctrl == wellCtrlMode::Group) {
                controllable = true;
                return;
            }
        }
    }
}

bool groupCurrentlyInjectionControllable(const Opm::Schedule& sched, const Opm::SummaryState& sumState, const Opm::Group& group, const Opm::Phase& iPhase, const size_t simStep) {
    bool controllable = false;
    groupCurrentlyInjectionControllable(sched, sumState, group, iPhase, simStep, controllable);
    return controllable;
}


/*
  Searches upwards in the group tree for the first parent group with active
  control different from NONE and FLD. The function will return an empty
  optional if no such group can be found.
*/

std::optional<Opm::Group> controlGroup(const Opm::Schedule& sched,
                                       const Opm::SummaryState& sumState,
                                       const Opm::Group& group,
                                       const std::size_t simStep) {
    auto current = group;
    double cur_prod_ctrl = 0.;
    while (current.name() != "FIELD") {
        current = sched.getGroup(current.parent(), simStep);
        if (current.name() != "FIELD") {
            cur_prod_ctrl = sumState.get_group_var(current.name(), "GMCTP", 0);
        } else {
            cur_prod_ctrl = sumState.get("FMCTP", 0);
        }
        if (cur_prod_ctrl > 0)
            return current;
    }
    return {};
}


std::optional<Opm::Group>  injectionControlGroup(const Opm::Schedule& sched,
        const Opm::SummaryState& sumState,
        const Opm::Group& group,
        const std::string curGroupInjCtrlKey,
        const std::string curFieldInjCtrlKey,
        const size_t simStep)
//
// returns group of higher (highest) level group with active control different from (NONE or FLD)
//
{
    auto current = group;
    double cur_inj_ctrl = 0.;
    while (current.name() != "FIELD" ) {
        current = sched.getGroup(current.parent(), simStep);
        if (current.name() != "FIELD" ) {
                cur_inj_ctrl = sumState.get_group_var(current.name(), curGroupInjCtrlKey, 0.);
        } else {
            cur_inj_ctrl = sumState.get(curFieldInjCtrlKey, 0);
        }
        if (cur_inj_ctrl > 0) {
            return current;
        }
#if ENABLE_GCNTL_DEBUG_OUTPUT
        else {
            std::cout << "Current injection group control: " << curInjCtrlKey
                      << " is not defined for group: " << current.name() << " at timestep: " << simStep << std::endl;
        }
#endif // ENABLE_GCNTL_DEBUG_OUTPUT
    }
    return {};
} // namespace


std::vector<std::size_t> groupParentSeqIndex(const Opm::Schedule& sched,
        const Opm::Group& group,
        const size_t simStep)
//
// returns a vector with the group sequence index of all parent groups from current parent group to Field level
//
{
    std::vector<std::size_t> seq_numbers;
    auto current = group;
    while (current.name() != "FIELD") {
        current = sched.getGroup(current.parent(), simStep);
        seq_numbers.push_back(current.insert_index());
    }
    return seq_numbers;
}

namespace IGrp {
std::size_t entriesPerGroup(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NIGRPZ];
}

Opm::RestartIO::Helpers::WindowedArray<int>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

    return WV {
        WV::NumWindows{ ngmaxz(inteHead) },
        WV::WindowSize{ entriesPerGroup(inteHead) }
    };
}



template <class IGrpArray>
void gconprodCMode(const Opm::Group& group,
                   const int nwgmax,
                   IGrpArray& iGrp) {
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;

    const auto& prod_cmode = group.prod_cmode();
    iGrp[nwgmax + IGroup::GConProdCMode] = Opm::Group::ProductionCMode2Int(prod_cmode);
}


template <class IGrpArray>
void productionGroup(const Opm::Schedule&     sched,
                     const Opm::Group&        group,
                     const int                nwgmax,
                     const std::size_t        simStep,
                     const Opm::SummaryState& sumState,
                     IGrpArray&               iGrp)
{
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;
    namespace Value = ::Opm::RestartIO::Helpers::VectorItems::IGroup::Value;
    gconprodCMode(group, nwgmax, iGrp);
    const bool is_field = group.name() == "FIELD";
    const auto& production_controls = group.productionControls(sumState);
    const auto& prod_guide_rate_def = production_controls.guide_rate_def;
    Opm::Group::ProductionCMode active_cmode = Opm::Group::ProductionCMode::NONE;
    auto cur_prod_ctrl = (group.name() == "FIELD") ? sumState.get("FMCTP", -1) :
                         sumState.get_group_var(group.name(), "GMCTP", -1);
    if (cur_prod_ctrl >= 0)
        active_cmode = Opm::Group::ProductionCModeFromInt(static_cast<int>(cur_prod_ctrl));

#if ENABLE_GCNTL_DEBUG_OUTPUT
    else {
        // std::stringstream str;
        // str << "Current group production control is not defined for group: " << group.name() << " at timestep: " <<
        // simStep;
        std::cout << "Current group production control is not defined for group: " << group.name()
                  << " at timestep: " << simStep << std::endl;
        // throw std::invalid_argument(str.str());
    }
#endif // ENABLE_GCNTL_DEBUG_OUTPUT

    const auto& cgroup = controlGroup(sched, sumState, group, simStep);
    const auto& deck_cmode = group.prod_cmode();

    if (cgroup && (group.getGroupType() != Opm::Group::GroupType::NONE)) {
        auto cgroup_control = (cgroup->name() == "FIELD") ? static_cast<int>(sumState.get("FMCTP", 0)) : static_cast<int>(sumState.get_group_var(cgroup->name(), "GMCTP", 0));
        iGrp[nwgmax + IGroup::ProdActiveCMode]
            = (prod_guide_rate_def != Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE) ? cgroup_control : 0;
    } else {
        iGrp[nwgmax + IGroup::ProdActiveCMode] = Opm::Group::ProductionCMode2Int(active_cmode);

        // The PRBL and CRAT modes where not handled in a previous explicit if
        // statement; whether that was an oversight or a feature?
        if (active_cmode == Opm::Group::ProductionCMode::PRBL || active_cmode == Opm::Group::ProductionCMode::CRAT)
            iGrp[nwgmax + IGroup::ProdActiveCMode] = 0;
    }
    iGrp[nwgmax + 9] = iGrp[nwgmax + IGroup::ProdActiveCMode];

    iGrp[nwgmax + IGroup::GuideRateDef] = GuideRateModeFromGuideRateProdTarget(prod_guide_rate_def);

    // Set iGrp for [nwgmax + IGroup::ExceedAction]
    /*
    For the reduction option RATE the value is generally = 4

    For the reduction option NONE the values are as shown below, however, this is not a very likely case.

    = 0 for group with  "FLD" or "NONE"
    = 4 for "GRAT" FIELD
    = -40000 for production group with "ORAT"
    = -4000  for production group with "WRAT"
    = -400    for production group with "GRAT"
    = -40     for production group with "LRAT"

    Other reduction options are currently not covered in the code
    */

    const auto& p_exceed_act = production_controls.exceed_action;
    switch (deck_cmode) {
    case Opm::Group::ProductionCMode::NONE:
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? 0 : 4;
        break;
    case Opm::Group::ProductionCMode::ORAT:
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? -40000 : 4;
        break;
    case Opm::Group::ProductionCMode::WRAT:
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? -4000 : 4;
        break;
    case Opm::Group::ProductionCMode::GRAT:
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? -400 : 4;
        break;
    case Opm::Group::ProductionCMode::LRAT:
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? -40 : 4;
        break;
    case Opm::Group::ProductionCMode::RESV:
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? -4 : 4; // need to be checked
        break;
    case Opm::Group::ProductionCMode::FLD:
        if (cgroup && (prod_guide_rate_def != Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::GuideRateDef] = Value::GuideRateMode::Form;
        }
        iGrp[nwgmax + IGroup::ExceedAction] = (p_exceed_act == Opm::Group::ExceedAction::NONE) ? 4 : 4;
        break;
    default:
        iGrp[nwgmax + IGroup::ExceedAction] = 0;
    }

    // Start branching for determining iGrp[nwgmax + IGroup::ProdHighLevCtrl]
    // use default value if group is not available for group control

    if (group.getGroupType() == Opm::Group::GroupType::NONE) {
        if (is_field) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 0;
        } else {
            //set default value for the group's availability for higher level control for injection
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = (groupCurrentlyProductionControllable(sched, sumState, group, simStep) ) ? 1 : -1;
        }
        return;
    }

    if (cgroup && cgroup->name() == "FIELD")
        throw std::logic_error("Got cgroup == FIELD - uncertain logic");
    // group is available for higher level control, but is currently constrained by own limits
    iGrp[nwgmax + IGroup::ProdHighLevCtrl] = -1;
    if ((deck_cmode != Opm::Group::ProductionCMode::FLD) && !group.productionGroupControlAvailable()) {
        //group is not free to respond to higher level control)
        iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 0;
    } else if (cgroup && ((active_cmode == Opm::Group::ProductionCMode::FLD) || (active_cmode == Opm::Group::ProductionCMode::NONE))) {
        //a higher level group control is active constraint
        if ((deck_cmode != Opm::Group::ProductionCMode::FLD) && (deck_cmode != Opm::Group::ProductionCMode::NONE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = cgroup->insert_index();
        } else if ((deck_cmode == Opm::Group::ProductionCMode::FLD) && (prod_guide_rate_def != Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = cgroup->insert_index();
        } else if ((deck_cmode == Opm::Group::ProductionCMode::NONE) && group.productionGroupControlAvailable() &&
                   (prod_guide_rate_def != Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = cgroup->insert_index();
            //group is directly under higher level controlGroup
        } else if ((deck_cmode == Opm::Group::ProductionCMode::FLD) && (prod_guide_rate_def == Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 1;
        } else if ((deck_cmode == Opm::Group::ProductionCMode::NONE) && group.productionGroupControlAvailable() &&
                   (prod_guide_rate_def == Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 1;
        }
    } else if (!cgroup && active_cmode == Opm::Group::ProductionCMode::NONE) {
        //group is directly under higher level controlGroup
        if ((deck_cmode == Opm::Group::ProductionCMode::FLD) && (prod_guide_rate_def == Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 1;
        } else if ((deck_cmode == Opm::Group::ProductionCMode::NONE) && group.productionGroupControlAvailable() &&
                   (prod_guide_rate_def == Opm::Group::GuideRateProdTarget::NO_GUIDE_RATE)) {
            iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 1;
        }
    }
}

std::tuple<int, int, int> injectionGroup(const Opm::Schedule&     sched,
                                         const Opm::Group&        group,
                                         const std::size_t        simStep,
                                         const Opm::SummaryState& sumState,
                                         const Opm::Phase         phase)
{
    const bool is_field = group.name() == "FIELD";
    auto group_parent_list = groupParentSeqIndex(sched, group, simStep);
    int high_level_ctrl;
    int current_cmode = 0;
    int gconinje_cmode = 0;

    const std::string field_key = (phase == Opm::Phase::WATER) ? "FMCTW" : "FMCTG";
    const std::string group_key = (phase == Opm::Phase::WATER) ? "GMCTW" : "GMCTG";

    // WATER INJECTION GROUP CONTROL
    if (group.hasInjectionControl(phase)) {

        if (is_field) {
            //set value for the group's availability for higher level control for injection
            high_level_ctrl = 0;
        } else {

            const auto& injection_controls = group.injectionControls(phase, sumState);
            const auto& guide_rate_def = injection_controls.guide_rate_def;
            const auto& cur_inj_ctrl = group.name() == "FIELD" ? static_cast<int>(sumState.get(field_key, -1)) : static_cast<int>(sumState.get_group_var(group.name(), group_key, -1));
            Opm::Group::InjectionCMode active_cmode = Opm::Group::InjectionCModeFromInt(cur_inj_ctrl);
            const auto& deck_cmode = (group.hasInjectionControl(phase))
                                     ? injection_controls.cmode : Opm::Group::InjectionCMode::NONE;
            const auto& cgroup = injectionControlGroup(sched, sumState, group, group_key, field_key, simStep);
            const auto& group_control_available = group.injectionGroupControlAvailable(phase);

            // group is available for higher level control, but is currently constrained by own limits
            high_level_ctrl = -1;
            if ((deck_cmode != Opm::Group::InjectionCMode::FLD) && !group_control_available) {
                //group is not free to respond to higher level control)
                high_level_ctrl = 0;
            }

            if (cgroup) {
                if ((active_cmode == Opm::Group::InjectionCMode::FLD) || (active_cmode == Opm::Group::InjectionCMode::NONE)) {
                    //a higher level group control is active constraint
                    if ((deck_cmode != Opm::Group::InjectionCMode::FLD) && (deck_cmode != Opm::Group::InjectionCMode::NONE)) {
                        high_level_ctrl = cgroup->insert_index();
                    } else {
                        if (guide_rate_def == Opm::Group::GuideRateInjTarget::NO_GUIDE_RATE) {
                            if (deck_cmode == Opm::Group::InjectionCMode::FLD) {
                                high_level_ctrl = 1;
                            } else if ((deck_cmode == Opm::Group::InjectionCMode::NONE) && group_control_available) {
                                high_level_ctrl = 1;
                            }
                        } else {
                            if (deck_cmode == Opm::Group::InjectionCMode::FLD) {
                                high_level_ctrl = cgroup->insert_index();
                            } else if ((deck_cmode == Opm::Group::InjectionCMode::NONE) && group_control_available) {
                                high_level_ctrl = cgroup->insert_index();
                            }
                        }
                    }
                }
            } else {
                if ((active_cmode == Opm::Group::InjectionCMode::NONE) && (guide_rate_def == Opm::Group::GuideRateInjTarget::NO_GUIDE_RATE)) {
                    //group is directly under higher level controlGroup
                    if (deck_cmode == Opm::Group::InjectionCMode::FLD) {
                        high_level_ctrl = 1;
                    } else if ((deck_cmode == Opm::Group::InjectionCMode::NONE) && group_control_available) {
                        high_level_ctrl = 1;
                    }
                }
            }

            gconinje_cmode = Opm::Group::InjectionCMode2Int(deck_cmode);
            if (cgroup && (group.getGroupType() != Opm::Group::GroupType::NONE)) {
                auto cgroup_control = (cgroup->name() == "FIELD") ? static_cast<int>(sumState.get(field_key, 0)) : static_cast<int>(sumState.get_group_var(cgroup->name(), group_key, 0));
                current_cmode = (guide_rate_def != Opm::Group::GuideRateInjTarget::NO_GUIDE_RATE) ? cgroup_control : 0;
            } else {
                current_cmode = cur_inj_ctrl;
            }
        }
    }
    else {
        //set default value for the group's availability for higher level control for water injection for groups with no GCONINJE - WATER
        high_level_ctrl = (groupCurrentlyInjectionControllable(sched, sumState, group, phase, simStep) ) ? 1 : -1;
    }

    return {high_level_ctrl, current_cmode, gconinje_cmode};
}



template <class IGrpArray>
void injectionGroup(const Opm::Schedule&     sched,
                    const Opm::Group&        group,
                    const int                nwgmax,
                    const std::size_t        simStep,
                    const Opm::SummaryState& sumState,
                    IGrpArray&               iGrp)
{
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;
    const bool is_field = group.name() == "FIELD";
    auto group_parent_list = groupParentSeqIndex(sched, group, simStep);
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;


    // set "default value" for production higher level control in case a group is only injection group
    if (group.isInjectionGroup() && !group.isProductionGroup()) {
        iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 1;
    }
    //Special treatment of groups with no GCONINJE data
    if (group.getGroupType() == Opm::Group::GroupType::NONE) {
        if (is_field) {
            iGrp[nwgmax + IGroup::WInjHighLevCtrl] = 0;
            iGrp[nwgmax + IGroup::GInjHighLevCtrl] = 0;
        } else {
            //set default value for the group's availability for higher level control for injection
            iGrp[nwgmax + IGroup::WInjHighLevCtrl] = (groupCurrentlyInjectionControllable(sched, sumState, group, Opm::Phase::WATER, simStep) ) ? 1 : -1;
            iGrp[nwgmax + IGroup::GInjHighLevCtrl] = (groupCurrentlyInjectionControllable(sched, sumState, group, Opm::Phase::GAS, simStep) ) ? 1 : -1;
        }
        return;
    }

    {
        auto [high_level_ctrl, active_cmode, gconinje_cmode] = injectionGroup(sched, group, simStep, sumState, Opm::Phase::WATER);
        iGrp[nwgmax + IGroup::WInjHighLevCtrl] = high_level_ctrl;
        iGrp[nwgmax + IGroup::WInjActiveCMode] = active_cmode;
        iGrp[nwgmax + IGroup::GConInjeWInjCMode] = gconinje_cmode;
    }
    {
        auto [high_level_ctrl, active_cmode, gconinje_cmode] = injectionGroup(sched, group, simStep, sumState, Opm::Phase::GAS);
        iGrp[nwgmax + IGroup::GInjHighLevCtrl] = high_level_ctrl;
        iGrp[nwgmax + IGroup::GInjActiveCMode] = active_cmode;
        iGrp[nwgmax + IGroup::GConInjeGInjCMode] = gconinje_cmode;
    }
}

template <class IGrpArray>
void storeGroupTree(const Opm::Schedule& sched,
                    const Opm::Group& group,
                    const int nwgmax,
                    const int ngmaxz,
                    const std::size_t simStep,
                    IGrpArray& iGrp) {

    namespace Value = ::Opm::RestartIO::Helpers::VectorItems::IGroup::Value;
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;
    const bool is_field = group.name() == "FIELD";

    // Store index of all child wells or child groups.
    if (group.wellgroup()) {
        int igrpCount = 0;
        for (const auto& well_name : group.wells()) {
            const auto& well = sched.getWell(well_name, simStep);
            iGrp[igrpCount] = well.seqIndex() + 1;
            igrpCount += 1;
        }
        iGrp[nwgmax] = group.wells().size();
        iGrp[nwgmax + IGroup::GroupType] = Value::GroupType::WellGroup;
    } else  {
        int igrpCount = 0;
        for (const auto& group_name : group.groups()) {
            const auto& child_group = sched.getGroup(group_name, simStep);
            iGrp[igrpCount] = child_group.insert_index();
            igrpCount += 1;
        }
        iGrp[nwgmax+ IGroup::NoOfChildGroupsWells] = (group.wellgroup()) ? group.wells().size() : group.groups().size();
        iGrp[nwgmax + IGroup::GroupType] = Value::GroupType::TreeGroup;
    }


    // Store index of parent group
    if (is_field)
        iGrp[nwgmax + IGroup::ParentGroup] = 0;
    else {
        const auto& parent_group = sched.getGroup(group.parent(), simStep);
        if (parent_group.name() == "FIELD")
            iGrp[nwgmax + IGroup::ParentGroup] = ngmaxz;
        else
            iGrp[nwgmax + IGroup::ParentGroup] = parent_group.insert_index();
    }

    iGrp[nwgmax + IGroup::GroupLevel] = currentGroupLevel(sched, group, simStep);
}


template <class IGrpArray>
void storeFlowingWells(const Opm::Group&        group,
                       const int                nwgmax,
                       const Opm::SummaryState& sumState,
                       IGrpArray&               iGrp) {
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;
    const bool is_field = group.name() == "FIELD";
    const double g_act_pwells = is_field ? sumState.get("FMWPR", 0) : sumState.get_group_var(group.name(), "GMWPR", 0);
    const double g_act_iwells = is_field ? sumState.get("FMWIN", 0) : sumState.get_group_var(group.name(), "GMWIN", 0);
    iGrp[nwgmax + IGroup::FlowingWells] = static_cast<int>(g_act_pwells) + static_cast<int>(g_act_iwells);
}


template <class IGrpArray>
void staticContrib(const Opm::Schedule&     sched,
                   const Opm::Group&        group,
                   const int                nwgmax,
                   const int                ngmaxz,
                   const std::size_t        simStep,
                   const Opm::SummaryState& sumState,
                   IGrpArray&               iGrp)
{
    using IGroup = ::Opm::RestartIO::Helpers::VectorItems::IGroup::index;
    const bool is_field = group.name() == "FIELD";

    storeGroupTree(sched, group, nwgmax, ngmaxz, simStep, iGrp);
    storeFlowingWells(group, nwgmax, sumState, iGrp);

    // Treat all groups for production controls
    productionGroup(sched, group, nwgmax, simStep, sumState, iGrp);

    // Treat all groups for injection controls
    injectionGroup(sched, group, nwgmax, simStep, sumState, iGrp);

    if (is_field)
    {
        //the maximum number of groups in the model
        iGrp[nwgmax + IGroup::ProdHighLevCtrl] = 0;
        iGrp[nwgmax + IGroup::WInjHighLevCtrl] = 0;
        iGrp[nwgmax + IGroup::GInjHighLevCtrl] = 0;
        iGrp[nwgmax+88] = ngmaxz;
        iGrp[nwgmax+89] = ngmaxz;
        iGrp[nwgmax+95] = ngmaxz;
        iGrp[nwgmax+96] = ngmaxz;
    }
    else
    {
        //parameters connected to oil injection - not implemented in flow yet
        iGrp[nwgmax+11] = 0;
        iGrp[nwgmax+12] = -1;

        //assign values to group number (according to group sequence)
        iGrp[nwgmax+88] = group.insert_index();
        iGrp[nwgmax+89] = group.insert_index();
        iGrp[nwgmax+95] = group.insert_index();
        iGrp[nwgmax+96] = group.insert_index();
    }
}
} // Igrp

namespace SGrp {
std::size_t entriesPerGroup(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NSGRPZ];
}

Opm::RestartIO::Helpers::WindowedArray<float>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<float>;

    return WV {
        WV::NumWindows{ ngmaxz(inteHead) },
        WV::WindowSize{ entriesPerGroup(inteHead) }
    };
}

template <class SGrpArray>
void staticContrib(const Opm::Group&        group,
                   const Opm::SummaryState& sumState,
                   const Opm::UnitSystem& units,
                   SGrpArray&               sGrp)
{
    using Isp = ::Opm::RestartIO::Helpers::VectorItems::SGroup::prod_index;
    using Isi = ::Opm::RestartIO::Helpers::VectorItems::SGroup::inj_index;
    using M = ::Opm::UnitSystem::measure;

    const auto dflt   = -1.0e+20f;
    const auto dflt_2 = -2.0e+20f;
    const auto infty  =  1.0e+20f;
    const auto zero   =  0.0f;
    const auto one    =  1.0f;

    const auto init = std::vector<float> { // 112 Items (0..111)
        // 0     1      2      3      4
        infty, infty, dflt , infty,  zero ,     //   0..  4  ( 0)
        zero , infty, infty, infty , infty,     //   5..  9  ( 1)
        infty, infty, infty, infty , dflt ,     //  10.. 14  ( 2)
        infty, infty, infty, infty , dflt ,     //  15.. 19  ( 3)
        infty, infty, infty, infty , dflt ,     //  20.. 24  ( 4)
        zero , zero , zero , dflt_2, zero ,     //  24.. 29  ( 5)
        zero , zero , zero , zero  , zero ,     //  30.. 34  ( 6)
        infty ,zero , zero , zero  , infty,     //  35.. 39  ( 7)
        zero , zero , zero , zero  , zero ,     //  40.. 44  ( 8)
        zero , zero , zero , zero  , zero ,     //  45.. 49  ( 9)
        zero , infty, infty, infty , infty,     //  50.. 54  (10)
        infty, infty, infty, infty , infty,     //  55.. 59  (11)
        infty, infty, infty, infty , infty,     //  60.. 64  (12)
        infty, infty, infty, infty , zero ,     //  65.. 69  (13)
        zero , zero , zero , zero  , zero ,     //  70.. 74  (14)
        zero , zero , zero , zero  , infty,     //  75.. 79  (15)
        infty, zero , infty, zero  , zero ,     //  80.. 84  (16)
        zero , zero , zero , zero  , zero ,     //  85.. 89  (17)
        zero , zero , one  , zero  , zero ,     //  90.. 94  (18)
        zero , zero , zero , zero  , zero ,     //  95.. 99  (19)
        zero , zero , zero , zero  , zero ,     // 100..104  (20)
        zero , zero , zero , zero  , zero ,     // 105..109  (21)
        zero , zero                             // 110..111  (22)
    };

    const auto sz = static_cast<
                    decltype(init.size())>(sGrp.size());

    auto b = std::begin(init);
    auto e = b + std::min(init.size(), sz);

    std::copy(b, e, std::begin(sGrp));

    auto sgprop = [&units](const M u, const double x) -> float
    {
        return static_cast<float>(units.from_si(u, x));
    };

    if (group.isProductionGroup()) {
        const auto& prod_cntl = group.productionControls(sumState);

        if (prod_cntl.oil_target > 0.) {
            sGrp[Isp::OilRateLimit] = sgprop(M::liquid_surface_rate, prod_cntl.oil_target);
            sGrp[52] = sGrp[Isp::OilRateLimit];  // "ORAT" control
        }
        if (prod_cntl.water_target > 0.) {
            sGrp[Isp::WatRateLimit] = sgprop(M::liquid_surface_rate, prod_cntl.water_target);
            sGrp[53] = sGrp[Isp::WatRateLimit];  //"WRAT" control
        }
        if (prod_cntl.gas_target > 0.) {
            sGrp[Isp::GasRateLimit] = sgprop(M::gas_surface_rate, prod_cntl.gas_target);
            sGrp[39] = sGrp[Isp::GasRateLimit];
        }
        if (prod_cntl.liquid_target > 0.) {
            sGrp[Isp::LiqRateLimit] = sgprop(M::liquid_surface_rate, prod_cntl.liquid_target);
            sGrp[54] = sGrp[Isp::LiqRateLimit];  //"LRAT" control
        }
    }

    if ((group.name() == "FIELD") && (group.getGroupType() == Opm::Group::GroupType::NONE)) {
        sGrp[Isp::GuideRate] = 0.;
        sGrp[14] = 0.;
        sGrp[19] = 0.;
        sGrp[24] = 0.;
    }

    if (group.isInjectionGroup()) {
        if (group.hasInjectionControl(Opm::Phase::GAS)) {
            const auto& inj_cntl = group.injectionControls(Opm::Phase::GAS, sumState);
            if (inj_cntl.surface_max_rate > 0.) {
                sGrp[Isi::gasSurfRateLimit] = sgprop(M::gas_surface_rate, inj_cntl.surface_max_rate);
                sGrp[65] =  sGrp[Isi::gasSurfRateLimit];
            }
            if (inj_cntl.resv_max_rate > 0.) {
                sGrp[Isi::gasResRateLimit] = sgprop(M::rate, inj_cntl.resv_max_rate);
                sGrp[66] =  sGrp[Isi::gasResRateLimit];
            }
            if (inj_cntl.target_reinj_fraction > 0.) {
                sGrp[Isi::gasReinjectionLimit] = inj_cntl.target_reinj_fraction;
                sGrp[67] =  sGrp[Isi::gasReinjectionLimit];
            }
            if (inj_cntl.target_void_fraction > 0.) {
                sGrp[Isi::gasVoidageLimit] = inj_cntl.target_void_fraction;
                sGrp[68] =  sGrp[Isi::gasVoidageLimit];
            }
        }

        if (group.hasInjectionControl(Opm::Phase::WATER)) {
            const auto& inj_cntl = group.injectionControls(Opm::Phase::WATER, sumState);
            if (inj_cntl.surface_max_rate > 0.) {
                sGrp[Isi::waterSurfRateLimit] = sgprop(M::liquid_surface_rate, inj_cntl.surface_max_rate);
                sGrp[61] =  sGrp[Isi::waterSurfRateLimit];
            }
            if (inj_cntl.resv_max_rate > 0.) {
                sGrp[Isi::waterResRateLimit] = sgprop(M::rate, inj_cntl.resv_max_rate);
                sGrp[62] =  sGrp[Isi::waterResRateLimit];
            }
            if (inj_cntl.target_reinj_fraction > 0.) {
                sGrp[Isi::waterReinjectionLimit] = inj_cntl.target_reinj_fraction;
                sGrp[63] =  sGrp[Isi::waterReinjectionLimit];
            }
            if (inj_cntl.target_void_fraction > 0.) {
                sGrp[Isi::waterVoidageLimit] = inj_cntl.target_void_fraction;
                sGrp[64] =  sGrp[Isi::waterVoidageLimit];
            }
        }

        if (group.hasInjectionControl(Opm::Phase::OIL)) {
            const auto& inj_cntl = group.injectionControls(Opm::Phase::OIL, sumState);
            if (inj_cntl.surface_max_rate > 0.) {
                sGrp[Isi::oilSurfRateLimit] = sgprop(M::liquid_surface_rate, inj_cntl.surface_max_rate);
                sGrp[57] =  sGrp[Isi::oilSurfRateLimit];
            }
            if (inj_cntl.resv_max_rate > 0.) {
                sGrp[Isi::oilResRateLimit] = sgprop(M::rate, inj_cntl.resv_max_rate);
                sGrp[58] =  sGrp[Isi::oilResRateLimit];
            }
            if (inj_cntl.target_reinj_fraction > 0.) {
                sGrp[Isi::oilReinjectionLimit] = inj_cntl.target_reinj_fraction;
                sGrp[59] =  sGrp[Isi::oilReinjectionLimit];
            }
            if (inj_cntl.target_void_fraction > 0.) {
                sGrp[Isi::oilVoidageLimit] = inj_cntl.target_void_fraction;
                sGrp[60] =  sGrp[Isi::oilVoidageLimit];
            }
        }

    }
}
} // SGrp

namespace XGrp {
std::size_t entriesPerGroup(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NXGRPZ];
}

Opm::RestartIO::Helpers::WindowedArray<double>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<double>;

    return WV {
        WV::NumWindows{ ngmaxz(inteHead) },
        WV::WindowSize{ entriesPerGroup(inteHead) }
    };
}

// here define the dynamic group quantities to be written to the restart file
template <class XGrpArray>
void dynamicContrib(const std::vector<std::string>&      restart_group_keys,
                    const std::vector<std::string>&      restart_field_keys,
                    const std::map<std::string, size_t>& groupKeyToIndex,
                    const std::map<std::string, size_t>& fieldKeyToIndex,
                    const Opm::Group&                    group,
                    const Opm::SummaryState&             sumState,
                    XGrpArray&                           xGrp)
{
    using Ix = ::Opm::RestartIO::Helpers::VectorItems::XGroup::index;

    std::string groupName = group.name();
    const std::vector<std::string>& keys = (groupName == "FIELD")
                                           ? restart_field_keys : restart_group_keys;
    const std::map<std::string, size_t>& keyToIndex = (groupName == "FIELD")
            ? fieldKeyToIndex : groupKeyToIndex;

    for (const auto& key : keys) {
        std::string compKey = (groupName == "FIELD")
                              ? key : key + ":" + groupName;

        if (sumState.has(compKey)) {
            double keyValue = sumState.get(compKey);
            const auto itr = keyToIndex.find(key);
            xGrp[itr->second] = keyValue;
        }
    }

    xGrp[Ix::OilPrGuideRate_2]  = xGrp[Ix::OilPrGuideRate];
    xGrp[Ix::WatPrGuideRate_2]  = xGrp[Ix::WatPrGuideRate];
    xGrp[Ix::GasPrGuideRate_2]  = xGrp[Ix::GasPrGuideRate];
    xGrp[Ix::VoidPrGuideRate_2] = xGrp[Ix::VoidPrGuideRate];

    xGrp[Ix::WatInjGuideRate_2] = xGrp[Ix::WatInjGuideRate];
}
} // XGrp

namespace ZGrp {
std::size_t entriesPerGroup(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NZGRPZ];
}

Opm::RestartIO::Helpers::WindowedArray<
Opm::EclIO::PaddedOutputString<8>
>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<
               Opm::EclIO::PaddedOutputString<8>
               >;

    return WV {
        WV::NumWindows{ ngmaxz(inteHead) },
        WV::WindowSize{ entriesPerGroup(inteHead) }
    };
}

template <class ZGroupArray>
void staticContrib(const Opm::Group& group, ZGroupArray& zGroup)
{
    zGroup[0] = group.name();
}
} // ZGrp
} // Anonymous



// =====================================================================

Opm::RestartIO::Helpers::AggregateGroupData::
AggregateGroupData(const std::vector<int>& inteHead)
    : iGroup_ (IGrp::allocate(inteHead))
    , sGroup_ (SGrp::allocate(inteHead))
    , xGroup_ (XGrp::allocate(inteHead))
    , zGroup_ (ZGrp::allocate(inteHead))
    , nWGMax_ (nwgmax(inteHead))
    , nGMaxz_ (ngmaxz(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateGroupData::
captureDeclaredGroupData(const Opm::Schedule&                 sched,
                         const Opm::UnitSystem&               units,
                         const std::size_t                    simStep,
                         const Opm::SummaryState&             sumState,
                         const std::vector<int>&              inteHead)
{
    const auto& curGroups = sched.restart_groups(simStep);

    groupLoop(curGroups, [&sched, simStep, sumState, this]
              (const Group& group, const std::size_t groupID) -> void
    {
        auto ig = this->iGroup_[groupID];
        IGrp::staticContrib(sched, group, this->nWGMax_, this->nGMaxz_,
        simStep, sumState, ig);
    });

    // Define Static Contributions to SGrp Array.
    groupLoop(curGroups,
              [&sumState, &units, this](const Group& group , const std::size_t groupID) -> void
    {
        auto sw = this->sGroup_[groupID];
        SGrp::staticContrib(group, sumState, units, sw);
    });

    // Define Dynamic Contributions to XGrp Array.
    groupLoop(curGroups, [&sumState, this]
              (const Group& group, const std::size_t groupID) -> void
    {
        auto xg = this->xGroup_[groupID];

        XGrp::dynamicContrib(this->restart_group_keys, this->restart_field_keys,
        this->groupKeyToIndex, this->fieldKeyToIndex, group,
        sumState, xg);
    });

    // Define Static Contributions to ZGrp Array.
    groupLoop(curGroups, [this, &inteHead]
              (const Group& group, const std::size_t /* groupID */) -> void
    {
        std::size_t group_index = group.insert_index() - 1;
        if (group.name() == "FIELD")
            group_index = ngmaxz(inteHead) - 1;
        auto zg = this->zGroup_[ group_index ];

        ZGrp::staticContrib(group, zg);
    });
}

