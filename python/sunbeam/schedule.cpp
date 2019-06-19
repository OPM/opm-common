#include <ctime>
#include <chrono>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include "sunbeam.hpp"


namespace {

    using system_clock = std::chrono::system_clock;


    /*
      timezones - the stuff that make you wonder why didn't do social science in
      university. The situation here is as follows:

      1. In the C++ code Eclipse style string literals like "20. NOV 2017" are
         converted to time_t values using the utc based function timegm() which
         does not take timezones into account.

      2. Here we use the function gmtime( ) to convert back from a time_t value
         to a broken down struct tm representation.

      3. The broken down representation is then converted to a time_t value
         using the timezone aware function mktime().

      4. The time_t value is converted to a std::chrono::system_clock value.

      Finally std::chrono::system_clock value is automatically converted to a
      python datetime object as part of the pybind11 process. This latter
      conversion *is* timezone aware, that is the reason we must go through
      these hoops.
    */
    system_clock::time_point datetime( std::time_t utc_time) {
        struct tm utc_tm;
        time_t local_time;

        gmtime_r(&utc_time, &utc_tm);
        local_time = mktime(&utc_tm);

        return system_clock::from_time_t(local_time);
    }

    const Well2& get_well( const Schedule& sch, const std::string& name, const size_t& timestep ) try {
        return sch.getWell2( name, timestep );
    } catch( const std::invalid_argument& e ) {
        throw py::key_error( name );
    }

    const GroupTree& get_grouptree ( const Schedule& sch, const size_t& timestep) {
        return sch.getGroupTree(timestep);
    }
    system_clock::time_point get_start_time( const Schedule& s ) {
        return datetime(s.posixStartTime());
    }

    system_clock::time_point get_end_time( const Schedule& s ) {
        return datetime(s.posixEndTime());
    }

    std::vector<system_clock::time_point> get_timesteps( const Schedule& s ) {
        const auto& tm = s.getTimeMap();
        std::vector< system_clock::time_point > v;
        v.reserve( tm.size() );

        for( size_t i = 0; i < tm.size(); ++i )
            v.push_back( datetime( tm[ i ] ));

        return v;
    }

    std::vector<Group> get_groups( const Schedule& sch ) {
        std::vector< Group > groups;
        for( const auto& g : sch.getGroups() )
            groups.push_back( *g );
        return groups;
    }

    bool has_well( const Schedule& sch, const std::string& wellName) {
        return sch.hasWell( wellName );
    }

}

void sunbeam::export_Schedule(py::module& module) {

    py::class_< Schedule >( module, "Schedule")
    .def_property_readonly( "_groups", &get_groups )
    .def_property_readonly( "start",  &get_start_time )
    .def_property_readonly( "end",    &get_end_time )
    .def_property_readonly( "timesteps", &get_timesteps )
    .def( "get_wells", &Schedule::getWells2)
    .def("_getwell", &get_well)
    .def( "__contains__", &has_well )
    .def( "_group", &Schedule::getGroup, ref_internal)
    .def( "_group_tree", &get_grouptree, ref_internal);

}
