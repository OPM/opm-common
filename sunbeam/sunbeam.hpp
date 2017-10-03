#ifndef SUNBEAM_HPP
#define SUNBEAM_HPP

#include <boost/python.hpp>

#include "converters.hpp"

namespace Opm { }
namespace py = boost::python;

using namespace Opm;
using ref = py::return_internal_reference<>;
using copy = py::return_value_policy< py::copy_const_reference >;

namespace sunbeam {

void export_Completion();
void export_Deck();
void export_DeckKeyword();
void export_Eclipse3DProperties();
void export_EclipseConfig();
void export_EclipseGrid();
void export_EclipseState();
void export_Group();
void export_Parser();
void export_Schedule();
void export_TableManager();
void export_Well();
void export_GroupTree();

}

#endif //SUNBEAM_HPP
