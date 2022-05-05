#include <ctime>
#include <chrono>
#include <map>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <fmt/format.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include "export.hpp"


namespace {

    using system_clock = std::chrono::system_clock;


    /*
      timezones - the stuff that make you wonder why didn't do social science in
      university. The situation here is as follows:

      1. In the C++ code Eclipse style string literals like "20. NOV 2017" are
         converted to time_t values using the utc based function timegm() which
         does not take timezones into account.

      2. Here we use the function gmtime( ) to convert back from a time_t value
         to a broken down struct tm representation.

      3. The broken down representation is then converted to a time_t value
         using the timezone aware function mktime().

      4. The time_t value is converted to a std::chrono::system_clock value.

      Finally std::chrono::system_clock value is automatically converted to a
      python datetime object as part of the pybind11 process. This latter
      conversion *is* timezone aware, that is the reason we must go through
      these hoops.
    */
    system_clock::time_point datetime( std::time_t utc_time) {
        struct tm utc_tm;
        time_t local_time;

        gmtime_r(&utc_time, &utc_tm);
        local_time = mktime(&utc_tm);

        return system_clock::from_time_t(local_time);
    }

    const Well& get_well( const Schedule& sch, const std::string& name, const size_t& timestep ) try {
        return sch.getWell( name, timestep );
    } catch( const std::invalid_argument& e ) {
        throw py::key_error( name );
    }

    std::map<std::string, double> get_production_properties(
        const Schedule& sch, const std::string& name, const size_t& timestep)
    {
        const Well* well = nullptr;
        try{
            well = &(sch.getWell( name, timestep ));
        } catch( const std::invalid_argument& e ) {
            throw py::key_error( fmt::format("well {} is not defined", name ));
        }
        if (well->isProducer()) {
            auto& prod_prop = well->getProductionProperties();
            return {
                { "oil_rate", prod_prop.OilRate.get<double>() },
                { "gas_rate", prod_prop.GasRate.get<double>() },
                { "water_rate", prod_prop.WaterRate.get<double>() },
                { "liquid_rate", prod_prop.LiquidRate.get<double>() },
                { "resv_rate", prod_prop.ResVRate.get<double>() },
                { "bhp_target", prod_prop.BHPTarget.get<double>() },
                { "thp_target", prod_prop.THPTarget.get<double>() },
                { "alq_value", prod_prop.ALQValue.get<double>() },
            };
        }
        else {
            throw py::key_error( fmt::format("well {} is not a producer", name) );
        }
    }
    system_clock::time_point get_start_time( const Schedule& s ) {
        return datetime(s.posixStartTime());
    }

    system_clock::time_point get_end_time( const Schedule& s ) {
        return datetime(s.posixEndTime());
    }

    std::vector<system_clock::time_point> get_timesteps( const Schedule& s ) {
        std::vector< system_clock::time_point > v;

        for( size_t i = 0; i < s.size(); ++i )
            v.push_back( datetime( std::chrono::system_clock::to_time_t(s[i].start_time() )));

        return v;
    }

    std::vector<Group> get_groups( const Schedule& sch, size_t timestep ) {
        std::vector< Group > groups;
        for( const auto& group_name : sch.groupNames())
            groups.push_back( sch.getGroup(group_name, timestep) );

        return groups;
    }

    bool has_well( const Schedule& sch, const std::string& wellName) {
        return sch.hasWell( wellName );
    }

    const Group& get_group(const ScheduleState& st, const std::string& group_name) {
        return st.groups.get(group_name);
    }


    const ScheduleState& getitem(const Schedule& sch, std::size_t index) {
        return sch[index];
    }

    void insert_keywords(
        Schedule& sch,
        const std::string& deck_string,
        std::size_t index,
        const UnitSystem& unit_system
    )
    {
        Parser parser;
        std::string str {unit_system.deck_name() + "\n\n" + deck_string};
        auto deck = parser.parseString(str);
        std::vector<DeckKeyword*> keywords;
        for (auto &keyword : deck) {
            keywords.push_back(&keyword);
        }
        sch.applyKeywords(keywords, index);
    }

    // NOTE: this overload does currently not work, see PR #2833. The plan
    //  is to fix this in a later commit. For now, the overload insert_keywords()
    //  above taking a deck_string (std::string) instead of a list of DeckKeywords
    //  has to be used instead.
    void insert_keywords(
        Schedule& sch, py::list& deck_keywords, std::size_t index)
    {
        Parser parser;
        std::vector<DeckKeyword*> keywords;
        for (py::handle item : deck_keywords) {
            DeckKeyword &keyword = item.cast<DeckKeyword&>();
            keywords.push_back(&keyword);
        }
        sch.applyKeywords(keywords, index);
    }
}



void python::common::export_Schedule(py::module& module) {


    py::class_<ScheduleState>(module, "ScheduleState")
        .def_property_readonly("nupcol", py::overload_cast<>(&ScheduleState::nupcol, py::const_))
        .def("group", &get_group, ref_internal);


    // Note: In the below class we use std::shared_ptr as the holder type, see:
    //
    //  https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
    //
    // this makes it possible to share the returned object with e.g. and
    //   opm.simulators.BlackOilSimulator Python object
    //
    py::class_< Schedule, std::shared_ptr<Schedule> >( module, "Schedule")
    .def(py::init<const Deck&, const EclipseState& >())
    .def("_groups", &get_groups )
    .def_property_readonly( "start",  &get_start_time )
    .def_property_readonly( "end",    &get_end_time )
    .def_property_readonly( "timesteps", &get_timesteps )
    .def("__len__", &Schedule::size)
    .def("__getitem__", &getitem)
    .def( "shut_well", &Schedule::shut_well)
    .def( "open_well", &Schedule::open_well)
    .def( "stop_well", &Schedule::stop_well)
    .def( "get_wells", &Schedule::getWells)
    .def( "get_production_properties", &get_production_properties)
    .def("well_names", py::overload_cast<const std::string&>(&Schedule::wellNames, py::const_))
    .def( "get_well", &get_well)
    .def( "insert_keywords",
        py::overload_cast<Schedule&, py::list&, std::size_t>(&insert_keywords),
        py::arg("keywords"), py::arg("step"))
    .def( "insert_keywords",
        py::overload_cast<
               Schedule&, const std::string&, std::size_t, const UnitSystem&
           >(&insert_keywords),
        py::arg("data"), py::arg("step"), py::arg("unit_system"))
    .def( "__contains__", &has_well );

}
