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

GuideRate::GuideRate(const Schedule& schedule_arg) :
    schedule(schedule_arg)
{}


double GuideRate::get(const std::string& wgname) const {
    const auto& value = this->values.at(wgname);
    return value.value;
}


double GuideRate::update(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot) {
    const auto& config = this->schedule.guideRateConfig(report_step);

    if (config.has_well(wgname))
        this->well_update(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);
    else if (config.has_group(wgname)) {
        this->group_update(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);
    } else
        throw std::out_of_range("No such well/group: ");

    return this->get(wgname);
}


void GuideRate::group_update(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot) {
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


    double guide_rate = group.guide_rate;

    if (group.guide_rate == 0 || group.target == Group2::GuideRateTarget::POTN)
        guide_rate = this->eval_group_pot();

    if (group.target == Group2::GuideRateTarget::INJV)
        guide_rate = this->eval_group_resvinj();

    if (group.target == Group2::GuideRateTarget::FORM) {
        if (!config.has_model())
            throw std::logic_error("When specifying GUIDERATE target FORM you must enter a guiderate model with the GUIDERAT keyword");

        if (iter != this->values.end())
            guide_rate = this->eval_form(config.model(),  oil_pot,  gas_pot,  wat_pot, nullptr);
        else
            guide_rate = this->eval_form(config.model(),  oil_pot,  gas_pot,  wat_pot, std::addressof(iter->second));
    }

    this->values[wgname] = GuideRateValue{sim_time, guide_rate};
}


void GuideRate::well_update(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot) {
    const auto& config = this->schedule.guideRateConfig(report_step);
    const auto& well = config.well(wgname);

    if (well.guide_rate > 0)
        this->values[wgname] = GuideRateValue( sim_time, well.guide_rate );
    else {
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

        this->values[wgname] = GuideRateValue{sim_time, guide_rate};
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
