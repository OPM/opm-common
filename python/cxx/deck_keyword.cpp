#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>

#include <opm/parser/eclipse/Deck/DeckValue.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Utility/Typetools.hpp>

#include "export.hpp"
#include "converters.hpp"

#include <iostream>


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
            throw py::error_already_set();
        }
        return item_to_pylist(*(it++));
    }
};


bool is_int(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}


void push_string_as_deck_value(std::vector<DeckValue>& record, const std::string str) {

    std::size_t star_pos = str.find('*');
    if (star_pos != std::string::npos) {
        int multiplier = 1;
        
        std::string mult_str = str.substr(0, star_pos);

        if (mult_str.length() > 0) {
            if (is_int(mult_str))
                multiplier = std::stoi( mult_str );
            else
                throw py::type_error();
        }

        std::string value_str = str.substr(star_pos + 1, str.length());
        DeckValue value;

        if (value_str.length() > 0) {
            if (is_int(value_str))
                value = DeckValue( stoi(value_str) );
            else
                value = DeckValue( stod(value_str) );
        }

        for (int i = 0; i < multiplier; i++)
            record.push_back( value );
      
    }
    else
        record.push_back( DeckValue(str) );

}

}

void python::common::export_DeckKeyword(py::module& module) {
    py::class_< DeckKeyword >( module, "DeckKeyword")
        .def(py::init<const ParserKeyword& >())

        .def(py::init([](const ParserKeyword& parser_keyword, py::list record_list) {

            std::vector< std::vector<DeckValue> > value_record_list;

            for (py::handle record_obj : record_list) {
                 py::list record = record_obj.cast<py::list>();
                 std::vector<DeckValue> value_record;

                 for (py::handle value_obj : record) { 

                      try {
                          int val_int = value_obj.cast<int>();
                          value_record.push_back( DeckValue(val_int) );
                          continue;
                      }
                      catch (std::exception e_int) {}

                      try {
                           double val_double = value_obj.cast<double>();
                           value_record.push_back( DeckValue(val_double) );
                           continue;
                      }
                      catch (std::exception e_double) {}

                      try {
                           std::string val_string = value_obj.cast<std::string>();                                      
                           push_string_as_deck_value(value_record, val_string);
                           continue;
                      }
                      catch (std::exception e_string) {}

                      throw py::type_error("DeckKeyword: tried to add unkown type to record.");
             
                 }
                 value_record_list.push_back( value_record );
             }
             UnitSystem unit_system(UnitSystem::UnitType::UNIT_TYPE_METRIC);
             return DeckKeyword(parser_keyword, value_record_list, unit_system, unit_system);
         }  )  )

        .def( "__repr__", &DeckKeyword::name )
        .def( "__str__", &str<DeckKeyword> )
        .def("__iter__",  [] (const DeckKeyword &keyword) { return py::make_iterator(keyword.begin(), keyword.end()); }, py::keep_alive<0,1>())
        .def( "__getitem__", getRecord, ref_internal)
        .def( "__len__", &DeckKeyword::size )
        .def_property_readonly("name", &DeckKeyword::name )
        ;


    py::class_< DeckRecord >( module, "DeckRecord")
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


    py::class_< DeckRecordIterator >( module, "DeckRecordIterator")
        .def( "__next__", &DeckRecordIterator::next )
        .def( "next", &DeckRecordIterator::next )
        ;

}
