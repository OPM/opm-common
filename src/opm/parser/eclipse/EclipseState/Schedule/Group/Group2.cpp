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



#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>

namespace Opm {
Group2::Group2(const std::string& name, std::size_t insert_index_arg, std::size_t init_step_arg) :
    m_name(name),
    m_insert_index(insert_index_arg),
    init_step(init_step_arg),
    group_type(GroupType::NONE),
    gefac(1),
    transfer_gefac(true),
    vfp_table(0)
{}

std::size_t Group2::insert_index() const {
    return this->m_insert_index;
}

bool Group2::defined(size_t timeStep) const {
    if (timeStep < this->init_step)
        return false;
    else
        return true;
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
        this->target_void_fraction  == other.target_void_fraction;
}


bool Group2::GroupInjectionProperties::operator!=(const GroupInjectionProperties& other) const {
    return !(*this == other);
}


bool Group2::GroupProductionProperties::operator==(const GroupProductionProperties& other) const {
    return
        this->cmode         == other.cmode &&
        this->exceed_action == other.exceed_action &&
        this->oil_target    == other.oil_target &&
        this->water_target  == other.oil_target &&
        this->gas_target    == other.gas_target &&
        this->liquid_target == other.liquid_target &&
        this->resv_target   == other.resv_target;
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


bool Group2::addWell(const std::string& well_name) {
    if (!this->m_groups.empty())
        throw std::logic_error("Groups can not mix group and well children");

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
        throw std::invalid_argument("Group does not have well: " + well_name);
}


bool Group2::addGroup(const std::string& group_name) {
    if (!this->m_wells.empty())
        throw std::logic_error("Groups can not mix group and well children");

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

}
