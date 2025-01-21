#include <string>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/Logger.hpp>

#include "export.hpp"

namespace {

void info(const std::string& msg) {
    OpmLog::info(msg);
}

void warning(const std::string& msg) {
    OpmLog::warning(msg);
}

void error(const std::string& msg) {
    OpmLog::error(msg);
}

void problem(const std::string& msg) {
    OpmLog::problem(msg);
}

void bug(const std::string& msg) {
    OpmLog::bug(msg);
}

void debug(const std::string& msg) {
    OpmLog::debug(msg);
}

void note(const std::string& msg) {
    OpmLog::note(msg);
}

}

void python::common::export_Log(py::module& module)
{
    py::class_<Logger, std::shared_ptr<Logger>>(module, "Logger");
    py::class_<OpmLog>( module, "OpmLog", R"(
            The Opm::OpmLog class - this is a fully static class which manages a proper
            Logger instance.
        )")
        .def_static("_setLogger", &OpmLog::setLogger, "Internal function to set the logger.")
        .def_static("info", info, "Add an info message to the opm log.")
        .def_static("warning", warning, "Add a warning message to the opm log.")
        .def_static("error", error, "Add an error message to the opm log.")
        .def_static("problem", problem, "Add a problem message to the opm log.")
        .def_static("bug", bug, "Add a bug message to the opm log.")
        .def_static("debug", debug, "Add a debug message to the opm log.")
        .def_static("note", note, "Add a note to the opm log.");
}
