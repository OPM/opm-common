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
#include <ert/ecl/EclKW.hpp>
#include <ert/util/ert_unique_ptr.hpp>

#include <opm/output/eclipse/EclipseReader.hpp>
#include <opm/output/Cells.hpp>
#include <opm/output/Wells.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <algorithm>

namespace Opm {
namespace {

    inline data::Solution restoreSOLUTION( ecl_file_type* file,
                                           int numcells,
                                           const UnitSystem& units ) {

        for( const auto* key : { "PRESSURE", "TEMP", "SWAT", "SGAS" } ) {
            if( !ecl_file_has_kw( file, key ) )
                throw std::runtime_error("Read of restart file: "
                                         "File does not contain "
                                         + std::string( key )
                                         + " data" );
        }

        using ERT::EclKW_ref;

        EclKW_ref< float > pres( ecl_file_iget_named_kw( file, "PRESSURE", 0 ) );
        EclKW_ref< float > temp( ecl_file_iget_named_kw( file, "TEMP", 0 ) );
        EclKW_ref< float > swat( ecl_file_iget_named_kw( file, "SWAT", 0 ) );
        EclKW_ref< float > sgas( ecl_file_iget_named_kw( file, "SGAS", 0 ) );

        for( const auto& kw : { pres, temp, swat, sgas } ) {
            if( kw.size() != size_t( numcells ) )
                throw std::runtime_error("Restart file: Could not restore "
                                        + std::string( kw.name() )
                                        + ", mismatched number of cells" );
        }

        using ds = data::Solution::key;
        data::Solution sol;
        sol.insert( ds::PRESSURE, { pres.data(), pres.data() + pres.size() } );
        sol.insert( ds::TEMP,     { temp.data(), temp.data() + temp.size() } );
        sol.insert( ds::SWAT,     { swat.data(), swat.data() + swat.size() } );
        sol.insert( ds::SGAS,     { sgas.data(), sgas.data() + sgas.size() } );

        const auto apply_pressure = [&]( double x ) {
            return units.to_si( UnitSystem::measure::pressure, x );
        };

        const auto apply_temperature = [=]( double x ) {
            return units.to_si( UnitSystem::measure::temperature, x );
        };

        std::transform( sol[ ds::PRESSURE ].begin(), sol[ ds::PRESSURE ].end(),
                        sol[ ds::PRESSURE ].begin(), apply_pressure );
        std::transform( sol[ ds::TEMP ].begin(), sol[ ds::TEMP ].end(),
                        sol[ ds::TEMP ].begin(), apply_temperature );

        /* optional keywords */
        if( ecl_file_has_kw( file, "RS" ) ) {
            EclKW_ref< float > kw( ecl_file_iget_named_kw( file, "RS", 0 ) );
            sol.insert( ds::RS, { kw.data(), kw.data() + kw.size() } );
        }

        if( ecl_file_has_kw( file, "RV" ) ) {
            EclKW_ref< float > kw( ecl_file_iget_named_kw( file, "RV", 0 ) );
            sol.insert( ds::RV, { kw.data(), kw.data() + kw.size() } );
        }

        return sol;
    }

    inline data::Wells restoreOPM_XWEL( ecl_file_type* file,
                                        int num_wells,
                                        int num_phases ) {
        const char* keyword = "OPM_XWEL";

        ecl_kw_type* xwel = ecl_file_iget_named_kw( file, keyword, 0 );
        const double* xwel_data = ecl_kw_get_double_ptr(xwel);
        const double* xwel_end  = xwel_data + ecl_kw_get_size( xwel );

        const double* bhp_begin = xwel_data;
        const double* bhp_end = bhp_begin + num_wells;
        const double* temp_begin = bhp_end;
        const double* temp_end = temp_begin + num_wells;
        const double* wellrate_begin = temp_end;
        const double* wellrate_end = wellrate_begin + (num_wells * num_phases);

        const auto remaining = std::distance( wellrate_end, xwel_end );
        const auto perf_elems = remaining / 2;

        const double* perfpres_begin = wellrate_end;
        const double* perfpres_end = perfpres_begin + perf_elems;
        const double* perfrate_begin = perfpres_end;
        const double* perfrate_end = perfrate_begin + perf_elems;

        return { {},
            { bhp_begin, bhp_end },
            { temp_begin, temp_end },
            { wellrate_begin, wellrate_end },
            { perfpres_begin, perfpres_end },
            { perfrate_begin, perfrate_end }
        };
    }
}

std::pair< data::Solution, data::Wells >
init_from_restart_file( const EclipseState& es, int numcells ) {

    InitConfigConstPtr initConfig        = es.getInitConfig();
    IOConfigConstPtr ioConfig            = es.getIOConfig();
    int restart_step                     = initConfig->getRestartStep();
    const std::string& restart_file_root = initConfig->getRestartRootName();
    bool output                          = false;
    const std::string filename           = ioConfig->getRestartFileName(
                                                        restart_file_root,
                                                        restart_step,
                                                        output);
    const bool unified                   = ioConfig->getUNIFIN();
    const int num_wells = es.getSchedule()->numWells( restart_step );
    const int num_phases = es.getTableManager().getNumPhases();

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

    return {
        restoreSOLUTION( file.get(), numcells, es.getUnits() ),
        restoreOPM_XWEL( file.get(), num_wells, num_phases )
    };
}

} // namespace Opm
