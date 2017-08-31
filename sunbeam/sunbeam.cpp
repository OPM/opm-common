

#include "completion.hpp"
#include "converters.hpp"
#include "deck.hpp"
#include "deck_keyword.hpp"
#include "eclipse_3d_properties.hpp"
#include "eclipse_config.hpp"
#include "eclipse_grid.hpp"
#include "eclipse_state.hpp"
#include "group.hpp"
#include "parser.hpp"
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

    completion::export_Completion();
    deck::export_Deck();
    deck_keyword::export_DeckKeyword();
    eclipse_3d_properties::export_Eclipse3DProperties();
    eclipse_config::export_EclipseConfig();
    eclipse_grid::export_EclipseGrid();
    eclipse_state::export_EclipseState();
    group::export_Group();
    parser::export_Parser();
    schedule::export_Schedule();
    table_manager::export_TableManager();
    well::export_Well();


}
