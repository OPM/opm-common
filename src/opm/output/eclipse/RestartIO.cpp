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

#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/AggregateWellData.hpp>
#include <opm/output/eclipse/AggregateConnectionData.hpp>
#include <opm/output/eclipse/AggregateMSWData.hpp>
#include <opm/output/eclipse/SummaryState.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <string>
#include <vector>
#include <cstddef>
#include <time.h>
#include <ctime>
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <cstdint>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
//#include <ert/util/@TYPE@_vector.h>



//  This function is purely a helper function for util_abort().
// start define for util_abort
//#define __USE_GNU       // Must be defined to get access to the dladdr() function; Man page says the symbol should be: _GNU_SOURCE but that does not seem to work?
//#define _GNU_SOURCE     // Must be defined _before_ #include <errno.h> to get the symbol 'program_invocation_name'.

//#include <errno.h>
//#include <stdlib.h>
//#include <string.h>

//#include <ert/util/util.h>
//#include <ert/util/test_util.h>

//#include <stdbool.h>

//#include <dlfcn.h>
//#include <execinfo.h>
//#include <pthread.h>
//#include <pwd.h>
//#include <signal.h>
//#include <unistd.h>

//#if !defined(__GLIBC__)         /* note: not same as __GNUC__ */
//#  if defined (__APPLE__)
//#    include <stdlib.h>         /* alloca   */
//#    include <sys/syslimits.h>  /* PATH_MAX */
//#    include <mach-o/dyld.h>    /* _NSGetExecutablePath */
//#  elif defined (__LINUX__)
//#    include <stdlib.h>         /* alloca   */
//#    include <limits.h>         /* PATH_MAX */
//#    include <unistd.h>         /* readlink */
//#  else
//#    error No known program_invocation_name in runtime library
//#  endif
//#endif

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Eqldims.hpp>

#include <opm/output/eclipse/libECLRestart.hpp>

//#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/EclFilename.hpp>
//#include <ert/ecl/ecl_kw_magic.h>
//#include <ert/ecl/ecl_init_file.h>
//#include <ert/ecl/ecl_file.h>
//#include <ert/ecl/ecl_kw.h>
//#include <ert/ecl/ecl_type.h>
//#include <ert/ecl/ecl_grid.h>
//#include <ert/ecl/ecl_util.h>
//#include <ert/ecl/ecl_rft_file.h>
//#include <ert/ecl/ecl_file_view.h>
//#include <ert/ecl_well/well_const.h>
//#include <ert/ecl/ecl_rsthead.h>
//#include <ert/util/util.h>
#include <ert/ecl/fortio.h>
#define OPM_XWEL      "OPM_XWEL"
#define OPM_IWEL      "OPM_IWEL"
#define IWEL_KW      "IWEL"
#define ZWEL_KW      "ZWEL"
#define ICON_KW      "ICON"


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

namespace Opm {
namespace RestartIO  {

//namespace {
    /*
      The RestartValue structure has an 'extra' container which can be used to
      add extra fields to the restart file. The extra field is used both to add
      OPM specific fields like 'OPMEXTRA', and eclipse standard fields like
      THRESHPR. In the case of e.g. THRESHPR this should - if present - be added
      in the SOLUTION section of the restart file. The std::set extra_solution
      just enumerates the keys which should be in the solution section.
    */

    static const std::set<std::string> extra_solution = {"THRESHPR"};



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
	    if( !grid.cellActive( i, j, k ) || sc.state() == WellCompletion::SHUT ) {
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

    if (!ecl_file_view_has_kw(file_view, "OPM_XWEL"))
        throw std::runtime_error("Sorry - this file is missing the OPM_XWEL keyword - required for flow based restart\n");

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

namespace {


std::vector< int > serialize_OPM_IWEL( const data::Wells& wells,
				       const std::vector< const Well* >& sched_wells ) {

    const auto getctrl = [&]( const Well* w ) {
	const auto itr = wells.find( w->name() );
	return itr == wells.end() ? 0 : itr->second.control;
    };

    std::vector< int > iwel( sched_wells.size(), 0.0 );
    std::transform( sched_wells.begin(), sched_wells.end(), iwel.begin(), getctrl );
    return iwel;
}

std::vector< double > serialize_OPM_XWEL( const data::Wells& wells,
                                          int sim_step,
                                          const std::vector< const Well* >& sched_wells,
                                          const Phases& phase_spec,
                                          const EclipseGrid& grid ) {

    using rt = data::Rates::opt;

    std::vector< rt > phases;
    if( phase_spec.active( Phase::WATER ) ) phases.push_back( rt::wat );
    if( phase_spec.active( Phase::OIL ) )   phases.push_back( rt::oil );
    if( phase_spec.active( Phase::GAS ) )   phases.push_back( rt::gas );

    std::vector< double > xwel;
    for( const auto* sched_well : sched_wells ) {

	if( wells.count( sched_well->name() ) == 0 || sched_well->getStatus(sim_step) == Opm::WellCommon::SHUT) {
	    const auto elems = (sched_well->getConnections( sim_step ).size()
			       * (phases.size() + data::Connection::restart_size))
		+ 2 /* bhp, temperature */
		+ phases.size();

	    // write zeros if no well data is provided
	    xwel.insert( xwel.end(), elems, 0.0 );
	    continue;
	}

	const auto& well = wells.at( sched_well->name() );

	xwel.push_back( well.bhp );
	xwel.push_back( well.temperature );
	for( auto phase : phases )
	    xwel.push_back( well.rates.get( phase ) );

	for( const auto& sc : sched_well->getConnections( sim_step ) ) {
	    const auto i = sc.getI(), j = sc.getJ(), k = sc.getK();

	    const auto rs_size = phases.size() + data::Connection::restart_size;
	    if( !grid.cellActive( i, j, k ) || sc.state() == WellCompletion::SHUT ) {
		xwel.insert( xwel.end(), rs_size, 0.0 );
		continue;
	    }

	    const auto active_index = grid.activeIndex( i, j, k );
	    const auto at_index = [=]( const data::Connection& c ) {
		return c.index == active_index;
	    };
	    const auto& connection = std::find_if( well.connections.begin(),
						   well.connections.end(),
						   at_index );

	    if( connection == well.connections.end() ) {
		xwel.insert( xwel.end(), rs_size, 0.0 );
		continue;
	    }

	    xwel.push_back( connection->pressure );
	    xwel.push_back( connection->reservoir_rate );
	    for( auto phase : phases )
		xwel.push_back( connection->rates.get( phase ) );
	}

    }

    return xwel;
}


std::vector<const char*> serialize_ZWEL( const std::vector<Opm::RestartIO::Helpers::CharArrayNullTerm<8>>& zwel) {
    std::vector<const char*> data( zwel.size( ), "");
    size_t it = 0;

    for (const auto& well : zwel) {
	data[ it ] = well.c_str();
	it += 1;
    }
    return data;
}

template< typename T >
void write_kw(::Opm::RestartIO::ecl_rst_file_type * rst_file , Opm::RestartIO::EclKW< T >&& kw) {
  ::Opm::RestartIO::ecl_rst_file_add_kw( rst_file, kw.get() );
}

std::vector<int>
writeHeader(::Opm::RestartIO::ecl_rst_file_type* rst_file,
		 int                 sim_step,
		 int                 report_step,
		 double              simTime,
		 int                 ert_phase_mask,
		 const UnitSystem&   units,
		 const Schedule&     schedule,
		 const EclipseGrid&  grid,
		 const EclipseState& es)
{
    if (rst_file->unified)
        ::Opm::RestartIO::ecl_rst_file_fwrite_SEQNUM(rst_file, report_step);

    // write INTEHEAD to restart file
    const auto ih = Helpers::createInteHead(es, grid, schedule, simTime, sim_step, sim_step);

    write_kw(rst_file, EclKW<int>("INTEHEAD", ih));

    // write LOGIHEAD to restart file
    const auto lh = Helpers::createLogiHead(es);

    write_kw(rst_file, EclKW<bool>("LOGIHEAD", lh));

    // write DOUBHEAD to restart file
    const auto dh = Helpers::createDoubHead(es, schedule, sim_step, simTime);

    write_kw(rst_file, EclKW<double>("DOUBHEAD", dh));

    // return the inteHead vector
    return ih;
}


void writeMSWData(::Opm::RestartIO::ecl_rst_file_type * rst_file,
		 int                 sim_step,
		 double              simTime,
		 int                 ert_phase_mask,
		 const UnitSystem&   units,
		 const Schedule&     schedule,
		 const EclipseGrid&  grid,
		 const EclipseState& es,
		 const SummaryState& sumState,
		 const std::vector<int>& ih
		)
{
    // write ISEG, RSEG, ILBS and ILBR to restart file
    const size_t simStep = static_cast<size_t> (sim_step);
    auto  MSWData = Helpers::AggregateMSWData(ih);
    MSWData.captureDeclaredMSWData(schedule, simStep, units, ih, grid);
    write_kw(rst_file, EclKW<int>   ("ISEG", MSWData.getISeg()));
    write_kw(rst_file, EclKW<int>   ("ILBS", MSWData.getILBs()));
    write_kw(rst_file, EclKW<int>   ("ILBR", MSWData.getILBr()));
    write_kw(rst_file, EclKW<double>("RSEG", MSWData.getRSeg()));
}

void writeGroup(::Opm::RestartIO::ecl_rst_file_type * rst_file,
		 int                      sim_step,
		 double                   simTime,
		 int                      ert_phase_mask,
		 const UnitSystem&        units,
		 const Schedule&          schedule,
		 const EclipseGrid&       grid,
		 const EclipseState&      es,
		 const Opm::SummaryState& sumState,
		 const std::vector<int>&  ih)
{
    // write IGRP to restart file
    //std::cout << "writeGroup before initializing groupData" << std::endl;
    const size_t simStep = static_cast<size_t> (sim_step);

    auto  groupData = Helpers::AggregateGroupData(ih);

    auto & rst_g_keys = groupData.restart_group_keys;
    auto & rst_f_keys = groupData.restart_field_keys;
    auto & grpKeyToInd = groupData.groupKeyToIndex;
    auto & fldKeyToInd = groupData.fieldKeyToIndex;

    groupData.captureDeclaredGroupData(schedule, rst_g_keys, rst_f_keys, grpKeyToInd, fldKeyToInd, simStep, sumState, ih);

    write_kw(rst_file, EclKW<int>   ("IGRP", groupData.getIGroup()));
    write_kw(rst_file, EclKW<float> ("SGRP", groupData.getSGroup()));
    write_kw(rst_file, EclKW<double>("XGRP", groupData.getXGroup()));
}


  ::Opm::RestartIO::ert_unique_ptr< ::Opm::RestartIO::ecl_kw_type, ecl_kw_free >
  ecl_kw( const std::string& kw, const std::vector<double>& data, bool write_double)
  {
      Opm::RestartIO::ert_unique_ptr< ::Opm::RestartIO::ecl_kw_type, ecl_kw_free > kw_ptr;

      if (write_double) {
	  ::Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_kw_alloc( kw.c_str() , data.size() , ECL_DOUBLE );
	  ::Opm::RestartIO::ecl_kw_set_memcpy_data( ecl_kw , data.data() );
	  kw_ptr.reset( ecl_kw );
      } else {
	  ::Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_kw_alloc( kw.c_str() , data.size() , ECL_FLOAT );
	  //float * float_data = ecl_kw_get_float_ptr( ecl_kw );
	  float * float_data = ecl_kw_get_type_ptr<float>( ecl_kw, ECL_FLOAT_TYPE );
	  for (size_t i=0; i < data.size(); i++)
	    float_data[i] = data[i];
	  kw_ptr.reset( ecl_kw );
      }

      return kw_ptr;
  }



  void writeSolution(ecl_rst_file_type* rst_file, const RestartValue& value, bool write_double) {
      ecl_rst_file_start_solution( rst_file );
      for (const auto& elm: value.solution) {
          if (elm.second.target == data::TargetType::RESTART_SOLUTION)
              ecl_rst_file_add_kw( rst_file , ecl_kw(elm.first, elm.second.data, write_double).get());
      }
      for (const auto& elm: value.extra) {
          const std::string& key = elm.first.key;
          const std::vector<double>& data = elm.second;
          if (extra_solution.find(key) != extra_solution.end())
              /*
                Observe that the extra data is unconditionally written in double precision.
              */
              ecl_rst_file_add_kw(rst_file, ecl_kw(key, data, true).get());
      }
      ecl_rst_file_end_solution( rst_file );

      for (const auto& elm: value.solution) {
          if (elm.second.target == data::TargetType::RESTART_AUXILIARY)
              ecl_rst_file_add_kw( rst_file , ecl_kw(elm.first, elm.second.data, write_double).get());
      }
      //  temporarily comment out jals original version
      /*void writeSolution(::Opm::RestartIO::ecl_rst_file_type* rst_file, const data::Solution& solution, bool write_double) {
     void writeSolution(::Opm::RestartIO::ecl_rst_file_type* rst_file, const data::Solution& solution, bool write_double) {
    void writeSolution(::Opm::RestartIO::ecl_rst_file_type* rst_file, const data::Solution& solution, const RestartValue::ExtraVector& extra_data, bool write_double) {
    ::Opm::RestartIO::ecl_rst_file_start_solution( rst_file );
    for (const auto& elm: solution) {
	if (elm.first == "TEMP") continue;
	if (elm.second.target == data::TargetType::RESTART_SOLUTION)
	    ::Opm::RestartIO::ecl_rst_file_add_kw( rst_file , ecl_kw(elm.first, elm.second.data, write_double).get());
     }
     
     for (const auto& extra_value : extra_data) {
	const std::string& key = extra_value.first.key;
	const std::vector<double>& data = extra_value.second;
	if ( key == "THPRES") 
	{
	     ::Opm::RestartIO::ecl_rst_file_add_kw( rst_file , ecl_kw(key, data, write_double).get());
	}
     }	
     ::Opm::RestartIO::ecl_rst_file_end_solution( rst_file );

     for (const auto& elm: solution) {
	if (elm.second.target == data::TargetType::RESTART_AUXILIARY)
	    ::Opm::RestartIO::ecl_rst_file_add_kw( rst_file , ecl_kw(elm.first, elm.second.data, write_double).get());
     }
  }


void writeExtraData(::Opm::RestartIO::ecl_rst_file_type* rst_file, const RestartValue::ExtraVector& extra_data) {
    for (const auto& extra_value : extra_data) {
        const std::string& key = extra_value.first.key;
        const std::vector<double>& data = extra_value.second;
        if (extra_solution.find(key) == extra_solution.end()) {
            ecl_kw_type * ecl_kw = ecl_kw_alloc_new_shared( key.c_str() , data.size() , ECL_DOUBLE , const_cast<double *>(data.data()));
            ecl_rst_file_add_kw( rst_file , ecl_kw);
            ecl_kw_free( ecl_kw );
        }

	  /*const std::string& key = extra_value.first;
	const std::string& key = extra_value.first.key;
	const std::vector<double>& data = extra_value.second;
	if (key != "THPRES")
	{
	    ::Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_kw_alloc_new_shared( key.c_str() , data.size() , ECL_DOUBLE , const_cast<double *>(data.data()));
	    ::Opm::RestartIO::ecl_rst_file_add_kw( rst_file , ecl_kw);
	    ::Opm::RestartIO::ecl_kw_free( ecl_kw );
	}*/
    }
}


void writeWell(::Opm::RestartIO::ecl_rst_file_type* 	rst_file, 
	       int 					sim_step, 
	       const UnitSystem&        		units,
	       const EclipseState& 			es , 
	       const EclipseGrid& 			grid, 
	       const Schedule& 				schedule, 
	       const data::Wells& 			wells,
	       const Opm::SummaryState& 		sumState,
	       const std::vector<int>&  		ih)
{
    auto wellData = Helpers::AggregateWellData(ih);
    wellData.captureDeclaredWellData(schedule, units, sim_step, sumState, ih);
    wellData.captureDynamicWellData(schedule, sim_step, wells, sumState);

    auto connectionData = Helpers::AggregateConnectionData(ih);
    connectionData.captureDeclaredConnData(schedule, grid, units, sim_step);
    
    std::vector<const char*> zwel_data = serialize_ZWEL( wellData.getZWell() );
    ::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< int >	( "IWEL", wellData.getIWell()) );
    ::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< float >	( "SWEL", wellData.getSWell() ) );
    ::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< double >	( "XWEL", wellData.getXWell() ) );
    ::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< const char* >( "ZWEL", zwel_data) );

    if (!es.getIOConfig().getEclCompatibleRST()) {
        const auto opm_xwel  = serialize_OPM_XWEL( wells, sim_step, sched_wells, phases, grid );
        const auto opm_iwel  = serialize_OPM_IWEL( wells, sched_wells );
	::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< double >( OPM_XWEL, opm_xwel ) );
	::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< int >( OPM_IWEL, opm_iwel ) );
    }

    ::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< int >	( "ICON", connectionData.getIConn() ) );
    ::Opm::RestartIO::write_kw( rst_file, ::Opm::RestartIO::EclKW< float >	( "SCON", connectionData.getSConn() ) );
}

void checkSaveArguments(const EclipseState& es,
                        const RestartValue& restart_value,
                        const EclipseGrid& grid) {

  for (const auto& elm: restart_value.solution)
    if (elm.second.data.size() != grid.getNumActive())
      throw std::runtime_error("Wrong size on solution vector: " + elm.first);


  if (es.getSimulationConfig().getThresholdPressure().size() > 0) {
      // If the the THPRES option is active the restart_value should have a
      // THPRES field. This is not enforced here because not all the opm
      // simulators have been updated to include the THPRES values.
      if (!restart_value.hasExtra("THRESHPR")) {
          OpmLog::warning("This model has THPRES active - should have THPRES as part of restart data.");
          return;
      }

      size_t num_regions = es.getTableManager().getEqldims().getNumEquilRegions();
      const auto& thpres = restart_value.getExtra("THRESHPR");
      if (thpres.size() != num_regions * num_regions)
          throw std::runtime_error("THPRES vector has invalid size - should have num_region * num_regions.");

		const RestartValue& restart_value,
			const EclipseGrid& grid) {

    for (const auto& elm: restart_value.solution)
	if (elm.second.data.size() != grid.getNumActive())
	    throw std::runtime_error("Wrong size on solution vector: " + elm.first);


	    /*if (es.getSimulationConfig().getThresholdPressure().size() > 0) {
	// If the the THPRES option is active the restart_value should have a
	// THPRES field. This is not enforced here because not all the opm
	// simulators have been updated to include the THPRES values.
	if (!restart_value.hasExtra("THPRES")) {
	    OpmLog::warning("This model has THPRES active - should have THPRES as part of restart data.");
	    return;
	}

	size_t num_regions = es.getTableManager().getEqldims().getNumEquilRegions();
	const auto& thpres = restart_value.getExtra("THPRES");
	if (thpres.size() != num_regions * num_regions)
	    throw std::runtime_error("THPRES vector has invalid size - should have num_region * num_regions.");

      size_t num_regions = es.getTableManager().getEqldims().getNumEquilRegions();
      const auto& thpres = restart_value.getExtra("THPRES");
      if (thpres.size() != num_regions * num_regions)
          throw std::runtime_error("THPRES vector has invalid size - should have num_region * num_regions.");*/
  }
}
} // Anonymous namespace


void save(const std::string&  filename,
	  int                 report_step,
	  double              seconds_elapsed,
	  RestartValue        value,
	  const EclipseState& es,
	  const EclipseGrid&  grid,
	  const Schedule&     schedule,
	  const SummaryState& sumState,
	  bool                write_double)
{
    ::Opm::RestartIO::checkSaveArguments(es, value, grid);
    {
        bool ecl_compatible_rst = es.getIOConfig().getEclCompatibleRST();
	int sim_step = std::max(report_step - 1, 0);
	int ert_phase_mask = es.runspec().eclPhaseMask( );
	const auto& units = es.getUnits();

	::Opm::RestartIO::ert_unique_ptr< ::Opm::RestartIO::ecl_rst_file_type, ::Opm::RestartIO::ecl_rst_file_close > rst_file;

	if (::Opm::RestartIO::EclFiletype( filename ) == ECL_UNIFIED_RESTART_FILE)
	    rst_file.reset( ::Opm::RestartIO::ecl_rst_file_open_write_seek( filename.c_str(), report_step ) );
	else
	    rst_file.reset( ::Opm::RestartIO::ecl_rst_file_open_write( filename.c_str() ) );

        if (ecl_compatible_rst)
            write_double = false;

	// Convert solution fields and extra values from SI to user units.
	value.solution.convertFromSI(units);
	for (auto & extra_value : value.extra) {
	    const auto& restart_key = extra_value.first;
	    auto & data = extra_value.second;

	    units.from_si(restart_key.dim, data);
	}

        std::vector<int> inteHD = ::Opm::RestartIO::writeHeader(rst_file.get() , sim_step, report_step, seconds_elapsed, ert_phase_mask, units, schedule , grid, es);
        ::Opm::RestartIO::writeGroup(rst_file.get() , sim_step, seconds_elapsed, ert_phase_mask, units, schedule, grid, es, sumState, inteHD);
	::Opm::RestartIO::writeMSWData(rst_file.get() , sim_step, seconds_elapsed, ert_phase_mask, units, schedule, grid, es, sumState, inteHD);
        ::Opm::RestartIO::writeWell( rst_file.get(), sim_step, es , grid, schedule, value.wells);
        ::Opm::RestartIO::writeSolution( rst_file.get(), value, write_double );
        if (!ecl_compatible_rst)
	  ::Opm::RestartIO:: writeExtraData( rst_file.get(), value.extra );
    }
}

}} // Opm::RestartIO
