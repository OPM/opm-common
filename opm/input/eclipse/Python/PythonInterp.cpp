/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifdef EMBEDDED_PYTHON
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <fmt/format.h>

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/common/utility/FileSystem.hpp>

#include "python/cxx/export.hpp"
#include "PythonInterp.hpp"

namespace py = pybind11;
namespace Opm {

/**
 * @brief Executes Python code within a specified context.
 *
 * This function executes the provided Python code within the given context.
 *
 * @param python_code The Python code to execute, provided as a string.
 * @param context The Python module providing the context for execution.
 *
 * @return bool The return value set at the "result" attribute in the context module.
 *
 */
bool PythonInterp::exec(const std::string& python_code, py::module& context) {
    py::bool_ def_result = false;
    context.attr("result") = &def_result;
    py::exec(python_code, py::globals() , py::dict(py::arg("context") = context));
    const auto& result = static_cast<py::bool_>(context.attr("result"));
    return result;
}


/**
 * @brief Executes Python code within a specified context.
 *
 * This function imports the module "opm_embedded" (initialized at opm-common/python/opm_embeded/__init__.py)
 * as the context module, adds the parser and deck to the context and executes the given Python code.
 *
 * @param python_code The Python code to execute, provided as a string.
 * @param parser The parser.
 * @param deck   The current deck.
 *
 * @return bool The return value set at the "result" attribute in the context module.
 *
 */
bool PythonInterp::exec(const std::string& python_code, const Parser& parser, Deck& deck) {
    if (!this->guard)
        throw std::logic_error("Python interpreter not enabled");

    py::module context;
    try {
        context = py::module::import("opm_embedded");
    } catch (const std::exception& e) {
        OpmLog::error(fmt::format("Exception thrown when loading Python module opm_embedded: {}", e.what()));
        throw e;
    } catch (...) {
        OPM_THROW(std::runtime_error, "General exception thrown when loading Python module opm_embedded!");
    }
    context.attr("deck") = &deck;
    context.attr("parser") = &parser;
    return this->exec(python_code, context);
}


/**
 * @brief Executes Python code within a specified context.
 *
 * This function imports the module "opm_embedded" (initialized at opm-common/python/opm_embeded/__init__.py)
 * as the context module and executes the given Python code.
 *
 * @param python_code The Python code to execute, provided as a string.
 *
 * @return bool The return value set at the "result" attribute in the context module.
 *
 */
bool PythonInterp::exec(const std::string& python_code) {
    if (!this->guard)
        throw std::logic_error("Python interpreter not enabled");

    py::module context;
    try {
        context = py::module::import("opm_embedded");
    } catch (const std::exception& e) {
        OpmLog::error(fmt::format("Exception thrown when loading Python module opm_embedded: {}", e.what()));
        throw e;
    } catch (...) {
        OPM_THROW(std::runtime_error, "General exception thrown when loading Python module opm_embedded!");
    }
    return this->exec(python_code, context);
}

PythonInterp::PythonInterp(bool enable) {
    if (enable) {
        if (Py_IsInitialized())
            throw std::logic_error("An instance of the Python interpreter is already running");

        this->guard = std::make_unique<py::scoped_interpreter>();
        py::exec(R"~~(import signal
signal.signal(signal.SIGINT, signal.SIG_DFL))~~");
    }
}

}
#endif
