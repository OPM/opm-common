/*
  Copyright 2019 Equinor ASA.

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


#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>

#include "../eval_uda.hpp"

namespace Opm {

Group2::Group2(const std::string& name, std::size_t insert_index_arg, std::size_t init_step_arg, double udq_undefined_arg, const UnitSystem& unit_system_arg) :
    m_name(name),
    m_insert_index(insert_index_arg),
    init_step(init_step_arg),
    udq_undefined(udq_undefined_arg),
    unit_system(unit_system_arg),
    group_type(GroupType::NONE),
    gefac(1),
    transfer_gefac(true),
    vfp_table(0)
{
    // All groups are initially created as children of the "FIELD" group.
    if (name != "FIELD")
        this->parent_group = "FIELD";
}

std::size_t Group2::insert_index() const {
    return this->m_insert_index;
}

bool Group2::defined(size_t timeStep) const {
    return (timeStep >= this->init_step);
}

const std::string& Group2::name() const {
    return this->m_name;
}

const Group2::GroupProductionProperties& Group2::productionProperties() const {
    return this->production_properties;
}

const Group2::GroupInjectionProperties& Group2::injectionProperties() const {
    return this->injection_properties;
}

int Group2::getGroupNetVFPTable() const {
    return this->vfp_table;
}

bool Group2::updateNetVFPTable(int vfp_arg) {
    if (this->vfp_table != vfp_arg) {
        this->vfp_table = vfp_arg;
        return true;
    } else
        return false;
}

bool Group2::updateInjection(const GroupInjectionProperties& injection) {
    bool update = false;

    if (this->injection_properties != injection) {
        this->injection_properties = injection;
        update = true;
    }

    if (!this->hasType(GroupType::INJECTION)) {
        this->addType(GroupType::INJECTION);
        update = true;
    }

    return update;
}


bool Group2::updateProduction(const GroupProductionProperties& production) {
    bool update = false;

    if (this->production_properties != production) {
        this->production_properties = production;
        update = true;
    }

    if (!this->hasType(GroupType::PRODUCTION)) {
        this->addType(GroupType::PRODUCTION);
        update = true;
    }

    return update;
}


bool Group2::GroupInjectionProperties::operator==(const GroupInjectionProperties& other) const {
    return
        this->phase                 == other.phase &&
        this->cmode                 == other.cmode &&
        this->surface_max_rate      == other.surface_max_rate &&
        this->resv_max_rate         == other.resv_max_rate &&
        this->target_reinj_fraction == other.target_reinj_fraction &&
        this->injection_controls    == other.injection_controls &&
        this->target_void_fraction  == other.target_void_fraction;
}


bool Group2::GroupInjectionProperties::operator!=(const GroupInjectionProperties& other) const {
    return !(*this == other);
}


bool Group2::GroupProductionProperties::operator==(const GroupProductionProperties& other) const {
    return
        this->cmode               == other.cmode &&
        this->exceed_action       == other.exceed_action &&
        this->oil_target          == other.oil_target &&
        this->water_target        == other.oil_target &&
        this->gas_target          == other.gas_target &&
        this->liquid_target       == other.liquid_target &&
        this->production_controls == other.production_controls &&
        this->resv_target         == other.resv_target;
}


bool Group2::GroupProductionProperties::operator!=(const GroupProductionProperties& other) const {
    return !(*this == other);
}

bool Group2::hasType(GroupType gtype) const {
    return ((this->group_type & gtype) == gtype);
}

void Group2::addType(GroupType new_gtype) {
    this->group_type = this->group_type | new_gtype;
}

bool Group2::isProductionGroup() const {
    return this->hasType(GroupType::PRODUCTION);
}

bool Group2::isInjectionGroup() const {
    return this->hasType(GroupType::INJECTION);
}

void Group2::setProductionGroup() {
    this->addType(GroupType::PRODUCTION);
}

void Group2::setInjectionGroup() {
    this->addType(GroupType::INJECTION);
}


std::size_t Group2::numWells() const {
    return this->m_wells.size();
}

const std::vector<std::string>& Group2::wells() const {
    return this->m_wells.data();
}

const std::vector<std::string>& Group2::groups() const {
    return this->m_groups.data();
}

bool Group2::wellgroup() const {
    if (this->m_groups.size() > 0)
        return false;
    return true;
}

bool Group2::addWell(const std::string& well_name) {
    if (!this->m_groups.empty())
        throw std::logic_error("Groups can not mix group and well children. Trying to add well: " + well_name + " to group: " + this->name());

    if (this->m_wells.count(well_name) == 0) {
        this->m_wells.insert(well_name);
        return true;
    }
    return false;
}

bool Group2::hasWell(const std::string& well_name) const  {
    return (this->m_wells.count(well_name) == 1);
}

void Group2::delWell(const std::string& well_name) {
    auto rm_count = this->m_wells.erase(well_name);
    if (rm_count == 0)
        throw std::invalid_argument("Group: " + this->name() + " does not have well: " + well_name);
}

bool Group2::addGroup(const std::string& group_name) {
    if (!this->m_wells.empty())
        throw std::logic_error("Groups can not mix group and well children. Trying to add group: " + group_name + " to group: " + this->name());

    if (this->m_groups.count(group_name) == 0) {
        this->m_groups.insert(group_name);
        return true;
    }
    return false;
}

bool Group2::hasGroup(const std::string& group_name) const  {
    return (this->m_groups.count(group_name) == 1);
}

void Group2::delGroup(const std::string& group_name) {
    auto rm_count = this->m_groups.erase(group_name);
    if (rm_count == 0)
        throw std::invalid_argument("Group does not have group: " + group_name);
}

bool Group2::update_gefac(double gf, bool transfer_gf) {
    bool update = false;
    if (this->gefac != gf) {
        this->gefac = gf;
        update = true;
    }

    if (this->transfer_gefac != transfer_gf) {
        this->transfer_gefac = transfer_gf;
        update = true;
    }

    return update;
}

double Group2::getGroupEfficiencyFactor() const {
    return this->gefac;
}

bool Group2::getTransferGroupEfficiencyFactor() const {
    return this->transfer_gefac;
}

const std::string& Group2::parent() const {
    return this->parent_group;
}


bool Group2::updateParent(const std::string& parent) {
    if (this->parent_group != parent) {
        this->parent_group = parent;
        return true;
    }

    return false;
}

Group2::ProductionControls Group2::productionControls(const SummaryState& st) const {
    Group2::ProductionControls pc;

    pc.cmode = this->production_properties.cmode;
    pc.exceed_action = this->production_properties.exceed_action;
    pc.oil_target = UDA::eval_group_uda(this->production_properties.oil_target, this->m_name, st, this->udq_undefined);
    pc.water_target = UDA::eval_group_uda(this->production_properties.water_target, this->m_name, st, this->udq_undefined);
    pc.gas_target = UDA::eval_group_uda(this->production_properties.gas_target, this->m_name, st, this->udq_undefined);
    pc.liquid_target = UDA::eval_group_uda(this->production_properties.liquid_target, this->m_name, st, this->udq_undefined);
    pc.guide_rate = this->production_properties.guide_rate;
    pc.guide_rate_def = this->production_properties.guide_rate_def;
    pc.resv_target = this->production_properties.resv_target;

    return pc;
}

Group2::InjectionControls Group2::injectionControls(const SummaryState& st) const {
    Group2::InjectionControls ic;

    ic.phase = this->injection_properties.phase;
    ic.cmode = this->injection_properties.cmode;
    ic.surface_max_rate = UDA::eval_group_uda_rate(this->injection_properties.surface_max_rate, this->m_name, st, this->udq_undefined, ic.phase, this->unit_system);
    ic.resv_max_rate = UDA::eval_group_uda(this->injection_properties.resv_max_rate, this->m_name, st, this->udq_undefined);
    ic.target_reinj_fraction = UDA::eval_group_uda(this->injection_properties.target_reinj_fraction, this->m_name, st, this->udq_undefined);
    ic.target_void_fraction = UDA::eval_group_uda(this->injection_properties.target_void_fraction, this->m_name, st, this->udq_undefined);
    return ic;
}

Group2::ProductionCMode Group2::production_cmode() const {
    return this->production_properties.cmode;
}

Group2::InjectionCMode Group2::injection_cmode() const {
    return this->injection_properties.cmode;
}

Phase Group2::injection_phase() const {
    return this->injection_properties.phase;
}


bool Group2::ProductionControls::has_control(Group2::ProductionCMode control) const {
    return (this->production_controls & static_cast<int>(control)) != 0;
}


bool Group2::InjectionControls::has_control(InjectionCMode cmode_arg) const {
    return (this->injection_controls & static_cast<int>(cmode_arg)) != 0;
}

bool Group2::has_control(Group2::ProductionCMode control) const {
    return (this->production_properties.production_controls & static_cast<int>(control)) != 0;
}

bool Group2::has_control(InjectionCMode control) const {
    return (this->injection_properties.injection_controls & static_cast<int>(control)) != 0;
}


const std::string Group2::ExceedAction2String( ExceedAction enumValue ) {
    switch(enumValue) {
    case ExceedAction::NONE:
        return "NONE";
    case ExceedAction::CON:
        return "CON";
    case ExceedAction::CON_PLUS:
        return "+CON";
    case ExceedAction::WELL:
        return "WELL";
    case ExceedAction::PLUG:
        return "PLUG";
    case ExceedAction::RATE:
        return "RATE";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}


Group2::ExceedAction Group2::ExceedActionFromString( const std::string& stringValue ) {

    if (stringValue == "NONE")
        return ExceedAction::NONE;
    else if (stringValue == "CON")
        return ExceedAction::CON;
    else if (stringValue == "+CON")
        return ExceedAction::CON_PLUS;
    else if (stringValue == "WELL")
        return ExceedAction::WELL;
    else if (stringValue == "PLUG")
        return ExceedAction::PLUG;
    else if (stringValue == "RATE")
        return ExceedAction::RATE;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}


const std::string Group2::InjectionCMode2String( InjectionCMode enumValue ) {
    switch( enumValue ) {
    case InjectionCMode::NONE:
        return "NONE";
    case InjectionCMode::RATE:
        return "RATE";
    case InjectionCMode::RESV:
        return "RESV";
    case InjectionCMode::REIN:
        return "REIN";
    case InjectionCMode::VREP:
        return "VREP";
    case InjectionCMode::FLD:
        return "FLD";
    default:
        throw std::invalid_argument("Unhandled enum value");
    }
}


Group2::InjectionCMode Group2::InjectionCModeFromString( const std::string& stringValue ) {
    if (stringValue == "NONE")
        return InjectionCMode::NONE;
    else if (stringValue == "RATE")
        return InjectionCMode::RATE;
    else if (stringValue == "RESV")
        return InjectionCMode::RESV;
    else if (stringValue == "REIN")
        return InjectionCMode::REIN;
    else if (stringValue == "VREP")
        return InjectionCMode::VREP;
    else if (stringValue == "FLD")
        return InjectionCMode::FLD;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

Group2::GroupType operator|(Group2::GroupType lhs, Group2::GroupType rhs) {
    return static_cast<Group2::GroupType>(static_cast<std::underlying_type<Group2::GroupType>::type>(lhs) | static_cast<std::underlying_type<Group2::GroupType>::type>(rhs));
}


Group2::GroupType operator&(Group2::GroupType lhs, Group2::GroupType rhs) {
    return static_cast<Group2::GroupType>(static_cast<std::underlying_type<Group2::GroupType>::type>(lhs) & static_cast<std::underlying_type<Group2::GroupType>::type>(rhs));
}


const std::string Group2::ProductionCMode2String( ProductionCMode enumValue ) {
    switch( enumValue ) {
    case ProductionCMode::NONE:
        return "NONE";
    case ProductionCMode::ORAT:
        return "ORAT";
    case ProductionCMode::WRAT:
        return "WRAT";
    case ProductionCMode::GRAT:
        return "GRAT";
    case ProductionCMode::LRAT:
        return "LRAT";
    case ProductionCMode::CRAT:
        return "CRAT";
    case ProductionCMode::RESV:
        return "RESV";
    case ProductionCMode::PRBL:
        return "PRBL";
    case ProductionCMode::FLD:
        return "FLD";
    default:
        throw std::invalid_argument("Unhandled enum value");
    }
}


Group2::ProductionCMode Group2::ProductionCModeFromString( const std::string& stringValue ) {
    if (stringValue == "NONE")
        return ProductionCMode::NONE;
    else if (stringValue == "ORAT")
        return ProductionCMode::ORAT;
    else if (stringValue == "WRAT")
        return ProductionCMode::WRAT;
    else if (stringValue == "GRAT")
        return ProductionCMode::GRAT;
    else if (stringValue == "LRAT")
        return ProductionCMode::LRAT;
    else if (stringValue == "CRAT")
        return ProductionCMode::CRAT;
    else if (stringValue == "RESV")
        return ProductionCMode::RESV;
    else if (stringValue == "PRBL")
        return ProductionCMode::PRBL;
    else if (stringValue == "FLD")
        return ProductionCMode::FLD;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

}
