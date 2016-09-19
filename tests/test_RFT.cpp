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

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE EclipseRFTWriter
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/output/eclipse/EclipseWriter.hpp>
#include <opm/output/Wells.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
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

    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell1), 210088*0.00001, 0.00001);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell2), 210188*0.00001, 0.00001);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell3), 210288*0.00001, 0.00001);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_sgas(ecl_rft_cell1), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_sgas(ecl_rft_cell2), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_sgas(ecl_rft_cell3), 0.0);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_swat(ecl_rft_cell1), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_swat(ecl_rft_cell2), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_swat(ecl_rft_cell3), 0.0);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_soil(ecl_rft_cell1), 1.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_soil(ecl_rft_cell2), 1.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_soil(ecl_rft_cell3), 1.0);

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
    sol.insert( data::Solution::key::PRESSURE, pressure );
    sol.insert( data::Solution::key::SWAT, swat );
    sol.insert( data::Solution::key::SGAS, sgas );
    return sol;
}

}

BOOST_AUTO_TEST_CASE(test_RFT) {

    std::string eclipse_data_filename    = "testRFT.DATA";
    ERT::TestArea test_area("test_RFT");
    test_area.copyFile( eclipse_data_filename );

    auto eclipseState = std::make_shared< EclipseState >( Parser::parse( eclipse_data_filename ) );
    {
        /* eclipseWriter is scoped here to ensure it is destroyed after the
         * file itself has been written, because we're going to reload it
         * immediately. first upon destruction can we guarantee it being
         * written to disk and flushed.
         */

        const auto numCells = eclipseState->getInputGrid()->getCartesianSize( );
        const auto& grid = *eclipseState->getInputGrid();

        EclipseWriter eclipseWriter( eclipseState, grid);
        time_t start_time = eclipseState->getSchedule()->posixStartTime();
        /* step time read from deck and hard-coded here */
        time_t step_time = ecl_util_make_date(10, 10, 2008 );

        Opm::data::Wells wells {
            { { "OP_1", { {}, 1.0, 1.1, {} } },
              { "OP_2", { {}, 1.0, 1.1, {} } } },
            { 2.1, 2.2 },
            { 3.1, 3.2 },
            { 4.11, 4.12, 4.13, 4.21, 4.22, 4.23 },
            { },
            { }
        };

        eclipseWriter.writeTimeStep( 2,
                                     false,
                                     step_time - start_time,
                                     createBlackoilState( 2, numCells ),
                                     wells);
    }

    verifyRFTFile("TESTRFT.RFT");
}
