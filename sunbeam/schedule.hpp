#ifndef SUNBEAM_SCHEDULE_HPP
#define SUNBEAM_SCHEDULE_HPP

#include <boost/python.hpp>


namespace Opm {
    class Schedule;
    class Well;
}

namespace schedule {
    namespace py = boost::python;
    using namespace Opm;


    std::vector< Well > get_wells( const Schedule& sch );
    const Well& get_well( const Schedule& sch, const std::string& name );
    boost::posix_time::ptime get_start_time( const Schedule& s );
    boost::posix_time::ptime get_end_time( const Schedule& s );
    py::list get_timesteps( const Schedule& s );
    py::list get_groups( const Schedule& sch );

    void export_Schedule();

}

#endif //SUNBEAM_SCHEDULE_HPP
