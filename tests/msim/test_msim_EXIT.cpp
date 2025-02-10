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

#define BOOST_TEST_MODULE ACTIONX_SIM

#include <boost/test/unit_test.hpp>

#include <opm/msim/msim.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>

#include <tests/WorkArea.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

    double prod_opr(const Opm::EclipseState& es,
                    const Opm::Schedule& /* sched */,
                    const Opm::SummaryState&,
                    const Opm::data::Solution& /* sol */,
                    const std::size_t /* report_step */,
                    const double /* seconds_elapsed */)
    {
        const auto& units = es.getUnits();
        const double oil_rate = 1.0;
        return -units.to_si(Opm::UnitSystem::measure::rate, oil_rate);
    }

    double prod_wpr_P1(const Opm::EclipseState& es,
                       const Opm::Schedule& /* sched */,
                       const Opm::SummaryState&,
                       const Opm::data::Solution& /* sol */,
                       const std::size_t /* report_step */,
                       const double /* seconds_elapsed */)
    {
        const auto& units = es.getUnits();
        const double water_rate = 0.0;
        return -units.to_si(Opm::UnitSystem::measure::rate, water_rate);
    }

    double prod_wpr_P2(const Opm::EclipseState& es,
                       const Opm::Schedule& /* sched */,
                       const Opm::SummaryState&,
                       const Opm::data::Solution& /* sol */,
                       const std::size_t report_step,
                       const double /* seconds_elapsed */)
    {
        const auto& units = es.getUnits();
        double water_rate = 0.0;
        if (report_step > 5) {
            water_rate = 2.0;  // => WWCT = WWPR / (WOPR + WWPR) = 2/3
        }

        return -units.to_si(Opm::UnitSystem::measure::rate, water_rate);
    }

    double prod_wpr_P3(const Opm::EclipseState& es,
                       const Opm::Schedule& /* sched */,
                       const Opm::SummaryState&,
                       const Opm::data::Solution& /* sol */,
                       const std::size_t /* report_step */,
                       const double /* seconds_elapsed */)
    {
        const auto& units = es.getUnits();
        const double water_rate = 0.0;
        return -units.to_si(Opm::UnitSystem::measure::rate, water_rate);
    }

    double prod_wpr_P4(const Opm::EclipseState& es,
                       const Opm::Schedule& /* sched */,
                       const Opm::SummaryState&,
                       const Opm::data::Solution& /* sol */,
                       const std::size_t report_step,
                       const double /* seconds_elapsed */)
    {
        const auto& units = es.getUnits();
        double water_rate = 0.0;
        if (report_step > 10) {
            water_rate = 2.0;
        }

        return -units.to_si(Opm::UnitSystem::measure::rate, water_rate);
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MSIM_EXIT_TEST)
{
    std::string deck_file = "EXIT_TEST.DATA";

    const Opm::Deck deck = Opm::Parser{}.parseFile(deck_file);
    const Opm::EclipseState state(deck);
    const Opm::Schedule schedule(deck, state, std::make_shared<Opm::Python>());
    Opm::SummaryConfig summary_config(deck, schedule, state.fieldProps(), state.aquifer());

    {
        WorkArea work_area("test_msim");
        Opm::msim msim(state, schedule);
        Opm::EclipseIO io(state, state.getInputGrid(), schedule, summary_config);
        msim.well_rate("P1", Opm::data::Rates::opt::oil, &prod_opr);
        msim.well_rate("P2", Opm::data::Rates::opt::oil, &prod_opr);
        msim.well_rate("P3", Opm::data::Rates::opt::oil, &prod_opr);
        msim.well_rate("P4", Opm::data::Rates::opt::oil, &prod_opr);

        msim.well_rate("P1", Opm::data::Rates::opt::wat, &prod_wpr_P1);
        msim.well_rate("P2", Opm::data::Rates::opt::wat, &prod_wpr_P2);
        msim.well_rate("P3", Opm::data::Rates::opt::wat, &prod_wpr_P3);
        msim.well_rate("P4", Opm::data::Rates::opt::wat, &prod_wpr_P4);
        msim.run(io, false);

        auto exit_status = msim.schedule.exitStatus();
        BOOST_CHECK( exit_status.has_value() );
        BOOST_CHECK_EQUAL(exit_status.value(), 99);
    }
}
