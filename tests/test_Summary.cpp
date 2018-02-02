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

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE Wells
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdexcept>

#include <ert/ecl/ecl_sum.h>
#include <ert/util/util.h>
#include <ert/util/TestArea.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/data/Cells.hpp>
#include <opm/output/eclipse/Summary.hpp>

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


static data::Solution make_solution( const EclipseGrid& grid ) {
    int numCells = grid.getNumActive( );
    data::Solution sol = {
        {"TEMP" , { UnitSystem::measure::temperature, std::vector<double>( numCells ), data::TargetType::RESTART_SOLUTION} },
        {"SWAT" , { UnitSystem::measure::identity,    std::vector<double>( numCells ), data::TargetType::RESTART_SOLUTION} },
        {"SGAS" , { UnitSystem::measure::identity,    std::vector<double>( numCells ), data::TargetType::RESTART_SOLUTION} }};


    sol.data("TEMP").assign( numCells, 7.0 );
    sol.data("SWAT").assign( numCells, 8.0 );
    sol.data("SGAS").assign( numCells, 9.0 );


    {
        std::vector<double> pres(numCells);
        std::vector<double> roip(numCells);
        std::vector<double> roipl(numCells);
        std::vector<double> roipg(numCells);
        std::vector<double> rgip(numCells);
        std::vector<double> rgipl(numCells);
        std::vector<double> rgipg(numCells);
        std::vector<double> rwip(numCells);

        size_t g = 0;
        for (size_t k=0; k < grid.getNZ(); k++) {
            for (size_t j=0; j < grid.getNY(); j++) {
                for (size_t i=0; i < grid.getNX(); i++) {
                    if (grid.cellActive(i,j,k)) {
                        pres[g] = 1.0*(k + 1);
                        roip[g] = 2.0*(k + 1);
                        roipl[g] = roip[g] - 1;
                        roipg[g] = roip[g] + 1;
                        rgip[g] = 2.1*(k + 1);
                        rgipl[g] = rgip[g] - 1;
                        rgipg[g] = rgip[g] + 1;
                        rwip[g] = 2.2*(k + 1);
                        g++;
                    }
                }
            }
        }

        sol.insert( "PRESSURE", UnitSystem::measure::pressure , pres , data::TargetType::RESTART_SOLUTION);
        sol.insert( "OIP"     , UnitSystem::measure::volume   , roip , data::TargetType::RESTART_AUXILIARY);
        sol.insert( "OIPL"    , UnitSystem::measure::volume   , roipl, data::TargetType::RESTART_AUXILIARY);
        sol.insert( "OIPG"    , UnitSystem::measure::volume   , roipg, data::TargetType::RESTART_AUXILIARY);
        sol.insert( "GIP"     , UnitSystem::measure::volume   , rgip , data::TargetType::RESTART_AUXILIARY);
        sol.insert( "GIPL"    , UnitSystem::measure::volume   , rgipl, data::TargetType::RESTART_AUXILIARY);
        sol.insert( "GIPG"    , UnitSystem::measure::volume   , rgipg, data::TargetType::RESTART_AUXILIARY);
        sol.insert( "WIP"     , UnitSystem::measure::volume   , rwip , data::TargetType::RESTART_AUXILIARY);
    }
    return sol;
}


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
    crates2.set( rt::reservoir_water, 300.6 / day );
    crates2.set( rt::reservoir_oil, 300.7 / day );
    crates2.set( rt::reservoir_gas, 300.8 / day );

    /*
      The active index assigned to the completion must be manually
      syncronized with the active index in the COMPDAT keyword in the
      input deck.
    */
    data::Completion well1_comp1 { 0  , crates1, 1.9 , 123.4};
    data::Completion well2_comp1 { 1  , crates2, 1.10 , 123.4};
    data::Completion well2_comp2 { 101, crates3, 1.11 , 123.4};
    data::Completion well3_comp1 { 2  , crates3, 1.11 , 123.4};

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

    data::Solution solution;
    std::unordered_map<int , std::vector<size_t>> cells;

    setup( const std::string& fname , const char* path = "summary_deck.DATA", const ParseContext& parseContext = ParseContext( )) :
        deck( Parser().parseFile( path, parseContext ) ),
        es( deck, ParseContext() ),
        grid( es.getInputGrid() ),
        schedule( deck, grid, es.get3DProperties(), es.runspec().phases(), parseContext),
        config( deck, schedule, es.getTableManager(), parseContext ),
        wells( result_wells() ),
        name( fname ),
        ta( ERT::TestArea("test_summary") ),
        solution( make_solution( grid ) )
    {
    }

};


/*
 * Tests works by reading the Deck, write the summary output, then immediately
 * read it again (with ERT), and compare the read values with the input.
 */
BOOST_AUTO_TEST_CASE(well_keywords) {
    setup cfg( "test_Summary_well" );

    // Force to run in a directory, to make sure the basename with
    // leading path works.
    util_make_path( "PATH" );
    cfg.name = "PATH/CASE";

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule , cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* Production rates */
    BOOST_CHECK_CLOSE( 10.0, ecl_sum_get_well_var( resp, 1, "W_1", "WWPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 20.0, ecl_sum_get_well_var( resp, 1, "W_2", "WWPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WOPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WOPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.2, ecl_sum_get_well_var( resp, 1, "W_1", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2, ecl_sum_get_well_var( resp, 1, "W_2", "WGPR" ), 1e-5 );
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
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_well_var( resp, 1, "W_3", "WGIR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_well_var( resp, 1, "W_3", "WNIR" ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_well_var( resp, 1, "W_3", "WWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_well_var( resp, 1, "W_3", "WGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_well_var( resp, 1, "W_3", "WNIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.6 + 30.7 + 30.8),
                       ecl_sum_get_well_var( resp, 1, "W_3", "WVIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.0, ecl_sum_get_well_var( resp, 2, "W_3", "WWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2, ecl_sum_get_well_var( resp, 2, "W_3", "WGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.3, ecl_sum_get_well_var( resp, 2, "W_3", "WNIT" ), 1e-5 );
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
    setup cfg( "test_Summary_group" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
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
    BOOST_CHECK_CLOSE( (30.6 + 30.7 + 30.8),
                       ecl_sum_get_group_var( resp, 1, "G_2", "GVIR" ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_group_var( resp, 1, "G_2", "GWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_group_var( resp, 1, "G_2", "GGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.3, ecl_sum_get_group_var( resp, 1, "G_2", "GNIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.6 + 30.7 + 30.8),
                       ecl_sum_get_group_var( resp, 1, "G_2", "GVIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.0, ecl_sum_get_group_var( resp, 2, "G_2", "GWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2, ecl_sum_get_group_var( resp, 2, "G_2", "GGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.3, ecl_sum_get_group_var( resp, 2, "G_2", "GNIT" ), 1e-5 );
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
    setup cfg( "test_Summary_group_group" , "group_group.DATA");

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution , {});
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
    setup cfg( "test_Summary_completion" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
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
    BOOST_CHECK_CLOSE( 300.0,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CWIR", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.2,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CGIR", 3 ), 1e-5 );

    /* Injection totals */
    BOOST_CHECK_CLOSE( 300.0,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CWIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.2,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CGIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 300.3,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CNIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 300.0, ecl_sum_get_well_completion_var( resp, 2, "W_3", "CWIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 300.2, ecl_sum_get_well_completion_var( resp, 2, "W_3", "CGIT", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 300.3, ecl_sum_get_well_completion_var( resp, 2, "W_3", "CNIT", 3 ), 1e-5 );

    /* Solvent flow rate + or - Note OPM uses negative values for producers, while CNFR outputs positive
    values for producers*/
    BOOST_CHECK_CLOSE( -300.3,     ecl_sum_get_well_completion_var( resp, 1, "W_3", "CNFR", 3 ), 1e-5 );
    BOOST_CHECK_CLOSE(  200.3,    ecl_sum_get_well_completion_var( resp, 1, "W_2", "CNFR", 2 ), 1e-5 );
}

BOOST_AUTO_TEST_CASE(field_keywords) {
    setup cfg( "test_Summary_field" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
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

    /* Injection totals */
    BOOST_CHECK_CLOSE( 30.0,     ecl_sum_get_field_var( resp, 1, "FWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2,     ecl_sum_get_field_var( resp, 1, "FGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.6 + 30.7 + 30.8, ecl_sum_get_field_var( resp, 1, "FVIT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 30.0, ecl_sum_get_field_var( resp, 2, "FWIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2, ecl_sum_get_field_var( resp, 2, "FGIT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (30.6 + 30.7 + 30.8), ecl_sum_get_field_var( resp, 2, "FVIT" ), 1e-5 );

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

    const double foip = 11.0 * 1000 - 2*10;    // Cell (1,2,10) is inactive.
    const double foipl = 10.0 * 1000 - (2*10 - 1);    // Cell (1,2,10) is inactive.
    const double fgip = 11.55 * 1000 - 2.1*10; // Cell (1,2,10) is inactive.
    const double fgipg = 12.55 * 1000 - (2.1*10 + 1); // Cell (1,2,10) is inactive.
    const double fwip = 12.1 * 1000 - 2.2*10; // Cell (1,2,10) is inactive.
    BOOST_CHECK_CLOSE( foip, ecl_sum_get_field_var( resp, 1, "FOIP" ), 1e-5 );
    BOOST_CHECK_CLOSE( foipl, ecl_sum_get_field_var( resp, 1, "FOIPL" ), 1e-5 );
    BOOST_CHECK_CLOSE( fgip, ecl_sum_get_field_var( resp, 1, "FGIP" ), 1e-5 );
    BOOST_CHECK_CLOSE( fgipg, ecl_sum_get_field_var( resp, 1, "FGIPG" ), 1e-5 );
    BOOST_CHECK_CLOSE( fwip, ecl_sum_get_field_var( resp, 1, "FWIP" ), 1e-5 );

    BOOST_CHECK_EQUAL( 1, ecl_sum_get_field_var( resp, 1, "FMWIN" ) );
    BOOST_CHECK_EQUAL( 2, ecl_sum_get_field_var( resp, 1, "FMWPR" ) );

    UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    const double fpr_si = (5.5 * 1000 - 10) / 999;
    const double fpr = units.from_si( UnitSystem::measure::pressure, fpr_si );
    BOOST_CHECK_CLOSE( fpr, ecl_sum_get_field_var( resp, 1, "FPR" ), 1e-5 ); //

    /* in this test, the initial OIP wasn't set */
    BOOST_CHECK_EQUAL( 0.0, ecl_sum_get_field_var( resp, 1, "FOE" ) );
    BOOST_CHECK_EQUAL( 0.0, ecl_sum_get_field_var( resp, 2, "FOE" ) );
}

BOOST_AUTO_TEST_CASE(foe_test) {
    setup cfg( "foe" );

    std::vector< double > oip( cfg.grid.getNumActive(), 12.0 );
    data::Solution sol;
    sol.insert( "OIP", UnitSystem::measure::volume, oip, data::TargetType::RESTART_AUXILIARY );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.set_initial( sol );
    writer.add_timestep( 1, 2 *  day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 5 *  day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 10 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    const double oip0 = 12 * cfg.grid.getNumActive();
    const double oip1 = 11.0 * 1000 - 2*10;
    const double foe = (oip0 - oip1) / oip0;
    BOOST_CHECK_CLOSE( foe, ecl_sum_get_field_var( resp, 1, "FOE" ), 1e-5 );
    BOOST_CHECK_CLOSE( foe, ecl_sum_get_field_var( resp, 2, "FOE" ), 1e-5 );
}


BOOST_AUTO_TEST_CASE(report_steps_time) {
    setup cfg( "test_Summary_report_steps_time" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 1, 2 *  day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 5 *  day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 10 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
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
    setup cfg( "test_Summary_skip_unknown_var" );

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 1, 2 *  day, cfg.es,  cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 5 *  day, cfg.es,  cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 10 * day, cfg.es,  cfg.schedule, cfg.wells , cfg.solution, {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    /* verify that some non-supported keywords aren't written to the file */
    BOOST_CHECK( !ecl_sum_has_well_var( resp, "W_1", "WPI" ) );
    BOOST_CHECK( !ecl_sum_has_field_var( resp, "FGST" ) );
}



BOOST_AUTO_TEST_CASE(region_vars) {
    setup cfg( "region_vars" );

    {
        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
        writer.add_timestep( 1, 2 *  day, cfg.es, cfg.schedule, cfg.wells, cfg.solution, {});
        writer.add_timestep( 1, 5 *  day, cfg.es, cfg.schedule, cfg.wells, cfg.solution, {});
        writer.add_timestep( 2, 10 * day, cfg.es, cfg.schedule, cfg.wells, cfg.solution, {});
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

        BOOST_CHECK_CLOSE(   r * 1.0        , units.to_si( UnitSystem::measure::pressure , ecl_sum_get_general_var( resp, 1, rpr_key.c_str())) , 1e-5);

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
        writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
        writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
        writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
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
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
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

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
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


BOOST_AUTO_TEST_CASE(fpr) {
    setup cfg( "test_fpr", "summary_deck_non_constant_porosity.DATA");

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    // fpr = sum_ (p * hcpv ) / hcpv, hcpv = pv * (1 - sw)
    const double fpr_si =  ( (3 * 0.1 + 8 * 0.2) * 500 * (1 - 8.0) ) / ( (500*0.1 + 500*0.2) * (1 - 8.0));
    const double fpr = units.from_si( UnitSystem::measure::pressure, fpr_si );
    BOOST_CHECK_CLOSE( fpr, ecl_sum_get_field_var( resp, 1, "FPR" ), 1e-5 ); //

    // change sw and pressure and test again.
    size_t g = 0;
    for (size_t k=0; k < cfg.grid.getNZ(); k++) {
        for (size_t j=0; j < cfg.grid.getNY(); j++) {
            for (size_t i=0; i < cfg.grid.getNX(); i++) {
                if (cfg.grid.cellActive(i,j,k)) {
                    cfg.solution.data("SWAT")[g] = 0.1*(k);
                    cfg.solution.data("PRESSURE")[g] = units.to_si( UnitSystem::measure::pressure, 1 );
                    g++;
                }
            }
        }
    }

    out::Summary writer2( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer2.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer2.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer2.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer2.write();

    auto res2 = readsum( cfg.name );
    const auto* resp2 = res2.get();

    // fpr = sum_ (p * hcpv ) / hcpv, hcpv = pv * (1 - sw)
    const double fpr2_si =  ( (0.8 * 0.1 + 0.3 * 0.2) * 500 * 1 ) / ( (0.8 * 0.1 + 0.3 * 0.2) * 500);
    BOOST_CHECK_CLOSE( fpr2_si, ecl_sum_get_field_var( resp2, 1, "FPR" ), 1e-5 ); //
}

BOOST_AUTO_TEST_CASE(fprp) {
    setup cfg( "test_fprp", "summary_deck_non_constant_porosity.DATA");

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );
    // fprp = sum_ (p * pv ) / pv
    const double fprp_si =  ( (3 * 0.1 + 8 * 0.2) * 500 ) / ( (500*0.1 + 500*0.2));
    const double fprp = units.from_si( UnitSystem::measure::pressure, fprp_si );
    BOOST_CHECK_CLOSE( fprp, ecl_sum_get_field_var( resp, 1, "FPRP" ), 1e-5 );

    // Change pressure and check again
    for (size_t g = 0; g < cfg.grid.getNumActive(); g++){
            cfg.solution.data("PRESSURE")[g] = units.to_si( UnitSystem::measure::pressure, 1 );
    }

    out::Summary writer2( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
    writer2.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer2.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer2.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer2.write();

    auto res2 = readsum( cfg.name );
    const auto* resp2 = res2.get();

    // fprp = sum_ (p * pv ) / pv
    const double fprp2_si =  ( (0.8 * 0.1 + 0.3 * 0.2) * 500) / ( (0.8 * 0.1 + 0.3 * 0.2) * 500);
    BOOST_CHECK_CLOSE( fprp2_si, ecl_sum_get_field_var( resp2, 1, "FPRP" ), 1e-5 );
}

BOOST_AUTO_TEST_CASE(rpr) {
    setup cfg( "test_rpr", "summary_deck_non_constant_porosity.DATA");

    {
        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule, cfg.name );
        writer.add_timestep( 1, 2 *  day, cfg.es, cfg.schedule, cfg.wells, cfg.solution, {});
        writer.add_timestep( 1, 5 *  day, cfg.es, cfg.schedule, cfg.wells, cfg.solution, {});
        writer.add_timestep( 2, 10 * day, cfg.es, cfg.schedule, cfg.wells, cfg.solution, {});
        writer.write();
    }

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK( ecl_sum_has_general_var( resp , "RPR:1"));
    BOOST_CHECK( ecl_sum_has_general_var( resp , "RPR:3"));
    BOOST_CHECK( !ecl_sum_has_general_var( resp , "RPR:4"));
    UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );

    // rpr = sum_ (p * hcpv ) / hcpv, hcpv = pv * (1 - sw)
    // region 1; layer 1:4
    {
        const double rpr_si =  ( 2.5 * 0.1 * 400 * (1 - 8.0) ) / ( (400*0.1) * (1 - 8.0));
        std::string rpr_key   = "RPR:1";
        BOOST_CHECK_CLOSE(   rpr_si        , units.to_si( UnitSystem::measure::pressure , ecl_sum_get_general_var( resp, 1, rpr_key.c_str())) , 1e-5);
    }
    // region 2; layer 5:6
    {
        const double rpr_si =  ( (5 * 0.1 + 6 * 0.2) * 100 * (1 - 8.0) ) / ( (0.1 + 0.2) * 100 * (1 - 8.0));
        std::string rpr_key   = "RPR:2";
        BOOST_CHECK_CLOSE(   rpr_si        , units.to_si( UnitSystem::measure::pressure , ecl_sum_get_general_var( resp, 1, rpr_key.c_str())) , 1e-5);
    }
    // region 3; layer 7:10
    {
        const double rpr_si =  ( 8.5 * 0.2 * 400 * (1 - 8.0) ) / ( (400*0.2) * (1 - 8.0));
        std::string rpr_key   = "RPR:3";
        BOOST_CHECK_CLOSE(   rpr_si        , units.to_si( UnitSystem::measure::pressure , ecl_sum_get_general_var( resp, 1, rpr_key.c_str())) , 1e-5);
    }

}

BOOST_AUTO_TEST_CASE(MISC) {
    setup cfg( "test_MISC");

    out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule , cfg.name );
    writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, {});
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();
    BOOST_CHECK( ecl_sum_has_key( resp , "TCPU" ));
}


BOOST_AUTO_TEST_CASE(EXTRA) {
    setup cfg( "test_EXTRA");

    {
        out::Summary writer( cfg.es, cfg.config, cfg.grid, cfg.schedule , cfg.name );
        writer.add_timestep( 0, 0 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, { {"TCPU" , 0 }});
        writer.add_timestep( 1, 1 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, { {"TCPU" , 1 }});
        writer.add_timestep( 2, 2 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, { {"TCPU" , 2}});

        /* Add a not-recognized key; that is OK */
        BOOST_CHECK_NO_THROW(  writer.add_timestep( 3, 3 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, { {"MISSING" , 2 }}));

        /* Override a NOT MISC variable - ignored. */
        writer.add_timestep( 4, 4 * day, cfg.es, cfg.schedule, cfg.wells , cfg.solution, { {"FOPR" , -1 }});
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
