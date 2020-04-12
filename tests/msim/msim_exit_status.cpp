/*
  Copyright 2020 Equinor ASA.

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

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/msim/msim.hpp>

namespace Opm {

double prod_opr(const EclipseState&  es, const Schedule& /* sched */, const SummaryState&, const data::Solution& /* sol */, size_t /* report_step */, double /* seconds_elapsed */) {
    const auto& units = es.getUnits();
    double oil_rate = 1.0;
    return -units.to_si(UnitSystem::measure::rate, oil_rate);
}


double prod_wpr_P1(const EclipseState&  es, const Schedule& /* sched */, const SummaryState&, const data::Solution& /* sol */, size_t /* report_step */, double /* seconds_elapsed */) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P2(const EclipseState&  es, const Schedule& /* sched */, const SummaryState&, const data::Solution& /* sol */, size_t report_step, double /* seconds_elapsed */) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    if (report_step > 5)
        water_rate = 2.0;  // => WWCT = WWPR / (WOPR + WWPR) = 2/3

    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P3(const EclipseState&  es, const Schedule& /* sched */, const SummaryState&, const data::Solution& /* sol */, size_t /* report_step */, double /* seconds_elapsed */) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P4(const EclipseState&  es, const Schedule& /* sched */, const SummaryState&, const data::Solution& /* sol */, size_t report_step, double /* seconds_elapsed */) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    if (report_step > 10)
        water_rate = 2.0;

    return -units.to_si(UnitSystem::measure::rate, water_rate);
}
}

int main(int , char **argv) {
    std::string deck_file = argv[1];
    Opm::Parser parser;
    Opm::ParseContext parse_context;
    Opm::ErrorGuard error_guard;
    auto python = std::make_shared<Opm::Python>();

    Opm::Deck deck = parser.parseFile(deck_file, parse_context, error_guard);
    Opm::EclipseState state(deck);
    Opm::Schedule schedule(deck, state, parse_context, error_guard, python);
    Opm::SummaryConfig summary_config(deck, schedule, state.getTableManager(), parse_context, error_guard);

    if (error_guard) {
        error_guard.dump();
        error_guard.terminate();
    }

    Opm::msim msim(state);
    Opm::EclipseIO io(state, state.getInputGrid(), schedule, summary_config);
    msim.well_rate("P1", Opm::data::Rates::opt::oil, Opm::prod_opr);
    msim.well_rate("P2", Opm::data::Rates::opt::oil, Opm::prod_opr);
    msim.well_rate("P3", Opm::data::Rates::opt::oil, Opm::prod_opr);
    msim.well_rate("P4", Opm::data::Rates::opt::oil, Opm::prod_opr);

    msim.well_rate("P1", Opm::data::Rates::opt::wat, Opm::prod_wpr_P1);
    msim.well_rate("P2", Opm::data::Rates::opt::wat, Opm::prod_wpr_P2);
    msim.well_rate("P3", Opm::data::Rates::opt::wat, Opm::prod_wpr_P3);
    msim.well_rate("P4", Opm::data::Rates::opt::wat, Opm::prod_wpr_P4);
    msim.run(schedule, io, false);
}
