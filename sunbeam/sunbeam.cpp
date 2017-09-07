#include <boost/python.hpp>

#include "converters.hpp"
#include "sunbeam.hpp"

BOOST_PYTHON_MODULE(libsunbeam) {

    /*
     * Python C-API requires this macro to be invoked before anything from
     * datetime.h is used.
     */
    PyDateTime_IMPORT;

    /* register all converters */
    py::to_python_converter< const boost::posix_time::ptime,
                             ptime_to_python_datetime >();

    py::register_exception_translator< key_error >( &key_error::translate );

    sunbeam::export_Completion();
    sunbeam::export_Deck();
    sunbeam::export_DeckKeyword();
    sunbeam::export_Eclipse3DProperties();
    sunbeam::export_EclipseConfig();
    sunbeam::export_EclipseGrid();
    sunbeam::export_EclipseState();
    sunbeam::export_Group();
    sunbeam::export_Parser();
    sunbeam::export_Schedule();
    sunbeam::export_TableManager();
    sunbeam::export_Well();


}
