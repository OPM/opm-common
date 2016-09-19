
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
#include "config.h"

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE EclipseWriter
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/EclipseWriter.hpp>
#include <opm/output/eclipse/EclipseReader.hpp>
#include <opm/output/Cells.hpp>
#include <opm/output/Wells.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>

// ERT stuff
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_util.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl_well/well_info.h>
#include <ert/ecl_well/well_state.h>
#include <ert/util/test_work_area.h>

using namespace Opm;

std::string input =
           "RUNSPEC\n"
           "OIL\n"
           "GAS\n"
           "WATER\n"
           "DISGAS\n"
           "VAPOIL\n"
           "UNIFOUT\n"
           "UNIFIN\n"
           "DIMENS\n"
           " 10 10 10 /\n"

           "GRID\n"
           "DXV\n"
           "10*0.25 /\n"
           "DYV\n"
           "10*0.25 /\n"
           "DZV\n"
           "10*0.25 /\n"
           "TOPS\n"
           "100*0.25 /\n"
           "\n"

           "SOLUTION\n"
           "RESTART\n"
           "FIRST_SIM 1/\n"
           "\n"

           "START             -- 0 \n"
           "1 NOV 1979 / \n"

           "SCHEDULE\n"
           "SKIPREST\n"
           "RPTRST\n"
           "BASIC=1\n"
           "/\n"
           "DATES             -- 1\n"
           " 10  OKT 2008 / \n"
           "/\n"
           "WELSPECS\n"
           "    'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "    'OP_2'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           " 'OP_2'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 / \n"
           " 'OP_1'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_1' 'OPEN' 'ORAT' 20000  4* 1000 /\n"
           "/\n"
           "WCONINJE\n"
               "'OP_2' 'GAS' 'OPEN' 'RATE' 100 200 400 /\n"
           "/\n"

           "DATES             -- 2\n"
           " 20  JAN 2011 / \n"
           "/\n"
           "WELSPECS\n"
           "    'OP_3'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_3'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_3' 'OPEN' 'ORAT' 20000  4* 1000 /\n"
           "/\n"

           "DATES             -- 3\n"
           " 15  JUN 2013 / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_2'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           " 'OP_1'  9  9   7  7 'SHUT' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"

           "DATES             -- 4\n"
           " 22  APR 2014 / \n"
           "/\n"
           "WELSPECS\n"
           "    'OP_4'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_4'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           " 'OP_3'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_4' 'OPEN' 'ORAT' 20000  4* 1000 /\n"
           "/\n"

           "DATES             -- 5\n"
           " 30  AUG 2014 / \n"
           "/\n"
           "WELSPECS\n"
           "    'OP_5'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_5'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_5' 'OPEN' 'ORAT' 20000  4* 1000 /\n"
           "/\n"

           "DATES             -- 6\n"
           " 15  SEP 2014 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_3' 'SHUT' 'ORAT' 20000  4* 1000 /\n"
           "/\n"

           "DATES             -- 7\n"
           " 9  OCT 2014 / \n"
           "/\n"
           "WELSPECS\n"
           "    'OP_6'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_6'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_6' 'OPEN' 'ORAT' 20000  4* 1000 /\n"
           "/\n"
           "TSTEP            -- 8\n"
           "10 /"
           "/\n";

data::Wells mkWells() {
    std::vector< double > bhp = { 1.23, 2.34 };
    std::vector< double > temp = { 3.45, 4.56 };
    std::vector< double > well_rates =
        { 5.67, 6.78, 7.89, 8.90, 9.01, 10.12 };

    std::vector< double > perf_press = {
        20.41, 21.19, 22.41,
        23.19, 24.41, 25.19,
        26.41, 27.19, 28.41
    };

    std::vector< double > perf_rate = {
        30.45, 31.19, 32.45,
        33.19, 34.45, 35.19,
        36.45, 37.19, 38.45,
    };

    return { {}, bhp, temp, well_rates, perf_press, perf_rate };
}

data::Solution mkSolution( int numCells ) {
    using ds = data::Solution::key;
    data::Solution sol;

    for( auto k : { ds::PRESSURE, ds::TEMP, ds::SWAT, ds::SGAS } ) {
        sol.insert( k, std::vector< double >( numCells ) );
    }

    sol[ ds::PRESSURE ].assign( numCells, 6.0 );
    sol[ ds::TEMP ].assign( numCells, 7.0 );
    sol[ ds::SWAT ].assign( numCells, 8.0 );
    sol[ ds::SGAS ].assign( numCells, 9.0 );

    fun::iota rsi( 300, 300 + numCells );
    fun::iota rvi( 400, 400 + numCells );

    sol.insert( ds::RS, { rsi.begin(), rsi.end() } );
    sol.insert( ds::RV, { rvi.begin(), rvi.end() } );

    return sol;
}

std::pair< data::Solution, data::Wells >
first_sim(test_work_area_type * test_area) {

    std::string eclipse_data_filename    = "FIRST_SIM.DATA";
    test_work_area_copy_file(test_area, eclipse_data_filename.c_str());

    auto eclipseState = std::make_shared< EclipseState >(
            Parser::parse( eclipse_data_filename ) );

    const auto& grid = *eclipseState->getInputGrid();
    auto num_cells = grid.getNX() * grid.getNY() * grid.getNZ();

    EclipseWriter eclWriter( eclipseState, grid);
    auto start_time = ecl_util_make_date( 1, 11, 1979 );
    auto first_step = ecl_util_make_date( 10, 10, 2008 );

    auto sol = mkSolution( num_cells );
    auto wells = mkWells();

    eclWriter.writeTimeStep( 1,
                             false,
                             first_step - start_time,
                             sol, wells);

    return { sol, wells };
}

std::pair< data::Solution, data::Wells > second_sim() {
    auto eclipseState = Parser::parseData( input );

    const auto& grid = *eclipseState.getInputGrid();
    auto num_cells = grid.getNX() * grid.getNY() * grid.getNZ();

    return init_from_restart_file( eclipseState, num_cells );
}

void compare( std::pair< data::Solution, data::Wells > fst,
              std::pair< data::Solution, data::Wells > snd ) {

    using ds = data::Solution::key;
    for( auto key : { ds::PRESSURE, ds::TEMP, ds::SWAT, ds::SGAS,
                      ds::RS, ds::RV } ) {

        auto first = fst.first[ key ].begin();
        auto second = snd.first[ key ].begin();

        for( ; first != fst.first[ key ].end(); ++first, ++second )
            BOOST_CHECK_CLOSE( *first, *second, 0.00001 );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS(
            fst.second.bhp.begin(), fst.second.bhp.end(),
            snd.second.bhp.begin(), snd.second.bhp.end()
        );
    BOOST_CHECK_EQUAL_COLLECTIONS(
            fst.second.temperature.begin(), fst.second.temperature.end(),
            snd.second.temperature.begin(), snd.second.temperature.end()
        );
    BOOST_CHECK_EQUAL_COLLECTIONS(
            fst.second.well_rate.begin(), fst.second.well_rate.end(),
            snd.second.well_rate.begin(), snd.second.well_rate.end()
        );
    BOOST_CHECK_EQUAL_COLLECTIONS(
            fst.second.perf_pressure.begin(), fst.second.perf_pressure.end(),
            snd.second.perf_pressure.begin(), snd.second.perf_pressure.end()
        );
    BOOST_CHECK_EQUAL_COLLECTIONS(
            fst.second.perf_rate.begin(), fst.second.perf_rate.end(),
            snd.second.perf_rate.begin(), snd.second.perf_rate.end()
        );
}

BOOST_AUTO_TEST_CASE(EclipseReadWriteWellStateData) {
    test_work_area_type * test_area = test_work_area_alloc("test_Restart");

    auto state1 = first_sim(test_area);
    auto state2 = second_sim();
    compare(state1, state2);

    test_work_area_free(test_area);
}
