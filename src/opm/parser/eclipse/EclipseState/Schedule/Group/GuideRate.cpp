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
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <fmt/core.h>
#include <stddef.h>
#include <iostream>
#include <cassert>

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

    if (target == GuideRateModel::Target::RES)
        return this->oil_rat + this->wat_rat + this->gas_rat;

    throw std::logic_error {
        "Don't know how to convert target type " + std::to_string(static_cast<int>(target))
    };
}

GuideRate::RateVector
GuideRate::RateVector::rateVectorFromGuideRate(const double guide_rate, const GuideRateModel::Target target,
                                               const GuideRate::RateVector& rates)
{
    double scale = 0.;
    // std::cout << " when calcuating the guide rate, what type it is " << GuideRateModel::targetToString(target) << std::endl;

    switch(target) {
        case GuideRateModel::Target::OIL: {
            assert(rates.oil_rat != 0.);
            scale = guide_rate / rates.oil_rat;
            break;
        }
        case GuideRateModel::Target::GAS: {
            assert(rates.gas_rat != 0);
            scale = guide_rate / rates.gas_rat;
            break;
        }
            /* if (target == GuideRateModel::Target::GAS)
            if (target == GuideRateModel::Target::LIQ)
            if (target == GuideRateModel::Target::WAT)
            if (target == GuideRateModel::Target::RES) */
        default:
            throw std::logic_error{
                    "do not know what to do to generate the full guide rate vector" +
                    std::to_string(static_cast<int>(target))
            };
    }
    return {scale * rates.oil_rat, scale * rates.gas_rat, scale * rates.wat_rat};
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
    // TODO: this function needs to be refactored, since now we have values for different phases
    using namespace unit;
    using prefix::micro;

    auto iter = this->values.find(name);
    if (iter == this->values.end()) {
        return this->potentials.at(name).eval(model_target);
    }

    const auto& value = *iter->second;
    assert(value.curr.sim_time >= 0.);
    // const auto temp = value.curr.value.eval(value.curr.target);
    // if (model_target != value.curr.target) {
    //     std::cout << " two targets are of different types " << std::endl;
    // }
    // std::cout << " well " << name << " guide rate is " << temp << " of type " << GuideRateModel::targetToString(value.curr.target) << " model_target is " << GuideRateModel::targetToString(model_target) << std::endl;
    // TODO: whether it should be value.curr.target?
    return value.curr.value.eval(model_target);
}

double GuideRate::get(const std::string& name, const Phase& phase) const
{
    auto iter = this->injection_group_values.find(std::make_pair(phase, name));
    if (iter == this->injection_group_values.end()) {
        auto message = fmt::format("Did not find any guiderate values for injection group {}:{}", name, std::to_string(static_cast<int>(phase)));
        throw std::logic_error {message};
    }
    return iter->second;
}

bool GuideRate::has(const std::string& name) const
{
    return this->values.count(name) > 0;
}

bool GuideRate::has(const std::string& name, const Phase& phase) const
{
    return this->injection_group_values.count(std::pair(phase, name)) > 0;
}

void GuideRate::compute(const std::string& wgname,
                        size_t             report_step,
                        double             sim_time,
                        double             oil_pot,
                        double             gas_pot,
                        double             wat_pot,
                        const bool update_now)
{
    this->potentials[wgname] = RateVector{oil_pot, gas_pot, wat_pot};

    const auto& config = this->schedule.guideRateConfig(report_step);
    if (config.has_production_group(wgname)) {
        this->group_compute(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot);
    }
    else {
        // std::cout << " whether config has this well " << wgname << "  " << config.has_well(wgname) << std::endl;
        // TODO: here sometimes, it is a group entering this function
        // at the same time, it should only happens when GCONPROD specifies `FORM`, that you can use
        // potentials to calculate the guide rate?
        // Let us double check whether it is always zero pot enter here.
        this->well_compute(wgname, report_step, sim_time, oil_pot, gas_pot, wat_pot, update_now);
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
    const auto& group = config.production_group(wgname);
    if (group.guide_rate > 0.0) {
        auto model_target = GuideRateModel::convert_target(group.target);

        const auto& model = config.has_model() ? config.model() : GuideRateModel{};
        const auto guide_rates = RateVector::rateVectorFromGuideRate(group.guide_rate, model_target, {oil_pot, wat_pot, gas_pot});
        this->assign_grvalue(wgname, model, { sim_time, guide_rates, model_target });
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

                // TODO: here will also use the a bool variable to decide whether to update
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

            const auto guide_rates = this->eval_form(config.model(), oil_pot, gas_pot, wat_pot);
            this->assign_grvalue(wgname, config.model(), { sim_time, guide_rates, config.model().target() });
        }
    }
}

void GuideRate::compute(const std::string& wgname,
                        const Phase& phase,
                        size_t report_step,
                        double guide_rate)
{
    const auto& config = this->schedule.guideRateConfig(report_step);
    if (!config.has_injection_group(phase, wgname))
        return;

    if (guide_rate > 0) {
        this->injection_group_values[std::make_pair(phase, wgname)] = guide_rate;
        return;
    }

    const auto& group = config.injection_group(phase, wgname);
    if (group.target == Group::GuideRateInjTarget::POTN) {
        return;
    }
    this->injection_group_values[std::make_pair(phase, wgname)] = group.guide_rate;
}

void GuideRate::well_compute(const std::string& wgname,
                             size_t             report_step,
                             double             sim_time,
                             double             oil_pot,
                             double             gas_pot,
                             double             wat_pot,
                             const bool update_now)
{
    const auto& config = this->schedule.guideRateConfig(report_step);

    // guide rates spesified with WGRUPCON
    if (config.has_well(wgname)) {
        const auto& well = config.well(wgname);
        if (well.guide_rate > 0.0) {
            // TODO: what is the difference between model_target and model.target() here?
            // TODO: adding an assertion to check
            auto model_target = GuideRateModel::convert_target(well.target);
            const auto& model = config.has_model() ? config.model() : GuideRateModel{};
            assert(model_target == model.target());
            const auto guide_rates = RateVector::rateVectorFromGuideRate(well.guide_rate, model.target(), {oil_pot, gas_pot, wat_pot});
            this->assign_grvalue(wgname, model, { sim_time, guide_rates, model_target });
        }
    }
    else if (config.has_model()) { // GUIDERAT
        // only look for wells not groups
        if (! this->schedule.hasWell(wgname, report_step)) {
            return;
        }

        // new well always need to calculate the guide rate
        if (!update_now && this->values.count(wgname) > 0) {
            return;
        }

        const auto& well = this->schedule.getWell(wgname, report_step);

        // GUIDERAT does not apply to injectors
        if (well.isInjector()) {
            return;
        }
        const auto guide_rates = this->eval_form(config.model(), oil_pot, gas_pot, wat_pot);
        // std::cout << " well " << wgname << " guide rates " << guide_rates.oil_rat  * 86400. << " "
        //           << guide_rates.gas_rat * 86400. << " " << guide_rates.wat_rat * 86400. << " "
        //           << " oil_pot " << oil_pot * 86400. << " gas_pot " << gas_pot * 86400. << " wat_pot " << wat_pot * 86400. << std::endl;
        auto& v = this->values[wgname];
        if (v == nullptr) {
            v = std::make_unique<GRValState>();
        }
        if (sim_time > v->curr.sim_time) {
            // We've advanced in time since we previously calculated/stored this
            // guiderate value.  Push current value into the past and prepare to
            // capture new value.
            using std::swap;

            swap(v->prev, v->curr);
        }
        auto new_guide_rate = guide_rates.oil_rat;
        if (!((v->prev.sim_time < 0.0) || v->prev.value.oil_rat <= 0.0)) {

            // Incorporate damping &c.
            // TODO: the following should be applied to the defined phase, then other rateds need to be updated accordingly
            new_guide_rate = config.model().allow_increase()
                             ? guide_rates.oil_rat : std::min(guide_rates.oil_rat, v->prev.value.oil_rat);

            const auto damping_factor = config.model().damping_factor();
            new_guide_rate = damping_factor * new_guide_rate + (1 - damping_factor) * v->prev.value.oil_rat;

        }
        const auto new_guide_rates = RateVector::rateVectorFromGuideRate(new_guide_rate, config.model().target(),
                                                                         {oil_pot, gas_pot, wat_pot});

        this->assign_grvalue(wgname, config.model(), { sim_time, new_guide_rates, config.model().target() });
    }
    // If neither WGRUPCON nor GUIDERAT is specified potentials are used
}

GuideRate::RateVector GuideRate::eval_form(const GuideRateModel& model, double oil_pot, double gas_pot, double wat_pot) const
{
    const double guide_rate = model.eval(oil_pot, gas_pot, wat_pot);
    // std::cout << " guide_rate is " << guide_rate * 86400. << " model_target is " << GuideRateModel::targetToString(model.target()) << std::endl;
    return RateVector::rateVectorFromGuideRate(guide_rate, model.target(), {oil_pot, gas_pot, wat_pot});
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

    //  TODO: (v->prev.value > 0.0) needs to be more sphosicated
    // maybe `value` should be a function
    /* if ((v->prev.sim_time < 0.0) || ! (v->prev.value.oil_rat > 0.0)) {
        // No previous non-zero guiderate exists.  No further actions.
        return;
    }

    // Incorporate damping &c.
    // TODO: the following should be applied to the defined phase, then other rateds need to be updated accordingly
    const auto new_guide_rate = model.allow_increase()
        ? v->curr.value.oil_rat : std::min(v->curr.value.oil_rat, v->prev.value.oil_rat);

    const auto damping_factor = model.damping_factor();
    v->curr.value.oil_rat = damping_factor*new_guide_rate + (1 - damping_factor)*v->prev.value.oil_rat;
     */
}

/* double GuideRate::get_grvalue_result(const GRValState& gr) const
{
    return (gr.curr.sim_time < 0.0)
        ? 0.0
        : std::max(gr.curr.value.oil_rat, 0.0);
} */

bool GuideRate::timeToUpdate(const double sim_time, const double time_interval) const {
    // getting the last update time
    // when there is no guide rates yet, we should always update when necessary
    if (this->values.size() == 0) return true;

    double last_update_time = 1.e99;
    for ([[maybe_unused]] const auto& [wgname, value] : this->values) {
        const double update_time = value->curr.sim_time;
        if (update_time < last_update_time) {
            last_update_time = update_time;
        }
    }

    return (sim_time >= (last_update_time + time_interval));
}

}
