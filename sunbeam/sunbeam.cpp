
#include "converters.hpp"
#include "completion.hpp"
#include "eclipse_3d_properties.hpp"
#include "eclipse_config.hpp"
#include "eclipse_grid.hpp"
#include "eclipse_state.hpp"
#include "group.hpp"
#include "schedule.hpp"
#include "table_manager.hpp"
#include "well.hpp"


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

    eclipse_state::export_EclipseState();
    eclipse_config::export_EclipseConfig();
    completion::export_Completion();
    eclipse_3d_properties::export_Eclipse3DProperties();
    eclipse_grid::export_EclipseGrid();
    group::export_Group();
    schedule::export_Schedule();
    table_manager::export_TableManager();
    well::export_Well();

    // parser::export_Parser();
    // deck::export_Deck();
    // deck_keyword::export_DeckKeyword();

}
