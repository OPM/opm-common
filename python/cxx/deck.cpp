#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <pybind11/pybind11.h>
#include "converters.hpp"
#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

namespace {

    size_t size( const Deck& deck ) {
        return deck.size();
    }


    size_t count( const Deck& deck, const std::string& kw ) {
        return deck.count(kw);
    }

    bool hasKeyword( const Deck& deck, const std::string& kw ) {
        return deck.hasKeyword(kw);
    }

    const DeckKeyword& getKeyword_tuple( const Deck& deck, py::tuple kw_index ) {
        const std::string kw = py::cast<const std::string>(kw_index[0]);
        const size_t index = py::cast<size_t>(kw_index[1]);
        return deck[kw][index];
    }

    const DeckKeyword& getKeyword_string( const Deck& deck, const std::string& kw ) {
        return deck[kw].back();
    }

    const DeckKeyword& getKeyword_int( const Deck& deck, size_t index ) {
        return deck[index];
    }

    //This adds a keyword by copy
    void addKeyword(Deck& deck, const DeckKeyword kw) {
         deck.addKeyword(kw);
    }


}

void python::common::export_Deck(py::module &module) {

    using namespace Opm::Common::DocStrings;

    // Note: In the below class we use std::shared_ptr as the holder type, see:
    //
    //  https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
    //
    // this makes it possible to share the returned object with e.g. and
    //   opm.simulators.BlackOilSimulator Python object
    //
    py::class_<Deck, std::shared_ptr<Deck>>(module, "Deck", Deck_docstring)
        .def("__len__", &size, Deck_len_docstring)
        .def("__contains__", &hasKeyword, py::arg("keyword"), Deck_contains_docstring)
        .def("__iter__",
             [](const Deck &deck) { return py::make_iterator(deck.begin(), deck.end()); },
             py::keep_alive<0, 1>(), Deck_iter_docstring)
        .def("__getitem__", &getKeyword_int, py::arg("index"), ref_internal, Deck_getitem_int_docstring)
        .def("__getitem__", &getKeyword_string, py::arg("keyword"), ref_internal, Deck_getitem_string_docstring)
        .def("__getitem__", &getKeyword_tuple, py::arg("keyword_index"), ref_internal, Deck_getitem_tuple_docstring)
        .def("__str__", &str<Deck>, Deck_str_docstring)
        .def("active_unit_system",
             [](const Deck& deck) -> const UnitSystem& { return deck.getActiveUnitSystem(); },
             Deck_active_unit_system_docstring)
        .def("default_unit_system",
             [](const Deck& deck) -> const UnitSystem& { return deck.getDefaultUnitSystem(); },
             Deck_default_unit_system_docstring)
        .def("count", &count, py::arg("keyword"), Deck_count_docstring)
        .def("add", &addKeyword, py::arg("keyword"), Deck_add_docstring);

}


