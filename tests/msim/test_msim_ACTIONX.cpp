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

#define BOOST_TEST_MODULE ACTIONX_SIM

#include <boost/test/unit_test.hpp>

#include <opm/msim/msim.hpp>

#include <opm/io/eclipse/ESmry.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionAST.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionContext.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <tests/WorkArea.hpp>

#include <cstddef>
#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Opm;

namespace {

double prod_opr(const EclipseState&  es,
                const Schedule& /* sched */,
                const SummaryState&,
                const data::Solution& /* sol */,
                const std::size_t /* report_step */,
                const double /* seconds_elapsed */)
{
    const double oil_rate = 1.0;

    return -es.getUnits().to_si(UnitSystem::measure::rate, oil_rate);
}

double prod_gpr(const EclipseState&  es,
                const Schedule& /* sched */,
                const SummaryState&,
                const data::Solution& /* sol */,
                const std::size_t /* report_step */,
                const double /* seconds_elapsed */)
{
    const double gas_rate = 20.0;

    return -es.getUnits().to_si(UnitSystem::measure::rate, gas_rate);
}

double prod_opr_low(const EclipseState&  es,
                    const Schedule& /* sched */,
                    const SummaryState&,
                    const data::Solution& /* sol */,
                    const std::size_t /* report_step */,
                    const double /* seconds_elapsed */)
{
    const double oil_rate = 0.5;

    return -es.getUnits().to_si(UnitSystem::measure::rate, oil_rate);
}

double prod_wpr_P1(const EclipseState&  es,
                   const Schedule& /* sched */,
                   const SummaryState&,
                   const data::Solution& /* sol */,
                   const std::size_t /* report_step */,
                   const double /* seconds_elapsed */)
{
    const double water_rate = 0.0;
    return -es.getUnits().to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P2(const EclipseState&  es,
                   const Schedule& /* sched */,
                   const SummaryState&,
                   const data::Solution& /* sol */,
                   const std::size_t report_step,
                   const double /* seconds_elapsed */)
{
    double water_rate = 0.0;
    if (report_step > 5) {
        water_rate = 2.0;  // => WWCT = WWPR / (WOPR + WWPR) = 2/3
    }

    return -es.getUnits().to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P3(const EclipseState&  es,
                   const Schedule& /* sched */,
                   const SummaryState&,
                   const data::Solution& /* sol */,
                   const std::size_t /* report_step */,
                   const double /* seconds_elapsed */)
{
    const double water_rate = 0.0;

    return -es.getUnits().to_si(UnitSystem::measure::rate, water_rate);
}

double prod_wpr_P4(const EclipseState&  es,
                   const Schedule& /* sched */,
                   const SummaryState&,
                   const data::Solution& /* sol */,
                   const std::size_t report_step,
                   const double /* seconds_elapsed */)
{
    double water_rate = 0.0;
    if (report_step > 10) {
        water_rate = 2.0;
    }

    return -es.getUnits().to_si(UnitSystem::measure::rate, water_rate);
}

double inj_wir_INJ(const EclipseState&,
                   const Schedule& sched,
                   const SummaryState& st,
                   const data::Solution& /* sol */,
                   const std::size_t report_step,
                   const double /* seconds_elapsed */)
{
    if (! st.has("FUINJ")) {
        return -99.0;
    }

    return sched.getWell("INJ", report_step)
        .injectionControls(st).surface_rate;
}

bool ecl_sum_has_general_var(const EclIO::ESmry& smry, const std::string& var)
{
    return smry.hasKey(var);
}

float ecl_sum_get_general_var(const EclIO::ESmry& smry, const int timeIdx, const std::string& var)
{
    return smry.get(var)[timeIdx];
}

int ecl_sum_get_data_length(const EclIO::ESmry& smry)
{
    return static_cast<int>(smry.get("TIME").size());
}

int ecl_sum_get_last_report_step(const EclIO::ESmry& smry)
{
    return static_cast<int>(smry.get_at_rstep("TIME").size());
}

int ecl_sum_iget_report_end(const EclIO::ESmry& smry, const int reportStep)
{
    return smry.timestepIdxAtReportstepStart(reportStep + 1) - 1;
}

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Common_Features)

namespace {

    ParseContext ignoreUDQCantEval()
    {
        auto ctx = ParseContext{};
        ctx.update(ParseContext::UDQ_DEFINE_CANNOT_EVAL,
                   InputErrorAction::IGNORE);

        return ctx;
    }

    struct test_data
    {
        EclipseState state;
        Schedule schedule;
        SummaryConfig summary_config;

        explicit test_data(const Deck& deck)
            : state    { deck }
            , schedule { deck, state, ignoreUDQCantEval(), ErrorGuard{}, msim::python }
            , summary_config { deck, schedule, state.fieldProps(), state.aquifer() }
        {
            this->state.getIOConfig().setBaseName("MSIM");
        }

        explicit test_data(const std::string& deck_string)
            : test_data { Parser{}.parseString(deck_string) }
        {}
    };

} // Anonymous namespace

// The deck tested here has a UDQ DEFINE statement which sorts wells by oil
// production rate.  We close the smallest OPR well in an ACTIONX block.

BOOST_AUTO_TEST_CASE(UDQ_SORTA_EXAMPLE) {
#include "actionx2.include"

    test_data td(actionx);
    msim sim(td.state, td.schedule);
    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr_low);

        sim.run(io, false);
        {
            const auto& w1 = sim.schedule.getWell("P1", 1);
            const auto& w4 = sim.schedule.getWell("P4", 1);
            BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );
            BOOST_CHECK(w4.getStatus() == Well::Status::OPEN );
        }
        {
            const auto& w1 = sim.schedule.getWellatEnd("P1");
            const auto& w4 = sim.schedule.getWellatEnd("P4");
            BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );
            BOOST_CHECK(w4.getStatus() == Well::Status::SHUT );
        }
    }
}

BOOST_AUTO_TEST_CASE(WELL_CLOSE_EXAMPLE) {
#include "actionx1.include"

    test_data td(actionx1);
    msim sim(td.state, td.schedule);
    {
        WorkArea work_area("test_msim");
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
            const auto& w1 = sim.schedule.getWell("P1", 15);
            const auto& w2 = sim.schedule.getWell("P2", 15);
            const auto& w3 = sim.schedule.getWell("P3", 15);
            const auto& w4 = sim.schedule.getWell("P4", 15);

            BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );
            BOOST_CHECK(w2.getStatus() == Well::Status::OPEN );
            BOOST_CHECK(w3.getStatus() == Well::Status::OPEN );
            BOOST_CHECK(w4.getStatus() == Well::Status::OPEN );
        }


        sim.run(io, false);
        {
            const auto& w1 = sim.schedule.getWell("P1", 15);
            const auto& w3 = sim.schedule.getWell("P3", 15);
            BOOST_CHECK(w1.getStatus() ==  Well::Status::OPEN );
            BOOST_CHECK(w3.getStatus() ==  Well::Status::OPEN );
        }
        {
            const auto& w2_6 = sim.schedule.getWell("P2", 6);
            BOOST_CHECK(w2_6.getStatus() == Well::Status::SHUT );
        }
        {
            const auto& w4_11 = sim.schedule.getWell("P4", 11);
            BOOST_CHECK(w4_11.getStatus() == Well::Status::SHUT );
        }
    }
}

BOOST_AUTO_TEST_CASE(UDQ_ASSIGN) {
#include "actionx1.include"

    test_data td(actionx1);
    msim sim(td.state, td.schedule);
    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr);

        sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

        sim.run(io, false);

        const auto& base_name = td.state.getIOConfig().getBaseName();

        const EclIO::ESmry ecl_sum(base_name + ".SMSPEC");
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUBHP:P1") );
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUBHP:P2") );
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUOPRL:P3") );
        BOOST_CHECK( ecl_sum_has_general_var(ecl_sum, "WUOPRL:P4") );

#if 0
        BOOST_CHECK_EQUAL( ecl_sum_get_unit(ecl_sum, "WUBHP:P1"), "BARSA");
        BOOST_CHECK_EQUAL( ecl_sum_get_unit(ecl_sum, "WUOPRL:P1"), "SM3/DAY");
#endif

        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P1"), 11);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P2"), 12);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P3"), 13);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUBHP:P4"), 14);

        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPRL:P1"), 20);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPRL:P2"), 20);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPRL:P3"), 20);
        BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, 1, "WUOPRL:P4"), 20);
    }
}

BOOST_AUTO_TEST_CASE(UDQ_WUWCT) {
#include "actionx1.include"

    test_data td(actionx1);
    msim sim(td.state, td.schedule);
    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr_low);

        sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

        sim.run(io, false);

        const auto& base_name = td.state.getIOConfig().getBaseName();
        const EclIO::ESmry ecl_sum(base_name + ".SMSPEC");

        for (int step = 0; step < ecl_sum_get_data_length(ecl_sum); step++) {
            double wopr_sum = 0;
            for (const auto& well : {"P1", "P2", "P3", "P4"}) {
                std::string wwct_key  = std::string("WWCT:") + well;
                std::string wuwct_key = std::string("WUWCT:") + well;
                std::string wopr_key  = std::string("WOPR:") + well;

                if (ecl_sum_get_general_var(ecl_sum, step, wwct_key) != 0)
                    BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, step, wwct_key),
                                       ecl_sum_get_general_var(ecl_sum, step, wuwct_key));

                wopr_sum += ecl_sum_get_general_var(ecl_sum, step , wopr_key);
            }
            BOOST_CHECK_EQUAL( ecl_sum_get_general_var(ecl_sum, step, "FOPR"),
                               ecl_sum_get_general_var(ecl_sum, step, "FUOPR"));
            BOOST_CHECK_EQUAL( wopr_sum, ecl_sum_get_general_var(ecl_sum, step, "FOPR"));
        }

        {
            const auto& fu_time = ecl_sum.get_at_rstep("FU_TIME");
            BOOST_CHECK_CLOSE(fu_time[7 - 1], 212, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[8 - 1], 243, 1e-5);
            // UPDATE OFF
            BOOST_CHECK_CLOSE(fu_time[9 - 1] , 243, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[10 - 1], 243, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[11 - 1], 243, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[12 - 1], 243, 1e-5);
            // UPDATE NEXT
            BOOST_CHECK_CLOSE(fu_time[13 - 1], 372, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[14 - 1], 372, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[15 - 1], 372, 1e-5);
            // UPDATE ON
            BOOST_CHECK_CLOSE(fu_time[16 - 1], 487, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[17 - 1], 517, 1e-5);
            BOOST_CHECK_CLOSE(fu_time[18 - 1], 548, 1e-5);
        }
    }
}

BOOST_AUTO_TEST_CASE(UDQ_IN_ACTIONX) {
#include "udq_in_actionx.include"

    test_data td(actionx1);
    msim sim(td.state, td.schedule);
    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        sim.well_rate("P4", data::Rates::opt::oil, prod_opr);

        sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

        sim.well_rate("P1", data::Rates::opt::gas, prod_gpr);
        sim.well_rate("P2", data::Rates::opt::gas, prod_gpr);
        sim.well_rate("P3", data::Rates::opt::gas, prod_gpr);
        sim.well_rate("P4", data::Rates::opt::gas, prod_gpr);

        {
            const auto& w1 = sim.schedule.getWell("P1", 15);
            BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );

            const auto& udq1 = sim.schedule.getUDQConfig(15);
            BOOST_CHECK(!udq1.has_keyword("FUNEW"));

            const auto& udq2 = sim.schedule.getUDQConfig(25);
            BOOST_CHECK(udq2.has_keyword("FUPROD"));
        }


        sim.run(io, false);
        {
            const auto& w1 = sim.schedule.getWell("P1", 15);
            BOOST_CHECK(w1.getStatus() ==  Well::Status::OPEN );

            const auto& udq1 = sim.schedule.getUDQConfig(15);
            BOOST_CHECK(udq1.has_keyword("FUNEW"));

            const auto& udq2 = sim.schedule.getUDQConfig(25);
            BOOST_CHECK(udq2.has_keyword("FUPROD"));
            BOOST_CHECK(udq2.has_keyword("FUNEW"));
        }

        const auto& base_name = td.state.getIOConfig().getBaseName();
        const EclIO::ESmry ecl_sum(base_name + ".SMSPEC");
        BOOST_CHECK( !ecl_sum.hasKey("FLPR") );
        BOOST_CHECK( ecl_sum.hasKey("FUGPR") );

        BOOST_CHECK( !ecl_sum.hasKey("FGLIR") );
        BOOST_CHECK( ecl_sum.hasKey("FUGPR") );
    }
}

BOOST_AUTO_TEST_CASE(UDA) {
#include "uda.include"

    test_data td(uda_deck);
    msim sim(td.state, td.schedule);
    auto eps_lim = sim.uda_val().epsilonLimit();

    EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

    sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
    sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
    sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
    sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);
    sim.well_rate("INJ", data::Rates::opt::wat, inj_wir_INJ);
    {
        WorkArea work_area("uda_sim");

        sim.run(io, true);

        const auto& base_name = td.state.getIOConfig().getBaseName();
        const EclIO::ESmry ecl_sum(base_name + ".SMSPEC");

        // Should only get at report steps
        const auto last_report = ecl_sum_get_last_report_step(ecl_sum);
        for (int report_step = 2; report_step < last_report; report_step++) {
            double wwpr_sum = 0;
            {
                int prev_tstep = ecl_sum_iget_report_end(ecl_sum, report_step - 1);
                for (const auto& well : {"P1", "P2", "P3", "P4"}) {
                    std::string wwpr_key  = std::string("WWPR:") + well;
                    wwpr_sum += ecl_sum_get_general_var(ecl_sum, prev_tstep, wwpr_key);
                }
                wwpr_sum = 0.90 * wwpr_sum;
                wwpr_sum = std::max(eps_lim, wwpr_sum);
            }
            BOOST_CHECK_CLOSE( wwpr_sum, ecl_sum_get_general_var(ecl_sum, ecl_sum_iget_report_end(ecl_sum, report_step), "WWIR:INJ"), 1e-3);
        }
    }
}

BOOST_AUTO_TEST_CASE(COMPDAT) {
#include "compdat.include"

    test_data td(compdat_deck);
    msim sim(td.state, td.schedule);
    EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

    sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
    sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
    sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
    sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);
    sim.well_rate("INJ", data::Rates::opt::wat, inj_wir_INJ);
    {
        WorkArea work_area("compdat_sim");

        BOOST_CHECK_NO_THROW(sim.run(io, true));
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Common_Features

// ===========================================================================

#ifdef EMBEDDED_PYTHON

BOOST_AUTO_TEST_SUITE(Embedded_Python)

namespace {

    struct test_data
    {
        EclipseState state;
        Schedule schedule;
        SummaryConfig summary_config;

        explicit test_data(const Deck& deck)
            : state          { deck }
            , schedule       { deck, state, msim::python }
            , summary_config { deck, schedule, state.fieldProps(), state.aquifer() }
        {
            this->state.getIOConfig().setBaseName("MSIM");
        }

        explicit test_data(const std::string& file_name)
            : test_data { Parser{}.parseFile(file_name) }
        {}
    };

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MSIM_EXIT_TEST_PYACTION) {
    test_data td("msim/MSIM_PYACTION_EXIT.DATA");

    {
        WorkArea work_area("test_msim");

        msim msim(td.state, td.schedule);
        EclipseIO io(td.state, td.state.getInputGrid(), td.schedule, td.summary_config);

        msim.well_rate("P1", data::Rates::opt::oil, prod_opr);
        msim.well_rate("P2", data::Rates::opt::oil, prod_opr);
        msim.well_rate("P3", data::Rates::opt::oil, prod_opr);
        msim.well_rate("P4", data::Rates::opt::oil, prod_opr);

        msim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
        msim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
        msim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
        msim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);
        msim.run(io, false);

        auto exit_status = msim.schedule.exitStatus();
        BOOST_CHECK( exit_status.has_value() );
        BOOST_CHECK_EQUAL(exit_status.value(), 99);
    }
}

BOOST_AUTO_TEST_CASE(MSIM_PYACTION_INSERT_KEYWORD) {
    test_data td("msim/MSIM_PYACTION_INSERT_KEYWORD.DATA");
    msim sim(td.state, td.schedule);

    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), sim.schedule, td.summary_config);
        {
            const auto& w1 = sim.schedule.getWell("P1", 1);
            BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );
        }

        sim.run(io, false);

        {
            const auto& w1_2 = sim.schedule.getWell("P1", 2); // Closed well P1 at report step 2
            const auto& w1_3 = sim.schedule.getWell("P1", 3); // And scheduled for reopening at the report step after that
            BOOST_CHECK(w1_2.getStatus() ==  Well::Status::SHUT);
            BOOST_CHECK(w1_3.getStatus() ==  Well::Status::OPEN);
        }
    }
}

BOOST_AUTO_TEST_CASE(PYTHON_WELL_CLOSE_EXAMPLE) {
    for (const auto& deck : {
            "msim/MSIM_PYACTION.DATA",
            "msim/MSIM_PYACTION_NO_RUN_FUNCTION.DATA",
        })
    {
        test_data td(deck);
        msim sim(td.state, td.schedule);

        {
            WorkArea work_area("test_msim");
            EclipseIO io(td.state, td.state.getInputGrid(), sim.schedule, td.summary_config);

            sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
            sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
            sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
            sim.well_rate("P4", data::Rates::opt::oil, prod_opr);

            sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
            sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
            sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
            sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

            {
                const auto& w1 = sim.schedule.getWell("P1", 15);
                const auto& w2 = sim.schedule.getWell("P2", 15);
                const auto& w3 = sim.schedule.getWell("P3", 15);
                const auto& w4 = sim.schedule.getWell("P4", 15);

                BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );
                BOOST_CHECK(w2.getStatus() == Well::Status::OPEN );
                BOOST_CHECK(w3.getStatus() == Well::Status::OPEN );
                BOOST_CHECK(w4.getStatus() == Well::Status::OPEN );
            }


            sim.run(io, false);
            {
                const auto& w1 = sim.schedule.getWell("P1", 15);
                const auto& w3 = sim.schedule.getWell("P3", 15);
                BOOST_CHECK(w1.getStatus() ==  Well::Status::OPEN );
                BOOST_CHECK(w3.getStatus() ==  Well::Status::OPEN );
            }
            {
                const auto& w2_6 = sim.schedule.getWell("P2", 6);
                BOOST_CHECK(w2_6.getStatus() == Well::Status::SHUT );
            }
            {
                const auto& w4_11 = sim.schedule.getWell("P4", 11);
                BOOST_CHECK(w4_11.getStatus() == Well::Status::SHUT );
            }
        }
        BOOST_CHECK_EQUAL( sim.st.get("run_count"), 13);
    }
}

BOOST_AUTO_TEST_CASE(PYTHON_CHANGING_SCHEDULE) {
    // Both input decks test the same actions.  Deck1 without an
    // actionx_callback function, deck2 with an actionx_callback function

    for (const auto& deck : {
            "msim/MSIM_PYACTION_CHANGING_SCHEDULE.DATA",
            "msim/MSIM_PYACTION_CHANGING_SCHEDULE_ACTIONX_CALLBACK.DATA",
        })
    {
        test_data td(deck);
        msim sim(td.state, td.schedule);

        {
            WorkArea work_area("test_msim");
            EclipseIO io(td.state, td.state.getInputGrid(), sim.schedule, td.summary_config);

            sim.well_rate("P1", data::Rates::opt::oil, prod_opr);
            sim.well_rate("P2", data::Rates::opt::oil, prod_opr);
            sim.well_rate("P3", data::Rates::opt::oil, prod_opr);
            sim.well_rate("P4", data::Rates::opt::oil, prod_opr);

            sim.well_rate("P1", data::Rates::opt::wat, prod_wpr_P1);
            sim.well_rate("P2", data::Rates::opt::wat, prod_wpr_P2);
            sim.well_rate("P3", data::Rates::opt::wat, prod_wpr_P3);
            sim.well_rate("P4", data::Rates::opt::wat, prod_wpr_P4);

            {
                const auto& w1 = sim.schedule.getWell("P1", 0);
                const auto& w2 = sim.schedule.getWell("P2", 0);
                const auto& w3 = sim.schedule.getWell("P3", 0);
                const auto& w4 = sim.schedule.getWell("P4", 0);

                BOOST_CHECK(w1.getStatus() == Well::Status::OPEN );
                BOOST_CHECK(w2.getStatus() == Well::Status::OPEN );
                BOOST_CHECK(w3.getStatus() == Well::Status::OPEN );
                BOOST_CHECK(w4.getStatus() == Well::Status::OPEN );
            }


            sim.run(io, false);

            {
                const auto& w1_at_reportstep1 = sim.schedule.getWell("P1", 1);
                const auto& w2_at_reportstep2 = sim.schedule.getWell("P2", 2);
                const auto& w3_at_reportstep3 = sim.schedule.getWell("P3", 3);
                const auto& w4_at_reportstep4 = sim.schedule.getWell("P4", 4);
                BOOST_CHECK(w1_at_reportstep1.getStatus() ==  Well::Status::SHUT );
                BOOST_CHECK(w2_at_reportstep2.getStatus() ==  Well::Status::SHUT );
                BOOST_CHECK(w3_at_reportstep3.getStatus() ==  Well::Status::SHUT );
                BOOST_CHECK(w4_at_reportstep4.getStatus() ==  Well::Status::SHUT );
            }
            {
                const auto& w1_at_reportstep4 = sim.schedule.getWell("P1", 4);
                const auto& w1_at_reportstep5 = sim.schedule.getWell("P1", 5);
                const auto& w1_at_reportstep6 = sim.schedule.getWell("P1", 6);
                const auto& w2_at_reportstep6 = sim.schedule.getWell("P2", 6);
                const auto& w3_at_reportstep6 = sim.schedule.getWell("P3", 6);
                const auto& w4_at_reportstep6 = sim.schedule.getWell("P4", 6);
                BOOST_CHECK(w1_at_reportstep4.getStatus() ==  Well::Status::SHUT ); // Opened P1 again at step 5
                BOOST_CHECK(w1_at_reportstep5.getStatus() ==  Well::Status::OPEN ); // Opened P1 again at step 5
                BOOST_CHECK(w1_at_reportstep6.getStatus() ==  Well::Status::OPEN ); // Opened P1 again at step 5
                BOOST_CHECK(w2_at_reportstep6.getStatus() ==  Well::Status::SHUT );
                BOOST_CHECK(w3_at_reportstep6.getStatus() ==  Well::Status::SHUT );
                BOOST_CHECK(w4_at_reportstep6.getStatus() ==  Well::Status::SHUT );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(MSIM_PYACTION_INSERT_INVALID_KEYWORD) {
    test_data td("msim/MSIM_PYACTION_INSERT_INVALID_KEYWORD.DATA");
    msim sim(td.state, td.schedule);

    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), sim.schedule, td.summary_config);

        BOOST_CHECK_THROW(sim.run(io, false), std::exception);
    }
}

BOOST_AUTO_TEST_CASE(PYTHON_OPEN_WELL_AT_INVALID_REPORT_STEP) {
    for (const auto& deck : {
            "msim/MSIM_PYACTION_OPEN_WELL_AT_PAST_REPORT_STEP.DATA",
            "msim/MSIM_PYACTION_OPEN_WELL_AT_TOO_LATE_REPORT_STEP.DATA",
        })
    {
        test_data td(deck);
        msim sim(td.state, td.schedule);

        {
            WorkArea work_area("test_msim");
            EclipseIO io(td.state, td.state.getInputGrid(), sim.schedule, td.summary_config);
            BOOST_CHECK_THROW(sim.run(io, false), std::exception);
        }
    }
}

BOOST_AUTO_TEST_CASE(MSIM_PYACTION_RETRIEVE_INFO) {
    test_data td("msim/MSIM_PYACTION_RETRIEVE_INFO.DATA");
    msim sim(td.state, td.schedule);

    {
        WorkArea work_area("test_msim");
        EclipseIO io(td.state, td.state.getInputGrid(), sim.schedule, td.summary_config);

        BOOST_CHECK_NO_THROW(sim.run(io, false));
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Embedded_Python

#endif  // EMBEDDED_PYTHON
