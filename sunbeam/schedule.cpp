#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include "converters.hpp"

#include "sunbeam.hpp"


namespace {

    std::vector< Well > get_wells( const Schedule& sch ) {
        std::vector< Well > wells;
        for( const auto& w : sch.getWells() )
            wells.push_back( *w );

        return wells;
    }

    const Well& get_well( const Schedule& sch, const std::string& name ) try {
        return *sch.getWell( name );
    } catch( const std::invalid_argument& e ) {
        throw key_error( name );
    }


    boost::posix_time::ptime get_start_time( const Schedule& s ) {
        return boost::posix_time::from_time_t( s.posixStartTime() );
    }

    boost::posix_time::ptime get_end_time( const Schedule& s ) {
        return boost::posix_time::from_time_t( s.posixEndTime() );
    }

    py::list get_timesteps( const Schedule& s ) {
        namespace time = boost::posix_time;
        const auto& tm = s.getTimeMap();
        std::vector< time::ptime > v;
        v.reserve( tm.size() );

        for( size_t i = 0; i < tm.size(); ++i )
            v.push_back( time::from_time_t(tm[ i ]) );

        return iterable_to_pylist( v );
    }

    py::list get_groups( const Schedule& sch ) {
        std::vector< Group > groups;
        for( const auto& g : sch.getGroups() )
            groups.push_back( *g );

        return iterable_to_pylist( groups );
    }

}

void sunbeam::export_Schedule() {

    py::class_< Schedule >( "Schedule", py::no_init )
        .add_property( "_wells", &get_wells )
        .add_property( "_groups", &get_groups )
        .add_property( "start",  &get_start_time )
        .add_property( "end",    &get_end_time )
        .add_property( "timesteps", &get_timesteps )
        .def( "__contains__", &Schedule::hasWell )
        .def( "__getitem__", &get_well, ref() )
        .def( "_group", &Schedule::getGroup, ref() )
        ;

}
