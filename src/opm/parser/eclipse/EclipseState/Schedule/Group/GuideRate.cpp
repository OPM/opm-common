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
#include <type_traits>
#include <utility>

#include <stddef.h>
#include <cassert>

#include <fmt/core.h>

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


GuideRate::GuideRate(const Schedule& schedule_arg) :
    schedule(schedule_arg)
{}

double GuideRate::get(const std::string& well, Well::GuideRateTarget target) const
{
    return this->get(well, GuideRateModel::convert_target(target));
}

double GuideRate::get(const std::string& group, Group::GuideRateProdTarget target) const
{
    return this->get(group, GuideRateModel::convert_target(target));
}

double GuideRate::get(const std::string& name, GuideRateModel::Target model_target) const
{
    using namespace unit;
    // using prefix::micro;

    auto iter = this->values.find(name);
    if (iter == this->values.end()) {
        return this->potentials.at(name).eval(model_target);
    }

    const auto& value = *iter->second;
    return value.curr.value.eval(model_target);

    /* const auto value_target_rate = rates.eval(value.curr.target);
    if (value_target_rate < 1.0*micro*liter/day) {
        return grvalue;
    } */
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

double GuideRate::getSI(const std::string&          well,
                        const Well::GuideRateTarget target,
                        const RateVector&           rates) const
{
    return this->getSI(well, GuideRateModel::convert_target(target), rates);
}

double GuideRate::getSI(const std::string&               group,
                        const Group::GuideRateProdTarget target,
                        const RateVector&                rates) const
{
    return this->getSI(group, GuideRateModel::convert_target(target), rates);
}

double GuideRate::getSI(const std::string&           wgname,
                        const GuideRateModel::Target target,
                        const RateVector&            rates) const
{
    using M = UnitSystem::measure;

    const auto gr = this->get(wgname, target, rates);

    switch (target) {
    case GuideRateModel::Target::OIL:
    case GuideRateModel::Target::WAT:
    case GuideRateModel::Target::LIQ:
        return this->schedule.getUnits().to_si(M::liquid_surface_rate, gr);

    case GuideRateModel::Target::GAS:
        return this->schedule.getUnits().to_si(M::gas_surface_rate, gr);

    case GuideRateModel::Target::RES:
        return this->schedule.getUnits().to_si(M::rate, gr);

    case GuideRateModel::Target::NONE:
    case GuideRateModel::Target::COMB:
        return gr;
    }

    throw std::invalid_argument {
        fmt::format("Unsupported Guiderate Target '{}'",
                    static_cast<std::underlying_type_t<GuideRateModel::Target>>(target))
    };
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
                        double             wat_pot)
{
    this->potentials[wgname] = RateVector{oil_pot, gas_pot, wat_pot};

    const auto& config = this->schedule[report_step].guide_rate();
    if (config.has_production_group(wgname)) {
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
    const auto& config = this->schedule[report_step].guide_rate();
    const auto& group = config.production_group(wgname);
    if (group.guide_rate > 0.0) {
        auto gr_target = GuideRateModel::convert_target(group.target);
        const auto& model = config.has_model() ? config.model() : GuideRateModel{};
        // \Note: the model here might have a `NONE` guide rate type, we have to use the one from the group
        this->assign_grvalue(wgname, model, sim_time, group.guide_rate, gr_target, {oil_pot, gas_pot, wat_pot});
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

                if (!this->guide_rates_expired && this->values.at(wgname)->curr.get_value() > 0) return;
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
            const auto model_target = config.model().target();
            this->assign_grvalue(wgname, config.model(), sim_time, guide_rate, model_target, {oil_pot, gas_pot, wat_pot});
        }
    }
}

void GuideRate::compute(const std::string& wgname,
                        const Phase& phase,
                        size_t report_step,
                        double guide_rate)
{
    const auto& config = this->schedule[report_step].guide_rate();
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
                             double             wat_pot)
{
    const auto& config = this->schedule[report_step].guide_rate();
    // guide rates specified with WGRUPCON
    if (config.has_well(wgname)) {
        const auto& well = config.well(wgname);
        if (well.guide_rate > 0.0) {
            const auto& model = config.has_model() ? config.model() : GuideRateModel{};
            const auto well_target = GuideRateModel::convert_target(well.target);
            this->assign_grvalue(wgname, model, sim_time, well.guide_rate, well_target, {oil_pot, gas_pot, wat_pot});
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

        // Newly opened wells without calculated guide rates always need to update guide rates
        // With a non-zero guide rate value basically mean the well exisits in GuideRate already, aka, NOT new well
        if (!this->guide_rates_expired &&
            (this->values.count(wgname) > 0 && this->values.at(wgname)->curr.get_value() > 0) ) {
            return;
        }

        const auto guide_rate = this->eval_form(config.model(), oil_pot, gas_pot, wat_pot);
        const auto model_target = config.model().target();
        this->assign_grvalue(wgname, config.model(), sim_time, guide_rate, model_target, {oil_pot, gas_pot, wat_pot});
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

void GuideRate::assign_grvalue(const std::string& wgname, const GuideRateModel& model, double sim_time, double value,
                               const GuideRateModel::Target target, const RateVector& rates)
{
    // TODO: with current way,  assign_grvalue is given too much content to take care of
    // It is possible, we can create another function do all the processing and make the function assign_grvalue
    // very simple and straightforward

    auto& v = this->values[wgname];
    if (v == nullptr) {
        v = std::make_unique<GRValState>();
    }

    if (sim_time > v->curr.sim_time) {
        // We've advanced in time since we previously calculated/stored this
        // guiderate value.  Push current value into the past and prepare to
        // capture new value.
        std::swap(v->prev, v->curr);
    }

    auto new_gr_value = value;

    // processing the value based on allow_increase and damping_factor
    if (v->prev.sim_time >= 0.0 && v->prev.get_value() > 0.0) {
        const auto prev_gr_value = v->prev.get_value();

        // Incorporate damping &c.
        new_gr_value = model.allow_increase()
                       ? new_gr_value : std::min(new_gr_value, prev_gr_value);

        const auto damping_factor = model.damping_factor();
        new_gr_value = damping_factor * new_gr_value + (1 - damping_factor) * prev_gr_value;
    }

    const auto guide_rates = GuideRate::rateVectorFromGuideRate(new_gr_value, target, rates);
    v->curr = {sim_time, guide_rates, target};
}


double GuideRate::get_grvalue_result(const GRValState& gr) const
{
    return (gr.curr.sim_time < 0.0)
        ? 0.0
        : std::max(gr.curr.get_value(), 0.0);
}

GuideRate::RateVector GuideRate::rateVectorFromGuideRate(const double guide_rate, GuideRateModel::Target target,
                                                         const GuideRate::RateVector& rates) {
    using namespace unit;
    using prefix::micro;

    if (guide_rate < 1.0 * micro * liter / day) {
        return {guide_rate, guide_rate, guide_rate};
    }

    double scale = 0.;

    switch(target) {
        case GuideRateModel::Target::OIL: {
            assert(rates.oil_rat != 0.);
            scale = guide_rate / rates.oil_rat;
            break;
        }
        case GuideRateModel::Target::GAS: {
            assert(rates.gas_rat != 0.);
            scale = guide_rate / rates.gas_rat;
            break;
        }
        case GuideRateModel::Target::LIQ: {
            const double liq_rat = rates.wat_rat + rates.oil_rat;
            assert(liq_rat != 0.);
            scale = guide_rate / liq_rat;
            break;
        }
        case GuideRateModel::Target::RES: {
            // TODO: not totally sure how to handle RES rate
            const double total_rate = rates.wat_rat + rates.oil_rat + rates.gas_rat;
            assert(total_rate != 0.);
            scale = guide_rate / total_rate;
            break;
        }
        case GuideRateModel::Target::WAT: {
            // Note: GUIDERAT does not have a WAT nominated phase, we might need it
            // for injectors?
           assert(rates.wat_rat != 0.);
           scale = guide_rate / rates.wat_rat;
           break;
        }
        default:
            throw std::logic_error{
                    "do not know what to do to generate the full guide rate vector" +
                    std::to_string(static_cast<int>(target))
            };
    }
    return {scale * rates.oil_rat, scale * rates.gas_rat, scale * rates.wat_rat};
}


void GuideRate::updateGuideRateExpiration(double sim_time, size_t report_step)
{
    const auto& config = this->schedule[report_step].guide_rate();

    if (!config.has_model()) {
        this->guide_rates_expired = false;
        return;
    }

    // getting the last update time
    double last_update_time = std::numeric_limits<double>::max();
    for ([[maybe_unused]] const auto& [wgname, value] : this->values) {
        const double update_time = value->curr.sim_time;
        if (value->curr.get_value() > 0 && update_time < last_update_time) {
            last_update_time = update_time;
        }
    }

    const double update_delay = config.model().update_delay();
    this->guide_rates_expired = (sim_time >= (last_update_time + update_delay));
}

}
