/*
  Copyright 2018 Equinor ASA.

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

#include <iostream>

#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/msim/msim.hpp>

namespace Opm {

msim::msim(const EclipseState& state) :
    state(state)
{}

void msim::run(Schedule& schedule, EclipseIO& io) {
    const double week = 7 * 86400;
    data::Solution sol;
    data::Wells well_data;

    io.writeInitial();
    for (size_t report_step = 1; report_step < schedule.size(); report_step++) {
        double time_step = std::min(week, 0.5*schedule.stepLength(report_step - 1));
        run_step(schedule, sol, well_data, report_step, time_step, io);
        post_step(schedule, sol, well_data, report_step, io);
    }
}


void msim::post_step(Schedule& schedule, data::Solution& /* sol */, data::Wells& /* well_data */, size_t report_step, EclipseIO& io) const {
    const auto& actions = schedule.actions();
    if (actions.empty())
        return;

    const SummaryState& summary_state = io.summaryState();
    ActionContext context( summary_state );
    std::vector<std::string> matching_wells;

    auto sim_time = schedule.simTime(report_step);
    for (const auto& action : actions.pending(sim_time)) {
        if (action->eval(sim_time, context, matching_wells))
            schedule.applyAction(report_step, *action, matching_wells);
    }
}



void msim::run_step(const Schedule& schedule, data::Solution& sol, data::Wells& well_data, size_t report_step, EclipseIO& io) const {
    this->run_step(schedule, sol, well_data, report_step, schedule.stepLength(report_step - 1), io);
}


void msim::run_step(const Schedule& schedule, data::Solution& sol, data::Wells& well_data, size_t report_step, double dt, EclipseIO& io) const {
    double start_time = schedule.seconds(report_step - 1);
    double end_time = schedule.seconds(report_step);
    double seconds_elapsed = start_time;

    while (seconds_elapsed < end_time) {
        double time_step = dt;
        if ((seconds_elapsed + time_step) > end_time)
            time_step = end_time - seconds_elapsed;

        this->simulate(schedule, sol, well_data, report_step, seconds_elapsed, time_step);

        seconds_elapsed += time_step;
        this->output(report_step,
                     (seconds_elapsed < end_time),
                     seconds_elapsed,
                     sol,
                     well_data,
                     io);
    }
}



void msim::output(size_t report_step, bool /* substep */, double seconds_elapsed, const data::Solution& sol, const data::Wells& well_data, EclipseIO& io) const {
    RestartValue value(sol, well_data);
    io.writeTimeStep(report_step,
                     false,
                     seconds_elapsed,
                     value,
                     {},
                     {},
                     {});
}


void msim::simulate(const Schedule& schedule, data::Solution& sol, data::Wells& well_data, size_t report_step, double seconds_elapsed, double time_step) const {
    for (const auto& sol_pair : this->solutions) {
        auto func = sol_pair.second;
        func(this->state, schedule, sol, report_step, seconds_elapsed + time_step);
    }

    for (const auto& well_pair : this->well_rates) {
        const std::string& well_name = well_pair.first;
        data::Well& well = well_data[well_name];
        for (const auto& rate_pair : well_pair.second) {
            auto rate = rate_pair.first;
            auto func = rate_pair.second;

            well.rates.set(rate, func(this->state, schedule, sol, report_step, seconds_elapsed + time_step));
        }

        // This is complete bogus; a temporary fix to pass an assert() in the
        // the restart output.
        well.connections.resize(100);
    }
}


void msim::well_rate(const std::string& well, data::Rates::opt rate, std::function<well_rate_function> func) {
    this->well_rates[well][rate] = func;
}


void msim::solution(const std::string& field, std::function<solution_function> func) {
    this->solutions[field] = func;
}


}


