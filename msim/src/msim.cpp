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

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/msim/msim.hpp>

namespace Opm {

msim::msim(const std::string& deck_file, const Parser& parser, const ParseContext& parse_context) :
    deck(parser.parseFile(deck_file, parse_context)),
    state(deck, parse_context),
    schedule(deck, state.getInputGrid(), state.get3DProperties(), state.runspec(), parse_context),
    summary_config(deck, schedule, state.getTableManager(), parse_context)
{
}


msim::msim(const std::string& deck_file) :
    msim(deck_file, Parser(), ParseContext())
{}


void msim::run() {
    const double week = 7 * 86400;
    EclipseIO io(this->state, this->state.getInputGrid(), this->schedule, this->summary_config);
    data::Solution sol;
    data::Wells well_data;

    io.writeInitial();
    for (size_t report_step = 1; report_step < this->schedule.size(); report_step++) {
        double time_step = std::min(week, 0.5*this->schedule.stepLength(report_step - 1));
        run_step(sol, well_data, report_step, time_step, io);
    }
}


void msim::run_step(data::Solution& sol, data::Wells& well_data, size_t report_step, EclipseIO& io) const {
    this->run_step(sol, well_data, report_step, this->schedule.stepLength(report_step - 1), io);
}


void msim::run_step(data::Solution& sol, data::Wells& well_data, size_t report_step, double dt, EclipseIO& io) const {
    double start_time = this->schedule.seconds(report_step - 1);
    double end_time = this->schedule.seconds(report_step);
    double seconds_elapsed = start_time;

    while (seconds_elapsed < end_time) {
        double time_step = dt;
        if ((seconds_elapsed + time_step) > end_time)
            time_step = end_time - seconds_elapsed;

        // Simulate

        seconds_elapsed += time_step;
        this->output(report_step,
                     (seconds_elapsed < end_time),
                     seconds_elapsed,
                     sol,
                     well_data,
                     io);
    }
}



void msim::output(size_t report_step, bool substep, double seconds_elapsed, const data::Solution& sol, const data::Wells& well_data, EclipseIO& io) const {
    RestartValue value(sol, well_data);
    io.writeTimeStep(report_step,
                     false,
                     seconds_elapsed,
                     value,
                     {},
                     {},
                     {});
}

}
