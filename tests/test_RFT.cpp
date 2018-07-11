/*
  Copyright 2015 Statoil ASA.

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
#include "config.h"

#define BOOST_TEST_MODULE EclipseRFTWriter
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>

#include <ert/ecl/ecl_rft_file.h>
#include <ert/ecl/ecl_util.h>
#include <ert/util/test_work_area.h>
#include <ert/util/util.h>
#include <ert/util/TestArea.hpp>

#include <vector>

using namespace Opm;

namespace {

void verifyRFTFile(const std::string& rft_filename) {

    ecl_rft_file_type * new_rft_file = ecl_rft_file_alloc(rft_filename.c_str());
    std::shared_ptr<ecl_rft_file_type> rft_file;
    rft_file.reset(new_rft_file, ecl_rft_file_free);

    //Get RFT node for well/time OP_1/10 OKT 2008
    time_t recording_time = ecl_util_make_date(10, 10, 2008);
    ecl_rft_node_type * ecl_rft_node = ecl_rft_file_get_well_time_rft(rft_file.get() , "OP_1" , recording_time);
    BOOST_CHECK(ecl_rft_node_is_RFT(ecl_rft_node));

    //Verify RFT data for completions (ijk) 9 9 1, 9 9 2 and 9 9 3 for OP_1
    const ecl_rft_cell_type * ecl_rft_cell1 = ecl_rft_node_lookup_ijk(ecl_rft_node, 8, 8, 0);
    const ecl_rft_cell_type * ecl_rft_cell2 = ecl_rft_node_lookup_ijk(ecl_rft_node, 8, 8, 1);
    const ecl_rft_cell_type * ecl_rft_cell3 = ecl_rft_node_lookup_ijk(ecl_rft_node, 8, 8, 2);

    double tol = 0.00001;
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell1), 0.00000, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell2), 0.00001, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell3), 0.00002, tol);

    BOOST_CHECK_CLOSE(ecl_rft_cell_get_sgas(ecl_rft_cell1), 0.0, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_sgas(ecl_rft_cell2), 0.2, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_sgas(ecl_rft_cell3), 0.4, tol);

    BOOST_CHECK_CLOSE(ecl_rft_cell_get_swat(ecl_rft_cell1), 0.0, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_swat(ecl_rft_cell2), 0.1, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_swat(ecl_rft_cell3), 0.2, tol);

    BOOST_CHECK_CLOSE(ecl_rft_cell_get_soil(ecl_rft_cell1), 1.0, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_soil(ecl_rft_cell2), 0.7, tol);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_soil(ecl_rft_cell3), 0.4, tol);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_depth(ecl_rft_cell1), (0.250 + (0.250/2)));
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_depth(ecl_rft_cell2), (2*0.250 + (0.250/2)));
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_depth(ecl_rft_cell3), (3*0.250 + (0.250/2)));
}

data::Solution createBlackoilState( int timeStepIdx, int numCells ) {
    std::vector< double > pressure( numCells );
    std::vector< double > swat( numCells, 0 );
    std::vector< double > sgas( numCells, 0 );

    for( int i = 0; i < numCells; ++i )
        pressure[ i ] = timeStepIdx * 1e5 + 1e4 + i;

    data::Solution sol;
    sol.insert( "PRESSURE", UnitSystem::measure::pressure, pressure , data::TargetType::RESTART_SOLUTION );
    sol.insert( "SWAT", UnitSystem::measure::identity, swat , data::TargetType::RESTART_SOLUTION );
    sol.insert( "SGAS", UnitSystem::measure::identity, sgas, data::TargetType::RESTART_SOLUTION );
    return sol;
}

}

BOOST_AUTO_TEST_CASE(test_RFT) {
    ParseContext parse_context;
    std::string eclipse_data_filename    = "testrft.DATA";
    ERT::TestArea test_area("test_RFT");
    test_area.copyFile( eclipse_data_filename );

    auto deck = Parser().parseFile( eclipse_data_filename, parse_context );
    auto eclipseState = Parser::parse( deck );
    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto& grid = eclipseState.getInputGrid();
        const auto numCells = grid.getCartesianSize( );
        Schedule schedule(deck, grid, eclipseState.get3DProperties(), eclipseState.runspec().phases(), parse_context);
        SummaryConfig summary_config( deck, schedule, eclipseState.getTableManager( ), parse_context);
        EclipseIO eclipseWriter( eclipseState, grid, schedule, summary_config );
        time_t start_time = schedule.posixStartTime();
        time_t step_time = ecl_util_make_date(10, 10, 2008 );

        data::Rates r1, r2;
        r1.set( data::Rates::opt::wat, 4.11 );
        r1.set( data::Rates::opt::oil, 4.12 );
        r1.set( data::Rates::opt::gas, 4.13 );

        r2.set( data::Rates::opt::wat, 4.21 );
        r2.set( data::Rates::opt::oil, 4.22 );
        r2.set( data::Rates::opt::gas, 4.23 );

        std::vector<Opm::data::Connection> well1_comps(9);
        for (size_t i = 0; i < 9; ++i) {
            Opm::data::Connection well_comp { grid.getGlobalIndex(8,8,i) ,r1, 0.0 , 0.0, (double)i, 0.1*i,0.2*i, 1.2e3};
            well1_comps[i] = well_comp;
        }
        std::vector<Opm::data::Connection> well2_comps(6);
        for (size_t i = 0; i < 6; ++i) {
            Opm::data::Connection well_comp { grid.getGlobalIndex(3,3,i+3) ,r2, 0.0 , 0.0, (double)i, i*0.1,i*0.2, 0.15};
            well2_comps[i] = well_comp;
        }

        Opm::data::Solution solution = createBlackoilState(2, numCells);
        Opm::data::Wells wells;
        wells["OP_1"] = { r1, 1.0, 1.1, 3.1, 1, well1_comps };
        wells["OP_2"] = { r2, 1.0, 1.1, 3.2, 1, well2_comps };


        RestartValue restart_value(solution, wells);

        eclipseWriter.writeTimeStep( 2,
                                     false,
                                     step_time - start_time,
                                     restart_value,
                                     {},
                                     {},
                                     {});
    }

    verifyRFTFile("TESTRFT.RFT");
}

namespace {
void verifyRFTFile2(const std::string& rft_filename) {
    ecl_rft_file_type * rft_file = ecl_rft_file_alloc(rft_filename.c_str());
    /*
      Expected occurences:

      WELL   |  DATE
      ---------------------
      OP_1   | 10. OKT 2008
      OP_2   | 10. OKT 2008
      OP_2   | 10. NOV 2008
      ---------------------
    */

    BOOST_CHECK_EQUAL( 3 , ecl_rft_file_get_size( rft_file ));
    BOOST_CHECK( ecl_rft_file_get_well_time_rft(rft_file , "OP_1" , ecl_util_make_date(10, 10, 2008)) != NULL );
    BOOST_CHECK( ecl_rft_file_get_well_time_rft(rft_file , "OP_2" , ecl_util_make_date(10, 10, 2008)) != NULL );
    BOOST_CHECK( ecl_rft_file_get_well_time_rft(rft_file , "OP_2" , ecl_util_make_date(10, 11, 2008)) != NULL );

    ecl_rft_file_free( rft_file );
}
}


BOOST_AUTO_TEST_CASE(test_RFT2) {
    ParseContext parse_context;
    std::string eclipse_data_filename    = "testrft.DATA";
    ERT::TestArea test_area("test_RFT");
    test_area.copyFile( eclipse_data_filename );

    auto deck = Parser().parseFile( eclipse_data_filename, parse_context );
    auto eclipseState = Parser::parse( deck );
    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto& grid = eclipseState.getInputGrid();
        const auto numCells = grid.getCartesianSize( );

        Schedule schedule(deck, grid, eclipseState.get3DProperties(), eclipseState.runspec().phases(), parse_context);
        SummaryConfig summary_config( deck, schedule, eclipseState.getTableManager( ), parse_context);
        EclipseIO eclipseWriter( eclipseState, grid, schedule, summary_config );
        time_t start_time = schedule.posixStartTime();
        const auto& time_map = schedule.getTimeMap( );

        for (int counter = 0; counter < 2; counter++) {
            for (size_t step = 0; step < time_map.size(); step++) {
                time_t step_time = time_map[step];

                data::Rates r1, r2;
                r1.set( data::Rates::opt::wat, 4.11 );
                r1.set( data::Rates::opt::oil, 4.12 );
                r1.set( data::Rates::opt::gas, 4.13 );

                r2.set( data::Rates::opt::wat, 4.21 );
                r2.set( data::Rates::opt::oil, 4.22 );
                r2.set( data::Rates::opt::gas, 4.23 );

                std::vector<Opm::data::Connection> well1_comps(9);
                for (size_t i = 0; i < 9; ++i) {
                    Opm::data::Connection well_comp { grid.getGlobalIndex(8,8,i) ,r1, 0.0 , 0.0, (double)i, 0.1*i,0.2*i, 3.14e5};
                    well1_comps[i] = well_comp;
                }
                std::vector<Opm::data::Connection> well2_comps(6);
                for (size_t i = 0; i < 6; ++i) {
                    Opm::data::Connection well_comp { grid.getGlobalIndex(3,3,i+3) ,r2, 0.0 , 0.0, (double)i, i*0.1,i*0.2, 355.113};
                    well2_comps[i] = well_comp;
                }

                Opm::data::Wells wells;
                Opm::data::Solution solution = createBlackoilState(2, numCells);
                wells["OP_1"] = { r1, 1.0, 1.1, 3.1, 1, well1_comps };
                wells["OP_2"] = { r2, 1.0, 1.1, 3.2, 1, well2_comps };

                RestartValue restart_value(solution, wells);
                eclipseWriter.writeTimeStep( step,
                                             false,
                                             step_time - start_time,
                                             restart_value,
                                             {},
                                             {},
                                             {});
            }
            verifyRFTFile2("TESTRFT.RFT");
        }
    }
}
