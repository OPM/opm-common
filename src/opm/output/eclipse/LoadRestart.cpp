/*
  Copyright (c) 2016 Statoil ASA
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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

#include <opm/output/eclipse/RestartIO.hpp>

#include <opm/output/eclipse/RestartValue.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/output/eclipse/libECLRestart.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    Opm::UnitSystem::UnitType
    getUnitSystem(const Opm::RestartIO::ecl_kw_type* intehead)
    {
	using UType = Opm::UnitSystem::UnitType;

	//switch (Opm::RestartIO::ecl_kw_iget_int(intehead, INTEHEAD_UNIT_INDEX)) {
	switch (Opm::RestartIO::ecl_kw_iget_type<int>(intehead, Opm::RestartIO::ECL_INT_TYPE, INTEHEAD_UNIT_INDEX)) {
	case 1: return UType::UNIT_TYPE_METRIC;
	case 2: return UType::UNIT_TYPE_FIELD;
	case 3: return UType::UNIT_TYPE_LAB;
	case 4: return UType::UNIT_TYPE_PVT_M;
	}

	return UType::UNIT_TYPE_METRIC; // questionable…
    }
}

namespace Opm { namespace RestartIO  {

//namespace {


    inline int to_ert_welltype( const Well& well, size_t timestep ) {
	if( well.isProducer( timestep ) ) return IWEL_PRODUCER;

	switch( well.getInjectionProperties( timestep ).injectorType ) {
	case WellInjector::WATER:
	    return IWEL_WATER_INJECTOR;
	case WellInjector::GAS:
	    return IWEL_GAS_INJECTOR;
	case WellInjector::OIL:
	    return IWEL_OIL_INJECTOR;
	default:
	    return IWEL_UNDOCUMENTED_ZERO;
	}
    }

  std::vector<double> double_vector( const ::Opm::RestartIO::ecl_kw_type * ecl_kw ) {
	size_t size = ::Opm::RestartIO::ecl_kw_get_size( ecl_kw );

	if (::Opm::RestartIO::ecl_type_get_type( ::Opm::RestartIO::ecl_kw_get_data_type( ecl_kw ) ) == ECL_DOUBLE_TYPE ) {
	    //const double * ecl_data = ::Opm::RestartIO::ecl_kw_get_double_ptr( ecl_kw );
	    const double * ecl_data = ::Opm::RestartIO::ecl_kw_get_type_ptr<double>( ecl_kw, ECL_DOUBLE_TYPE );
	    return { ecl_data , ecl_data + size };
	} else {
	    //const float * ecl_data = ::Opm::RestartIO::ecl_kw_get_float_ptr( ecl_kw );
	    const float * ecl_data = ::Opm::RestartIO::ecl_kw_get_type_ptr<float>( ecl_kw, ECL_FLOAT_TYPE );
	    return { ecl_data , ecl_data + size };
	}

    }


  inline data::Solution restoreSOLUTION( ::Opm::RestartIO::ecl_file_view_type* file_view,
					   const std::vector<RestartKey>& solution_keys,
					   int numcells) {

	data::Solution sol( false );
	for (const auto& value : solution_keys) {
	    const std::string& key = value.key;
	    UnitSystem::measure dim = value.dim;
	    bool required = value.required;

	    if( !::Opm::RestartIO::ecl_file_view_has_kw( file_view, key.c_str() ) ) {
		if (required)
		    throw std::runtime_error("Read of restart file: "
					     "File does not contain "
					     + key
					     + " data" );
		else
		    continue;
	    }

	    const Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_file_view_iget_named_kw( file_view , key.c_str() , 0 );
	    if( Opm::RestartIO::ecl_kw_get_size(ecl_kw) != numcells)
		throw std::runtime_error("Restart file: Could not restore "
					 + std::string( Opm::RestartIO::ecl_kw_get_header( ecl_kw ) )
					 + ", mismatched number of cells" );

	    std::vector<double> data = ::Opm::RestartIO::double_vector( ecl_kw );
	    sol.insert( key, dim, data , data::TargetType::RESTART_SOLUTION );
	}

	return sol;
    }


using rt = data::Rates::opt;
data::Wells restore_wells( const ::Opm::RestartIO::ecl_kw_type * opm_xwel,
			   const ::Opm::RestartIO::ecl_kw_type * opm_iwel,
			   int sim_step,
			   const EclipseState& es,
			   const EclipseGrid& grid,
			   const Schedule& schedule) {

    const auto& sched_wells = schedule.getWells( sim_step );
    std::vector< rt > phases;
    {
	const auto& phase = es.runspec().phases();
	if( phase.active( Phase::WATER ) ) phases.push_back( rt::wat );
	if( phase.active( Phase::OIL ) )   phases.push_back( rt::oil );
	if( phase.active( Phase::GAS ) )   phases.push_back( rt::gas );
    }

    const auto well_size = [&]( size_t acc, const Well* w ) {
        return acc
            + 2 + phases.size()
	    + (  w->getConnections( sim_step ).size()
	      * (phases.size() + data::Connection::restart_size) );
    };

    const auto expected_xwel_size = std::accumulate( sched_wells.begin(),
						     sched_wells.end(),
						     0,
						     well_size );

    if( ::Opm::RestartIO::ecl_kw_get_size( opm_xwel ) != expected_xwel_size ) {
	throw std::runtime_error(
		"Mismatch between OPM_XWEL and deck; "
		"OPM_XWEL size was " + std::to_string( ::Opm::RestartIO::ecl_kw_get_size( opm_xwel ) ) +
		", expected " + std::to_string( expected_xwel_size ) );
    }

    if( ::Opm::RestartIO::ecl_kw_get_size( opm_iwel ) != int(sched_wells.size()) )
	throw std::runtime_error(
		"Mismatch between OPM_IWEL and deck; "
		"OPM_IWEL size was " + std::to_string( ::Opm::RestartIO::ecl_kw_get_size( opm_iwel ) ) +
		", expected " + std::to_string( sched_wells.size() ) );

    data::Wells wells;
    const double * opm_xwel_data = ::Opm::RestartIO::ecl_kw_get_type_ptr<double>( opm_xwel, ECL_DOUBLE_TYPE );
    const int * opm_iwel_data = ::Opm::RestartIO::ecl_kw_get_type_ptr<int>( opm_iwel, ECL_INT_TYPE );
    for( const auto* sched_well : sched_wells ) {
	data::Well& well = wells[ sched_well->name() ];

	well.bhp = *opm_xwel_data++;
	well.temperature = *opm_xwel_data++;
	well.control = *opm_iwel_data++;

	for( auto phase : phases )
	    well.rates.set( phase, *opm_xwel_data++ );

	for( const auto& sc : sched_well->getConnections( sim_step ) ) {
	    const auto i = sc.getI(), j = sc.getJ(), k = sc.getK();
	    if( !grid.cellActive( i, j, k ) || sc.state == WellCompletion::SHUT ) {
		opm_xwel_data += data::Connection::restart_size + phases.size();
		continue;
	    }

	    const auto active_index = grid.activeIndex( i, j, k );

	    well.connections.emplace_back();
	    auto& connection = well.connections.back();
	    connection.index = active_index;
	    connection.pressure = *opm_xwel_data++;
	    connection.reservoir_rate = *opm_xwel_data++;
	    for( auto phase : phases )
		connection.rates.set( phase, *opm_xwel_data++ );
	}
    }

    return wells;
}
//}


//* should take grid as argument because it may be modified from the simulator */
RestartValue load( const std::string& filename,
		   int report_step,
		   const std::vector<RestartKey>& solution_keys,
		   const EclipseState& es,
		   const EclipseGrid& grid,
		   const Schedule& schedule,
		   const std::vector<RestartKey>& extra_keys) {

    int sim_step = std::max(report_step - 1, 0);
    const bool unified                   = ( ::Opm::RestartIO::EclFiletype( filename ) == ::Opm::RestartIO::ECL_UNIFIED_RESTART_FILE );
    ::Opm::RestartIO::ert_unique_ptr< ::Opm::RestartIO::ecl_file_type, ::Opm::RestartIO::ecl_file_close > file(::Opm::RestartIO::ecl_file_open( filename.c_str(), 0 ));
    ::Opm::RestartIO::ecl_file_view_type * file_view;

    if( !file )
	throw std::runtime_error( "Restart file " + filename + " not found!" );

    if( unified ) {
	file_view = ::Opm::RestartIO::ecl_file_get_restart_view( file.get() , -1 , report_step , -1 , -1 );
	if (!file_view)
	    throw std::runtime_error( "Restart file " + filename
				      + " does not contain data for report step "
				      + std::to_string( report_step ) + "!" );
    } else
	file_view = ::Opm::RestartIO::ecl_file_get_global_view( file.get() );

    const ::Opm::RestartIO::ecl_kw_type * intehead = ::Opm::RestartIO::ecl_file_view_iget_named_kw( file_view , "INTEHEAD", 0 );
    const ::Opm::RestartIO::ecl_kw_type * opm_xwel = ::Opm::RestartIO::ecl_file_view_iget_named_kw( file_view , "OPM_XWEL", 0 );
    const ::Opm::RestartIO::ecl_kw_type * opm_iwel = ::Opm::RestartIO::ecl_file_view_iget_named_kw( file_view, "OPM_IWEL", 0 );

    UnitSystem units(getUnitSystem(intehead));
    RestartValue rst_value( ::Opm::RestartIO::restoreSOLUTION( file_view, solution_keys, grid.getNumActive( )),
			    ::Opm::RestartIO::restore_wells( opm_xwel, opm_iwel, sim_step , es, grid, schedule));

    for (const auto& extra : extra_keys) {
	const std::string& key = extra.key;
	bool required = extra.required;

	if (ecl_file_view_has_kw( file_view , key.c_str())) {
	    const ::Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_file_view_iget_named_kw( file_view , key.c_str() , 0 );
	    const double * data_ptr = ::Opm::RestartIO::ecl_kw_get_type_ptr<double>( ecl_kw, ECL_DOUBLE_TYPE );
	    const double * end_ptr  = data_ptr + ::Opm::RestartIO::ecl_kw_get_size( ecl_kw );
	    rst_value.addExtra(key, extra.dim, {data_ptr, end_ptr});
	} else if (required)
	    throw std::runtime_error("No such key in file: " + key);
    }

    // Convert solution fields and extra data from user units to SI
    rst_value.solution.convertToSI(units);
    for (auto & extra_value : rst_value.extra) {
        const auto& restart_key = extra_value.first;
        auto & data = extra_value.second;

        units.to_si(restart_key.dim, data);
    }

    return rst_value;
}

}} // Opm::RestartIO
