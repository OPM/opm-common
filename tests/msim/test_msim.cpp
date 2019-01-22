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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE WTEST
#include <boost/test/unit_test.hpp>

#include <ert/util/test_work_area.h>
#include <ert/util/util.h>
#include <ert/ecl/ecl_sum.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_file_view.hpp>
#include <ert/ecl/ecl_rsthead.hpp>


#include <opm/msim/msim.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

using namespace Opm;


double prod_opr(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    return -units.to_si(UnitSystem::measure::rate, seconds_elapsed);
}

void pressure(const EclipseState& es, const Schedule& sched, data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& grid = es.getInputGrid();
    const auto& units = es.getUnits();
    if (!sol.has("PRESSURE"))
        sol.insert("PRESSURE", UnitSystem::measure::pressure, std::vector<double>(grid.getNumActive()), data::TargetType::RESTART_SOLUTION);

    auto& data = sol.data("PRESSURE");
    std::fill(data.begin(), data.end(), units.to_si(UnitSystem::measure::pressure, seconds_elapsed));
}


BOOST_AUTO_TEST_CASE(RUN) {
    Parser parser;
    Deck deck = parser.parseFile("SPE1CASE1.DATA");
    EclipseState state(deck);
    Schedule schedule(deck, state.getInputGrid(), state.get3DProperties(), state.runspec());
    SummaryConfig summary_config(deck, schedule, state.getTableManager());
    msim msim(state);

    msim.well_rate("PROD", data::Rates::opt::oil, prod_opr);
    msim.solution("PRESSURE", pressure);
    {
        test_work_area_type * work_area = test_work_area_alloc("test_msim");
        EclipseIO io(state, state.getInputGrid(), schedule, summary_config);

        msim.run(schedule, io);

        for (const auto& fname : {"SPE1CASE1.INIT", "SPE1CASE1.UNRST", "SPE1CASE1.EGRID", "SPE1CASE1.SMSPEC", "SPE1CASE1.UNSMRY"})
            BOOST_CHECK( util_is_file( fname ));

        {
            ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case("SPE1CASE1", ":");
            int param_index = ecl_sum_get_general_var_params_index(ecl_sum, "WOPR:PROD");
            for (int time_index=0; time_index < ecl_sum_get_data_length(ecl_sum); time_index++) {
                double seconds_elapsed = ecl_sum_iget_sim_days(ecl_sum, time_index) * 86400;
                double opr = ecl_sum_iget(ecl_sum, time_index, param_index);

                opr = ecl_sum_iget(ecl_sum, time_index, param_index);
                BOOST_CHECK_CLOSE(seconds_elapsed, opr, 1e-3);
            }
            ecl_sum_free( ecl_sum );
        }

        {
            int report_step = 1;
            ecl_file_type * rst_file = ecl_file_open("SPE1CASE1.UNRST", 0);
            ecl_file_view_type * global_view = ecl_file_get_global_view( rst_file );
            while (true) {
                ecl_file_view_type * rst_view = ecl_file_view_add_restart_view( global_view, -1, report_step, -1, -1);
                if (!rst_view)
                    break;

                {
                    ecl_rsthead_type * rst_head = ecl_rsthead_alloc( rst_view, report_step);
                    const ecl_kw_type * p = ecl_file_view_iget_named_kw(rst_view, "PRESSURE", 0);
                    BOOST_CHECK_CLOSE( ecl_kw_iget_float(p, 0), rst_head->sim_days * 86400, 1e-3 );
                    ecl_rsthead_free( rst_head );
                }
                report_step++;
            }
            ecl_file_close(rst_file);
        }

        test_work_area_free( work_area );
    }
}
