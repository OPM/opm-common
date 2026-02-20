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

#define BOOST_TEST_MODULE Restart_File_IO

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

data::Wells mkWellsLGR_Global()
{
    // This function creates a Wells object with two wells, each having two connections matching the one in the LGR_BASESIM2WELLS.DATA
    data::Rates r1, r2, rc1, rc2, rc3;
    r1.set( data::Rates::opt::wat, 5.67 );
    r1.set( data::Rates::opt::oil, 6.78 );
    r1.set( data::Rates::opt::gas, 7.89 );

    r2.set( data::Rates::opt::wat, 8.90 );
    r2.set( data::Rates::opt::oil, 9.01 );
    r2.set( data::Rates::opt::gas, 10.12 );

    rc1.set( data::Rates::opt::wat, 20.41 );
    rc1.set( data::Rates::opt::oil, 21.19 );
    rc1.set( data::Rates::opt::gas, 22.41 );

    rc2.set( data::Rates::opt::wat, 23.19 );
    rc2.set( data::Rates::opt::oil, 24.41 );
    rc2.set( data::Rates::opt::gas, 25.19 );



    data::Well w1, w2;
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
    w1.connections.push_back( { 2, rc1, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );

    w2.rates = r2;
    w2.thp = 2.0;
    w2.bhp = 2.34;
    w2.temperature = 4.56;
    w2.control = 2;
    w2.connections.push_back( { 0, rc2, 36.22, 123.4, 256.1, 0.55, 0.0125, 314.15, 3.456, 0.0, 2.46, 2, con_filtrate } );

    {
        data::Wells wellRates;

        wellRates["PROD"] = w1;
        wellRates["INJ"] = w2;

        return wellRates;
    }
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
    w1.connections.push_back( { 1, r1c1, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );
    w1.connections.push_back( { 4, r1c2, 31.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );
    w1.connections.push_back( { 7, r1c3, 32.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );


    w2.rates = r2;
    w2.thp = 1.0;
    w2.bhp = 1.23;
    w2.temperature = 3.45;
    w2.control = 1;
    w2.connections.push_back( { 6, r2, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 2, con_filtrate } );



    w3.rates = r3;
    w3.thp = 2.0;
    w3.bhp = 2.34;
    w3.temperature = 4.56;
    w3.control = 2;
    w3.connections.push_back( { 1, r3, 36.22, 123.4, 256.1, 0.55, 0.0125, 314.15, 3.456, 0.0, 2.46, 0, con_filtrate } );

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

} // Anonymous namespace


BOOST_AUTO_TEST_CASE(ECL_LGRFORMATTED)
{
    namespace OS = ::Opm::EclIO::OutputStream;
    using measure = UnitSystem::measure;

    WorkArea test_area("test_Restart");
    test_area.copyIn("LGR_BASESIM2WELLS.DATA");

    Setup base_setup("LGR_BASESIM2WELLS.DATA");
    const auto& units = base_setup.es.getUnits();
    auto& io_config = base_setup.es.getIOConfig();
    {
        const auto& lgr_labels = base_setup.grid.get_all_lgr_labels();
        auto num_lgr_cells = lgr_labels.size();
        std::vector<int> lgr_grid_ids(num_lgr_cells+1);
        std::iota(lgr_grid_ids.begin(), lgr_grid_ids.end(), 1);
        std::vector <std::size_t> num_cells(num_lgr_cells+1);
        num_cells[0] = base_setup.grid.getNumActive();
        std::ranges::transform(lgr_labels, num_cells.begin() + 1 ,
                               [&base_setup](const std::string& lgr_tag)
                               { return base_setup.grid.getLGRCell(lgr_tag).getNumActive(); });

        std::vector<data::Solution> cells(num_lgr_cells+1);
        std::ranges::transform(num_cells, cells.begin(),
                               [](int n) { return mkSolution(n); });
        std::vector<data::Wells> wells;
        data::Wells lwells = mkWellsLGR_Global();

        auto groups = mkGroups();
        auto sumState = sim_stateLGR(base_setup.schedule);
        auto udqState = UDQState{1};
        auto aquiferData = std::optional<Opm::RestartIO::Helpers::AggregateAquiferData>{std::nullopt};
        Action::State action_state;
        WellTestState wtest_state;
        {

            std::vector<RestartValue> restart_value;

            for (std::size_t i = 0; i < num_lgr_cells + 1; ++i) {
                restart_value.emplace_back(cells[i], lwells, groups, data::Aquifers{}, lgr_grid_ids[i]);
            }


            io_config.setEclCompatibleRST( false );
            std::ranges::transform(restart_value, restart_value.begin(),
                                   [](RestartValue& rv)
                                   {
                                        rv.addExtra("EXTRA",
                                                    UnitSystem::measure::pressure,
                                                    std::vector<double>{10.0,1.0,2.0,3.0});
                                        return rv;
                                   });

            const auto outputDir = test_area.currentWorkingDirectory();

            {
                const auto seqnum = 1;
                auto rstFile = OS::Restart {
                    OS::ResultSet{ outputDir, "LGR-OPM" }, seqnum,
                    OS::Formatted{ false }, OS::Unified{ true }
                };

                RestartIO::save(rstFile, seqnum,
                                100,
                                restart_value,
                                base_setup.es,
                                base_setup.grid,
                                base_setup.schedule,
                                action_state,
                                wtest_state,
                                sumState,
                                udqState,
                                aquiferData,
                                true);
            }

            {
                const auto rstFile = ::Opm::EclIO::OutputStream::
                    outputFileName({outputDir, "LGR-OPM"}, "UNRST");

                EclIO::ERst rst{ rstFile };

                {
                    auto lgrnames_global = rst.getRestartData<std::string>("LGRNAMES", 1);
                    auto expected_lgrnames_global = base_setup.grid.get_all_lgr_labels();
                    for (const auto& lgrname : lgrnames_global) {
                        BOOST_CHECK_EQUAL(rst.hasLGR(lgrname, 1), true);
                    }
                    BOOST_CHECK_EQUAL(lgrnames_global.size(), lgr_labels.size());
                    BOOST_CHECK_EQUAL_COLLECTIONS(lgrnames_global.begin(), lgrnames_global.end(),
                                                  expected_lgrnames_global.begin(), expected_lgrnames_global.end());

                    for (const auto& lgrname : expected_lgrnames_global) {
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGRHEADI", 1, lgrname),
                                            "LGRHEADI array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGRHEADQ", 1, lgrname),
                                            "LGRHEADQ array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGRHEADD", 1, lgrname),
                                            "LGRHEADD array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("INTEHEAD", 1, lgrname),
                                            "INTEHEAD array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LOGIHEAD", 1, lgrname),
                                            "LOGIHEAD array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("DOUBHEAD", 1, lgrname),
                                            "DOUBHEAD array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("IGRP", 1, lgrname),
                                            "IGRP array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SGRP", 1, lgrname),
                                            "SGRP array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("XGRP", 1, lgrname),
                                            "XGRP array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("ZGRP", 1, lgrname),
                                            "ZGRP array must exist at report step 1 for local grid " << lgrname);


                        BOOST_CHECK_MESSAGE(rst.hasArray("IWEL", 1, lgrname),
                                            "IWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SWEL", 1, lgrname),
                                            "SWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("XWEL", 1, lgrname),
                                            "XWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("ZWEL", 1, lgrname),
                                            "ZWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGWEL", 1, lgrname),
                                            "LGWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("ICON", 1, lgrname),
                                            "ICON array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SCON", 1, lgrname),
                                            "SCON array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("PRESSURE", 1, lgrname),
                                            "PRESSURE array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SWAT", 1, lgrname),
                                            "SWAT array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SGAS", 1, lgrname),
                                            "SGAS array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("RS", 1, lgrname),
                                            "RS array must exist at report step 1 for local grid " << lgrname);
                    }

                }

                BOOST_CHECK_MESSAGE(rst.hasKey("SWAT"), "Restart file must have SWAT vector");
                BOOST_CHECK_MESSAGE(rst.hasKey("EXTRA"), "Restart file must have EXTRA vector");

                {
                    auto convert_unit = [&units](measure m, const double x) { return static_cast<double>(units.to_si(m, x)); };
                    auto convert_vector = [&](measure m, const std::vector<double>& input) {
                        std::vector<double> output;
                        output.resize(input.size());
                        std::ranges::transform(input, output.begin(),
                                               [&](double x) { return convert_unit(m, x); });
                        return output;
                    };

                    // checking dynamic data for LGR
                    {
                    //LGR1
                        auto pressure_lgr1 = rst.getRestartData<double>("PRESSURE", 1, "LGR1");
                        check_vec_close(convert_vector(UnitSystem::measure::pressure, pressure_lgr1), std::vector<double>(num_cells[1], 6.0));

                        auto temperature_lgr1 = rst.getRestartData<double>("TEMP", 1, "LGR1");
                        check_vec_close(convert_vector(UnitSystem::measure::temperature, temperature_lgr1), std::vector<double>(num_cells[1], 7.0));

                        auto swat_lgr1 = rst.getRestartData<double>("SWAT", 1, "LGR1");
                        check_vec_close(swat_lgr1, std::vector<double>(num_cells[1], 8.0));
                        auto sgas_lgr1 = rst.getRestartData<double>("SGAS", 1, "LGR1");
                        check_vec_close(sgas_lgr1, std::vector<double>(num_cells[1], 9.0));

                        auto rs_lgr1 = rst.getRestartData<double>("RS", 1, "LGR1");
                        fun::iota rs_expected(300.0, 300.0 + num_cells[1]);
                        std::vector<double> rs_expected_vec(rs_expected.begin(), rs_expected.end());
                        check_vec_close(rs_lgr1, rs_expected_vec);

                        auto rv_lgr1 = rst.getRestartData<double>("RV", 1, "LGR1");
                        fun::iota rv_expected(400.0, 400.0 + num_cells[1]);
                        std::vector<double> rv_expected_vec(rv_expected.begin(), rv_expected.end());
                        check_vec_close(rv_lgr1, rv_expected_vec);
                    }
                    {
                        //LGR2
                        auto pressure_lgr2 = rst.getRestartData<double>("PRESSURE", 1, "LGR2");
                        check_vec_close(convert_vector(UnitSystem::measure::pressure, pressure_lgr2), std::vector<double>(num_cells[1], 6.0));

                        auto temperature_lgr2 = rst.getRestartData<double>("TEMP", 1, "LGR2");
                        check_vec_close(convert_vector(UnitSystem::measure::temperature, temperature_lgr2), std::vector<double>(num_cells[1], 7.0));

                        auto swat_lgr2 = rst.getRestartData<double>("SWAT", 1, "LGR2");
                        check_vec_close(swat_lgr2, std::vector<double>(num_cells[1], 8.0));
                        auto sgas_lgr2 = rst.getRestartData<double>("SGAS", 1, "LGR2");
                        check_vec_close(sgas_lgr2, std::vector<double>(num_cells[1], 9.0));

                        auto rs_lgr2 = rst.getRestartData<double>("RS", 1, "LGR2");
                        fun::iota rs_expected(300.0, 300.0 + num_cells[1]);
                        std::vector<double> rs_expected_vec(rs_expected.begin(), rs_expected.end());
                        check_vec_close(rs_lgr2, rs_expected_vec);

                        auto rv_lgr2 = rst.getRestartData<double>("RV", 1, "LGR2");
                        fun::iota rv_expected(400.0, 400.0 + num_cells[1]);
                        std::vector<double> rv_expected_vec(rv_expected.begin(), rv_expected.end());
                        check_vec_close(rv_lgr2, rv_expected_vec);
                    }


            }




        }
    }
}
}


BOOST_AUTO_TEST_CASE(ECL_LGRFORMATTEDCOMPLEX)
{
    namespace OS = ::Opm::EclIO::OutputStream;
    using measure = UnitSystem::measure;

    WorkArea test_area("test_Restart");
    test_area.copyIn("LGR_3WELLS.DATA");

    Setup base_setup("LGR_3WELLS.DATA");
    const auto& units = base_setup.es.getUnits();
    auto& io_config = base_setup.es.getIOConfig();
    {
        const auto& lgr_labels = base_setup.grid.get_all_lgr_labels();
        auto num_lgr_cells = lgr_labels.size();
        std::vector<int> lgr_grid_ids(num_lgr_cells+1);
        std::iota(lgr_grid_ids.begin(), lgr_grid_ids.end(), 1);
        std::vector <std::size_t> num_cells(num_lgr_cells+1);
        num_cells[0] = base_setup.grid.getNumActive();
        std::ranges::transform(lgr_labels, num_cells.begin() + 1 ,
                               [&base_setup](const std::string& lgr_tag)
                               { return base_setup.grid.getLGRCell(lgr_tag).getNumActive(); });

        std::vector<data::Solution> cells(num_lgr_cells+1);
        std::ranges::transform(num_cells, cells.begin(),
                               [](int n) { return mkSolution(n); });
        std::vector<data::Wells> wells;
        data::Wells lwells = mkWellsLGR_Global_Complex();

        auto groups = mkGroups();
        auto sumState = sim_stateLGR(base_setup.schedule);
        auto udqState = UDQState{1};
        auto aquiferData = std::optional<Opm::RestartIO::Helpers::AggregateAquiferData>{std::nullopt};
        Action::State action_state;
        WellTestState wtest_state;
        {

            std::vector<RestartValue> restart_value;

            for (std::size_t i = 0; i < num_lgr_cells + 1; ++i) {
                restart_value.emplace_back(cells[i], lwells, groups, data::Aquifers{}, lgr_grid_ids[i]);
            }


            io_config.setEclCompatibleRST( false );
            std::ranges::transform(restart_value, restart_value.begin(),
                                   [](RestartValue& rv)
                                   {
                                       rv.addExtra("EXTRA",
                                                   UnitSystem::measure::pressure,
                                                   std::vector<double>{10.0,1.0,2.0,3.0});
                                       return rv;
                                   });

            const auto outputDir = test_area.currentWorkingDirectory();

            {
                const auto seqnum = 1;
                auto rstFile = OS::Restart {
                    OS::ResultSet{ outputDir, "LGR-OPM" }, seqnum,
                    OS::Formatted{ false }, OS::Unified{ true }
                };

                RestartIO::save(rstFile, seqnum,
                                100,
                                restart_value,
                                base_setup.es,
                                base_setup.grid,
                                base_setup.schedule,
                                action_state,
                                wtest_state,
                                sumState,
                                udqState,
                                aquiferData,
                                true);
            }

            {
                const auto rstFile = ::Opm::EclIO::OutputStream::
                    outputFileName({outputDir, "LGR-OPM"}, "UNRST");

                EclIO::ERst rst{ rstFile };

                {
                    auto lgrnames_global = rst.getRestartData<std::string>("LGRNAMES", 1);
                    auto expected_lgrnames_global = base_setup.grid.get_all_lgr_labels();
                    for (const auto& lgrname : lgrnames_global) {
                        BOOST_CHECK_EQUAL(rst.hasLGR(lgrname, 1), true);
                    }
                    BOOST_CHECK_EQUAL(lgrnames_global.size(), lgr_labels.size());
                    BOOST_CHECK_EQUAL_COLLECTIONS(lgrnames_global.begin(), lgrnames_global.end(),
                                                  expected_lgrnames_global.begin(), expected_lgrnames_global.end());

                    for (const auto& lgrname : expected_lgrnames_global) {
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGRHEADI", 1, lgrname),
                                            "LGRHEADI array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGRHEADQ", 1, lgrname),
                                            "LGRHEADQ array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGRHEADD", 1, lgrname),
                                            "LGRHEADD array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("INTEHEAD", 1, lgrname),
                                            "INTEHEAD array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LOGIHEAD", 1, lgrname),
                                            "LOGIHEAD array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("DOUBHEAD", 1, lgrname),
                                            "DOUBHEAD array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("IGRP", 1, lgrname),
                                            "IGRP array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SGRP", 1, lgrname),
                                            "SGRP array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("XGRP", 1, lgrname),
                                            "XGRP array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("ZGRP", 1, lgrname),
                                            "ZGRP array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("IWEL", 1, lgrname),
                                            "IWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SWEL", 1, lgrname),
                                            "SWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("XWEL", 1, lgrname),
                                            "XWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("ZWEL", 1, lgrname),
                                            "ZWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("LGWEL", 1, lgrname),
                                            "LGWEL array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("ICON", 1, lgrname),
                                            "ICON array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SCON", 1, lgrname),
                                            "SCON array must exist at report step 1 for local grid " << lgrname);

                        BOOST_CHECK_MESSAGE(rst.hasArray("PRESSURE", 1, lgrname),
                                            "PRESSURE array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SWAT", 1, lgrname),
                                            "SWAT array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("SGAS", 1, lgrname),
                                            "SGAS array must exist at report step 1 for local grid " << lgrname);
                        BOOST_CHECK_MESSAGE(rst.hasArray("RS", 1, lgrname),
                                            "RS array must exist at report step 1 for local grid " << lgrname);
                    }

                }

                BOOST_CHECK_MESSAGE(rst.hasKey("SWAT"), "Restart file must have SWAT vector");
                BOOST_CHECK_MESSAGE(rst.hasKey("EXTRA"), "Restart file must have EXTRA vector");

                {
                    auto convert_length = [&units](measure m, const double x) { return static_cast<double>(units.to_si(m, x)); };
                    auto convert_vector = [&](measure m, const std::vector<double>& input) {
                        std::vector<double> output;
                        output.resize(input.size());
                        std::ranges::transform(input, output.begin(),
                                               [&](double x) { return convert_length(m, x); });
                        return output;
                    };

                    // checking dynamic data for LGR
                    {
                    //LGR1
                        auto pressure_lgr1 = rst.getRestartData<double>("PRESSURE", 1, "LGR1");
                        check_vec_close(convert_vector(UnitSystem::measure::pressure, pressure_lgr1), std::vector<double>(num_cells[1], 6.0));

                        auto temperature_lgr1 = rst.getRestartData<double>("TEMP", 1, "LGR1");
                        check_vec_close(convert_vector(UnitSystem::measure::temperature, temperature_lgr1), std::vector<double>(num_cells[1], 7.0));

                        auto swat_lgr1 = rst.getRestartData<double>("SWAT", 1, "LGR1");
                        check_vec_close(swat_lgr1, std::vector<double>(num_cells[1], 8.0));
                        auto sgas_lgr1 = rst.getRestartData<double>("SGAS", 1, "LGR1");
                        check_vec_close(sgas_lgr1, std::vector<double>(num_cells[1], 9.0));

                        auto rs_lgr1 = rst.getRestartData<double>("RS", 1, "LGR1");
                        fun::iota rs_expected(300.0, 300.0 + num_cells[1]);
                        std::vector<double> rs_expected_vec(rs_expected.begin(), rs_expected.end());
                        check_vec_close(rs_lgr1, rs_expected_vec);

                        auto rv_lgr1 = rst.getRestartData<double>("RV", 1, "LGR1");
                        fun::iota rv_expected(400.0, 400.0 + num_cells[1]);
                        std::vector<double> rv_expected_vec(rv_expected.begin(), rv_expected.end());
                        check_vec_close(rv_lgr1, rv_expected_vec);
                    }
                    {
                        //LGR2
                        auto pressure_lgr2 = rst.getRestartData<double>("PRESSURE", 1, "LGR2");
                        check_vec_close(convert_vector(UnitSystem::measure::pressure, pressure_lgr2), std::vector<double>(num_cells[1], 6.0));

                        auto temperature_lgr2 = rst.getRestartData<double>("TEMP", 1, "LGR2");
                        check_vec_close(convert_vector(UnitSystem::measure::temperature, temperature_lgr2), std::vector<double>(num_cells[1], 7.0));

                        auto swat_lgr2 = rst.getRestartData<double>("SWAT", 1, "LGR2");
                        check_vec_close(swat_lgr2, std::vector<double>(num_cells[1], 8.0));
                        auto sgas_lgr2 = rst.getRestartData<double>("SGAS", 1, "LGR2");
                        check_vec_close(sgas_lgr2, std::vector<double>(num_cells[1], 9.0));

                        auto rs_lgr2 = rst.getRestartData<double>("RS", 1, "LGR2");
                        fun::iota rs_expected(300.0, 300.0 + num_cells[1]);
                        std::vector<double> rs_expected_vec(rs_expected.begin(), rs_expected.end());
                        check_vec_close(rs_lgr2, rs_expected_vec);

                        auto rv_lgr2 = rst.getRestartData<double>("RV", 1, "LGR2");
                        fun::iota rv_expected(400.0, 400.0 + num_cells[1]);
                        std::vector<double> rv_expected_vec(rv_expected.begin(), rv_expected.end());
                        check_vec_close(rv_lgr2, rv_expected_vec);
                    }


                    // -------------------------- IWEL FOR GLOBAL WELLS --------------------------
                    // GLOBAL WELLS
                    // IWEL (PROD1)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
                        const auto niwelz = 155;
                        const auto start = 0*niwelz;
                        const auto& iwell = rst.getRestartData<int>("IWEL", 1);
                        //WELL 1 ->  PROD1 -> LGR1
                        // LGR1 -> 1,1,1 from GLOBAL
                        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // PROD1 -> I
                        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD1 -> J
                        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD1/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD1/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 3); // PROD1 #Compl
                        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD1 -> Producer
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] ,1); // LOCATED LGR1
                    }
                    // IWEL (PROD2)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
                        const auto niwelz = 155;
                        const auto start = 1*niwelz;
                        const auto& iwell = rst.getRestartData<int>("IWEL", 1);
                        //WELL 2 ->  PROD2 -> LGR2
                        // LGR1 -> 3,1,1 from GLOBAL
                        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD2 -> I
                        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD2 -> J
                        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD2/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD2/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD2 #Compl
                        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD2 -> Producer
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] ,2); // LOCATED LGR2
                    }
                    // IWEL (INJ)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
                        const auto niwelz = 155;
                        const auto start = 2*niwelz;
                        const auto& iwell = rst.getRestartData<int>("IWEL", 1);
                        //WELL 3 ->  INJ ->NO LGR
                        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 2); // PROD2 -> I
                        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD2 -> J
                        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD2/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD2/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD2 #Compl
                        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 3); // PROD2 -> INJECTOR
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] ,0); // NOT IN LGR
                    }

                    // LGR WELLS
                    // IWEL (PROD1)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
                        const auto niwelz = 155;
                        const auto start = 0*niwelz;
                        const auto& iwell = rst.getRestartData<int>("IWEL", 1,"LGR1");
                        //WELL 1 ->  PROD1 -> LGR1
                        // HEAD IN LGR1 -> 2,1,1
                        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 2); // PROD1 -> I
                        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD1 -> J
                        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD1/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD1/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 3); // PROD1 #Compl
                        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD1 -> Producer
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] ,1); // LOCATED LGR1
                    }
                    // LGR WELLS
                    // IWEL (PROD2)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
                        const auto niwelz = 155;
                        const auto start = 0*niwelz;
                        const auto& iwell = rst.getRestartData<int>("IWEL", 1,"LGR2");
                        //WELL 2 ->  PROD1 -> LGR2
                        // HEAD IN LGR1 -> 1,1,1
                        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // PROD2 -> I
                        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD2 -> J
                        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD2/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD2/Head -> K
                        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PRO2 #Compl
                        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD2 -> Producer
                        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex], 2); // LOCATED LGR2
                    }

                    // -------------------------- ICON FOR GLOBAL GRID --------------------------
                    // ICON (PROD1) - C1
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  3;
                        const auto i0 = niconz * ncwmax * 0;
                        const auto& icon = rst.getRestartData<int>("ICON", 1);
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }

                    // ICON (PROD1) - C2
                    {
                        const auto niconz =  26;
                        const auto ncwmax =  1;
                        const auto i0 = niconz * ncwmax * 1;
                        const auto& icon = rst.getRestartData<int>("ICON", 1);
                        BOOST_CHECK(std::all_of(icon.begin() + i0,
                                        icon.begin() + i0 + niconz,
                                        [](int x) { return x == 0; }));
                    }

                    // ICON (PROD1) - C3
                    {
                        const auto niconz =  26;
                        const auto ncwmax =  2;
                        const auto i0 = niconz * ncwmax * 1;
                        const auto& icon = rst.getRestartData<int>("ICON", 1);
                        BOOST_CHECK(std::all_of(icon.begin() + i0,
                                        icon.begin() + i0 + niconz,
                                        [](int x) { return x == 0; }));
                    }

                    // ICON (PROD2)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  3;
                        const auto i0 = niconz * ncwmax * 1;
                        const auto& icon = rst.getRestartData<int>("ICON", 1);
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }

                    // ICON (INJ)
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  3;
                        const auto i0 = niconz * ncwmax * 2;
                        const auto& icon = rst.getRestartData<int>("ICON", 1);
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 2); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }


                    // -------------------------- ICON FOR LGR GRID --------------------------
                    // ICON LGR1 (PROD1) - C1
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  1;
                        const auto i0 = niconz * ncwmax * 0;
                        const auto& icon = rst.getRestartData<int>("ICON", 1, "LGR1");
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 2); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }

                    // ICON LGR1 (PROD1) - C2
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  1;
                        const auto i0 = niconz * ncwmax * 1;
                        const auto& icon = rst.getRestartData<int>("ICON", 1, "LGR1");
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 2); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 2); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }

                    // ICON LGR1 (PROD1) - C3
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  2;
                        const auto i0 = niconz * ncwmax * 1;
                        const auto& icon = rst.getRestartData<int>("ICON", 1, "LGR1");
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 2); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 3); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }

                    // ICON LGR2 (PROD2) - C1
                    {
                        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
                        const auto niconz =  26;
                        const auto ncwmax =  1;
                        const auto i0 = niconz * ncwmax * 0;
                        const auto& icon = rst.getRestartData<int>("ICON", 1, "LGR2");
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
                        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
                    }


            }
        }
    }
}
}
