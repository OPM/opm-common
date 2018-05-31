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

#include <cstdlib>

#define BOOST_TEST_MODULE EclipseIO
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>
#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Eqldims.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
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

inline std::string input( const std::string& rst_name = "FIRST_SIM" ) {
           return std::string(
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
           ) + rst_name + std::string(
           " 1/\n"
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
           " 'OP_1'  9  9   3   3 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
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
           " 'OP_3'  9  9   1   1 'SHUT' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"
           "WCONPROD\n"
               "'OP_3' 'OPEN' 'ORAT' 20000  4* 1000 /\n"
           "/\n"

           "DATES             -- 3\n"
           " 15  JUN 2013 / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_2'  9  9   3  9 SHUT 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           " 'OP_1'  9  9   7  7 OPEN 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           "/\n"

           "DATES             -- 4\n"
           " 22  APR 2014 / \n"
           "/\n"
           "WELSPECS\n"
           "    'OP_4'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  / \n"
           "/\n"
           "COMPDAT\n"
           " 'OP_4'  9  9   3  9 'SHUT' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
           " 'OP_3'  9  9   3  9 'SHUT' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 / \n"
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
           "/\n"
           );
}

namespace Opm {
namespace data {

/*
 * Some test specific equivalence definitions and pretty-printing. Not fit as a
 * general purpose implementation, but does its job for testing and
 * pretty-pringing for debugging purposes.
 */

std::ostream& operator<<( std::ostream& stream, const Rates& r ) {
    return stream << "{ "
                  << "wat: " << r.get( Rates::opt::wat, 0.0 ) << ", "
                  << "oil: " << r.get( Rates::opt::oil, 0.0 ) << ", "
                  << "gas: " << r.get( Rates::opt::gas, 0.0 ) << " "
                  << "}";
}

std::ostream& operator<<( std::ostream& stream, const Connection& c ) {
    return stream << "{ index: "
                  << c.index << ", "
                  << c.rates << ", "
                  << c.pressure << " }";
}

std::ostream& operator<<( std::ostream& stream,
                          const std::map< std::string, Well >& m ) {
    stream << "\n";

    for( const auto& p : m ) {
        stream << p.first << ": \n"
               << "\t" << "bhp: " << p.second.bhp << "\n"
               << "\t" << "temp: " << p.second.temperature << "\n"
               << "\t" << "rates: " << p.second.rates << "\n"
               << "\t" << "connections: [\n";

        for( const auto& c : p.second.connections )
            stream << c << " ";

        stream << "]\n";
    }

    return stream;
}

bool operator==( const Rates& lhs, const Rates& rhs ) {
    using rt = Rates::opt;

    BOOST_CHECK_EQUAL( lhs.has( rt::wat ), rhs.has( rt::wat ) );
    BOOST_CHECK_EQUAL( lhs.has( rt::oil ), rhs.has( rt::oil ) );
    BOOST_CHECK_EQUAL( lhs.has( rt::gas ), rhs.has( rt::gas ) );
    BOOST_CHECK_EQUAL( lhs.has( rt::polymer ), rhs.has( rt::polymer ) );
    BOOST_CHECK_EQUAL( lhs.get( rt::wat, 0.0 ), rhs.get( rt::wat, 0.0 ) );
    BOOST_CHECK_EQUAL( lhs.get( rt::oil, 0.0 ), rhs.get( rt::oil, 0.0 ) );
    BOOST_CHECK_EQUAL( lhs.get( rt::gas, 0.0 ), rhs.get( rt::gas, 0.0 ) );
    BOOST_CHECK_EQUAL( lhs.get( rt::polymer, 0.0 ), rhs.get( rt::polymer, 0.0 ) );

    return true;
}

bool operator==( const Connection& lhs, const Connection& rhs ) {
    BOOST_CHECK_EQUAL( lhs.index, rhs.index );
    BOOST_CHECK_EQUAL( lhs.rates, rhs.rates );
    BOOST_CHECK_EQUAL( lhs.pressure, rhs.pressure );
    BOOST_CHECK_EQUAL( lhs.reservoir_rate, rhs.reservoir_rate );

    return true;
}

bool operator!=( const Connection& lhs, const Connection& rhs ) {
    return !( lhs == rhs );
}

bool operator==( const Well& lhs, const Well& rhs ) {
    BOOST_CHECK_EQUAL( lhs.rates, rhs.rates );
    BOOST_CHECK_EQUAL( lhs.bhp, rhs.bhp );
    BOOST_CHECK_EQUAL( lhs.temperature, rhs.temperature );
    BOOST_CHECK_EQUAL( lhs.control, rhs.control );

    BOOST_CHECK_EQUAL_COLLECTIONS(
            lhs.connections.begin(), lhs.connections.end(),
            rhs.connections.begin(), rhs.connections.end() );

    return true;
}

}


data::Wells mkWells() {
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

    rc3.set( data::Rates::opt::wat, 26.41 );
    rc3.set( data::Rates::opt::oil, 27.19 );
    rc3.set( data::Rates::opt::gas, 28.41 );

    data::Well w1, w2;
    w1.rates = r1;
    w1.bhp = 1.23;
    w1.temperature = 3.45;
    w1.control = 1;

    /*
     *  the completion keys (active indices) and well names correspond to the
     *  input deck. All other entries in the well structures are arbitrary.
     */
    w1.connections.push_back( { 88, rc1, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3 } );
    w1.connections.push_back( { 288, rc2, 33.19, 123.4, 432.1, 0.26, 0.45, 2.56 } );

    w2.rates = r2;
    w2.bhp = 2.34;
    w2.temperature = 4.56;
    w2.control = 2;
    w2.connections.push_back( { 188, rc3, 36.22, 123.4, 256.1, 0.55, 0.0125, 314.15 } );

    {
        data::Wells wellRates;

        wellRates["OP_1"] = w1;
        wellRates["OP_2"] = w2;

        return wellRates;
    }
}

data::Solution mkSolution( int numCells ) {

    using measure = UnitSystem::measure;
    using namespace data;

    data::Solution sol = {
        { "PRESSURE", { measure::pressure, std::vector<double>( numCells ), TargetType::RESTART_SOLUTION } },
        { "TEMP", { measure::temperature,  std::vector<double>( numCells ), TargetType::RESTART_SOLUTION } },
        { "SWAT", { measure::identity,     std::vector<double>( numCells ), TargetType::RESTART_SOLUTION } },
        { "SGAS", { measure::identity,     std::vector<double>( numCells ), TargetType::RESTART_SOLUTION } }
    };


    sol.data("PRESSURE").assign( numCells, 6.0 );
    sol.data("TEMP").assign( numCells, 7.0 );
    sol.data("SWAT").assign( numCells, 8.0 );
    sol.data("SGAS").assign( numCells, 9.0 );

    fun::iota rsi( 300, 300 + numCells );
    fun::iota rvi( 400, 400 + numCells );

    sol.insert( "RS", measure::identity, { rsi.begin(), rsi.end() } , TargetType::RESTART_SOLUTION );
    sol.insert( "RV", measure::identity, { rvi.begin(), rvi.end() } , TargetType::RESTART_SOLUTION );

    return sol;
}


RestartValue first_sim(const EclipseState& es, EclipseIO& eclWriter, bool write_double) {
    const auto& grid = es.getInputGrid();
    auto num_cells = grid.getNumActive( );

    auto start_time = ecl_util_make_date( 1, 11, 1979 );
    auto first_step = ecl_util_make_date( 10, 10, 2008 );

    auto sol = mkSolution( num_cells );
    auto wells = mkWells();
    RestartValue restart_value(sol, wells);

    eclWriter.writeTimeStep( 1,
                             false,
                             first_step - start_time,
                             restart_value,
			     {}, {}, {}, write_double);

    return restart_value;
}

RestartValue second_sim(const EclipseIO& writer, const std::vector<RestartKey>& solution_keys) {
    return writer.loadRestart( solution_keys );
}


void compare( const RestartValue& fst,
              const RestartValue& snd,
              const std::vector<RestartKey>& solution_keys) {

    for (const auto& value : solution_keys) {
        double tol = 0.00001;
        const std::string& key = value.key;
        auto first = fst.solution.data( key ).begin();
        auto second = snd.solution.data( key ).begin();

        if (key == "TEMP")
            tol *= 10;

        for( ; first != fst.solution.data( key).end(); ++first, ++second )
            BOOST_CHECK_CLOSE( *first, *second, tol );
    }

    BOOST_CHECK_EQUAL( fst.wells, snd.wells );
}


struct Setup {
    Deck deck;
    EclipseState es;
    const EclipseGrid& grid;
    Schedule schedule;
    SummaryConfig summary_config;

    Setup( const char* path, const ParseContext& parseContext = ParseContext( )) :
        deck( Parser().parseFile( path, parseContext ) ),
        es( deck, parseContext ),
        grid( es.getInputGrid( ) ),
        schedule( deck, grid, es.get3DProperties(), es.runspec().phases(), parseContext),
        summary_config( deck, schedule, es.getTableManager( ), parseContext)
    {
        auto& io_config = es.getIOConfig();
        io_config.setEclCompatibleRST(false);
    }

};



BOOST_AUTO_TEST_CASE(EclipseReadWriteWellStateData) {
    std::vector<RestartKey> keys {{"PRESSURE" , UnitSystem::measure::pressure},
                                  {"SWAT" , UnitSystem::measure::identity},
                                  {"SGAS" , UnitSystem::measure::identity},
                                  {"TEMP" , UnitSystem::measure::temperature}};
    test_work_area_type * test_area = test_work_area_alloc("test_restart");
    test_work_area_copy_file( test_area, "FIRST_SIM.DATA");

    Setup setup("FIRST_SIM.DATA");
    EclipseIO eclWriter( setup.es, setup.grid, setup.schedule, setup.summary_config);
    auto state1 = first_sim( setup.es , eclWriter , false );
    auto state2 = second_sim( eclWriter , keys );
    compare(state1, state2 , keys);

    BOOST_CHECK_THROW( second_sim( eclWriter, {{"SOIL", UnitSystem::measure::pressure}} ) , std::runtime_error );
    BOOST_CHECK_THROW( second_sim( eclWriter, {{"SOIL", UnitSystem::measure::pressure, true}}) , std::runtime_error );
    test_work_area_free( test_area );
}


BOOST_AUTO_TEST_CASE(ECL_FORMATTED) {
    Setup setup("FIRST_SIM.DATA");
    test_work_area_type * test_area = test_work_area_alloc("test_Restart");
    auto& io_config = setup.es.getIOConfig();
    {
        auto num_cells = setup.grid.getNumActive( );
        auto cells = mkSolution( num_cells );
        auto wells = mkWells();
        {
            RestartValue restart_value(cells, wells);

            io_config.setEclCompatibleRST( false );
            restart_value.addExtra("EXTRA", UnitSystem::measure::pressure, {10,1,2,3});
            RestartIO::save("OPM_FILE.UNRST", 1 ,
                            100,
                            restart_value,
                            setup.es,
                            setup.grid,
                            setup.schedule,
                            true);

            {
                ecl_file_type * rst_file = ecl_file_open( "OPM_FILE.UNRST" , 0 );
                ecl_kw_type * swat = ecl_file_iget_named_kw(rst_file, "SWAT", 0);

                BOOST_CHECK_EQUAL( ECL_DOUBLE_TYPE, ecl_kw_get_type(swat));
                BOOST_CHECK( ecl_file_has_kw(rst_file, "EXTRA"));
                ecl_file_close(rst_file);
            }

            io_config.setEclCompatibleRST( true );
            RestartIO::save("ECL_FILE.UNRST", 1 ,
                            100,
                            restart_value,
                            setup.es,
                            setup.grid,
                            setup.schedule,
                            true);
            {
                ecl_file_type * rst_file = ecl_file_open( "ECL_FILE.UNRST" , 0 );
                ecl_kw_type * swat = ecl_file_iget_named_kw(rst_file, "SWAT", 0);

                BOOST_CHECK_EQUAL( ECL_FLOAT_TYPE, ecl_kw_get_type(swat));
                BOOST_CHECK( !ecl_file_has_kw(rst_file, "EXTRA"));
                BOOST_CHECK( !ecl_file_has_kw(rst_file, "OPM_XWEL"));
                BOOST_CHECK( !ecl_file_has_kw(rst_file, "OPM_IWEL"));
                ecl_file_close(rst_file);
            }
        }
    }
    test_work_area_free(test_area);
}





void compare_equal( const RestartValue& fst,
                    const RestartValue& snd ,
                    const std::vector<RestartKey>& keys) {

    for (const auto& value : keys) {
        const std::string& key = value.key;
        auto first = fst.solution.data( key ).begin();
        auto second = snd.solution.data( key ).begin();

        for( ; first != fst.solution.data( key ).end(); ++first, ++second )
          BOOST_CHECK_EQUAL( *first, *second);
    }

    BOOST_CHECK_EQUAL( fst.wells, snd.wells );
    //BOOST_CHECK_EQUAL( fst.extra, snd.extra );
}

BOOST_AUTO_TEST_CASE(EclipseReadWriteWellStateData_double) {
    /*
      Observe that the purpose of this test is to verify that with
      write_double == true we can load solution fields which are
      bitwise equal to those we stored. Unfortunately the scaling back
      and forth between SI units and output units is enough to break
      this equality for the pressure. For this test we therefor only
      consider the saturations which have identity unit.
    */
    std::vector<RestartKey> solution_keys {RestartKey("SWAT", UnitSystem::measure::identity),
                                           RestartKey("SGAS", UnitSystem::measure::identity)};

    test_work_area_type * test_area = test_work_area_alloc("test_Restart");
    test_work_area_copy_file( test_area, "FIRST_SIM.DATA");
    Setup setup("FIRST_SIM.DATA");
    EclipseIO eclWriter( setup.es, setup.grid, setup.schedule, setup.summary_config);

    auto state1 = first_sim( setup.es , eclWriter , true);
    auto state2 = second_sim( eclWriter , solution_keys );
    compare_equal( state1 , state2 , solution_keys);
    test_work_area_free( test_area );
}


BOOST_AUTO_TEST_CASE(WriteWrongSOlutionSize) {
    Setup setup("FIRST_SIM.DATA");
    test_work_area_type * test_area = test_work_area_alloc("test_Restart");
    {
        auto num_cells = setup.grid.getNumActive( ) + 1;
        auto cells = mkSolution( num_cells );
        auto wells = mkWells();
	Opm::SummaryState sumState;

        BOOST_CHECK_THROW( RestartIO::save("FILE.UNRST", 1 ,
                                           100,
                                           RestartValue(cells, wells),
                                           setup.es,
                                           setup.grid ,
					   sumState,
                                           setup.schedule),
                                           std::runtime_error);
    }
    test_work_area_free(test_area);
}


BOOST_AUTO_TEST_CASE(ExtraData_KEYS) {
    Setup setup("FIRST_SIM.DATA");
    auto num_cells = setup.grid.getNumActive( );
    auto cells = mkSolution( num_cells );
    auto wells = mkWells();
    RestartValue restart_value(cells, wells);

    BOOST_CHECK_THROW( restart_value.addExtra("TOO-LONG-KEY", {0,1,2}), std::runtime_error);

    // Keys must be unique
    restart_value.addExtra("KEY", {0,1,1});
    BOOST_CHECK_THROW( restart_value.addExtra("KEY", {0,1,1}), std::runtime_error);

    /* The keys must be unique across solution and extra_data */
    BOOST_CHECK_THROW( restart_value.addExtra("PRESSURE", {0,1}), std::runtime_error); 

    /* Must avoid using reserved keys like 'LOGIHEAD' */
    BOOST_CHECK_THROW( restart_value.addExtra("LOGIHEAD", {0,1}), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ExtraData_content) {
    Setup setup("FIRST_SIM.DATA");
    test_work_area_type * test_area = test_work_area_alloc("test_Restart");
    {
        auto num_cells = setup.grid.getNumActive( );
        auto cells = mkSolution( num_cells );
        auto wells = mkWells();
        const auto& units = setup.es.getUnits();
        {
            RestartValue restart_value(cells, wells);
	    Opm::SummaryState sumState;
            restart_value.addExtra("EXTRA", UnitSystem::measure::pressure, {10,1,2,3});
            RestartIO::save("FILE.UNRST", 1 ,
                            100,
                            restart_value,
                            setup.es,
                            setup.grid,
			    sumState,
                            setup.schedule);

            {
                ecl_file_type * f = ecl_file_open( "FILE.UNRST" , 0 );
                BOOST_CHECK( ecl_file_has_kw( f , "EXTRA"));
                {
                    ecl_kw_type * ex = ecl_file_iget_named_kw( f , "EXTRA" , 0 );
                    BOOST_CHECK_EQUAL( ecl_kw_get_header( ex) , "EXTRA" );
                    BOOST_CHECK_EQUAL(  4 , ecl_kw_get_size( ex ));

                    BOOST_CHECK_CLOSE( 10 ,                                                units.to_si( UnitSystem::measure::pressure, ecl_kw_iget_double( ex, 0 )), 0.00001);
                    BOOST_CHECK_CLOSE( units.from_si( UnitSystem::measure::pressure, 3)  , ecl_kw_iget_double( ex, 3 ),                                              0.00001);
                }
                ecl_file_close( f );
            }

            BOOST_CHECK_THROW( RestartIO::load( "FILE.UNRST" , 1 , {}, setup.es, setup.grid , setup.schedule,
                                                {{"NOT-THIS", UnitSystem::measure::identity, true}}) , std::runtime_error );
            {
              const auto rst_value = RestartIO::load( "FILE.UNRST" , 1 , { RestartKey("SWAT", UnitSystem::measure::identity),
                                                                           RestartKey("NO", UnitSystem::measure::identity, false)},
                setup.es, setup.grid , setup.schedule,
                {{"EXTRA", UnitSystem::measure::pressure, true},
                 {"EXTRA2", UnitSystem::measure::identity, false}});

                BOOST_CHECK(!rst_value.hasExtra("EXTRA2"));
                BOOST_CHECK( rst_value.hasExtra("EXTRA"));
                BOOST_CHECK_THROW(rst_value.getExtra("EXTRA2"), std::invalid_argument);
                const auto& extraval = rst_value.getExtra("EXTRA");
                const std::vector<double> expected = {10,1,2,3};

                BOOST_CHECK_EQUAL( rst_value.solution.has("NO") , false );
                for (size_t i=0; i < expected.size(); i++)
                    BOOST_CHECK_CLOSE(extraval[i], expected[i], 1e-5);
            }
        }
    }
    test_work_area_free(test_area);
}


BOOST_AUTO_TEST_CASE(STORE_THPRES) {
    Setup setup("FIRST_SIM_THPRES.DATA");
    test_work_area_type * test_area = test_work_area_alloc("test_Restart_THPRES");
    {
        auto num_cells = setup.grid.getNumActive( );
        auto cells = mkSolution( num_cells );
        auto wells = mkWells();
        {
            RestartValue restart_value(cells, wells);
            RestartValue restart_value2(cells, wells);

            /* Missing THPRES data in extra container. */
            /* Because it proved to difficult to update the legacy simulators
               to pass THPRES values when writing restart files this BOOST_CHECK_THROW
               had to be disabled. The RestartIO::save() function will just log a warning.

            BOOST_CHECK_THROW( RestartIO::save("FILE.UNRST", 1 ,
                                               100,
                                               restart_value,
                                               setup.es,
                                               setup.grid,
                                               setup.schedule), std::runtime_error);
            */

            restart_value.addExtra("THRESHPR", UnitSystem::measure::pressure, {0,1});
	    Opm::SummaryState sumState;
            /* THPRES data has wrong size in extra container. */
            BOOST_CHECK_THROW( RestartIO::save("FILE.UNRST", 1 ,
                                               100,
                                               restart_value,
                                               setup.es,
                                               setup.grid,
					       sumState,
                                               setup.schedule), std::runtime_error);

            int num_regions = setup.es.getTableManager().getEqldims().getNumEquilRegions();
            std::vector<double>  thpres(num_regions * num_regions, 78);
            restart_value2.addExtra("THRESHPR", UnitSystem::measure::pressure, thpres);
            restart_value2.addExtra("EXTRA", UnitSystem::measure::pressure, thpres);

            RestartIO::save("FILE2.UNRST", 1,
                            100,
                            restart_value2,
                            setup.es,
                            setup.grid,
                            setup.schedule);

            {
                ecl_file_type * rst_file = ecl_file_open("FILE2.UNRST", 0);
                std::map<std::string,int> kw_pos;
                for (int i=0; i < ecl_file_get_size(rst_file); i++)
                    kw_pos[ ecl_file_iget_header(rst_file, i ) ] = i;

                BOOST_CHECK( kw_pos["STARTSOL"] < kw_pos["THRESHPR"] );
                BOOST_CHECK( kw_pos["THRESHPR"] < kw_pos["ENDSOL"] );
                BOOST_CHECK( kw_pos["ENDSOL"] < kw_pos["EXTRA"] );

                BOOST_CHECK_EQUAL( ecl_file_get_num_named_kw(rst_file, "THRESHPR"), 1);
                BOOST_CHECK_EQUAL( ecl_file_get_num_named_kw(rst_file, "EXTRA"), 1);
                BOOST_CHECK_EQUAL( ecl_kw_get_type(ecl_file_iget_named_kw(rst_file, "THRESHPR", 0)), ECL_DOUBLE_TYPE);
                ecl_file_close(rst_file);
            }

        }
    }
    test_work_area_free(test_area);
}



}
