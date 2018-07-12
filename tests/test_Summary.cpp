/*
  Copyright 2016 Statoil ASA.

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

#define BOOST_TEST_MODULE Wells
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdexcept>

#include <ert/ecl/ecl_sum.h>
#include <ert/ecl/smspec_node.h>
#include <ert/util/util.h>
#include <ert/util/TestArea.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/SummaryState.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;
using rt = data::Rates::opt;

/* conversion factor for whenever 'day' is the unit of measure, whereas we
 * expect input in SI units (seconds)
 */
static const int day = 24 * 60 * 60;


/*
  This is quite misleading, because the values prepared in the test
  input deck are NOT used.
*/
static data::Wells result_wells() {
    /* populate with the following pattern:
     *
     * Wells are named W_1, W_2 etc, i.e. wells are 1 indexed.
     *
     * rates on a well are populated with 10 * wellidx . type (where type is
     * 0-1-2 from owg)
     *
     * bhp is wellidx.1
     * bhp is wellidx.2
     *
     * completions are 100*wellidx . type
     */

    // conversion factor Pascal (simulator output) <-> barsa
    const double ps = 100000;

    data::Rates rates1;
    rates1.set( rt::wat, -10.0 / day );
    rates1.set( rt::oil, -10.1 / day );
    rates1.set( rt::gas, -10.2 / day );
    rates1.set( rt::solvent, -10.3 / day );
    rates1.set( rt::dissolved_gas, -10.4 / day );
    rates1.set( rt::vaporized_oil, -10.5 / day );
    rates1.set( rt::reservoir_water, -10.6 / day );
    rates1.set( rt::reservoir_oil, -10.7 / day );
    rates1.set( rt::reservoir_gas, -10.8 / day );

    data::Rates rates2;
    rates2.set( rt::wat, -20.0 / day );
    rates2.set( rt::oil, -20.1 / day );
    rates2.set( rt::gas, -20.2 / day );
    rates2.set( rt::solvent, -20.3 / day );
    rates2.set( rt::dissolved_gas, -20.4 / day );
    rates2.set( rt::vaporized_oil, -20.5 / day );
    rates2.set( rt::reservoir_water, -20.6 / day );
    rates2.set( rt::reservoir_oil, -20.7 / day );
    rates2.set( rt::reservoir_gas, -20.8 / day );

    data::Rates rates3;
    rates3.set( rt::wat, 30.0 / day );
    rates3.set( rt::oil, 30.1 / day );
    rates3.set( rt::gas, 30.2 / day );
    rates3.set( rt::solvent, 30.3 / day );
    rates3.set( rt::dissolved_gas, 30.4 / day );
    rates3.set( rt::vaporized_oil, 30.5 / day );
    rates3.set( rt::reservoir_water, 30.6 / day );
    rates3.set( rt::reservoir_oil, 30.7 / day );
    rates3.set( rt::reservoir_gas, 30.8 / day );


    /* completion rates */
    data::Rates crates1;
    crates1.set( rt::wat, -100.0 / day );
    crates1.set( rt::oil, -100.1 / day );
    crates1.set( rt::gas, -100.2 / day );
    crates1.set( rt::solvent, -100.3 / day );
    crates1.set( rt::dissolved_gas, -100.4 / day );
    crates1.set( rt::vaporized_oil, -100.5 / day );
    crates1.set( rt::reservoir_water, -100.6 / day );
    crates1.set( rt::reservoir_oil, -100.7 / day );
    crates1.set( rt::reservoir_gas, -100.8 / day );

    data::Rates crates2;
    crates2.set( rt::wat, -200.0 / day );
    crates2.set( rt::oil, -200.1 / day );
    crates2.set( rt::gas, -200.2 / day );
    crates2.set( rt::solvent, -200.3 / day );
    crates2.set( rt::dissolved_gas, -200.4 / day );
    crates2.set( rt::vaporized_oil, -200.5 / day );
    crates2.set( rt::reservoir_water, -200.6 / day );
    crates2.set( rt::reservoir_oil, -200.7 / day );
    crates2.set( rt::reservoir_gas, -200.8 / day );

    data::Rates crates3;
    crates3.set( rt::wat, 300.0 / day );
    crates3.set( rt::oil, 300.1 / day );
    crates3.set( rt::gas, 300.2 / day );
    crates3.set( rt::solvent, 300.3 / day );
    crates3.set( rt::dissolved_gas, 300.4 / day );
    crates3.set( rt::vaporized_oil, 300.5 / day );
    crates3.set( rt::reservoir_water, 300.6 / day );
    crates3.set( rt::reservoir_oil, 300.7 / day );
    crates3.set( rt::reservoir_gas, 300.8 / day );

    /*
      The global index assigned to the completion must be manually
      syncronized with the global index in the COMPDAT keyword in the
      input deck.
    */
    data::Connection well1_comp1 { 0  , crates1, 1.9 , 123.4, 314.15, 0.35, 0.25, 2.718e2};
    data::Connection well2_comp1 { 1  , crates2, 1.10 , 123.4, 212.1, 0.78, 0.0, 12.34};
    data::Connection well2_comp2 { 101, crates3, 1.11 , 123.4, 150.6, 0.001, 0.89, 100.0};
    data::Connection well3_comp1 { 2  , crates3, 1.11 , 123.4, 456.78, 0.0, 0.15, 432.1};

    /*
      The completions
    */
    data::Well well1 { rates1, 0.1 * ps, 0.2 * ps, 0.3 * ps, 1, { {well1_comp1} } };
    data::Well well2 { rates2, 1.1 * ps, 1.2 * ps, 1.3 * ps, 2, { {well2_comp1 , well2_comp2} } };
    data::Well well3 { rates3, 2.1 * ps, 2.2 * ps, 2.3 * ps, 3, { {well3_comp1} } };

    data::Wells wellrates;

    wellrates["W_1"] = well1;
    wellrates["W_2"] = well2;
    wellrates["W_3"] = well3;

    return wellrates;
}

ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free > readsum( const std::string& base ) {
    return ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free >(
            ecl_sum_fread_alloc_case( base.c_str(), ":" )
           );
}

struct setup {
    Deck deck;
    EclipseState es;
    const EclipseGrid& grid;
    Schedule schedule;
    SummaryConfig config;
    data::Wells wells;
    std::string name;
    ERT::TestArea ta;

    /*-----------------------------------------------------------------*/

    setup( const std::string& fname , const char* path = "summary_deck.DATA", const ParseContext& parseContext = ParseContext( )) :
        deck( Parser().parseFile( path, parseContext ) ),
        es( deck, ParseContext() ),
        grid( es.getInputGrid() ),
        schedule( deck, grid, es.get3DProperties(), es.runspec().phases(), parseContext),
        config( deck, schedule, es.getTableManager(), parseContext ),
        wells( result_wells() ),
        name( fname ),
        ta( ERT::TestArea("test_summary") )
    {
    }

};

BOOST_AUTO_TEST_SUITE(Summary)

/*
 * Tests works by reading the Deck, write the summary output, then immediately
 * read it again (with ERT), and compare the read values with the input.
 */
BOOST_AUTO_TEST_CASE(well_keywords) {
    setup cfg( "test_summary_well" );

    // Force to run in a directory, to make sure the basename with
    // leading path works.
    util_make_path( "PATH" );
    cfg.name = "PATH/CASE";

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule , cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* Production rates */
    BOOST_CHECK_CLOSE( 10.0, ecl_sum_get_well_var( resp, 1, "W_1", "WWPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 20.0, ecl_sum_get_well_var( resp, 1, "W_2", "WWPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WOPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WOPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.2, ecl_sum_get_well_var( resp, 1, "W_1", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.8, ecl_sum_get_well_var( resp, 1, "W_1", "WGVPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2, ecl_sum_get_well_var( resp, 1, "W_2", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.8, ecl_sum_get_well_var( resp, 1, "W_2", "WGVPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 + 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.0 + 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.3, ecl_sum_get_well_var( resp, 1, "W_1", "WNPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.3, ecl_sum_get_well_var( resp, 1, "W_2", "WNPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.4, ecl_sum_get_well_var( resp, 1, "W_1", "WGPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.4, ecl_sum_get_well_var( resp, 1, "W_2", "WGPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 - 10.4, ecl_sum_get_well_var( resp, 1, "W_1", "WGPRF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2 - 20.4, ecl_sum_get_well_var( resp, 1, "W_2", "WGPRF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.6 + 10.7 + 10.8,
                                    ecl_sum_get_well_var( resp, 1, "W_1", "WVPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.6 + 20.7 + 20.8,
                                    ecl_sum_get_well_var( resp, 1, "W_2", "WVPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.5, ecl_sum_get_well_var( resp, 1, "W_1", "WOPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.5, ecl_sum_get_well_var( resp, 1, "W_2", "WOPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.1 - 10.5), ecl_sum_get_well_var( resp, 1, "W_1", "WOPRF" ), 1e-5 );
    BOOST_CHECK_CLOSE( (20.1 - 20.5), ecl_sum_get_well_var( resp, 1, "W_2", "WOPRF" ), 1e-5 );

    /* Production totals */
    BOOST_CHECK_CLOSE( 10.0, ecl_sum_get_well_var( resp, 1, "W_1", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.0, ecl_sum_get_well_var( resp, 1, "W_2", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2, ecl_sum_get_well_var( resp, 1, "W_1", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2, ecl_sum_get_well_var( resp, 1, "W_2", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.3, ecl_sum_get_well_var( resp, 1, "W_1", "WNPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.3, ecl_sum_get_well_var( resp, 1, "W_2", "WNPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.0 + 10.1), ecl_sum_get_well_var( resp, 1, "W_1", "WLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (20.0 + 20.1), ecl_sum_get_well_var( resp, 1, "W_2", "WLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.5, ecl_sum_get_well_var( resp, 1, "W_1", "WOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.5, ecl_sum_get_well_var( resp, 1, "W_2", "WOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.1 - 10.5), ecl_sum_get_well_var( resp, 1, "W_1", "WOPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( (20.1 - 20.5), ecl_sum_get_well_var( resp, 1, "W_2", "WOPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.6 + 10.7 + 10.8,
                                        ecl_sum_get_well_var( resp, 1, "W_1", "WVPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.6 + 20.7 + 20.8,
                                        ecl_sum_get_well_var( resp, 1, "W_2", "WVPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 10.0, ecl_sum_get_well_var( resp, 2, "W_1", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.0, ecl_sum_get_well_var( resp, 2, "W_2", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.1, ecl_sum_get_well_var( resp, 2, "W_1", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.1, ecl_sum_get_well_var( resp, 2, "W_2", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.2, ecl_sum_get_well_var( resp, 2, "W_1", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.2, ecl_sum_get_well_var( resp, 2, "W_2", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( ( 20.0 + 20.1 ), ecl_sum_get_well_var( resp, 2, "W_2", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (20.0 + 20.1), ecl_sum_get_well_var( resp, 2, "W_2", "WLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.3, ecl_sum_get_well_var( resp, 2, "W_1", "WNPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.3, ecl_sum_get_well_var( resp, 2, "W_2", "WNPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 10.4, ecl_sum_get_well_var( resp, 2, "W_1", "WGPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.4, ecl_sum_get_well_var( resp, 2, "W_2", "WGPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * ( 10.2 - 10.4 ), ecl_sum_get_well_var( resp, 2, "W_1", "WGPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * ( 20.2 - 20.4 ), ecl_sum_get_well_var( resp, 2, "W_2", "WGPTF" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 10.5, ecl_sum_get_well_var( resp, 2, "W_1", "WOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.5, ecl_sum_get_well_var( resp, 2, "W_2", "WOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * ( 10.1 - 10.5 ), ecl_sum_get_well_var( resp, 2, "W_1", "WOPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * ( 20.1 - 20.5 ), ecl_sum_get_well_var( resp, 2, "W_2", "WOPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.6 + 10.7 + 10.8),
                                        ecl_sum_get_well_var( resp, 2, "W_1", "WVPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (20.6 + 20.7 + 20.8),
                                        ecl_sum_get_well_var( resp, 2, "W_2", "WVPT" ), 1e-5 );

    /* Production rates (history) */
    BOOST_CHECK_CLOSE( 10, ecl_sum_get_well_var( resp, 1, "W_1", "WWPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20, ecl_sum_get_well_var( resp, 1, "W_2", "WWPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WOPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WOPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2, ecl_sum_get_well_var( resp, 1, "W_1", "WGPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2, ecl_sum_get_well_var( resp, 1, "W_2", "WGPRH" ), 1e-5 );

    /* Production totals (history) */
    BOOST_CHECK_CLOSE( 2 * 10.0, ecl_sum_get_well_var( resp, 2, "W_1", "WWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.0, ecl_sum_get_well_var( resp, 2, "W_2", "WWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.1, ecl_sum_get_well_var( resp, 2, "W_1", "WOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.1, ecl_sum_get_well_var( resp, 2, "W_2", "WOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.2, ecl_sum_get_well_var( resp, 2, "W_1", "WGPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.2, ecl_sum_get_well_var( resp, 2, "W_2", "WGPTH" ), 1e-5 );

    /* Injection rates */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_well_var( resp, 1, "W_3", "WWIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.6, ecl_sum_get_well_var( resp, 1, "W_3", "WWVIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.8, ecl_sum_get_well_var( resp, 1, "W_3", "WGVIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_well_var( resp, 1, "W_3", "WGIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_well_var( resp, 1, "W_3", "WNIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5, ecl_sum_get_well_var( resp, 1, "W_3", "WCIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 2.5, ecl_sum_get_well_var( resp, 2, "W_3", "WCIR" ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_well_var( resp, 1, "W_3", "WWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_well_var( resp, 1, "W_3", "WGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_well_var( resp, 1, "W_3", "WNIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5, ecl_sum_get_well_var( resp, 1, "W_3", "WCIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.6 + 30.7 + 30.8),
                       ecl_sum_get_well_var( resp, 1, "W_3", "WVIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.0, ecl_sum_get_well_var( resp, 2, "W_3", "WWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2, ecl_sum_get_well_var( resp, 2, "W_3", "WGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.3, ecl_sum_get_well_var( resp, 2, "W_3", "WNIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5 + 30.0 * 2.5, ecl_sum_get_well_var( resp, 2, "W_3", "WCIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2* (30.6 + 30.7 + 30.8),
                       ecl_sum_get_well_var( resp, 2, "W_3", "WVIT" ), 1e-5 );

    /* Injection rates (history) */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_well_var( resp, 1, "W_3", "WWIRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,    ecl_sum_get_well_var( resp, 1, "W_3", "WGIRH" ), 1e-5 );

    /* Injection totals (history) */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_well_var( resp, 1, "W_3", "WWITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,    ecl_sum_get_well_var( resp, 1, "W_3", "WGITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 60.0, ecl_sum_get_well_var( resp, 2, "W_3", "WWITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,    ecl_sum_get_well_var( resp, 2, "W_3", "WGITH" ), 1e-5 );

    /* Production targets */
    BOOST_CHECK_CLOSE( 30.1 , ecl_sum_get_well_var( resp, 1, "W_5", "WVPRT" ), 1e-5 );

    /* WWCT - water cut */
    const double wwcut1 = 10.0 / ( 10.0 + 10.1 );
    const double wwcut2 = 20.0 / ( 20.0 + 20.1 );
    const double wwcut3 = 0;

    BOOST_CHECK_CLOSE( wwcut1, ecl_sum_get_well_var( resp, 1, "W_1", "WWCT" ), 1e-5 );
    BOOST_CHECK_CLOSE( wwcut2, ecl_sum_get_well_var( resp, 1, "W_2", "WWCT" ), 1e-5 );
    BOOST_CHECK_CLOSE( wwcut3, ecl_sum_get_well_var( resp, 1, "W_3", "WWCT" ), 1e-5 );

    /* gas-oil ratio */
    const double wgor1 = 10.2 / 10.1;
    const double wgor2 = 20.2 / 20.1;
    const double wgor3 = 0;

    BOOST_CHECK_CLOSE( wgor1, ecl_sum_get_well_var( resp, 1, "W_1", "WGOR" ), 1e-5 );
    BOOST_CHECK_CLOSE( wgor2, ecl_sum_get_well_var( resp, 1, "W_2", "WGOR" ), 1e-5 );
    BOOST_CHECK_CLOSE( wgor3, ecl_sum_get_well_var( resp, 1, "W_3", "WGOR" ), 1e-5 );

    BOOST_CHECK_CLOSE( wgor1, ecl_sum_get_well_var( resp, 1, "W_1", "WGORH" ), 1e-5 );
    BOOST_CHECK_CLOSE( wgor2, ecl_sum_get_well_var( resp, 1, "W_2", "WGORH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,     ecl_sum_get_well_var( resp, 1, "W_3", "WGORH" ), 1e-5 );

    /* WGLR - gas-liquid rate */
    const double wglr1 = 10.2 / ( 10.0 + 10.1 );
    const double wglr2 = 20.2 / ( 20.0 + 20.1 );
    const double wglr3 = 0;

    BOOST_CHECK_CLOSE( wglr1, ecl_sum_get_well_var( resp, 1, "W_1", "WGLR" ), 1e-5 );
    BOOST_CHECK_CLOSE( wglr2, ecl_sum_get_well_var( resp, 1, "W_2", "WGLR" ), 1e-5 );
    BOOST_CHECK_CLOSE( wglr3, ecl_sum_get_well_var( resp, 1, "W_3", "WGLR" ), 1e-5 );

    BOOST_CHECK_CLOSE( wglr1, ecl_sum_get_well_var( resp, 1, "W_1", "WGLRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( wglr2, ecl_sum_get_well_var( resp, 1, "W_2", "WGLRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0, ecl_sum_get_well_var( resp, 1, "W_3", "WGLRH" ), 1e-5 );

    /* BHP */
    BOOST_CHECK_CLOSE( 0.1, ecl_sum_get_well_var( resp, 1, "W_1", "WBHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 1.1, ecl_sum_get_well_var( resp, 1, "W_2", "WBHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2.1, ecl_sum_get_well_var( resp, 1, "W_3", "WBHP" ), 1e-5 );

    /* THP */
    BOOST_CHECK_CLOSE( 0.2, ecl_sum_get_well_var( resp, 1, "W_1", "WTHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 1.2, ecl_sum_get_well_var( resp, 1, "W_2", "WTHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2.2, ecl_sum_get_well_var( resp, 1, "W_3", "WTHP" ), 1e-5 );

    /* BHP (history) */
    BOOST_CHECK_CLOSE( 0.1, ecl_sum_get_well_var( resp, 1, "W_1", "WBHPH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 1.1, ecl_sum_get_well_var( resp, 1, "W_2", "WBHPH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2.1, ecl_sum_get_well_var( resp, 1, "W_3", "WBHPH" ), 1e-5 );

    /* THP (history) */
    BOOST_CHECK_CLOSE( 0.2, ecl_sum_get_well_var( resp, 1, "W_1", "WTHPH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 1.2, ecl_sum_get_well_var( resp, 1, "W_2", "WTHPH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2.2, ecl_sum_get_well_var( resp, 1, "W_3", "WTHPH" ), 1e-5 );
}

BOOST_AUTO_TEST_CASE(group_keywords) {
    setup cfg( "test_summary_group" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* Production rates */
    BOOST_CHECK_CLOSE( 10.0 + 20.0, ecl_sum_get_group_var( resp, 1, "G_1", "GWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 + 20.1, ecl_sum_get_group_var( resp, 1, "G_1", "GOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 + 20.2, ecl_sum_get_group_var( resp, 1, "G_1", "GGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.3 + 20.3, ecl_sum_get_group_var( resp, 1, "G_1", "GNPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.4 + 20.4, ecl_sum_get_group_var( resp, 1, "G_1", "GGPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE((10.2 - 10.4) + (20.2 - 20.4),
                                    ecl_sum_get_group_var( resp, 1, "G_1", "GGPRF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.5 + 20.5, ecl_sum_get_group_var( resp, 1, "G_1", "GOPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE((10.1 - 10.5) + (20.1 - 20.5),
                                    ecl_sum_get_group_var( resp, 1, "G_1", "GOPRF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.6 + 10.7 + 10.8 + 20.6 + 20.7 + 20.8,
                                    ecl_sum_get_group_var( resp, 1, "G_1", "GVPR" ), 1e-5 );

    /* Production totals */
    BOOST_CHECK_CLOSE( 10.0 + 20.0, ecl_sum_get_group_var( resp, 1, "G_1", "GWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 + 20.1, ecl_sum_get_group_var( resp, 1, "G_1", "GOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 + 20.2, ecl_sum_get_group_var( resp, 1, "G_1", "GGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.3 + 20.3, ecl_sum_get_group_var( resp, 1, "G_1", "GNPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.4 + 20.4, ecl_sum_get_group_var( resp, 1, "G_1", "GGPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.5 + 20.5, ecl_sum_get_group_var( resp, 1, "G_1", "GOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.1 - 10.5) + (20.1 - 20.5), ecl_sum_get_group_var( resp, 1, "G_1", "GOPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.2 - 10.4) + (20.2 - 20.4), ecl_sum_get_group_var( resp, 1, "G_1", "GGPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.6 + 10.7 + 10.8 + 20.6 + 20.7 + 20.8,
                                    ecl_sum_get_group_var( resp, 1, "G_1", "GVPT" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.0 + 20.0), ecl_sum_get_group_var( resp, 2, "G_1", "GWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.1 + 20.1), ecl_sum_get_group_var( resp, 2, "G_1", "GOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.2 + 20.2), ecl_sum_get_group_var( resp, 2, "G_1", "GGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.3 + 20.3), ecl_sum_get_group_var( resp, 2, "G_1", "GNPT" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.4 + 20.4), ecl_sum_get_group_var( resp, 2, "G_1", "GGPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.5 + 20.5), ecl_sum_get_group_var( resp, 2, "G_1", "GOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * ((10.2 - 10.4) + (20.2 - 20.4)), ecl_sum_get_group_var( resp, 2, "G_1", "GGPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * ((10.1 - 10.5) + (20.1 - 20.5)), ecl_sum_get_group_var( resp, 2, "G_1", "GOPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE(  2 * (10.6 + 10.7 + 10.8 + 20.6 + 20.7 + 20.8),
                                    ecl_sum_get_group_var( resp, 2, "G_1", "GVPT" ), 1e-5 );

    /* Production rates (history) */
    BOOST_CHECK_CLOSE( 10.0 + 20.0, ecl_sum_get_group_var( resp, 1, "G_1", "GWPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 + 20.1, ecl_sum_get_group_var( resp, 1, "G_1", "GOPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 + 20.2, ecl_sum_get_group_var( resp, 1, "G_1", "GGPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 + 10.1 + 20.0 + 20.1,
                                    ecl_sum_get_group_var( resp, 1, "G_1", "GLPRH" ), 1e-5 );

    /* Production totals (history) */
    BOOST_CHECK_CLOSE( (10.0 + 20.0), ecl_sum_get_group_var( resp, 1, "G_1", "GWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,             ecl_sum_get_group_var( resp, 1, "G_2", "GWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.1 + 20.1), ecl_sum_get_group_var( resp, 1, "G_1", "GOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,             ecl_sum_get_group_var( resp, 1, "G_2", "GOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.2 + 20.2), ecl_sum_get_group_var( resp, 1, "G_1", "GGPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,             ecl_sum_get_group_var( resp, 1, "G_2", "GGPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.0 + 20.0 + 10.1 + 20.1),
                                      ecl_sum_get_group_var( resp, 1, "G_1", "GLPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,             ecl_sum_get_group_var( resp, 1, "G_2", "GLPTH" ), 1e-5 );

    /* Production targets */
    BOOST_CHECK_CLOSE( 30.1 , ecl_sum_get_group_var( resp, 1, "G_3", "GVPRT" ), 1e-5 );

    /* Injection rates */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_group_var( resp, 1, "G_2", "GWIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_group_var( resp, 1, "G_2", "GGIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_group_var( resp, 1, "G_2", "GNIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5, ecl_sum_get_group_var( resp, 1, "G_2", "GCIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 2.5, ecl_sum_get_group_var( resp, 2, "G_2", "GCIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.6 + 30.7 + 30.8),
                       ecl_sum_get_group_var( resp, 1, "G_2", "GVIR" ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_group_var( resp, 1, "G_2", "GWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_group_var( resp, 1, "G_2", "GGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_group_var( resp, 1, "G_2", "GNIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5, ecl_sum_get_group_var( resp, 1, "G_2", "GCIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.6 + 30.7 + 30.8),
                       ecl_sum_get_group_var( resp, 1, "G_2", "GVIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.0, ecl_sum_get_group_var( resp, 2, "G_2", "GWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2, ecl_sum_get_group_var( resp, 2, "G_2", "GGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.3, ecl_sum_get_group_var( resp, 2, "G_2", "GNIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5 + 30.0 * 2.5, ecl_sum_get_group_var( resp, 2, "G_2", "GCIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (30.6 + 30.7 + 30.8),
                       ecl_sum_get_group_var( resp, 2, "G_2", "GVIT" ), 1e-5 );

    /* Injection totals (history) */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_group_var( resp, 1, "G_2", "GWITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,    ecl_sum_get_group_var( resp, 1, "G_2", "GGITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 60.0, ecl_sum_get_group_var( resp, 2, "G_2", "GWITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 0,    ecl_sum_get_group_var( resp, 2, "G_2", "GGITH" ), 1e-5 );

    /* gwct - water cut */
    const double gwcut1 = (10.0 + 20.0) / ( 10.0 + 10.1 + 20.0 + 20.1 );
    const double gwcut2 = 0;
    BOOST_CHECK_CLOSE( gwcut1, ecl_sum_get_group_var( resp, 1, "G_1", "GWCT" ), 1e-5 );
    BOOST_CHECK_CLOSE( gwcut2, ecl_sum_get_group_var( resp, 1, "G_2", "GWCT" ), 1e-5 );

    BOOST_CHECK_CLOSE( gwcut1, ecl_sum_get_group_var( resp, 1, "G_1", "GWCTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( gwcut2, ecl_sum_get_group_var( resp, 1, "G_2", "GWCTH" ), 1e-5 );

    /* ggor - gas-oil ratio */
    const double ggor1 = (10.2 + 20.2) / (10.1 + 20.1);
    const double ggor2 = 0;
    BOOST_CHECK_CLOSE( ggor1, ecl_sum_get_group_var( resp, 1, "G_1", "GGOR" ), 1e-5 );
    BOOST_CHECK_CLOSE( ggor2, ecl_sum_get_group_var( resp, 1, "G_2", "GGOR" ), 1e-5 );

    BOOST_CHECK_CLOSE( ggor1, ecl_sum_get_group_var( resp, 1, "G_1", "GGORH" ), 1e-5 );
    BOOST_CHECK_CLOSE( ggor2, ecl_sum_get_group_var( resp, 1, "G_2", "GGORH" ), 1e-5 );

    const double gglr1 = (10.2 + 20.2) / ( 10.0 + 10.1 + 20.0 + 20.1 );
    const double gglr2 = 0;
    BOOST_CHECK_CLOSE( gglr1, ecl_sum_get_group_var( resp, 1, "G_1", "GGLR" ), 1e-5 );
    BOOST_CHECK_CLOSE( gglr2, ecl_sum_get_group_var( resp, 1, "G_2", "GGLR" ), 1e-5 );

    BOOST_CHECK_CLOSE( gglr1, ecl_sum_get_group_var( resp, 1, "G_1", "GGLRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( gglr2, ecl_sum_get_group_var( resp, 1, "G_2", "GGLRH" ), 1e-5 );


    BOOST_CHECK_EQUAL( 0, ecl_sum_get_group_var( resp, 1, "G_1", "GMWIN" ) );
    BOOST_CHECK_EQUAL( 2, ecl_sum_get_group_var( resp, 1, "G_1", "GMWPR" ) );
    BOOST_CHECK_EQUAL( 1, ecl_sum_get_group_var( resp, 1, "G_2", "GMWIN" ) );
    BOOST_CHECK_EQUAL( 0, ecl_sum_get_group_var( resp, 1, "G_2", "GMWPR" ) );
}

BOOST_AUTO_TEST_CASE(group_group) {
    setup cfg( "test_summary_group_group" , "group_group.DATA");

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* Production rates */
    BOOST_CHECK_CLOSE( 10.0 , ecl_sum_get_well_var( resp, 1, "W_1", "WWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 , ecl_sum_get_group_var( resp, 1, "G_1", "GWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 , ecl_sum_get_well_var( resp, 1, "W_1", "WOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 , ecl_sum_get_group_var( resp, 1, "G_1", "GOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 , ecl_sum_get_well_var( resp, 1, "W_1", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 , ecl_sum_get_group_var( resp, 1, "G_1", "GGPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 20.0 , ecl_sum_get_well_var( resp, 1, "W_2", "WWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.0 , ecl_sum_get_group_var( resp, 1, "G_2", "GWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1 , ecl_sum_get_well_var( resp, 1, "W_2", "WOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1 , ecl_sum_get_group_var( resp, 1, "G_2", "GOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2 , ecl_sum_get_well_var( resp, 1, "W_2", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2 , ecl_sum_get_group_var( resp, 1, "G_2", "GGPR" ), 1e-5 );


    // Production totals
    for (int step = 1; step <= 2; step++) {
        BOOST_CHECK( ecl_sum_get_group_var( resp , step , "G_1" , "GWPT" ) == ecl_sum_get_well_var( resp , step , "W_1" , "WWPT"));
        BOOST_CHECK( ecl_sum_get_group_var( resp , step , "G_1" , "GOPT" ) == ecl_sum_get_well_var( resp , step , "W_1" , "WOPT"));
        BOOST_CHECK( ecl_sum_get_group_var( resp , step , "G_1" , "GGPT" ) == ecl_sum_get_well_var( resp , step , "W_1" , "WGPT"));

        BOOST_CHECK( ecl_sum_get_group_var( resp , step , "G_2" , "GWPT" ) == ecl_sum_get_well_var( resp , step , "W_2" , "WWPT"));
        BOOST_CHECK( ecl_sum_get_group_var( resp , step , "G_2" , "GOPT" ) == ecl_sum_get_well_var( resp , step , "W_2" , "WOPT"));
        BOOST_CHECK( ecl_sum_get_group_var( resp , step , "G_2" , "GGPT" ) == ecl_sum_get_well_var( resp , step , "W_2" , "WGPT"));
    }

    for (const auto& gvar : {"GGPR", "GOPR", "GWPR"})
        BOOST_CHECK_CLOSE( ecl_sum_get_group_var( resp , 1 , "G" , gvar) ,
                           ecl_sum_get_group_var( resp , 1 , "G_1" , gvar) + ecl_sum_get_group_var( resp , 1 , "G_2" , gvar) , 1e-5);

    for (int step = 1; step <= 2; step++) {
        for (const auto& gvar : {"GGPT", "GOPT", "GWPT"})
            BOOST_CHECK_CLOSE( ecl_sum_get_group_var( resp , step , "G" , gvar) ,
                               ecl_sum_get_group_var( resp , step , "G_1" , gvar) + ecl_sum_get_group_var( resp , step , "G_2" , gvar) , 1e-5);
    }
}



BOOST_AUTO_TEST_CASE(completion_kewords) {
    setup cfg( "test_summary_completion" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* Production rates */
    BOOST_CHECK_CLOSE( 100.0,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "CWPR", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 100.1,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "COPR", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 100.2,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "CGPR", 1 ), 1e-5 );

    /* Production totals */
    BOOST_CHECK_CLOSE( 100.0,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "CWPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 100.1,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "COPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 100.2,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "CGPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 100.3,     ecl_sum_get_well_completion_var( resp, 1, "W_1", "CNPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 100.0, ecl_sum_get_well_completion_var( resp, 2, "W_1", "CWPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 100.1, ecl_sum_get_well_completion_var( resp, 2, "W_1", "COPT", 1 ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 100.2, ecl_sum_get_well_completion_var( resp, 2, "W_1", "CGPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 200.2, ecl_sum_get_well_completion_var( resp, 2, "W_2", "CGPT", 2 ), 1e-5 );
    BOOST_CHECK_CLOSE( 0        , ecl_sum_get_well_completion_var( resp, 2, "W_3", "CGPT", 3 ), 1e-5 );

    BOOST_CHECK_CLOSE( 1 * 100.2, ecl_sum_get_well_completion_var( resp, 1, "W_1", "CGPT", 1 ), 1e-5 );
    BOOST_CHECK_CLOSE( 1 * 200.2, ecl_sum_get_well_completion_var( resp, 1, "W_2", "CGPT", 2 ), 1e-5 );
    BOOST_CHECK_CLOSE( 0        , ecl_sum_get_well_completion_var( resp, 1, "W_3", "CGPT", 3 ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 100.3, ecl_sum_get_well_completion_var( resp, 2, "W_1", "CNPT", 1 ), 1e-5 );

    /* Injection rates */
    BOOST_CHECK_CLOSE( 300.0,       ecl_sum_get_well_completion_var( resp, 1, "W_3", "CWIR", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.2,       ecl_sum_get_well_completion_var( resp, 1, "W_3", "CGIR", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.0 * 1.5, ecl_sum_get_well_completion_var( resp, 1, "W_3", "CCIR", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.0 * 2.5, ecl_sum_get_well_completion_var( resp, 2, "W_3", "CCIR", 3 ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 300.0,       ecl_sum_get_well_completion_var( resp, 1, "W_3", "CWIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.2,       ecl_sum_get_well_completion_var( resp, 1, "W_3", "CGIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.3,       ecl_sum_get_well_completion_var( resp, 1, "W_3", "CNIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.0 * 1.5, ecl_sum_get_well_completion_var( resp, 1, "W_3", "CCIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 300.0,   ecl_sum_get_well_completion_var( resp, 2, "W_3", "CWIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 300.2,   ecl_sum_get_well_completion_var( resp, 2, "W_3", "CGIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 300.3,   ecl_sum_get_well_completion_var( resp, 2, "W_3", "CNIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.0 * 1.5 + 300.0 * 2.5,
                                    ecl_sum_get_well_completion_var( resp, 2, "W_3", "CCIT", 3 ), 1e-5 );

    /* Solvent flow rate + or - Note OPM uses negative values for producers, while CNFR outputs positive
    values for producers*/
    BOOST_CHECK_CLOSE( -300.3,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CNFR", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE(  200.3,     ecl_sum_get_well_completion_var( resp, 1, "W_2", "CNFR", 2 ), 1e-5 );
}

BOOST_AUTO_TEST_CASE(field_keywords) {
    setup cfg( "test_summary_field" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* Production rates */
    BOOST_CHECK_CLOSE( 10.0 + 20.0, ecl_sum_get_field_var( resp, 1, "FWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 + 20.1, ecl_sum_get_field_var( resp, 1, "FOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 + 20.2, ecl_sum_get_field_var( resp, 1, "FGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 + 20.0 + 10.1 + 20.1,
                                    ecl_sum_get_field_var( resp, 1, "FLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.6 + 10.7 + 10.8 + 20.6 + 20.7 + 20.8,
                                    ecl_sum_get_field_var( resp, 1, "FVPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.4 + 20.4,
                                    ecl_sum_get_field_var( resp, 1, "FGPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 - 10.4 + 20.2 - 20.4,
                                    ecl_sum_get_field_var( resp, 1, "FGPRF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.5 + 20.5,
                                    ecl_sum_get_field_var( resp, 1, "FOPRS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 - 10.5 + 20.1 - 20.5,
                                    ecl_sum_get_field_var( resp, 1, "FOPRF" ), 1e-5 );

    /* Production totals */
    BOOST_CHECK_CLOSE( 10.0 + 20.0, ecl_sum_get_field_var( resp, 1, "FWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 + 20.1, ecl_sum_get_field_var( resp, 1, "FOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 + 20.2, ecl_sum_get_field_var( resp, 1, "FGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 + 20.0 + 10.1 + 20.1,
                                    ecl_sum_get_field_var( resp, 1, "FLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.6 + 10.7 + 10.8 + 20.6 + 20.7 + 20.8,
                                    ecl_sum_get_field_var( resp, 1, "FVPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.4 + 20.4,
                                    ecl_sum_get_field_var( resp, 1, "FGPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 - 10.4 + 20.2 - 20.4,
                                    ecl_sum_get_field_var( resp, 1, "FGPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.5 + 20.5,
                                    ecl_sum_get_field_var( resp, 1, "FOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 - 10.5 + 20.1 - 20.5,
                                    ecl_sum_get_field_var( resp, 1, "FOPTF" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * (10.0 + 20.0), ecl_sum_get_field_var( resp, 2, "FWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.1 + 20.1), ecl_sum_get_field_var( resp, 2, "FOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.2 + 20.2), ecl_sum_get_field_var( resp, 2, "FGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.0 + 20.0 + 10.1 + 20.1),
                                    ecl_sum_get_field_var( resp, 2, "FLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.6 + 10.7 + 10.8 + 20.6 + 20.7 + 20.8),
                                    ecl_sum_get_field_var( resp, 2, "FVPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.4 + 20.4),
                                    ecl_sum_get_field_var( resp, 2, "FGPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.2 - 10.4 + 20.2 - 20.4),
                                    ecl_sum_get_field_var( resp, 2, "FGPTF" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.5 + 20.5),
                                    ecl_sum_get_field_var( resp, 2, "FOPTS" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.1 - 10.5 + 20.1 - 20.5),
                                    ecl_sum_get_field_var( resp, 2, "FOPTF" ), 1e-5 );

    /* Production rates (history) */
    BOOST_CHECK_CLOSE( 10.0 + 20.0, ecl_sum_get_field_var( resp, 1, "FWPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.1 + 20.1, ecl_sum_get_field_var( resp, 1, "FOPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.2 + 20.2, ecl_sum_get_field_var( resp, 1, "FGPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 + 10.1 + 20.0 + 20.1,
                                    ecl_sum_get_field_var( resp, 1, "FLPRH" ), 1e-5 );

    /* Production totals (history) */
    BOOST_CHECK_CLOSE( (10.0 + 20.0), ecl_sum_get_field_var( resp, 1, "FWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.1 + 20.1), ecl_sum_get_field_var( resp, 1, "FOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.2 + 20.2), ecl_sum_get_field_var( resp, 1, "FGPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.0 + 20.0 + 10.1 + 20.1),
                                      ecl_sum_get_field_var( resp, 1, "FLPTH" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * (10.0 + 20.0), ecl_sum_get_field_var( resp, 2, "FWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.1 + 20.1), ecl_sum_get_field_var( resp, 2, "FOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.2 + 20.2), ecl_sum_get_field_var( resp, 2, "FGPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.0 + 20.0 + 10.1 + 20.1),
                                          ecl_sum_get_field_var( resp, 2, "FLPTH" ), 1e-5 );

    /* Injection rates */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_field_var( resp, 1, "FWIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_field_var( resp, 1, "FGIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.6 + 30.7 + 30.8, ecl_sum_get_field_var( resp, 1, "FVIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5, ecl_sum_get_field_var( resp, 1, "FCIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 2.5, ecl_sum_get_field_var( resp, 2, "FCIR" ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 30.0,     ecl_sum_get_field_var( resp, 1, "FWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2,     ecl_sum_get_field_var( resp, 1, "FGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.6 + 30.7 + 30.8, ecl_sum_get_field_var( resp, 1, "FVIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5,         ecl_sum_get_field_var( resp, 1, "FCIT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 30.0, ecl_sum_get_field_var( resp, 2, "FWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2, ecl_sum_get_field_var( resp, 2, "FGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (30.6 + 30.7 + 30.8), ecl_sum_get_field_var( resp, 2, "FVIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 * 1.5 + 30.0 * 2.5,  ecl_sum_get_field_var( resp, 2, "FCIT" ), 1e-5 );

    /* Injection totals (history) */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_field_var( resp, 1, "FWITH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 60.0, ecl_sum_get_field_var( resp, 2, "FWITH" ), 1e-5 );

    /* Production targets */
    BOOST_CHECK_CLOSE( 30.1 , ecl_sum_get_field_var( resp, 1, "FVPRT" ), 1e-5 );

    /* fwct - water cut */
    const double wcut = (10.0 + 20.0) / ( 10.0 + 10.1 + 20.0 + 20.1 );
    BOOST_CHECK_CLOSE( wcut, ecl_sum_get_field_var( resp, 1, "FWCT" ), 1e-5 );
    BOOST_CHECK_CLOSE( wcut, ecl_sum_get_field_var( resp, 1, "FWCTH" ), 1e-5 );

    /* ggor - gas-oil ratio */
    const double ggor = (10.2 + 20.2) / (10.1 + 20.1);
    BOOST_CHECK_CLOSE( ggor, ecl_sum_get_field_var( resp, 1, "FGOR" ), 1e-5 );
    BOOST_CHECK_CLOSE( ggor, ecl_sum_get_field_var( resp, 1, "FGORH" ), 1e-5 );

}

BOOST_AUTO_TEST_CASE(report_steps_time) {
    setup cfg( "test_summary_report_steps_time" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 1, 2 *  day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 5 *  day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 10 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK( ecl_sum_has_report_step( resp, 1 ) );
    BOOST_CHECK( ecl_sum_has_report_step( resp, 2 ) );
    BOOST_CHECK( !ecl_sum_has_report_step( resp, 3 ) );

    BOOST_CHECK_EQUAL( ecl_sum_iget_sim_days( resp, 0 ), 2 );
    BOOST_CHECK_EQUAL( ecl_sum_iget_sim_days( resp, 1 ), 5 );
    BOOST_CHECK_EQUAL( ecl_sum_iget_sim_days( resp, 2 ), 10 );
    BOOST_CHECK_EQUAL( ecl_sum_get_sim_length( resp ), 10 );
}

BOOST_AUTO_TEST_CASE(skip_unknown_var) {
    setup cfg( "test_summary_skip_unknown_var" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 1, 2 *  day, cfg.es,  cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 5 *  day, cfg.es,  cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 10 * day, cfg.es,  cfg.schedule, cfg.wells ,  {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* verify that some non-supported keywords aren't written to the file */
    BOOST_CHECK( !ecl_sum_has_well_var( resp, "W_1", "WPI" ) );
    BOOST_CHECK( !ecl_sum_has_field_var( resp, "FGST" ) );
}



BOOST_AUTO_TEST_CASE(region_vars) {
    setup cfg( "region_vars" );

    std::map<std::string, std::vector<double>> region_values;

    {
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            values[r - 1] = r *1.0;
        }
        region_values["RPR"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area *  2*r * 1.0;
        }
        region_values["ROIP"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area * 2.2*r * 1.0;
        }
        region_values["RWIP"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area *   2.1*r * 1.0;
        }
        region_values["RGIP"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area *  (2*r - 1) * 1.0;
        }
        region_values["ROIPL"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area *  (2*r + 1 ) * 1.0;
        }
        region_values["ROIPG"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area *  (2.1*r - 1) * 1.0;
        }
        region_values["RGIPL"] = values;
    }
    {
        double area = cfg.grid.getNX() * cfg.grid.getNY();
        std::vector<double> values(10, 0.0);
        for (size_t r=1; r <= 10; r++) {
            if (r == 10)
                area -= 1;

            values[r - 1] = area *  (2.1*r + 1) * 1.0;
        }
        region_values["RGIPG"] = values;
    }

    {
        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
        writer.add_timestep( 1, 2 *  day, cfg.es, cfg.schedule, cfg.wells,  {}, region_values);
        writer.add_timestep( 1, 5 *  day, cfg.es, cfg.schedule, cfg.wells,  {}, region_values);
        writer.add_timestep( 2, 10 * day, cfg.es, cfg.schedule, cfg.wells,  {}, region_values);
        writer.write();
    }

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK( ecl_sum_has_general_var( resp , "RPR:1"));
    BOOST_CHECK( ecl_sum_has_general_var( resp , "RPR:10"));
    BOOST_CHECK( !ecl_sum_has_general_var( resp , "RPR:11"));
    UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );

    for (size_t r=1; r <= 10; r++) {
        std::string rpr_key   = "RPR:"   + std::to_string( r );
        std::string roip_key  = "ROIP:"  + std::to_string( r );
        std::string rwip_key  = "RWIP:"  + std::to_string( r );
        std::string rgip_key  = "RGIP:"  + std::to_string( r );
        std::string roipl_key = "ROIPL:" + std::to_string( r );
        std::string roipg_key = "ROIPG:" + std::to_string( r );
        std::string rgipl_key = "RGIPL:" + std::to_string( r );
        std::string rgipg_key = "RGIPG:" + std::to_string( r );
        double area = cfg.grid.getNX() * cfg.grid.getNY();

        //BOOST_CHECK_CLOSE(   r * 1.0        , units.to_si( UnitSystem::measure::pressure , ecl_sum_get_general_var( resp, 1, rpr_key.c_str())) , 1e-5);

        // There is one inactive cell in the bottom layer.
        if (r == 10)
            area -= 1;

        BOOST_CHECK_CLOSE( area *  2*r * 1.0       , units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, roip_key.c_str())) , 1e-5);
        BOOST_CHECK_CLOSE( area * (2*r - 1) * 1.0  , units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, roipl_key.c_str())) , 1e-5);
        BOOST_CHECK_CLOSE( area * (2*r + 1 ) * 1.0 , units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, roipg_key.c_str())) , 1e-5);
        BOOST_CHECK_CLOSE( area *  2.1*r * 1.0     , units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, rgip_key.c_str())) , 1e-5);
        BOOST_CHECK_CLOSE( area * (2.1*r - 1) * 1.0, units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, rgipl_key.c_str())) , 1e-5);
        BOOST_CHECK_CLOSE( area * (2.1*r + 1) * 1.0, units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, rgipg_key.c_str())) , 1e-5);
        BOOST_CHECK_CLOSE( area *  2.2*r * 1.0     , units.to_si( UnitSystem::measure::volume   , ecl_sum_get_general_var( resp, 1, rwip_key.c_str())) , 1e-5);
    }
}


BOOST_AUTO_TEST_CASE(region_production) {
    setup cfg( "region_production" );

    {
        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
        writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
        writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
        writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
        writer.write();
    }

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK( ecl_sum_has_general_var( resp , "ROPR:1"));
    BOOST_CHECK_CLOSE(ecl_sum_get_general_var( resp , 1 , "ROPR:1" ) ,
                      ecl_sum_get_general_var( resp , 1 , "COPR:W_1:1") +
                      ecl_sum_get_general_var( resp , 1 , "COPR:W_2:2") +
                      ecl_sum_get_general_var( resp , 1 , "COPR:W_3:3"), 1e-5);



    BOOST_CHECK( ecl_sum_has_general_var( resp , "RGPT:1"));
    BOOST_CHECK_CLOSE(ecl_sum_get_general_var( resp , 2 , "RGPT:1" ) ,
                      ecl_sum_get_general_var( resp , 2 , "CGPT:W_1:1") +
                      ecl_sum_get_general_var( resp , 2 , "CGPT:W_2:2") +
                      ecl_sum_get_general_var( resp , 2 , "CGPT:W_3:3"), 1e-5);
}

BOOST_AUTO_TEST_CASE(region_injection) {
    setup cfg( "region_injection" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK( ecl_sum_has_general_var( resp , "RWIR:1"));
    BOOST_CHECK_CLOSE(ecl_sum_get_general_var( resp , 1 , "RWIR:1" ) ,
                      ecl_sum_get_general_var( resp , 1 , "CWIR:W_1:1") +
                      ecl_sum_get_general_var( resp , 1 , "CWIR:W_2:2") +
                      ecl_sum_get_general_var( resp , 1 , "CWIR:W_3:3"), 1e-5);



    BOOST_CHECK( ecl_sum_has_general_var( resp , "RGIT:1"));
    BOOST_CHECK_CLOSE(ecl_sum_get_general_var( resp , 2 , "RGIT:1" ) ,
                      ecl_sum_get_general_var( resp , 2 , "CGIT:W_1:1") +
                      ecl_sum_get_general_var( resp , 2 , "CGIT:W_2:2") +
                      ecl_sum_get_general_var( resp , 2 , "CGIT:W_3:3"), 1e-5);
}



BOOST_AUTO_TEST_CASE(BLOCK_VARIABLES) {
    setup cfg( "region_injection" );


    std::map<std::pair<std::string, int>, double> block_values;
    for (size_t r=1; r <= 10; r++) {
        block_values[std::make_pair("BPR", (r-1)*100 + 1)] = r*1.0;
    }
    block_values[std::make_pair("BSWAT", 1)] = 8.0;
    block_values[std::make_pair("BSGAS", 1)] = 9.0;

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {},{}, block_values);
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {},{}, block_values);
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  {},{}, block_values);
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    for (size_t r=1; r <= 10; r++) {
        std::string bpr_key   = "BPR:1,1,"   + std::to_string( r );
        BOOST_CHECK( ecl_sum_has_general_var( resp , bpr_key.c_str()));

        BOOST_CHECK_CLOSE( r * 1.0 , units.to_si( UnitSystem::measure::pressure , ecl_sum_get_general_var( resp, 1, bpr_key.c_str())) , 1e-5);

    }

    BOOST_CHECK_CLOSE( 8.0 , units.to_si( UnitSystem::measure::identity , ecl_sum_get_general_var( resp, 1, "BSWAT:1,1,1")) , 1e-5);
    BOOST_CHECK_CLOSE( 9.0 , units.to_si( UnitSystem::measure::identity , ecl_sum_get_general_var( resp, 1, "BSGAS:1,1,1")) , 1e-5);

    // Cell is not active
    BOOST_CHECK( !ecl_sum_has_general_var( resp , "BPR:2,1,10"));
}



/*
  The SummaryConfig.require3DField( ) implementation is slightly ugly:

  1. Which 3D fields are required is implicitly given by the
     implementation of the Summary() class here in opm-output.

  2. The implementation of the SummaryConfig.require3DField( ) is
     based on a hardcoded list in SummaryConfig.cpp - i.e. there is a
     inverse dependency between the opm-parser and opm-output modules.

  The test here just to ensure that *something* breaks if the
  opm-parser implementation is changed/removed.
*/



BOOST_AUTO_TEST_CASE( require3D )
{
    setup cfg( "XXXX" );
    const auto summaryConfig = cfg.config;

    BOOST_CHECK( summaryConfig.require3DField( "PRESSURE" ));
    BOOST_CHECK( summaryConfig.require3DField( "SGAS" ));
    BOOST_CHECK( summaryConfig.require3DField( "SWAT" ));
    BOOST_CHECK( summaryConfig.require3DField( "WIP" ));
    BOOST_CHECK( summaryConfig.require3DField( "GIP" ));
    BOOST_CHECK( summaryConfig.require3DField( "OIP" ));
    BOOST_CHECK( summaryConfig.require3DField( "OIPL" ));
    BOOST_CHECK( summaryConfig.require3DField( "OIPG" ));
    BOOST_CHECK( summaryConfig.require3DField( "GIPL" ));
    BOOST_CHECK( summaryConfig.require3DField( "GIPG" ));

    BOOST_CHECK( summaryConfig.requireFIPNUM( ));
}


BOOST_AUTO_TEST_CASE(MISC) {
    setup cfg( "test_misc");

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule , cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();
    BOOST_CHECK( ecl_sum_has_key( resp , "TCPU" ));
}


BOOST_AUTO_TEST_CASE(EXTRA) {
    setup cfg( "test_extra");

    {
        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule , cfg.name );
        writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells ,  { {"TCPU" , 0 }});
        writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells ,  { {"TCPU" , 1 }});
        writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells ,  { {"TCPU" , 2}});

        /* Add a not-recognized key; that is OK */
        BOOST_CHECK_NO_THROW(  writer.add_timestep( 3, 3 * day, cfg.es, cfg.schedule, cfg.wells ,  { {"MISSING" , 2 }}));

        /* Override a NOT MISC variable - ignored. */
        writer.add_timestep( 4, 4 * day, cfg.es, cfg.schedule, cfg.wells ,  { {"FOPR" , -1 }});
        writer.write();
    }

    auto res = readsum( cfg.name );
    const auto* resp = res.get();
    BOOST_CHECK( ecl_sum_has_key( resp , "TCPU" ));
    BOOST_CHECK_CLOSE( 1 , ecl_sum_get_general_var( resp , 1 , "TCPU") , 0.001);
    BOOST_CHECK_CLOSE( 2 , ecl_sum_get_general_var( resp , 2 , "TCPU") , 0.001);

    /* Not passed - should be zero. */
    BOOST_CHECK_CLOSE( 0 , ecl_sum_get_general_var( resp , 4 , "TCPU") , 0.001);

    /* Override a NOT MISC variable - ignored. */
    BOOST_CHECK(  ecl_sum_get_general_var( resp , 4 , "FOPR") > 0.0 );
}

struct MessageBuffer
{
  std::stringstream str_;

  template <class T>
  void read( T& value )
  {
    str_.read( (char *) &value, sizeof(value) );
  }

  template <class T>
  void write( const T& value )
  {
    str_.write( (char *) &value, sizeof(value) );
  }

  void write( const std::string& str)
  {
      int size = str.size();
      write(size);
      for (int k = 0; k < size; ++k) {
          write(str[k]);
      }
  }

  void read( std::string& str)
  {
      int size = 0;
      read(size);
      str.resize(size);
      for (int k = 0; k < size; ++k) {
          read(str[k]);
      }
  }

};

BOOST_AUTO_TEST_CASE(READ_WRITE_WELLDATA) {

            Opm::data::Wells wellRates = result_wells();

            MessageBuffer buffer;
            wellRates.write(buffer);

            Opm::data::Wells wellRatesCopy;
            wellRatesCopy.read(buffer);

            BOOST_CHECK_CLOSE( wellRatesCopy.get( "W_1" , rt::wat) , wellRates.get( "W_1" , rt::wat), 1e-16);
            BOOST_CHECK_CLOSE( wellRatesCopy.get( "W_2" , 101 , rt::wat) , wellRates.get( "W_2" , 101 , rt::wat), 1e-16);

}

BOOST_AUTO_TEST_CASE(efficiency_factor) {
        setup cfg( "test_efficiency_factor", "SUMMARY_EFF_FAC.DATA" );

        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
        writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells, {});
        writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells, {});
        writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells, {});
        writer.write();
        auto res = readsum( cfg.name );
        const auto* resp = res.get();

        /* No WEFAC assigned to W_1 */
        BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WOPR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WOPT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 2 * 10.1, ecl_sum_get_well_var( resp, 2, "W_1", "WOPT" ), 1e-5 );

        /* WEFAC 0.2 assigned to W_2.
         * W_2 assigned to group G2. GEFAC G2 = 0.01 */
        BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WOPR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 20.1 * 0.2 * 0.01, ecl_sum_get_well_var( resp, 1, "W_2", "WOPT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 2 * 20.1 * 0.2 * 0.01, ecl_sum_get_well_var( resp, 2, "W_2", "WOPT" ), 1e-5 );

        /* WEFAC 0.3 assigned to W_3.
         * W_3 assigned to group G3. GEFAC G_3 = 0.02
         * G_3 assigned to group G4. GEFAC G_4 = 0.03*/
        BOOST_CHECK_CLOSE( 30.1, ecl_sum_get_well_var( resp, 1, "W_3", "WOIR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03, ecl_sum_get_well_var( resp, 1, "W_3", "WOIT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03 + 30.1 * 0.3 * 0.02 * 0.04, ecl_sum_get_well_var( resp, 2, "W_3", "WOIT" ), 1e-5 );

        /* WEFAC 0.2 assigned to W_2.
         * W_2 assigned to group G2. GEFAC G2 = 0.01 */
        BOOST_CHECK_CLOSE( 20.1 * 0.2, ecl_sum_get_group_var( resp, 1, "G_2", "GOPR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 20.1 * 0.2 * 0.01, ecl_sum_get_group_var( resp, 1, "G_2", "GOPT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 2 * 20.1 * 0.2 * 0.01, ecl_sum_get_group_var( resp, 2, "G_2", "GOPT" ), 1e-5 );

        /* WEFAC 0.3 assigned to W_3.
         * W_3 assigned to group G3. GEFAC G_3 = 0.02
         * G_3 assigned to group G4. GEFAC G_4 = 0.03*/
        BOOST_CHECK_CLOSE( 30.1 * 0.3, ecl_sum_get_group_var( resp, 1, "G_3", "GOIR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03, ecl_sum_get_group_var( resp, 1, "G_3", "GOIT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03 + 30.1 * 0.3 * 0.02 * 0.04, ecl_sum_get_group_var( resp, 2, "G_3", "GOIT" ), 1e-5 );

        /* WEFAC 0.3 assigned to W_3.
         * W_3 assigned to group G3. GEFAC G_3 = 0.02
         * G_3 assigned to group G4. GEFAC G_4 = 0.03
         * The rate for a group is calculated including WEFAC and GEFAC for subgroups */
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02, ecl_sum_get_group_var( resp, 1, "G_4", "GOIR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03, ecl_sum_get_group_var( resp, 1, "G_4", "GOIT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03 + 30.1 * 0.3 * 0.02 * 0.04, ecl_sum_get_group_var( resp, 2, "G_4", "GOIT" ), 1e-5 );

        BOOST_CHECK_CLOSE( 10.1 + 20.1 * 0.2 * 0.01, ecl_sum_get_field_var( resp, 1, "FOPR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 10.1 + 20.1 * 0.2 * 0.01, ecl_sum_get_field_var( resp, 1, "FOPT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 2 * (10.1 + 20.1 * 0.2 * 0.01), ecl_sum_get_field_var( resp, 2, "FOPT" ), 1e-5 );

        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03, ecl_sum_get_field_var( resp, 1, "FOIR" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03, ecl_sum_get_field_var( resp, 1, "FOIT" ), 1e-5 );
        BOOST_CHECK_CLOSE( 30.1 * 0.3 * 0.02 * 0.03 + 30.1 * 0.3 * 0.02 * 0.04, ecl_sum_get_field_var( resp, 2, "FOIT" ), 1e-5 );

        BOOST_CHECK_CLOSE( 200.1 * 0.2 * 0.01, ecl_sum_get_general_var( resp , 1 , "ROPR:1" ) , 1e-5);

        BOOST_CHECK_CLOSE( 100.1, ecl_sum_get_general_var( resp , 1 , "ROPR:2" ) , 1e-5);

        BOOST_CHECK_CLOSE( 300 * 0.2 * 0.01, ecl_sum_get_general_var( resp , 1 , "RWIR:1" ) , 1e-5);

        BOOST_CHECK_CLOSE( 200.1, ecl_sum_get_well_completion_var( resp, 1, "W_2", "COPR", 2 ), 1e-5 );
        BOOST_CHECK_CLOSE( 200.1 * 0.2 * 0.01, ecl_sum_get_well_completion_var( resp, 1, "W_2", "COPT", 2 ), 1e-5 );
}




BOOST_AUTO_TEST_CASE(Test_SummaryState) {
    Opm::SummaryState st;
    st.add("WWCT:OP_2", 100);
    BOOST_CHECK_CLOSE(st.get("WWCT:OP_2"), 100, 1e-5);
    BOOST_CHECK_THROW(st.get("NO_SUCH_KEY"), std::invalid_argument);
    BOOST_CHECK(st.has("WWCT:OP_2"));
    BOOST_CHECK(!st.has("NO_SUCH_KEY"));
}

BOOST_AUTO_TEST_SUITE_END()

// ####################################################################

namespace {
    Opm::SummaryState calculateRestartVectors(const setup& config)
    {
        ::Opm::out::Summary smry {
            config.es, config.config, config.grid,
            config.schedule, "Ignore.This"
        };

        smry.add_timestep(0, 0*day, config.es, config.schedule, config.wells, {});
        smry.add_timestep(1, 1*day, config.es, config.schedule, config.wells, {});
        smry.add_timestep(2, 2*day, config.es, config.schedule, config.wells, {});

        return smry.get_restart_vectors();
    }

    auto calculateRestartVectors()
        -> decltype(calculateRestartVectors({"test.Restart"}))
    {
        return calculateRestartVectors({"test.Restart"});
    }

    auto calculateRestartVectorsEffFac()
        -> decltype(calculateRestartVectors({"test.Restart.EffFac",
                                             "SUMMARY_EFF_FAC.DATA"}))
    {
        return calculateRestartVectors({
            "test.Restart.EffFac", "SUMMARY_EFF_FAC.DATA"
        });
    }

    std::vector<std::string> restartVectors()
    {
        return {
            "WPR", "OPR", "GPR", "VPR",
            "WPT", "OPT", "GPT", "VPT",
            "WIR", "GIR", "WIT", "GIT",
            "GOR", "WCT",
        };
    }

    std::vector<std::string> activeWells()
    {
        return { "W_1", "W_2", "W_3" };
    }

    std::vector<std::string> activeGroups()
    {
        return { "G_1", "G_2" };
    }

    std::vector<std::string> activeGroupsEffFac()
    {
        return { "G_1", "G", "G_2", "G_3", "G_4" };
    }
}

// ====================================================================

BOOST_AUTO_TEST_SUITE(Restart)

BOOST_AUTO_TEST_CASE(Well_Vectors_Present)
{
    const auto rstrt = calculateRestartVectors();

    for (const auto& vector : restartVectors()) {
        for (const auto& w : activeWells()) {
            BOOST_CHECK( rstrt.has("W" + vector + ':' + w));
            BOOST_CHECK(!rstrt.has("W" + vector));
        }
    }

    for (const auto& w : activeWells()) {
        BOOST_CHECK( rstrt.has("WBHP:" + w));
        BOOST_CHECK(!rstrt.has("WBHP"));
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Well_Vectors_Correct)
{
    const auto rstrt = calculateRestartVectors();

    // W_1 (Producer)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("WWPR:W_1"), 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPR:W_1"), 10.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPR:W_1"), 10.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPR:W_1"), 10.6 + 10.7 + 10.8, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("WWPT:W_1"), 2 * 1.0 * 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPT:W_1"), 2 * 1.0 * 10.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPT:W_1"), 2 * 1.0 * 10.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPT:W_1"), 2 * 1.0 * (10.6 + 10.7 + 10.8), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("WWIR:W_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIR:W_1"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("WWIT:W_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIT:W_1"), 0.0, 1.0e-10);

        // BHP
        BOOST_CHECK_CLOSE(rstrt.get("WBHP:W_1"), 0.1, 1.0e-10);  // Bars

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("WWCT:W_1"), 10.0 / (10.0 + 10.1), 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("WGOR:W_1"), 10.2 / 10.1, 1.0e-10);
    }

    // W_2 (Producer)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("WWPR:W_2"), 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPR:W_2"), 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPR:W_2"), 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPR:W_2"), 20.6 + 20.7 + 20.8, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("WWPT:W_2"), 2 * 1.0 * 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPT:W_2"), 2 * 1.0 * 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPT:W_2"), 2 * 1.0 * 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPT:W_2"), 2 * 1.0 * (20.6 + 20.7 + 20.8), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("WWIR:W_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIR:W_2"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("WWIT:W_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIT:W_2"), 0.0, 1.0e-10);

        // BHP
        BOOST_CHECK_CLOSE(rstrt.get("WBHP:W_2"), 1.1, 1.0e-10);  // Bars

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("WWCT:W_2"), 20.0 / (20.0 + 20.1), 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("WGOR:W_2"), 20.2 / 20.1, 1.0e-10);
    }

    // W_3 (Injector)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("WWPR:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPR:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPR:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPR:W_3"), 0.0, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("WWPT:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPT:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPT:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPT:W_3"), 0.0, 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("WWIR:W_3"), 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIR:W_3"), 30.2, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("WWIT:W_3"), 2 * 1.0 * 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIT:W_3"), 2 * 1.0 * 30.2, 1.0e-10);

        // BHP
        BOOST_CHECK_CLOSE(rstrt.get("WBHP:W_3"), 2.1, 1.0e-10);  // Bars

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("WWCT:W_3"), 0.0, 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("WGOR:W_3"), 0.0, 1.0e-10);
    }
}

// ====================================================================

BOOST_AUTO_TEST_CASE(Group_Vectors_Present)
{
    const auto& rstrt = calculateRestartVectors();

    for (const auto& vector : restartVectors()) {
        for (const auto& g : activeGroups()) {
            BOOST_CHECK( rstrt.has("G" + vector + ':' + g));
            BOOST_CHECK(!rstrt.has("G" + vector));
        }
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Group_Vectors_Correct)
{
    const auto rstrt = calculateRestartVectors();

    // G_1 (Producer, W_1 + W_2)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G_1"), 10.0 + 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G_1"), 10.1 + 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G_1"), 10.2 + 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G_1"),
                          (10.6 + 10.7 + 10.8) +
                          (20.6 + 20.7 + 20.8), 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G_1"), 2 * 1.0 * (10.0 + 20.0), 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G_1"), 2 * 1.0 * (10.1 + 20.1), 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G_1"), 2 * 1.0 * (10.2 + 20.2), 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G_1"),
                          2 * 1.0 *
                          ((10.6 + 10.7 + 10.8) +
                           (20.6 + 20.7 + 20.8)), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G_1"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G_1"), 0.0, 1.0e-10);

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G_1"),
                          (10.0 + 20.0) / ((10.0 + 10.1) + (20.0 + 20.1)), 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G_1"),
                          (10.2 + 20.2) / (10.1 + 20.1), 1.0e-10);
    }

    // G_2 (Injector, W_3)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G_2"), 0.0, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G_2"), 0.0, 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G_2"), 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G_2"), 30.2, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G_2"), 2 * 1.0 * 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G_2"), 2 * 1.0 * 30.2, 1.0e-10);

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G_2"), 0.0, 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G_2"), 0.0, 1.0e-10);
    }
}

// ====================================================================

BOOST_AUTO_TEST_CASE(Field_Vectors_Present)
{
    const auto& rstrt = calculateRestartVectors();

    for (const auto& vector : restartVectors()) {
        BOOST_CHECK( rstrt.has("F" + vector));
        BOOST_CHECK(!rstrt.has("F" + vector + ":FIELD"));
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Field_Vectors_Correct)
{
    const auto rstrt = calculateRestartVectors();

    // Production rates (F = G_1 = W_1 + W_2)
    BOOST_CHECK_CLOSE(rstrt.get("FWPR"), 10.0 + 20.0, 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FOPR"), 10.1 + 20.1, 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FGPR"), 10.2 + 20.2, 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FVPR"),
                      (10.6 + 10.7 + 10.8) +
                      (20.6 + 20.7 + 20.8), 1.0e-10);

    // Production cumulative totals (F = G_1 = W_1 + W_2)
    BOOST_CHECK_CLOSE(rstrt.get("FWPT"), 2 * 1.0 * (10.0 + 20.0), 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FOPT"), 2 * 1.0 * (10.1 + 20.1), 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FGPT"), 2 * 1.0 * (10.2 + 20.2), 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FVPT"),
                      2 * 1.0 *
                      ((10.6 + 10.7 + 10.8) +
                       (20.6 + 20.7 + 20.8)), 1.0e-10);

    // Injection rates (F = G_2 = W_3)
    BOOST_CHECK_CLOSE(rstrt.get("FWIR"), 30.0, 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FGIR"), 30.2, 1.0e-10);

    // Injection totals (F = G_2 = W_3)
    BOOST_CHECK_CLOSE(rstrt.get("FWIT"), 2 * 1.0 * 30.0, 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FGIT"), 2 * 1.0 * 30.2, 1.0e-10);

    // Water cut (F = G_1 = W_1 + W_2)
    BOOST_CHECK_CLOSE(rstrt.get("FWCT"),
                      (10.0 + 20.0) / ((10.0 + 10.1) + (20.0 + 20.1)), 1.0e-10);

    // Producing gas/oil ratio (F = G_1 = W_1 + W_2)
    BOOST_CHECK_CLOSE(rstrt.get("FGOR"),
                      (10.2 + 20.2) / (10.1 + 20.1), 1.0e-10);
}

BOOST_AUTO_TEST_SUITE_END()

// ####################################################################

BOOST_AUTO_TEST_SUITE(Restart_EffFac)

BOOST_AUTO_TEST_CASE(Well_Vectors_Correct)
{
    const auto rstrt = calculateRestartVectorsEffFac();

    // W_1 (Producer, efficiency factor = 1--no difference)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("WWPR:W_1"), 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPR:W_1"), 10.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPR:W_1"), 10.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPR:W_1"), 10.6 + 10.7 + 10.8, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("WWPT:W_1"), 2 * 1.0 * 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPT:W_1"), 2 * 1.0 * 10.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPT:W_1"), 2 * 1.0 * 10.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPT:W_1"), 2 * 1.0 * (10.6 + 10.7 + 10.8), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("WWIR:W_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIR:W_1"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("WWIT:W_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIT:W_1"), 0.0, 1.0e-10);

        // BHP
        BOOST_CHECK_CLOSE(rstrt.get("WBHP:W_1"), 0.1, 1.0e-10);  // Bars

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("WWCT:W_1"), 10.0 / (10.0 + 10.1), 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("WGOR:W_1"), 10.2 / 10.1, 1.0e-10);
    }

    // W_2 (Producer, efficiency factor = 0.2)
    {
        const auto wefac = 0.2;
        const auto gefac = 0.01;

        // Production rates (unaffected by WEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("WWPR:W_2"), 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPR:W_2"), 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPR:W_2"), 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPR:W_2"), (20.6 + 20.7 + 20.8), 1.0e-10);

        // Production cumulative totals (affected by WEFAC and containing GEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("WWPT:W_2"), 2 * 1.0 * wefac * gefac * 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPT:W_2"), 2 * 1.0 * wefac * gefac * 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPT:W_2"), 2 * 1.0 * wefac * gefac * 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPT:W_2"), 2 * 1.0 * wefac * gefac * (20.6 + 20.7 + 20.8), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("WWIR:W_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIR:W_2"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("WWIT:W_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIT:W_2"), 0.0, 1.0e-10);

        // BHP
        BOOST_CHECK_CLOSE(rstrt.get("WBHP:W_2"), 1.1, 1.0e-10);  // Bars

        // Water cut (unaffected by WEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("WWCT:W_2"), 20.0 / (20.0 + 20.1), 1.0e-10);

        // Producing gas/oil ratio (unaffected by WEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("WGOR:W_2"), 20.2 / 20.1, 1.0e-10);
    }

    // W_3 (Injector, efficiency factor = 0.3)
    {
        const auto wefac = 0.3;
        const auto gefac = 0.02; // G_3

        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("WWPR:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPR:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPR:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPR:W_3"), 0.0, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("WWPT:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WOPT:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGPT:W_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WVPT:W_3"), 0.0, 1.0e-10);

        // Injection rates (unaffected by WEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("WWIR:W_3"), 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("WGIR:W_3"), 30.2, 1.0e-10);

        // Injection totals (affected by WEFAC and containing GEFAC)
        //    GEFAC(G_4) = 0.03 at sim_step = 1
        //    GEFAC(G_4) = 0.04 at sim_step = 2
        BOOST_CHECK_CLOSE(rstrt.get("WWIT:W_3"),
                          30.0 * wefac * gefac *
                          ((1.0 * 0.03) + (1.0 * 0.04)), 1.0e-10);

        BOOST_CHECK_CLOSE(rstrt.get("WGIT:W_3"),
                          30.2 * wefac * gefac *
                          ((1.0 * 0.03) + (1.0 * 0.04)), 1.0e-10);

        // BHP
        BOOST_CHECK_CLOSE(rstrt.get("WBHP:W_3"), 2.1, 1.0e-10);  // Bars

        // Water cut (zero for injectors)
        BOOST_CHECK_CLOSE(rstrt.get("WWCT:W_3"), 0.0, 1.0e-10);

        // Producing gas/oil ratio (zero for injectors)
        BOOST_CHECK_CLOSE(rstrt.get("WGOR:W_3"), 0.0, 1.0e-10);
    }
}

// ====================================================================

BOOST_AUTO_TEST_CASE(Group_Vectors_Present)
{
    const auto& rstrt = calculateRestartVectorsEffFac();

    for (const auto& vector : restartVectors()) {
        for (const auto& g : activeGroupsEffFac()) {
            BOOST_CHECK( rstrt.has("G" + vector + ':' + g));
            BOOST_CHECK(!rstrt.has("G" + vector));
        }
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Group_Vectors_Correct)
{
    const auto rstrt = calculateRestartVectorsEffFac();

    // G_1 (Producer, W_1, GEFAC = 1--no change)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G_1"), 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G_1"), 10.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G_1"), 10.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G_1"), (10.6 + 10.7 + 10.8), 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G_1"), 2 * 1.0 * 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G_1"), 2 * 1.0 * 10.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G_1"), 2 * 1.0 * 10.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G_1"),
                          2 * 1.0 * (10.6 + 10.7 + 10.8), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G_1"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G_1"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G_1"), 0.0, 1.0e-10);

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G_1"),
                          10.0 / (10.0 + 10.1), 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G_1"),
                          10.2 / 10.1, 1.0e-10);
    }

    // G_2 (Producer, W_2, GEFAC = 0.01)
    {
        const auto wefac = 0.2;
        const auto gefac = 0.01;

        // Production rates (affected by WEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G_2"), wefac * 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G_2"), wefac * 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G_2"), wefac * 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G_2"), wefac * (20.6 + 20.7 + 20.8), 1.0e-10);

        // Production cumulative totals (affected by both WEFAC and GEFAC)
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G_2"), 2 * 1.0 * gefac * wefac * 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G_2"), 2 * 1.0 * gefac * wefac * 20.1, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G_2"), 2 * 1.0 * gefac * wefac * 20.2, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G_2"), 2 * 1.0 * gefac * wefac * (20.6 + 20.7 + 20.8), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G_2"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G_2"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G_2"), 0.0, 1.0e-10);

        // Water cut (unaffected by WEFAC or GEFAC since G_2 = W_2)
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G_2"), 20.0 / (20.0 + 20.1), 1.0e-10);

        // Producing gas/oil ratio (unaffected by WEFAC or GEFAC since G_2 = W_2)
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G_2"), 20.2 / 20.1, 1.0e-10);
    }

    // G (Producer, G_1 + G_2)
    {
        const auto gwefac = 0.01 * 0.2;

        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G"), 10.0 + (gwefac * 20.0), 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G"), 10.1 + (gwefac * 20.1), 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G"), 10.2 + (gwefac * 20.2), 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G"),
                                    (10.6 + 10.7 + 10.8) +
                          (gwefac * (20.6 + 20.7 + 20.8)), 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G"),
                          2 * 1.0 * (10.0 + (gwefac * 20.0)), 1.0e-10);

        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G"),
                          2 * 1.0 * (10.1 + (gwefac * 20.1)), 1.0e-10);

        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G"),
                          2 * 1.0 * (10.2 + (gwefac * 20.2)), 1.0e-10);

        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G"),
                          2 * 1.0 *
                         (          (10.6 + 10.7 + 10.8) +
                          (gwefac * (20.6 + 20.7 + 20.8))), 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G"), 0.0, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G"), 0.0, 1.0e-10);

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G"),
                          (10.0 +        (gwefac *  20.0)) /
                          (10.0 + 10.1 + (gwefac * (20.0 + 20.1))), 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G"),
                          (10.2 + (gwefac * 20.2)) /
                          (10.1 + (gwefac * 20.1)), 1.0e-10);
    }

    // G_3 (Injector, W_3)
    {
        const auto wefac   = 0.3;
        const auto gefac_3 = 0.02;

        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G_3"), 0.0, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G_3"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G_3"), 0.0, 1.0e-10);

        // Injection rates
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G_3"), wefac * 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G_3"), wefac * 30.2, 1.0e-10);

        // Injection totals
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G_3"),
                          30.0 * gefac_3 * wefac *
                          ((1.0 * 0.03) + (1.0 * 0.04)), 1.0e-10);

        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G_3"),
                          30.2 * gefac_3 * wefac *
                          ((1.0 * 0.03) + (1.0 * 0.04)), 1.0e-10);

        // Water cut (zero for injectors)
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G_3"), 0.0, 1.0e-10);

        // Producing gas/oil ratio (zero for injectors)
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G_3"), 0.0, 1.0e-10);
    }

    // G_4 (Injector, G_3, GEFAC = 0.03 and 0.04)
    {
        // Production rates
        BOOST_CHECK_CLOSE(rstrt.get("GWPR:G_4"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPR:G_4"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPR:G_4"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPR:G_4"), 0.0, 1.0e-10);

        // Production cumulative totals
        BOOST_CHECK_CLOSE(rstrt.get("GWPT:G_4"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GOPT:G_4"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGPT:G_4"), 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GVPT:G_4"), 0.0, 1.0e-10);

        // Injection rates (at sim_step = 2)
        BOOST_CHECK_CLOSE(rstrt.get("GWIR:G_4"), 0.02 * 0.3 * 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(rstrt.get("GGIR:G_4"), 0.02 * 0.3 * 30.2, 1.0e-10);

        // Injection totals (GEFAC(G_4) = 0.03 at sim_step = 1,
        //                   GEFAC(G_4) = 0.04 at sim_step = 2)
        BOOST_CHECK_CLOSE(rstrt.get("GWIT:G_4"),
                          30.0 * 0.3 * 0.02 *
                          ((0.03 * 1.0) + (0.04 * 1.0)), 1.0e-10);

        BOOST_CHECK_CLOSE(rstrt.get("GGIT:G_4"),
                          30.2 * 0.3 * 0.02 *
                          ((0.03 * 1.0) + (0.04 * 1.0)), 1.0e-10);

        // Water cut
        BOOST_CHECK_CLOSE(rstrt.get("GWCT:G_4"), 0.0, 1.0e-10);

        // Producing gas/oil ratio
        BOOST_CHECK_CLOSE(rstrt.get("GGOR:G_4"), 0.0, 1.0e-10);
    }
}

// ====================================================================

BOOST_AUTO_TEST_CASE(Field_Vectors_Correct)
{
    const auto rstrt = calculateRestartVectorsEffFac();

    // Field = G + G_4
    const auto efac_G = 0.01 * 0.2;

    BOOST_CHECK_CLOSE(rstrt.get("FWPR"), 10.0 + (efac_G * 20.0), 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FOPR"), 10.1 + (efac_G * 20.1), 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FGPR"), 10.2 + (efac_G * 20.2), 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FVPR"),
                                (10.6 + 10.7 + 10.8) +
                      (efac_G * (20.6 + 20.7 + 20.8)), 1.0e-10);

    // Production cumulative totals
    BOOST_CHECK_CLOSE(rstrt.get("FWPT"),
                      2 * 1.0 * (10.0 + (efac_G * 20.0)), 1.0e-10);

    BOOST_CHECK_CLOSE(rstrt.get("FOPT"),
                      2 * 1.0 * (10.1 + (efac_G * 20.1)), 1.0e-10);

    BOOST_CHECK_CLOSE(rstrt.get("FGPT"),
                      2 * 1.0 * (10.2 + (efac_G * 20.2)), 1.0e-10);

    BOOST_CHECK_CLOSE(rstrt.get("FVPT"),
                      2 * 1.0 *
                      (          (10.6 + 10.7 + 10.8) +
                       (efac_G * (20.6 + 20.7 + 20.8))), 1.0e-10);

    // Injection rates (at sim_step = 2, GEFAC(G_4) = 0.04)
    BOOST_CHECK_CLOSE(rstrt.get("FWIR"), 0.02 * 0.04 * 0.3 * 30.0, 1.0e-10);
    BOOST_CHECK_CLOSE(rstrt.get("FGIR"), 0.02 * 0.04 * 0.3 * 30.2, 1.0e-10);

    // Injection totals (GEFAC(G_4) = 0.03 at sim_step = 1,
    //                   GEFAC(G_4) = 0.04 at sim_step = 2)
    BOOST_CHECK_CLOSE(rstrt.get("FWIT"),
                      30.0 * 0.3 * 0.02 *
                      ((0.03 * 1.0) + (0.04 * 1.0)), 1.0e-10);

    BOOST_CHECK_CLOSE(rstrt.get("FGIT"),
                      30.2 * 0.3 * 0.02 *
                      ((0.03 * 1.0) + (0.04 * 1.0)), 1.0e-10);

    // Water cut
    BOOST_CHECK_CLOSE(rstrt.get("FWCT"),
                      (10.0 +        (efac_G *  20.0)) /
                      (10.0 + 10.1 + (efac_G * (20.0 + 20.1))), 1.0e-10);

    // Producing gas/oil ratio
    BOOST_CHECK_CLOSE(rstrt.get("FGOR"),
                      (10.2 + (efac_G * 20.2)) /
                      (10.1 + (efac_G * 20.1)), 1.0e-10);
}

BOOST_AUTO_TEST_SUITE_END()
