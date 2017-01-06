/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM. If not, see <http://www.gnu.org/licenses/>.
*/

#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/EclKW.hpp>
#include <ert/util/ert_unique_ptr.hpp>

#include <opm/output/eclipse/EclipseReader.hpp>
#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <numeric>

namespace Opm {
namespace {

    std::vector<double> double_vector( const ecl_kw_type * ecl_kw ) {
        size_t size = ecl_kw_get_size( ecl_kw );

        if (ecl_kw_get_type( ecl_kw ) == ECL_DOUBLE_TYPE ) {
            const double * ecl_data = ecl_kw_get_double_ptr( ecl_kw );
            return { ecl_data , ecl_data + size };
        } else {
            const float * ecl_data = ecl_kw_get_float_ptr( ecl_kw );
            return { ecl_data , ecl_data + size };
        }

    }

    inline data::Solution restoreSOLUTION( ecl_file_type* file,
                                           const std::map<std::string, UnitSystem::measure>& keys,
                                           int numcells,
                                           const UnitSystem& units ) {

        data::Solution sol;
        for (const auto& pair : keys) {
            const std::string& key = pair.first;
            UnitSystem::measure dim = pair.second;
            if( !ecl_file_has_kw( file, key.c_str() ) )
                throw std::runtime_error("Read of restart file: "
                                         "File does not contain "
                                         + key
                                         + " data" );

            const ecl_kw_type * ecl_kw = ecl_file_iget_named_kw( file , key.c_str() , 0 );
            if( ecl_kw_get_size(ecl_kw) != numcells)
                throw std::runtime_error("Restart file: Could not restore "
                                         + std::string( ecl_kw_get_header( ecl_kw ) )
                                         + ", mismatched number of cells" );

            std::vector<double> data = double_vector( ecl_kw );
            units.to_si( dim , data );
            sol.insert( key, dim, data , data::TargetType::RESTART_SOLUTION );
        }

        return sol;
    }
}

using rt = data::Rates::opt;
data::Wells restore_wells( const double* xwel_data,
                           size_t xwel_data_size,
                           const int* iwel_data,
                           size_t iwel_data_size,
                           int restart_step,
                           const EclipseState& es ) {

    const auto& sched_wells = es.getSchedule().getWells( restart_step );
    const EclipseGrid& grid = es.getInputGrid( );
    std::vector< rt > phases;
    const auto& phase = es.runspec().phases();
    if( phase.active( Phase::WATER ) ) phases.push_back( rt::wat );
    if( phase.active( Phase::OIL ) )   phases.push_back( rt::oil );
    if( phase.active( Phase::GAS ) )   phases.push_back( rt::gas );


    const auto well_size = [&]( size_t acc, const Well* w ) {
        return acc
            + 2 + phases.size()
            + (  w->getCompletions( restart_step ).size()
              * (phases.size() + data::Completion::restart_size) );
    };

    const auto expected_xwel_size = std::accumulate( sched_wells.begin(),
                                                     sched_wells.end(),
                                                     size_t( 0 ),
                                                     well_size );

    if( xwel_data_size != expected_xwel_size ) {
        throw std::runtime_error(
                "Mismatch between OPM_XWEL and deck; "
                "OPM_XWEL size was " + std::to_string( xwel_data_size ) +
                ", expected " + std::to_string( expected_xwel_size ) );
    }

    if( iwel_data_size != sched_wells.size() )
        throw std::runtime_error(
                "Mismatch between OPM_IWEL and deck; "
                "OPM_IWEL size was " + std::to_string( iwel_data_size ) +
                ", expected " + std::to_string( sched_wells.size() ) );

    data::Wells wells;
    for( const auto* sched_well : sched_wells ) {
        data::Well& well = wells[ sched_well->name() ];

        well.bhp = *xwel_data++;
        well.temperature = *xwel_data++;
        well.control = *iwel_data++;

        for( auto phase : phases )
            well.rates.set( phase, *xwel_data++ );

        for( const auto& sc : sched_well->getCompletions( restart_step ) ) {
            const auto i = sc.getI(), j = sc.getJ(), k = sc.getK();
            if( !grid.cellActive( i, j, k ) || sc.getState() == WellCompletion::SHUT ) {
                xwel_data += data::Completion::restart_size + phases.size();
                continue;
            }

            const auto active_index = grid.activeIndex( i, j, k );

            well.completions.emplace_back();
            auto& completion = well.completions.back();
            completion.index = active_index;
            completion.pressure = *xwel_data++;
            completion.reservoir_rate = *xwel_data++;
            for( auto phase : phases )
                completion.rates.set( phase, *xwel_data++ );
        }
    }

    return wells;
}

/* should take grid as argument because it may be modified from the simulator */
std::pair< data::Solution, data::Wells >
load_from_restart_file( const EclipseState& es, const std::map<std::string, UnitSystem::measure>& keys, int numcells ) {

    const InitConfig& initConfig         = es.getInitConfig();
    const auto& ioConfig                 = es.getIOConfig();
    int restart_step                     = initConfig.getRestartStep();
    const std::string& restart_file_root = initConfig.getRestartRootName();
    bool output                          = false;
    const std::string filename           = ioConfig.getRestartFileName(
                                                        restart_file_root,
                                                        restart_step,
                                                        output);
    const bool unified                   = ioConfig.getUNIFIN();
    using ft = ERT::ert_unique_ptr< ecl_file_type, ecl_file_close >;
    ft file( ecl_file_open( filename.c_str(), 0 ) );

    if( !file )
        throw std::runtime_error( "Restart file " + filename + " not found!" );

    if( unified &&
        !ecl_file_select_rstblock_report_step( file.get(), restart_step ) ) {
        throw std::runtime_error( "Restart file " + filename
                + " does not contain data for report step "
                + std::to_string( restart_step ) + "!" );
    }

    ecl_kw_type* xwel = ecl_file_iget_named_kw( file.get(), "OPM_XWEL", 0 );
    const double* xwel_data = ecl_kw_get_double_ptr( xwel );
    const auto xwel_size = ecl_kw_get_size( xwel );

    ecl_kw_type* iwel = ecl_file_iget_named_kw( file.get(), "OPM_IWEL", 0 );
    const int* iwel_data = ecl_kw_get_int_ptr( iwel );
    const auto iwel_size = ecl_kw_get_size( iwel );

    return {
        restoreSOLUTION( file.get(), keys, numcells, es.getUnits() ),
        restore_wells( xwel_data, xwel_size,
                       iwel_data, iwel_size,
                       restart_step,
                       es )
    };
}

} // namespace Opm
