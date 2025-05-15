#include <string>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/Logger.hpp>

#include "export.hpp"

#include <python/cxx/OpmCommonPythonDoc.hpp>

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
    using namespace Opm::Common::DocStrings;

    py::class_<Logger, std::shared_ptr<Logger>>(module, "Logger");

    py::class_<OpmLog>(module, "OpmLog", OpmLog_docstring)
        // The function _setLogger will not appear in the documentation since it is not to be used from pyhton.
        // This function is used internally in PyRunModule.cpp to expose the OpmLog properly to opm_embedded.
        // If we don't set the logger there manually, any log messages from the Python script will not reach OpmLog.
        .def_static("_setLogger", &OpmLog::setLogger, py::arg("logger"), "Internal function to set the logger, not to be used from python.")
        .def_static("info", info, py::arg("msg"), OpmLog_info_docstring)
        .def_static("warning", warning, py::arg("msg"), OpmLog_warning_docstring)
        .def_static("error", error, py::arg("msg"), OpmLog_error_docstring)
        .def_static("problem", problem, py::arg("msg"), OpmLog_problem_docstring)
        .def_static("bug", bug, py::arg("msg"), OpmLog_bug_docstring)
        .def_static("debug", debug, py::arg("msg"), OpmLog_debug_docstring)
        .def_static("note", note, py::arg("msg"), OpmLog_note_docstring);

}
