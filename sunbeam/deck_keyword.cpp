#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Utility/Typetools.hpp>

#include "sunbeam.hpp"
#include "converters.hpp"


namespace {

/* DeckKeyword */
const DeckRecord& (DeckKeyword::*getRecord)(size_t index) const = &DeckKeyword::getRecord;

py::list item_to_pylist( const DeckItem& item )
{
    switch (item.getType())
    {
    case type_tag::integer:
        return iterable_to_pylist( item.getData< int >() );
        break;
    case type_tag::fdouble:
        return iterable_to_pylist( item.getData< double >() );
        break;
    case type_tag::string:
        return iterable_to_pylist( item.getData< std::string >() );
        break;
    default:
        throw std::logic_error( "Type not set." );
        break;
    }
}

struct DeckRecordIterator
{
    DeckRecordIterator(const DeckRecord* record) {
        this->record = record;
        this->it = this->record->begin();
    }
    const DeckRecord* record;
    DeckRecord::const_iterator it;
    py::list next() {
        if (it == record->end()) {
            PyErr_SetString(PyExc_StopIteration, "At end.");
            py::throw_error_already_set();
        }
        return item_to_pylist(*(it++));
    }
};

}

void sunbeam::export_DeckKeyword() {
    py::class_< DeckKeyword >( "DeckKeyword", py::no_init )
        .def( "__repr__", &DeckKeyword::name, copy() )
        .def( "__str__", &str<DeckKeyword> )
        .def( "__iter__", py::range< ref >( &DeckKeyword::begin, &DeckKeyword::end ))
        .def( "__getitem__", getRecord, ref() )
        .def( "__len__", &DeckKeyword::size )
        .add_property("name", py::make_function( &DeckKeyword::name, copy() ))
        ;

    py::class_< DeckRecord >( "DeckRecord", py::no_init )
        .def( "__repr__", &str<DeckRecord> )
        .def( "__iter__", +[](const DeckRecord& record){
            return DeckRecordIterator(&record);
        })
        .def( "__getitem__", +[](const DeckRecord& record, size_t index){
            return item_to_pylist( record.getItem(index) );
        })
        .def( "__getitem__", +[](const DeckRecord& record, const std::string& name){
            return item_to_pylist( record.getItem(name) );
        })
        .def( "__len__", &DeckRecord::size )
        ;

    py::class_< DeckRecordIterator >( "DeckRecordIterator", py::no_init )
        .def( "__next__", &DeckRecordIterator::next )
        .def( "next", &DeckRecordIterator::next )
        ;
}
