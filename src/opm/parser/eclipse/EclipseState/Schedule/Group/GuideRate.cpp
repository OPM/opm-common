/*
  Copyright 2019, 2020 Equinor ASA.

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

#include <opm/parser/eclipse/Units/Units.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <stddef.h>

namespace Opm {

double GuideRate::RateVector::eval(Well::GuideRateTarget target) const
{
    return this->eval(GuideRateModel::convert_target(target));
}

double GuideRate::RateVector::eval(Group::GuideRateProdTarget target) const
{
    return this->eval(GuideRateModel::convert_target(target));
}

double GuideRate::RateVector::eval(GuideRateModel::Target target) const
{
    if (target == GuideRateModel::Target::OIL)
        return this->oil_rat;

    if (target == GuideRateModel::Target::GAS)
        return this->gas_rat;

    if (target == GuideRateModel::Target::LIQ)
        return this->oil_rat + this->wat_rat;

    if (target == GuideRateModel::Target::WAT)
        return this->wat_rat;

    throw std::logic_error {
        "Don't know how to convert target type " + std::to_string(static_cast<int>(target))
    };
}


GuideRate::GuideRate(const Schedule& schedule_arg) :
    schedule(schedule_arg)
{}

double GuideRate::get(const std::string& well, Well::GuideRateTarget target, const RateVector& rates) const
{
    return this->get(well, GuideRateModel::convert_target(target), rates);
}

double GuideRate::get(const std::string& group, Group::GuideRateProdTarget target, const RateVector& rates) const
{
    return this->get(group, GuideRateModel::convert_target(target), rates);
}

double GuideRate::get(const std::string& name, GuideRateModel::Target model_target, const RateVector& rates) const
{
    using namespace unit;
    using prefix::micro;

    auto iter = this->values.find(name);
    if (iter == this->values.end()) {
        return this->potentials.at(name).eval(model_target);
    }

    const auto& value = *iter->second;
    const auto grvalue = this->get_grvalue_result(value);
    if (value.curr.target == model_target) {
        return grvalue;
    }

    const auto value_target_rate = rates.eval(value.curr.target);
    if (value_target_rate < 1.0*micro*liter/day) {
        return grvalue;
    }

    // Scale with the current production ratio when the control target
    // differs from the guide rate target.
    const auto scale = rates.eval(model_target) / value_target_rate;
    return grvalue * scale;
}

bool GuideRate::has(const std::string& name) const
{
    return this->values.count(name) > 0;
}

void GuideRate::compute(const std::string& wgname,
                        size_t             report_step,
                        double             sim_time,
                        double             oil_pot,
                        double             gas_pot,
                        double             wat_pot)
{
    this->potentials[wgname] = RateVector{oil_pot, gas_pot, wat_pot};

    const auto& config = this->schedule.guideRateConfig(report_step);
    if (config.has_group(wgname)) {
        this->group_compute(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);
    }
    else {
        this->well_compute(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);
    }
}

void GuideRate::group_compute(const std::string& wgname,
                              size_t             report_step,
                              double             sim_time,
                              double             oil_pot,
                              double             gas_pot,
                              double             wat_pot)
{
    const auto& config = this->schedule.guideRateConfig(report_step);
    const auto& group = config.group(wgname);

    if (group.guide_rate > 0.0) {
        auto model_target = GuideRateModel::convert_target(group.target);

        const auto& model = config.has_model() ? config.model() : GuideRateModel{};
        this->assign_grvalue(wgname, model, { sim_time, group.guide_rate, model_target });
    }
    else {
        auto iter = this->values.find(wgname);

        // If the FORM mode is used we check if the last computation is
        // recent enough; then we just return.
        if (iter != this->values.end()) {
            if (group.target == Group::GuideRateProdTarget::FORM) {
                if (! config.has_model()) {
                    throw std::logic_error {
                        "When specifying GUIDERATE target FORM you must "
                        "enter a guiderate model with the GUIDERAT keyword"
                    };
                }

                const auto& grv = iter->second->curr;
                const auto time_diff = sim_time - grv.sim_time;
                if (config.model().update_delay() > time_diff) {
                    return;
                }
            }
        }

        if (group.target == Group::GuideRateProdTarget::INJV) {
            throw std::logic_error("Group guide rate mode: INJV not implemented");
        }

        if (group.target == Group::GuideRateProdTarget::POTN) {
            throw std::logic_error("Group guide rate mode: POTN not implemented");
        }

        if (group.target == Group::GuideRateProdTarget::FORM) {
            if (! config.has_model()) {
                throw std::logic_error {
                    "When specifying GUIDERATE target FORM you must "
                    "enter a guiderate model with the GUIDERAT keyword"
                };
            }

            const auto guide_rate = this->eval_form(config.model(), oil_pot, gas_pot, wat_pot);
            this->assign_grvalue(wgname, config.model(), { sim_time, guide_rate, config.model().target() });
        }
    }
}

void GuideRate::well_compute(const std::string& wgname,
                             size_t             report_step,
                             double             sim_time,
                             double             oil_pot,
                             double             gas_pot,
                             double             wat_pot)
{
    const auto& config = this->schedule.guideRateConfig(report_step);

    // guide rates spesified with WGRUPCON
    if (config.has_well(wgname)) {
        const auto& well = config.well(wgname);
        if (well.guide_rate > 0.0) {
            auto model_target = GuideRateModel::convert_target(well.target);

            const auto& model = config.has_model() ? config.model() : GuideRateModel{};
            this->assign_grvalue(wgname, model, { sim_time, well.guide_rate, model_target });
        }
    }
    else if (config.has_model()) { // GUIDERAT
        // only look for wells not groups
        if (! this->schedule.hasWell(wgname, report_step)) {
            return;
        }

        const auto& well = this->schedule.getWell(wgname, report_step);

        // GUIDERAT does not apply to injectors
        if (well.isInjector()) {
            return;
        }

        auto iter = this->values.find(wgname);
        if (iter != this->values.end()) {
            const auto& grv = iter->second->curr;
            const auto time_diff = sim_time - grv.sim_time;
            if (config.model().update_delay() > time_diff) {
                return;
            }
        }

        const auto guide_rate = this->eval_form(config.model(), oil_pot, gas_pot, wat_pot);
        this->assign_grvalue(wgname, config.model(), { sim_time, guide_rate, config.model().target() });
    }
    // If neither WGRUPCON nor GUIDERAT is specified potentials are used
}

double GuideRate::eval_form(const GuideRateModel& model, double oil_pot, double gas_pot, double wat_pot) const
{
    return model.eval(oil_pot, gas_pot, wat_pot);
}

double GuideRate::eval_group_pot() const
{
    return 0.0;
}

double GuideRate::eval_group_resvinj() const
{
    return 0.0;
}

void GuideRate::assign_grvalue(const std::string&    wgname,
                               const GuideRateModel& model,
                               GuideRateValue&&      value)
{
    auto& v = this->values[wgname];
    if (v == nullptr) {
        v = std::make_unique<GRValState>();
    }

    if (value.sim_time > v->curr.sim_time) {
        // We've advanced in time since we previously calculated/stored this
        // guiderate value.  Push current value into the past and prepare to
        // capture new value.
        using std::swap;

        swap(v->prev, v->curr);
    }

    v->curr = std::move(value);

    if ((v->prev.sim_time < 0.0) || ! (v->prev.value > 0.0)) {
        // No previous non-zero guiderate exists.  No further actions.
        return;
    }

    // Incorporate damping &c.
    const auto new_guide_rate = model.allow_increase()
        ? v->curr.value : std::min(v->curr.value, v->prev.value);

    const auto damping_factor = model.damping_factor();
    v->curr.value = damping_factor*new_guide_rate + (1 - damping_factor)*v->prev.value;
}

double GuideRate::get_grvalue_result(const GRValState& gr) const
{
    return (gr.curr.sim_time < 0.0)
        ? 0.0
        : std::max(gr.curr.value, 0.0);
}

}
