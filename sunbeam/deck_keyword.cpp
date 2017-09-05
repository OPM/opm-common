#include <iostream>
#include <sstream>
#include <boost/python.hpp>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include "deck_keyword.hpp"

namespace py = boost::python;
using namespace Opm;

// using ref = py::return_internal_reference<>;
using copy = py::return_value_policy< py::copy_const_reference >;


namespace deck_keyword {

    std::string write( const DeckKeyword& kw ) {
        std::stringstream stream;
        stream << kw;
        return stream.str();
    }

    void export_DeckKeyword() {

        py::class_< DeckKeyword >( "DeckKeyword", py::no_init )
            .def( "__repr__", &DeckKeyword::name, copy() )
            .def( "__str__", &write )
            .def( "__len__", &DeckKeyword::size )
            ;

    }

}
