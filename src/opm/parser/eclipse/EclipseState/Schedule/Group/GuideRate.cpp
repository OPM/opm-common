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

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GuideRate.hpp>

namespace Opm {

double GuideRate::GuideRateValue::eval(GuideRateModel::Target target_arg) const
{
    if (target == this->target)
        return this->value;
    else
        throw std::logic_error("Don't know how to convert .... ");
}

double GuideRate::Potential::eval(GuideRateModel::Target target) const {
    if (target == GuideRateModel::Target::OIL)
        return this->oil_pot;

    if (target == GuideRateModel::Target::GAS)
        return this->gas_pot;

    if (target == GuideRateModel::Target::LIQ)
        return this->oil_pot + this->wat_pot;

    throw std::logic_error("Don't know how to convert .... ");
}


GuideRate::GuideRate(const Schedule& schedule_arg) :
    schedule(schedule_arg)
{}



double GuideRate::get(const std::string& well, Well2::GuideRateTarget target) const {
    const auto iter = this->values.find(well);
    auto model_target = GuideRateModel::convert_target(target);
    if (iter != this->values.end()) {
        const auto& value = iter->second;
        return value.eval(model_target);
    } else {
        const auto& pot = this->potentials.at(well);
        return pot.eval(model_target);
    }
}

double GuideRate::get(const std::string& group, Group2::GuideRateTarget target) const {
    const auto& value = this->values.at(group);
    auto model_target = GuideRateModel::convert_target(target);
    return value.eval(model_target);
}


void GuideRate::compute(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot) {
    const auto& config = this->schedule.guideRateConfig(report_step);
    this->potentials[wgname] = Potential{oil_pot, gas_pot, wat_pot};

    if (config.has_group(wgname))
        this->group_compute(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);
    else if (config.has_well(wgname))
        this->well_compute(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);

}


void GuideRate::group_compute(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot) {
    const auto& config = this->schedule.guideRateConfig(report_step);
    const auto& group = config.group(wgname);
    auto iter = this->values.find(wgname);

    // If the FORM mode is used we check if the last computation is recent enough;
    // then we just return.
    if (iter != this->values.end()) {
        const auto& grv = iter->second;
        if (group.target == Group2::GuideRateTarget::FORM) {
            if (!config.has_model())
                throw std::logic_error("When specifying GUIDERATE target FORM you must enter a guiderate model with the GUIDERAT keyword");

            auto time_diff = sim_time - grv.sim_time;
            if (config.model().update_delay() > time_diff)
                return;
        }
    }


    if (group.target == Group2::GuideRateTarget::INJV)
        throw std::logic_error("Group guide rate mode: INJV not implemented");

    if (group.target == Group2::GuideRateTarget::POTN)
        throw std::logic_error("Group guide rate mode: POTN not implemented");

    if (group.target == Group2::GuideRateTarget::FORM) {
        double guide_rate;
        if (!config.has_model())
            throw std::logic_error("When specifying GUIDERATE target FORM you must enter a guiderate model with the GUIDERAT keyword");

        if (iter != this->values.end())
            guide_rate = this->eval_form(config.model(),  oil_pot,  gas_pot,  wat_pot, nullptr);
        else
            guide_rate = this->eval_form(config.model(),  oil_pot,  gas_pot,  wat_pot, std::addressof(iter->second));

        this->values[wgname] = GuideRateValue{sim_time, guide_rate, config.model().target()};
    }
}


void GuideRate::well_compute(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot) {
    const auto& config = this->schedule.guideRateConfig(report_step);
    const auto& well = config.well(wgname);

    if (well.guide_rate > 0) {
        auto model_target = GuideRateModel::convert_target(well.target);
        this->values[wgname] = GuideRateValue( sim_time, well.guide_rate, model_target );
    } else {
        auto iter = this->values.find(wgname);

        if (iter != this->values.end()) {
            const auto& grv = iter->second;
            auto time_diff = sim_time - grv.sim_time;
            if (config.model().update_delay() > time_diff)
                return;
        }

        double guide_rate;
        if (!config.has_model())
            throw std::logic_error("When specifying GUIDERATE target FORM you must enter a guiderate model with the GUIDERAT keyword");

        if (iter == this->values.end())
            guide_rate = this->eval_form(config.model(),  oil_pot,  gas_pot,  wat_pot, nullptr);
        else
            guide_rate = this->eval_form(config.model(),  oil_pot,  gas_pot,  wat_pot, std::addressof(iter->second));

        this->values[wgname] = GuideRateValue{sim_time, guide_rate, config.model().target()};
    }
}

double GuideRate::eval_form(const GuideRateModel& model, double oil_pot, double gas_pot, double wat_pot, const GuideRateValue * prev) const {
    double new_guide_rate = model.eval(oil_pot, gas_pot, wat_pot);
    if (!prev)
        return new_guide_rate;

    if (new_guide_rate > prev->value && !model.allow_increase())
        new_guide_rate = prev->value;

    auto damping_factor = model.damping_factor();

    return damping_factor * new_guide_rate + (1 - damping_factor) * prev->value;
}

double GuideRate::eval_group_pot() const {
    return 0;
}

double GuideRate::eval_group_resvinj() const {
    return 0;
}

}
