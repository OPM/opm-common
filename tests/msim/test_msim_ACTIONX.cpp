/*
  Copyright 2018 Statoil ASA.

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

#define BOOST_TEST_MODULE ACTIONX_SIM

#include <boost/test/unit_test.hpp>

#include <ert/util/test_work_area.h>
#include <ert/ecl/ecl_sum.hpp>


#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionAST.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actions.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/msim/msim.hpp>

using namespace Opm;



struct test_data {
    Deck deck;
    EclipseState state;
    Schedule schedule;
    SummaryConfig summary_config;

    test_data(const std::string& deck_string) :
        deck( Parser().parseString(deck_string)),
        state( this->deck ),
        schedule( this->deck, this->state.getInputGrid(), this->state.get3DProperties(), this->state.runspec()),
        summary_config( this->deck, this->schedule, this->state.getTableManager())
    {
        auto& ioconfig = this->state.getIOConfig();
        ioconfig.setBaseName("MSIM");
    }
};


double prod_opr(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    double oil_rate = 1.0;
    return -units.to_si(UnitSystem::measure::rate, oil_rate);
}

double prod_opr_low(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    double oil_rate = 0.5;
    return -units.to_si(UnitSystem::measure::rate, oil_rate);
}

double prod_wpr_P1(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P2(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    if (report_step > 5)
        water_rate = 2.0;  // => WWCT = WWPR / (WOPR + WWPR) = 2/3

    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P3(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P4(const EclipseState&  es, const Schedule& sched, const data::Solution& sol, size_t report_step, double seconds_elapsed) {
    const auto& units = es.getUnits();
    double water_rate = 0.0;
    if (report_step > 10)
        water_rate = 2.0;

    return -units.to_si(UnitSystem::measure::rate, water_rate);
}

/*
  The deck tested here has a UDQ DEFINE statement which sorts the wells after
  oil production rate, and then subsequently closes the well with lowest OPR
  with a ACTIONX keyword.
*/

BOOST_AUTO_TEST_CASE(UDQ_SORTA_EXAMPLE) {
#include "actionx2.include"

    test_data td( actionx );
    msim sim(td.state);
    {
        test_work_area_type * work_area = test_work_area_alloc("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr_low);

        sim.run(td.schedule, io);
        {
            const auto& w1 = td.schedule.getWell("P1");
            const auto& w4 = td.schedule.getWell("P4");
            std::size_t last_step = td.schedule.size() - 1;

            BOOST_CHECK_EQUAL(w1->getStatus(1), WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w4->getStatus(1), WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w1->getStatus(last_step), WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w4->getStatus(last_step), WellCommon::StatusEnum::SHUT );
        }

        test_work_area_free(work_area);
    }
}


BOOST_AUTO_TEST_CASE(WELL_CLOSE_EXAMPLE) {
#include "actionx1.include"

    test_data td( actionx1 );
    msim sim(td.state);
    {
        test_work_area_type * work_area = test_work_area_alloc("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr);

        sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

        {
            const auto& w1 = td.schedule.getWell("P1");
            const auto& w2 = td.schedule.getWell("P2");
            const auto& w3 = td.schedule.getWell("P3");
            const auto& w4 = td.schedule.getWell("P4");

            BOOST_CHECK_EQUAL(w1->getStatus(15),  WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w2->getStatus(15),  WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w3->getStatus(15),  WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w4->getStatus(15),  WellCommon::StatusEnum::OPEN );
        }


        sim.run(td.schedule, io);
        {
            const auto& w1 = td.schedule.getWell("P1");
            const auto& w2 = td.schedule.getWell("P2");
            const auto& w3 = td.schedule.getWell("P3");
            const auto& w4 = td.schedule.getWell("P4");

            BOOST_CHECK_EQUAL(w1->getStatus(15),  WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w3->getStatus(15),  WellCommon::StatusEnum::OPEN );

            BOOST_CHECK_EQUAL(w2->getStatus(5),  WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w2->getStatus(6),  WellCommon::StatusEnum::SHUT );

            BOOST_CHECK_EQUAL(w4->getStatus(10),  WellCommon::StatusEnum::OPEN );
            BOOST_CHECK_EQUAL(w4->getStatus(11),  WellCommon::StatusEnum::SHUT );
        }



        test_work_area_free(work_area);
    }
}

BOOST_AUTO_TEST_CASE(UDQ_ASSIGN) {
#include "actionx1.include"

    test_data td( actionx1 );
    msim sim(td.state);
    {
        test_work_area_type * work_area = test_work_area_alloc("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr);

        sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

        sim.run(td.schedule, io);

        const auto& base_name = td.state.getIOConfig().getBaseName();

        ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case( base_name.c_str(), ":");
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUBHP:P1") );
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUBHP:P2") );
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUOPR:P3") );
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUOPR:P4") );

        BOOST_CHECK_EQUAL( ecl_sum_get_unit(ecl_sum, "WUBHP:P1"), "BARSA");
        BOOST_CHECK_EQUAL( ecl_sum_get_unit(ecl_sum, "WUOPR:P1"), "SM3/DAY");

        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P1"), 11);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P2"), 12);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P3"), 13);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P4"), 14);

        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPR:P1"), 20);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPR:P2"), 20);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPR:P3"), 20);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPR:P4"), 20);
        ecl_sum_free( ecl_sum );

        test_work_area_free(work_area);
    }
}

BOOST_AUTO_TEST_CASE(UDQ_WUWCT) {
#include "actionx1.include"

    test_data td( actionx1 );
    msim sim(td.state);
    {
        test_work_area_type * work_area = test_work_area_alloc("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr_low);

        sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

        sim.run(td.schedule, io);

        const auto& base_name = td.state.getIOConfig().getBaseName();
        ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case( base_name.c_str(), ":");
        for (const auto& well : {"P1", "P2", "P3", "P4"}) {
            std::string wwct_key  = std::string("WWCT:") + well;
            std::string wuwct_key = std::string("WUWCT:") + well;

            BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, wwct_key.c_str()));
            BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, wuwct_key.c_str()));

            for (int step = 0; step < ecl_sum_get_data_length(ecl_sum); step++)
                BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, step, wwct_key.c_str()),
                                   ecl_sum_get_general_var(ecl_sum, step, wuwct_key.c_str()));

        }

        test_work_area_free(work_area);
    }
}
