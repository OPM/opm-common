
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

#define BOOST_TEST_MODULE EclipseWriter
#include <opm/common/utility/platform_dependent/disable_warnings.h>
#include <boost/test/unit_test.hpp>
#include <opm/common/utility/platform_dependent/reenable_warnings.h>

#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>

// ERT stuff
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl_well/well_info.h>
#include <ert/ecl_well/well_state.h>
#include <ert/util/test_work_area.h>

using namespace Opm;

void verifyWellState(const std::string& rst_filename,
                     const EclipseGrid& ecl_grid,
                     const Schedule& schedule) {

  well_info_type* well_info = well_info_alloc(ecl_grid.c_ptr());
  well_info_load_rstfile(well_info, rst_filename.c_str(), false);

  //Verify numwells
  int numwells = well_info_get_num_wells(well_info);
  BOOST_CHECK_EQUAL( numwells, schedule.numWells() );

  auto wells = schedule.getWells2atEnd();

  for (int i = 0; i < numwells; ++i) {

    //Verify wellnames
    const char * wellname = well_info_iget_well_name(well_info, i);
    well_ts_type* well_ts = well_info_get_ts(well_info , wellname);
    well_state_type* well_state = well_ts_iget_state(well_ts, 0);
    {
        const auto& sched_well = wells.at(i);
        BOOST_CHECK_EQUAL( wellname, sched_well.name() );

        // Verify well-head position data
        const well_conn_type* well_head = well_state_get_wellhead(well_state, ECL_GRID_GLOBAL_GRID);
        BOOST_CHECK_EQUAL(well_conn_get_i(well_head), sched_well.getHeadI());
        BOOST_CHECK_EQUAL(well_conn_get_j(well_head), sched_well.getHeadJ());
    }

    for (int j = 0; j < well_ts_get_size(well_ts); ++j) {
      const auto& well_at_end = schedule.getWell2atEnd(wellname);
      if (!well_at_end.hasBeenDefined(j))
        continue;

      const auto& well = schedule.getWell2(wellname, j);
      well_state = well_ts_iget_state(well_ts, j);

      //Verify welltype
      int well_type = ERT_UNDOCUMENTED_ZERO;
      if( well.isProducer( ) ) {
          well_type = ERT_PRODUCER;
      }
      else {
          switch( well.getInjectionProperties(  ).injectorType ) {
              case WellInjector::WATER:
                  well_type = ERT_WATER_INJECTOR;
                  break;
              case WellInjector::GAS:
                  well_type = ERT_GAS_INJECTOR;
                  break;
              case WellInjector::OIL:
                  well_type = ERT_OIL_INJECTOR;
                  break;
              default:
                  break;
          }
      }

      int ert_well_type = well_state_get_type( well_state );
      BOOST_CHECK_EQUAL( ert_well_type, well_type );

      //Verify wellstatus
      int ert_well_status = well_state_is_open(well_state) ? 1 : 0;
      int wellstatus = well.getStatus(  ) == WellCommon::OPEN ? 1 : 0;

      BOOST_CHECK_EQUAL(ert_well_status, wellstatus);

      //Verify number of completion connections
      const well_conn_collection_type * well_connections = well_state_get_global_connections( well_state );
      size_t num_wellconnections = well_conn_collection_get_size(well_connections);

      const auto& connections_set = well.getConnections();

      BOOST_CHECK_EQUAL(num_wellconnections, connections_set.size());

      //Verify coordinates for each completion connection
      for (size_t k = 0; k < num_wellconnections; ++k) {
          const well_conn_type * well_connection = well_conn_collection_iget_const(well_connections , k);

          const auto& completion = connections_set.get(k);

          BOOST_CHECK_EQUAL(well_conn_get_i(well_connection), completion.getI());
          BOOST_CHECK_EQUAL(well_conn_get_j(well_connection), completion.getJ());
          BOOST_CHECK_EQUAL(well_conn_get_k(well_connection), completion.getK());
      }
    }
  }

  well_info_free(well_info);
}

BOOST_AUTO_TEST_CASE(EclipseWriteRestartWellInfo) {
    std::string eclipse_data_filename    = "testblackoilstate3.DATA";
    std::string eclipse_restart_filename = "TESTBLACKOILSTATE3.X0004";
    test_work_area_type * test_area = test_work_area_alloc("TEST_EclipseWriteNumWells");
    test_work_area_copy_file(test_area, eclipse_data_filename.c_str());

    Parser parser;
    Deck deck( parser.parseFile( eclipse_data_filename ));
    EclipseState es(deck);
    const EclipseGrid& grid = es.getInputGrid();
    Schedule schedule( deck, es);
    SummaryConfig summary_config( deck, schedule, es.getTableManager( ));
    const auto num_cells = grid.getCartesianSize();
    EclipseIO eclipseWriter( es,  grid , schedule, summary_config);
    int countTimeStep = schedule.getTimeMap().numTimesteps();
    SummaryState st;

    data::Solution solution;
    solution.insert( "PRESSURE",UnitSystem::measure::pressure , std::vector< double >( num_cells, 1 ) , data::TargetType::RESTART_SOLUTION);
    solution.insert( "SWAT"    ,UnitSystem::measure::identity , std::vector< double >( num_cells, 1 ) , data::TargetType::RESTART_SOLUTION);
    solution.insert( "SGAS"    ,UnitSystem::measure::identity , std::vector< double >( num_cells, 1 ) , data::TargetType::RESTART_SOLUTION);
    data::Wells wells;

    for(int timestep = 0; timestep <= countTimeStep; ++timestep){
        eclipseWriter.writeTimeStep( st,
                                     timestep,
                                     false,
                                     timestep,
                                     RestartValue(solution, wells));
    }

    verifyWellState(eclipse_restart_filename, grid, schedule);

    test_work_area_free(test_area);
}
