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

#ifndef GROUP2_HPP
#define GROUP2_HPP


#include <string>

#include <opm/parser/eclipse/Deck/UDAValue.hpp>
#include <opm/parser/eclipse/EclipseState/Util/IOrderSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

class SummaryState;
class Group2 {
public:

struct GroupInjectionProperties {
    Phase phase = Phase::WATER;
    GroupInjection::ControlEnum cmode = GroupInjection::NONE;
    UDAValue surface_max_rate;
    UDAValue resv_max_rate;
    UDAValue target_reinj_fraction;
    UDAValue target_void_fraction;

    bool operator==(const GroupInjectionProperties& other) const;
    bool operator!=(const GroupInjectionProperties& other) const;
};

struct InjectionControls {
    Phase phase;
    GroupInjection::ControlEnum cmode;
    double surface_max_rate;
    double resv_max_rate;
    double target_reinj_fraction;
    double target_void_fraction;
};

struct GroupProductionProperties {
    GroupProduction::ControlEnum cmode = GroupProduction::NONE;
    GroupProductionExceedLimit::ActionEnum exceed_action = GroupProductionExceedLimit::NONE;
    UDAValue oil_target;
    UDAValue water_target;
    UDAValue gas_target;
    UDAValue liquid_target;
    double resv_target = 0;

    bool operator==(const GroupProductionProperties& other) const;
    bool operator!=(const GroupProductionProperties& other) const;
};

struct ProductionControls {
    GroupProduction::ControlEnum cmode;
    GroupProductionExceedLimit::ActionEnum exceed_action;
    double oil_target;
    double water_target;
    double gas_target;
    double liquid_target;
    double resv_target = 0;
};

    Group2(const std::string& group_name, std::size_t insert_index_arg, std::size_t init_step_arg, double udq_undefined_arg, const UnitSystem& unit_system);

    bool defined(std::size_t timeStep) const;
    std::size_t insert_index() const;
    const std::string& name() const;
    int getGroupNetVFPTable() const;
    bool updateNetVFPTable(int vfp_arg);
    bool update_gefac(double gefac, bool transfer_gefac);

    const std::string& parent() const;
    bool updateParent(const std::string& parent);
    bool updateInjection(const GroupInjectionProperties& injection);
    bool updateProduction(const GroupProductionProperties& production);
    bool isProductionGroup() const;
    bool isInjectionGroup() const;
    void setProductionGroup();
    void setInjectionGroup();
    double getGroupEfficiencyFactor() const;
    bool   getTransferGroupEfficiencyFactor() const;

    std::size_t numWells() const;
    bool addGroup(const std::string& group_name);
    bool hasGroup(const std::string& group_name) const;
    void delGroup(const std::string& group_name);
    bool addWell(const std::string& well_name);
    bool hasWell(const std::string& well_name) const;
    void delWell(const std::string& well_name);

    const std::vector<std::string>& wells() const;
    const std::vector<std::string>& groups() const;
    bool wellgroup() const;
    ProductionControls productionControls(const SummaryState& st) const;
    InjectionControls injectionControls(const SummaryState& st) const;
private:
    bool hasType(GroupType gtype) const;
    void addType(GroupType new_gtype);
    const GroupProductionProperties& productionProperties() const;
    const GroupInjectionProperties& injectionProperties() const;

    std::string m_name;
    std::size_t m_insert_index;
    std::size_t init_step;
    double udq_undefined;
    UnitSystem unit_system;
    GroupType group_type;
    double gefac;
    bool transfer_gefac;
    int vfp_table;

    std::string parent_group;
    IOrderSet<std::string> m_wells;
    IOrderSet<std::string> m_groups;

    GroupInjectionProperties injection_properties{};
    GroupProductionProperties production_properties{};
};

}

#endif
