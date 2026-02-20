/*
  Copyright 2014 Statoil IT
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

#include "opm/output/eclipse/VectorItems/intehead.hpp"
#define BOOST_TEST_MODULE HeaderLGR

#include <boost/test/unit_test.hpp>

#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Groups.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/AggregateAquiferData.hpp>
#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>


#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>
#include <opm/input/eclipse/EclipseState/Grid/RegionSetMatcher.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Eqldims.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/input/eclipse/Utility/Functional.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <string>

#include <vector>

#include <tests/WorkArea.hpp>


using namespace Opm;

namespace {

template <typename T>
void check_vec_close(const T& actual, const T& expected, double tol = 1e-5)
{
    BOOST_REQUIRE_EQUAL(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i)
        BOOST_CHECK_CLOSE(actual[i], expected[i], tol);
}

data::GroupAndNetworkValues mkGroups()
{
    return {};
}

data::Wells mkWellsLGR_Global_Complex()
{
    // This function creates a Wells object with three wells, each having two connections matching the one in the LGR_BASESIM2WELLS.DATA
    data::Rates r1, r1c1, r1c2, r1c3, r2, r3;
    r1.set( data::Rates::opt::wat, 11 );
    r1.set( data::Rates::opt::oil, 13 );
    r1.set( data::Rates::opt::gas, 5 );
    r1c1.set( data::Rates::opt::wat, 5 );
    r1c1.set( data::Rates::opt::oil, 5 );
    r1c1.set( data::Rates::opt::gas, 3 );
    r1c2.set( data::Rates::opt::wat, 5 );
    r1c2.set( data::Rates::opt::oil, 7 );
    r1c2.set( data::Rates::opt::gas, 1 );
    r1c3.set( data::Rates::opt::wat, 1 );
    r1c3.set( data::Rates::opt::oil, 1 );
    r1c3.set( data::Rates::opt::gas, 1 );

    r2.set( data::Rates::opt::wat, 5 );
    r2.set( data::Rates::opt::oil, 7.2 );
    r2.set( data::Rates::opt::gas, 3 );

    r3.set( data::Rates::opt::wat, 10 );
    r3.set( data::Rates::opt::oil, 12 );
    r3.set( data::Rates::opt::gas, 4 );

    data::Well w1, w2, w3;
    w1.rates = r1;
    w1.thp = 1.0;
    w1.bhp = 1.23;
    w1.temperature = 3.45;
    w1.control = 1;

    /*
     *  the completion keys (active indices) and well names correspond to the
     *  input deck. All other entries in the well structures are arbitrary.
     */
    Opm::data::ConnectionFiltrate con_filtrate {0.1, 1, 3, 0.4, 1.e-9, 0.2, 0.05, 10.}; // values are not used in this test
    w1.connections.push_back(Opm::data::Connection{ 1, r1c1, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );
    w1.connections.push_back(Opm::data::Connection{ 4, r1c2, 31.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );
    w1.connections.push_back(Opm::data::Connection{ 7, r1c3, 32.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );


    w2.rates = r2;
    w2.thp = 1.0;
    w2.bhp = 1.23;
    w2.temperature = 3.45;
    w2.control = 1;
    w2.connections.push_back(Opm::data::Connection{ 6, r2, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 2, con_filtrate } );



    w3.rates = r3;
    w3.thp = 2.0;
    w3.bhp = 2.34;
    w3.temperature = 4.56;
    w3.control = 2;
    w3.connections.push_back(Opm::data::Connection{ 1, r3, 36.22, 123.4, 256.1, 0.55, 0.0125, 314.15, 3.456, 0.0, 2.46, 0, con_filtrate } );

    {
        data::Wells wellRates;

        wellRates["PROD1"] = w1;
        wellRates["PROD2"] = w2;
        wellRates["INJ"] = w3;

        return wellRates;
    }
}


data::Solution mkSolution(int numCells)
{
    using measure = UnitSystem::measure;

    auto sol = data::Solution {
        { "PRESSURE", data::CellData { measure::pressure,    {}, data::TargetType::RESTART_SOLUTION } },
        { "TEMP",     data::CellData { measure::temperature, {}, data::TargetType::RESTART_SOLUTION } },
        { "SWAT",     data::CellData { measure::identity,    {}, data::TargetType::RESTART_SOLUTION } },
        { "SGAS",     data::CellData { measure::identity,    {}, data::TargetType::RESTART_SOLUTION } },
    };

    sol.data<double>("PRESSURE").assign( numCells, 6.0 );
    sol.data<double>("TEMP").assign( numCells, 7.0 );
    sol.data<double>("SWAT").assign( numCells, 8.0 );
    sol.data<double>("SGAS").assign( numCells, 9.0 );

    fun::iota rsi( 300.0, 300.0 + numCells );
    fun::iota rvi( 400.0, 400.0 + numCells );

    sol.insert("RS", measure::identity,
               std::vector<double>{ rsi.begin(), rsi.end() },
               data::TargetType::RESTART_SOLUTION);
    sol.insert("RV", measure::identity,
               std::vector<double>{ rvi.begin(), rvi.end() },
               data::TargetType::RESTART_SOLUTION);

    return sol;
}

Opm::SummaryState sim_stateLGR(const Opm::Schedule& sched)
{
    auto state = Opm::SummaryState {
        TimeService::now(),
        sched.back().udq().params().undefinedValue()
    };

    for (const auto& well : sched.getWellsatEnd()) {
        for (const auto& connection : well.getConnections()) {
            state.update_conn_var(well.name(), "CPR", connection.global_index() + 1, 111);
            if (well.isInjector()) {
                state.update_conn_var(well.name(), "COIR", connection.global_index() + 1, 222);
                state.update_conn_var(well.name(), "CGIR", connection.global_index() + 1, 333);
                state.update_conn_var(well.name(), "CWIR", connection.global_index() + 1, 444);
                state.update_conn_var(well.name(), "CVIR", connection.global_index() + 1, 555);

                state.update_conn_var(well.name(), "COIT", connection.global_index() + 1, 222 * 2.0);
                state.update_conn_var(well.name(), "CGIT", connection.global_index() + 1, 333 * 2.0);
                state.update_conn_var(well.name(), "CWIT", connection.global_index() + 1, 444 * 2.0);
                state.update_conn_var(well.name(), "CWIT", connection.global_index() + 1, 555 * 2.0);
            } else {
                state.update_conn_var(well.name(), "COPR", connection.global_index() + 1, 666);
                state.update_conn_var(well.name(), "CGPR", connection.global_index() + 1, 777);
                state.update_conn_var(well.name(), "CWPR", connection.global_index() + 1, 888);
                state.update_conn_var(well.name(), "CVPR", connection.global_index() + 1, 999);

                state.update_conn_var(well.name(), "CGOR", connection.global_index() + 1, 777.0 / 666.0);

                state.update_conn_var(well.name(), "COPT", connection.global_index() + 1, 555 * 2.0);
                state.update_conn_var(well.name(), "CGPT", connection.global_index() + 1, 666 * 2.0);
                state.update_conn_var(well.name(), "CWPT", connection.global_index() + 1, 777 * 2.0);
                state.update_conn_var(well.name(), "CVPT", connection.global_index() + 1, 999 * 2.0);
            }
        }
    }

    state.update_well_var("INJ", "WOPR", 1.0);
    state.update_well_var("INJ", "WWPR", 2.0);
    state.update_well_var("INJ", "WGPR", 3.0);
    state.update_well_var("INJ", "WVPR", 4.0);
    state.update_well_var("INJ", "WOPT", 10.0);
    state.update_well_var("INJ", "WWPT", 20.0);
    state.update_well_var("INJ", "WGPT", 30.0);
    state.update_well_var("INJ", "WVPT", 40.0);
    state.update_well_var("INJ", "WWIR", 0.0);
    state.update_well_var("INJ", "WGIR", 0.0);
    state.update_well_var("INJ", "WWIT", 0.0);
    state.update_well_var("INJ", "WGIT", 0.0);
    state.update_well_var("INJ", "WVIT", 0.0);
    state.update_well_var("INJ", "WWCT", 0.625);
    state.update_well_var("INJ", "WGOR", 234.5);
    state.update_well_var("INJ", "WBHP", 314.15);
    state.update_well_var("INJ", "WTHP", 123.45);
    state.update_well_var("INJ", "WOPTH", 345.6);
    state.update_well_var("INJ", "WWPTH", 456.7);
    state.update_well_var("INJ", "WGPTH", 567.8);
    state.update_well_var("INJ", "WWITH", 0.0);
    state.update_well_var("INJ", "WGITH", 0.0);
    state.update_well_var("INJ", "WGVIR", 0.0);
    state.update_well_var("INJ", "WWVIR", 0.0);

    state.update_well_var("PROD", "WOPR", 0.0);
    state.update_well_var("PROD", "WWPR", 0.0);
    state.update_well_var("PROD", "WGPR", 0.0);
    state.update_well_var("PROD", "WVPR", 0.0);
    state.update_well_var("PROD", "WOPT", 0.0);
    state.update_well_var("PROD", "WWPT", 0.0);
    state.update_well_var("PROD", "WGPT", 0.0);
    state.update_well_var("PROD", "WVPT", 0.0);
    state.update_well_var("PROD", "WWIR", 100.0);
    state.update_well_var("PROD", "WGIR", 200.0);
    state.update_well_var("PROD", "WWIT", 1000.0);
    state.update_well_var("PROD", "WGIT", 2000.0);
    state.update_well_var("PROD", "WVIT", 1234.5);
    state.update_well_var("PROD", "WWCT", 0.0);
    state.update_well_var("PROD", "WGOR", 0.0);
    state.update_well_var("PROD", "WBHP", 400.6);
    state.update_well_var("PROD", "WTHP", 234.5);
    state.update_well_var("PROD", "WOPTH", 0.0);
    state.update_well_var("PROD", "WWPTH", 0.0);
    state.update_well_var("PROD", "WGPTH", 0.0);
    state.update_well_var("PROD", "WWITH", 1515.0);
    state.update_well_var("PROD", "WGITH", 3030.0);
    state.update_well_var("PROD", "WGVIR", 1234.0);
    state.update_well_var("PROD", "WWVIR", 4321.0);



    state.update_group_var("G1", "GOPR" ,     110.0);
    state.update_group_var("G1", "GWPR" ,     120.0);
    state.update_group_var("G1", "GGPR" ,     130.0);
    state.update_group_var("G1", "GVPR" ,     140.0);
    state.update_group_var("G1", "GOPT" ,    1100.0);
    state.update_group_var("G1", "GWPT" ,    1200.0);
    state.update_group_var("G1", "GGPT" ,    1300.0);
    state.update_group_var("G1", "GVPT" ,    1400.0);
    state.update_group_var("G1", "GWIR" , -   256.0);
    state.update_group_var("G1", "GGIR" , - 65536.0);
    state.update_group_var("G1", "GWIT" ,   31415.9);
    state.update_group_var("G1", "GGIT" ,   27182.8);
    state.update_group_var("G1", "GVIT" ,   44556.6);
    state.update_group_var("G1", "GWCT" ,       0.625);
    state.update_group_var("G1", "GGOR" ,    1234.5);
    state.update_group_var("G1", "GGVIR",     123.45);
    state.update_group_var("G1", "GWVIR",    1234.56);
    state.update_group_var("G1", "GOPTH",    5678.90);
    state.update_group_var("G1", "GWPTH",    6789.01);
    state.update_group_var("G1", "GGPTH",    7890.12);
    state.update_group_var("G1", "GWITH",    8901.23);
    state.update_group_var("G1", "GGITH",    9012.34);

    state.update("FOPR" ,     1100.0);
    state.update("FWPR" ,     1200.0);
    state.update("FGPR" ,     1300.0);
    state.update("FVPR" ,     1400.0);
    state.update("FOPT" ,    11000.0);
    state.update("FWPT" ,    12000.0);
    state.update("FGPT" ,    13000.0);
    state.update("FVPT" ,    14000.0);
    state.update("FWIR" , -   2560.0);
    state.update("FGIR" , - 655360.0);
    state.update("FWIT" ,   314159.2);
    state.update("FGIT" ,   271828.1);
    state.update("FVIT" ,   445566.77);
    state.update("FWCT" ,        0.625);
    state.update("FGOR" ,     1234.5);
    state.update("FOPTH",    56789.01);
    state.update("FWPTH",    67890.12);
    state.update("FGPTH",    78901.23);
    state.update("FWITH",    89012.34);
    state.update("FGITH",    90123.45);
    state.update("FGVIR",     1234.56);
    state.update("FWVIR",    12345.67);

    return state;
}


struct Setup
{
    EclipseState es;
    const EclipseGrid& grid;
    Schedule schedule;
    SummaryConfig summary_config;

    explicit Setup(const char* path)
        : Setup { Parser{}.parseFile(path) }
    {}

    explicit Setup(const Deck& deck)
        : es             { deck }
        , grid           { es.getInputGrid() }
        , schedule       { deck, es, std::make_shared<Python>() }
        , summary_config { deck, schedule, es.fieldProps(), es.aquifer() }
    {
        es.getIOConfig().setEclCompatibleRST(false);
    }
};

void generate_opm_rst(Setup& base_setup,
                      const data::Wells& lwells,
                      const SummaryState& simState,
                      WorkArea& test_area)
{
    namespace OS = ::Opm::EclIO::OutputStream;
    auto& io_config = base_setup.es.getIOConfig();
    const auto& lgr_labels = base_setup.grid.get_all_lgr_labels();
    auto num_lgr_cells = lgr_labels.size();

    std::vector<int> lgr_grid_ids(num_lgr_cells + 1);
    std::iota(lgr_grid_ids.begin(), lgr_grid_ids.end(), 1);

    std::vector<std::size_t> num_cells(num_lgr_cells + 1);
    num_cells[0] = base_setup.grid.getNumActive();

    std::ranges::transform(lgr_labels, num_cells.begin() + 1,
                           [&base_setup](const std::string& lgr_tag)
                           { return base_setup.grid.getLGRCell(lgr_tag).getNumActive(); });

    std::vector<data::Solution> cells(num_lgr_cells + 1);
    std::ranges::transform(num_cells, cells.begin(),
                           [](int n) { return mkSolution(n); });

    auto groups = mkGroups();
    auto udqState = UDQState{1};
    auto aquiferData = std::optional<Opm::RestartIO::Helpers::AggregateAquiferData>{std::nullopt};
    Action::State action_state;
    WellTestState wtest_state;

    std::vector<RestartValue> restart_value;
    for (std::size_t i = 0; i < num_lgr_cells + 1; ++i) {
        restart_value.emplace_back(cells[i], lwells, groups, data::Aquifers{}, lgr_grid_ids[i]);
    }

    io_config.setEclCompatibleRST(false);

    std::ranges::transform(restart_value, restart_value.begin(),
                           [](RestartValue& rv)
                           {
                               rv.addExtra("EXTRA", UnitSystem::measure::pressure,
                                           std::vector<double>{10.0,1.0,2.0,3.0});
                               return rv;
                           });

    const auto outputDir = test_area.currentWorkingDirectory();
    const auto seqnum = 1;

    auto rstFile = OS::Restart{
        OS::ResultSet{ outputDir, "LGR-OPM" }, seqnum,
        OS::Formatted{ false }, OS::Unified{ true }
    };

    RestartIO::save(rstFile, seqnum, 100,
                    restart_value,
                    base_setup.es,
                    base_setup.grid,
                    base_setup.schedule,
                    action_state,
                    wtest_state,
                    simState,
                    udqState,
                    aquiferData,
                    true);
    }

void check_grid_content_existence(const std::string& gridname, EclIO::ERst& rst, bool contains_lgr_well = true)
{
        BOOST_CHECK_EQUAL(rst.hasArray("LGRHEADI", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("LGRHEADQ", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("LGRHEADD", 1, gridname), true);

        BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("LOGIHEAD", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("DOUBHEAD", 1, gridname), true);

        BOOST_CHECK_EQUAL(rst.hasArray("IGRP", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("SGRP", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("XGRP", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("ZGRP", 1, gridname), true);

        if (contains_lgr_well) {
            BOOST_CHECK_EQUAL(rst.hasArray("IWEL", 1, gridname), true);
            BOOST_CHECK_EQUAL(rst.hasArray("SWEL", 1, gridname), true);
            BOOST_CHECK_EQUAL(rst.hasArray("XWEL", 1, gridname), true);
            BOOST_CHECK_EQUAL(rst.hasArray("ZWEL", 1, gridname), true);
            BOOST_CHECK_EQUAL(rst.hasArray("LGWEL", 1,gridname), true);
            BOOST_CHECK_EQUAL(rst.hasArray("ICON", 1, gridname), true);
            BOOST_CHECK_EQUAL(rst.hasArray("SCON", 1, gridname), true);
        }

        BOOST_CHECK_EQUAL(rst.hasArray("PRESSURE", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("SWAT", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("SGAS", 1, gridname), true);
        BOOST_CHECK_EQUAL(rst.hasArray("RS", 1, gridname), true);
    }



} // Anonymous namespace


BOOST_AUTO_TEST_CASE(LGRHEADERS_3WELLS)
{

    auto wells = mkWellsLGR_Global_Complex();
    WorkArea test_area("test_Restart");
    test_area.copyIn("LGR_3WELLS.DATA");

    Setup base_setup("LGR_3WELLS.DATA");
    auto simState = sim_stateLGR(base_setup.schedule);

    generate_opm_rst(base_setup, wells, simState, test_area);
    const auto outputDir = test_area.currentWorkingDirectory();

    {
        const auto rstFile = ::Opm::EclIO::OutputStream::
            outputFileName({outputDir, "LGR-OPM"}, "UNRST");

        EclIO::ERst rst{ rstFile };

        {
            auto expected_lgrnames_global = base_setup.grid.get_all_lgr_labels();
            check_grid_content_existence("LGR1",rst);
            check_grid_content_existence("LGR2",rst);
            //INTEHEAD GLOBAL GRID
            {
                using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;

                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1, "LGR1"), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1,  "LGR2"), true);

                const auto& intehead_global = rst.getRestartData<int>("INTEHEAD", 1);
                const auto& intehead_lgr1  = rst.getRestartData<int>("INTEHEAD", 1, "LGR1");
                const auto& intehead_lgr2 = rst.getRestartData<int>("INTEHEAD", 1, "LGR2");
                BOOST_CHECK_EQUAL(intehead_global[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_global[Ix::NY], 1);
                BOOST_CHECK_EQUAL(intehead_global[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NZ], 1);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NWELLS], 3);       // Number of wells
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWELLS], 1);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWELLS], 1);


                // I believe that for LGR grids it inherits from the global grid
                BOOST_CHECK_EQUAL(intehead_global[Ix::NCWMAX], 3);       // Maximum number of completions per well
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NCWMAX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NCWMAX], 3);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NGRP], 1);         // Actual number of groups
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NGRP], 1);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NGRP], 1);

                // I believe that for LGR grids it inherits from the global grid
                BOOST_CHECK_EQUAL(intehead_global[Ix::NWGMAX], 3);       // Maximum number of wells in any well group
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWGMAX], 3);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWGMAX], 3);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NGMAXZ], 4);     // Maximum number of groups in field
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NGMAXZ], 2);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NGMAXZ], 2);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NWMAXZ],3);     // Maximum number of groups in field
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWMAXZ], 2);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWMAXZ], 2);

            }


        }
    }
}



BOOST_AUTO_TEST_CASE(LGRHEADERS_DIFFGROUP)
{
    auto wells = mkWellsLGR_Global_Complex();
    WorkArea test_area("test_Restart");
    std::string testfile = "LGR_DIFFGROUP.DATA";
    test_area.copyIn(testfile);

    Setup base_setup(testfile.c_str());
    auto simState = sim_stateLGR(base_setup.schedule);

    generate_opm_rst(base_setup, wells, simState, test_area);
    const auto outputDir = test_area.currentWorkingDirectory();

    {
        const auto rstFile = ::Opm::EclIO::OutputStream::
            outputFileName({outputDir, "LGR-OPM"}, "UNRST");

        EclIO::ERst rst{ rstFile };

        {
            auto expected_lgrnames_global = base_setup.grid.get_all_lgr_labels();
            check_grid_content_existence("LGR1", rst);
            check_grid_content_existence("LGR2", rst, false); // LGR2 has no wells
            //INTEHEAD GLOBAL GRID
            {
                using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;

                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1, "LGR1"), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1,  "LGR2"), true);

                const auto& intehead_global = rst.getRestartData<int>("INTEHEAD", 1);
                const auto& intehead_lgr1  = rst.getRestartData<int>("INTEHEAD", 1, "LGR1");
                const auto& intehead_lgr2 = rst.getRestartData<int>("INTEHEAD", 1, "LGR2");
                BOOST_CHECK_EQUAL(intehead_global[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_global[Ix::NY], 1);
                BOOST_CHECK_EQUAL(intehead_global[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NZ], 1);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NWELLS], 3);       // Number of wells
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWELLS], 2);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWELLS], 0);


                // I believe that for LGR grids it inherits from the global grid
                BOOST_CHECK_EQUAL(intehead_global[Ix::NCWMAX], 2);       // Maximum number of completions per well
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NCWMAX], 2);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NCWMAX], 2);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NGRP], 3);         // Actual number of groups
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NGRP], 1);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NGRP], 1);

                // I believe that for LGR grids it inherits from the global grid
                BOOST_CHECK_EQUAL(intehead_global[Ix::NWGMAX], 3);       // Maximum number of wells in any well group
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWGMAX], 3);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWGMAX], 3);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NGMAXZ], 4);     // Maximum number of groups in field
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NGMAXZ], 2);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NGMAXZ], 2);

                BOOST_CHECK_EQUAL(intehead_global[Ix::NWMAXZ],3);     // Maximum number of groups in field
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWMAXZ], 2);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWMAXZ], 2);



            }
        }
    }
}


BOOST_AUTO_TEST_CASE(LGRHEADERS_GROUPEX01)
{
    auto wells = mkWellsLGR_Global_Complex();
    WorkArea test_area("test_Restart");
    std::string testfile = "LGR_GROUP_EX01.DATA";
    test_area.copyIn(testfile);

    Setup base_setup(testfile.c_str());
    auto simState = sim_stateLGR(base_setup.schedule);

    generate_opm_rst(base_setup, wells, simState, test_area);
    const auto outputDir = test_area.currentWorkingDirectory();

    {
        const auto rstFile = ::Opm::EclIO::OutputStream::
            outputFileName({outputDir, "LGR-OPM"}, "UNRST");

        EclIO::ERst rst{ rstFile };

        {
            auto expected_lgrnames_global = base_setup.grid.get_all_lgr_labels();

            check_grid_content_existence("LGR1",rst);
            check_grid_content_existence("LGR2",rst, false); // LGR2 has no wells
            check_grid_content_existence("LGR3",rst, false);

            //INTEHEAD GLOBAL GRID
            {
                using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;

                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1, "LGR1"), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1,  "LGR2"), true);
                BOOST_CHECK_EQUAL(rst.hasArray("INTEHEAD", 1,  "LGR3"), true);


                const auto& intehead_global = rst.getRestartData<int>("INTEHEAD", 1);
                const auto& intehead_lgr1  = rst.getRestartData<int>("INTEHEAD", 1, "LGR1");
                const auto& intehead_lgr2 = rst.getRestartData<int>("INTEHEAD", 1, "LGR2");
                const auto& intehead_lgr3 = rst.getRestartData<int>("INTEHEAD", 1, "LGR3");

                BOOST_CHECK_EQUAL(intehead_global[Ix::NX], 5);
                BOOST_CHECK_EQUAL(intehead_global[Ix::NY], 1);
                BOOST_CHECK_EQUAL(intehead_global[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NZ], 1);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NX], 3);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NY], 3);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NZ], 1);


                BOOST_CHECK_EQUAL(intehead_global[Ix::NWELLS], 3);       // Number of wells
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWELLS], 2);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWELLS], 0);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NWELLS], 0);


                // I believe that for LGR grids it inherits from the global grid
                BOOST_CHECK_EQUAL(intehead_global[Ix::NCWMAX], 4);       // Maximum number of completions per well
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NCWMAX], 4);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NCWMAX], 4);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NCWMAX], 4);


                BOOST_CHECK_EQUAL(intehead_global[Ix::NGRP], 3);         // Actual number of groups
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NGRP], 1);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NGRP], 1);
                // BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NGRP], 1);

                // I believe that for LGR grids it inherits from the global grid
                BOOST_CHECK_EQUAL(intehead_global[Ix::NWGMAX], 4);       // Maximum number of wells in any well group
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWGMAX], 4);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWGMAX], 4);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NWGMAX], 4);


                BOOST_CHECK_EQUAL(intehead_global[Ix::NGMAXZ], 5);     // Maximum number of groups in field
                // following code should be enabled once we fixed AggregateGroupData LGR
                // BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NGMAXZ], 2);
                // BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NGMAXZ], 2);
                // BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NGMAXZ], 2);


                BOOST_CHECK_EQUAL(intehead_global[Ix::NWMAXZ],4);     // Maximum number of groups in field
                BOOST_CHECK_EQUAL(intehead_lgr1[Ix::NWMAXZ], 4);
                BOOST_CHECK_EQUAL(intehead_lgr2[Ix::NWMAXZ], 4);
                BOOST_CHECK_EQUAL(intehead_lgr3[Ix::NWMAXZ], 4);


            }
        }
    }
}
