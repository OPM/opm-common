/*
  Copyright 2023 Equinor ASA.

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

#include <opm/input/eclipse/Deck/UDAValue.hpp>

#include <opm/input/eclipse/Schedule/Group/GroupEconProductionLimits.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include "../eval_uda.hpp"

namespace {
    template <class T>
    std::optional<T> get_positive_value(T value)
    {
        std::optional<T> result;
        if (value > 0.0) {
            result = value;
        }
        return result;
    }
}

namespace Opm {

void GroupEconProductionLimits::add_group(
        const int report_step, const std::string &group_name, const DeckRecord& record)
{
    // NOTE: report_step is needed when retrieving UDA values later.
    //  To get the correct UDQ config for the UDQ-undefined value
    auto group = GEconGroup {record, report_step};
    this->m_groups.insert_or_assign(group_name, group);
}

bool GroupEconProductionLimits::has_group(const std::string& gname) const {
    const auto iter = this->m_groups.find(gname);
    return (iter != this->m_groups.end());
}

const GroupEconProductionLimits::GEconGroup& GroupEconProductionLimits::get_group(
        const std::string& name) const
{
    auto it = this->m_groups.find(name);
    if (it == this->m_groups.end())
        throw std::invalid_argument("GroupEconProdctionLimits object does not contain group '" + name + "'.");
    else
        return it->second;
}

GroupEconProductionLimits::GEconGroupProp GroupEconProductionLimits::get_group_prop(
        const Schedule &schedule, const SummaryState &st, const std::string& name) const
{
    const GEconGroup& group0 = this->get_group(name);
    auto udq_undefined = schedule.getUDQConfig(group0.reportStep()).params().undefinedValue();
    auto min_oil_rate = UDA::eval_group_uda(group0.minOilRate(), name, st, udq_undefined);
    auto min_gas_rate = UDA::eval_group_uda(group0.minGasRate(), name, st, udq_undefined);
    auto max_water_cut = UDA::eval_group_uda(group0.maxWaterCut(), name, st, udq_undefined);
    auto max_gas_oil_ratio = UDA::eval_group_uda(group0.maxGasOilRatio(), name, st, udq_undefined);
    auto max_water_gas_ratio = UDA::eval_group_uda(group0.maxWaterGasRatio(), name, st, udq_undefined);

    return GEconGroupProp {
        min_oil_rate,
        min_gas_rate,
        max_water_cut,
        max_gas_oil_ratio,
        max_water_gas_ratio,
        group0.workover(),
        group0.endRun(),
        group0.maxOpenWells()
    };
}

bool GroupEconProductionLimits::operator==(const GroupEconProductionLimits& other) const
{
    return this->m_groups == other.m_groups;
}

bool GroupEconProductionLimits::operator!=(const GroupEconProductionLimits& other) const
{
    return !(*this == other);
}



GroupEconProductionLimits GroupEconProductionLimits::serializationTestObject()
{
    GroupEconProductionLimits gecon;
    gecon.m_groups["P1"] = {GEconGroup::serializationTestObject()};
    return gecon;
}

size_t GroupEconProductionLimits::size() const {
    return this->m_groups.size();
}

/* Methods for inner class GEconGroup */

GroupEconProductionLimits::GEconGroup::GEconGroup(const DeckRecord &record, const int report_step)
    : m_min_oil_rate{record.getItem("MIN_OIL_RATE").get<UDAValue>(0)}
    , m_min_gas_rate{record.getItem("MIN_GAS_RATE").get<UDAValue>(0)}
    , m_max_water_cut{record.getItem("MAX_WCT").get<UDAValue>(0)}
    , m_max_gas_oil_ratio{record.getItem("MAX_GOR").get<UDAValue>(0)}
    , m_max_water_gas_ratio{record.getItem("MAX_WATER_GAS_RATIO").get<UDAValue>(0)}
    , m_workover{econWorkoverFromString(record.getItem("WORKOVER").getTrimmedString(0))}
    , m_end_run{false}
    , m_max_open_wells{record.getItem("MAX_OPEN_WELLS").get<int>(0)}
    , m_report_step{report_step}
{
    if (record.getItem("END_RUN").hasValue(0)) {
        std::string string_endrun = record.getItem("END_RUN").getTrimmedString(0);
        if (string_endrun == "YES") {
            m_end_run = true;
        } else if (string_endrun != "NO") {
            throw std::invalid_argument("Unknown input: " + string_endrun + " for END_RUN in GECON");
        }
    }
}

GroupEconProductionLimits::EconWorkover
GroupEconProductionLimits::econWorkoverFromString(const std::string& string_value)
{
    if (string_value == "NONE")
        return EconWorkover::NONE;
    else if (string_value == "CON")
        return EconWorkover::CON;
    else if (string_value == "+CON")
        return EconWorkover::CONP;
    else if (string_value == "WELL")
        return EconWorkover::WELL;
    else if (string_value == "PLUG")
        return EconWorkover::PLUG;
    else if (string_value == "ALL")
        return EconWorkover::ALL;
    else
        throw std::invalid_argument("GroupEconProductionLimits: Unknown enum string value '"
                                     + string_value + "' for EconWorkover enum");
}

bool GroupEconProductionLimits::GEconGroup::endRun() const {
    return m_end_run;
}

UDAValue GroupEconProductionLimits::GEconGroup::maxGasOilRatio() const
{
    return m_max_gas_oil_ratio;
}

UDAValue GroupEconProductionLimits::GEconGroup::maxWaterCut() const
{
    return m_max_water_cut;
}

UDAValue GroupEconProductionLimits::GEconGroup::maxWaterGasRatio() const
{
    return m_max_water_gas_ratio;
}

int GroupEconProductionLimits::GEconGroup::maxOpenWells() const
{
    return m_max_open_wells;
}

UDAValue GroupEconProductionLimits::GEconGroup::minGasRate() const
{
    return m_min_gas_rate;
}

UDAValue GroupEconProductionLimits::GEconGroup::minOilRate() const
{
    return m_min_oil_rate;
}

bool GroupEconProductionLimits::GEconGroup::operator==(const GEconGroup& other) const
{
    return this->m_min_oil_rate == other.m_min_oil_rate &&
           this->m_min_gas_rate == other.m_min_gas_rate &&
           this->m_max_water_cut == other.m_max_water_cut &&
           this->m_max_gas_oil_ratio == other.m_max_gas_oil_ratio &&
           this->m_max_water_gas_ratio == other.m_max_water_gas_ratio &&
           this->m_workover == other.m_workover &&
           this->m_end_run == other.m_end_run &&
           this->m_max_open_wells == other.m_max_open_wells;
}

int GroupEconProductionLimits::GEconGroup::reportStep() const {
    return m_report_step;
}

GroupEconProductionLimits::GEconGroup GroupEconProductionLimits::GEconGroup::serializationTestObject()
{
    GEconGroup group;
    group.m_min_oil_rate = UDAValue{1.0};
    group.m_min_gas_rate = UDAValue{2.0};
    group.m_max_water_cut = UDAValue{3.0};
    group.m_max_gas_oil_ratio = UDAValue{4.0};
    group.m_max_water_gas_ratio = UDAValue{5.0};
    group.m_workover = EconWorkover::CON;
    group.m_end_run = false;
    group.m_max_open_wells = 6;
    return group;
}

GroupEconProductionLimits::EconWorkover GroupEconProductionLimits::GEconGroup::workover() const {
    return m_workover;
}


/* Methods for inner class GEconGroupProp */

GroupEconProductionLimits::GEconGroupProp::GEconGroupProp(
        const double min_oil_rate,
        const double min_gas_rate,
        const double max_water_cut,
        const double max_gas_oil_ratio,
        const double max_water_gas_ratio,
        EconWorkover workover,
        bool end_run,
        int max_open_wells)
    : m_min_oil_rate{get_positive_value(min_oil_rate)}
    , m_min_gas_rate{get_positive_value(min_gas_rate)}
    , m_max_water_cut{get_positive_value(max_water_cut)}
    , m_max_gas_oil_ratio{get_positive_value(max_gas_oil_ratio)}
    , m_max_water_gas_ratio{get_positive_value(max_water_gas_ratio)}
    , m_workover{workover}
    , m_end_run{end_run}
    , m_max_open_wells{max_open_wells}
{

}

bool GroupEconProductionLimits::GEconGroupProp::endRun() const {
    return m_end_run;
}

std::optional<double> GroupEconProductionLimits::GEconGroupProp::minOilRate() const
{
    return m_min_oil_rate;
}

std::optional<double> GroupEconProductionLimits::GEconGroupProp::minGasRate() const
{
    return m_min_gas_rate;
}

std::optional<double> GroupEconProductionLimits::GEconGroupProp::maxWaterCut() const
{
    return m_max_water_cut;
}

std::optional<double> GroupEconProductionLimits::GEconGroupProp::maxGasOilRatio() const
{
    return m_max_gas_oil_ratio;
}

int GroupEconProductionLimits::GEconGroupProp::maxOpenWells() const
{
    return m_max_open_wells;
}

std::optional<double> GroupEconProductionLimits::GEconGroupProp::maxWaterGasRatio() const
{
    return m_max_water_gas_ratio;
}

GroupEconProductionLimits::EconWorkover GroupEconProductionLimits::GEconGroupProp::workover() const {
    return m_workover;
}


} // namespace Opm

