/*
  Copyright 2020 Equinor ASA.

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
#ifndef EMBEDDED_PYTHON
error BUG: The PyRunModule.hpp header should *not* be included in a configuration without EMBEDDED_PYTHON
#endif


#include <opm/common/ErrorMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include "PyRunModule.hpp"

#include <fmt/format.h>

#include <pybind11/stl.h>
namespace Opm {

namespace fs = std::filesystem;

PyRunModule::PyRunModule(std::shared_ptr<const Python> python, const std::string& fname) {
    if (python->enabled())
        this->python_handle = python;
    else
        OPM_THROW(std::logic_error, "Tried to make a PYACTION object with an invalid Python handle");


    fs::path file(fname);
    if (!fs::is_regular_file(file))
        OPM_THROW(std::invalid_argument, "No such module: " + fname);

    this->module_path = fs::current_path().string() / file.parent_path();
    this->module_name = file.filename().stem();

    // opm_embedded needs to be loaded before user defined module.
    try {
        this->opm_embedded = py::module::import("opm_embedded");
    } catch (const std::exception& e) {
        OpmLog::error(fmt::format("Exception thrown when loading Python module opm_embedded: {}. Possibly the PYTHONPATH of the system is not set correctly.", e.what()));
        throw e;
    } catch (...) {
        OPM_THROW(std::runtime_error, "General exception thrown when loading Python module opm_embedded, possibly the PYTHONPATH of the system is not set correctly.");
    }
}

namespace {

py::cpp_function py_actionx_callback(const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    return py::cpp_function( actionx_callback );
}

}

bool PyRunModule::executeInnerRunFunction(const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    auto cpp_callback = py_actionx_callback(actionx_callback);
    try {
        py::object result = this->run_function(this->opm_embedded.attr("current_ecl_state"), this->opm_embedded.attr("current_schedule"), this->opm_embedded.attr("current_report_step"), this->opm_embedded.attr("current_summary_state"), cpp_callback);
        return result.cast<bool>();
    } catch (const std::exception& e) {
        OpmLog::error(fmt::format("Exception thrown when calling run(ecl_state, schedule, report_step, summary_state, actionx_callback) function of {}: {}", this->module_name, e.what()));
        throw e;
    } catch(...) {
        OPM_THROW(std::runtime_error, fmt::format("General exception thrown when calling run(ecl_state, schedule, report_step, summary_state, actionx_callback) function of {}", this->module_name));
    }
}

bool PyRunModule::run(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    // The attributes need to be set before the user defined module is loaded; it needs to be set in every call.
    this->opm_embedded.attr("current_report_step") = report_step;

    if (!this->module) {
        // The attributes need to be set before the user defined module is loaded; they need to be set only once since they are shared pointers.
        this->opm_embedded.attr("current_schedule") = &sched;
        this->opm_embedded.attr("current_summary_state") = &st;
        this->opm_embedded.attr("current_ecl_state") = &ecl_state;
        try {
            if (!this->module_path.empty()) {
                py::module sys = py::module::import("sys");
                py::list sys_path = sys.attr("path");
                {
                    bool have_path = false;
                    for (const auto& elm : sys_path) {
                        const std::string& path_elm = static_cast<py::str>(elm);
                        if (path_elm == this->module_path)
                            have_path = true;
                    }
                    if (!have_path)
                        sys_path.append(py::str(this->module_path));
                }
            }
            this->module = py::module::import(this->module_name.c_str());
        } catch (const std::exception& e) {
            OpmLog::error(fmt::format("Exception thrown when loading Python module {}: {}", this->module_name, e.what()));
            throw e;
        } catch (...) {
            OPM_THROW(std::runtime_error, fmt::format("General exception thrown when loading Python module {}", this->module_name));
        }

        if (this->module.is_none()) {
            OPM_THROW(std::runtime_error, fmt::format("Syntax error when loading Python module: {}", this->module_name));
        }

        this->module.attr("storage") = this->storage;

        if (py::hasattr(this->module, "run")) {
            OpmLog::info(R"(PYACTION can be used without a run(ecl_state, schedule, report_step, summary_state, actionx_callback) function, its arguments are available as attributes of the module opm_embedded, try the following in your python script:
                    
import opm_embedded

help(opm_embedded.current_ecl_state)
help(opm_embedded.current_schedule)
help(opm_embedded.current_report_step)
help(opm_embedded.current_summary_state)
)");
            this->run_function = this->module.attr("run");
            return this->executeInnerRunFunction(actionx_callback);
        }
    } else { // Module has been loaded already
        if (!this->run_function.is_none()) {
            return this->executeInnerRunFunction(actionx_callback);            
        } else {
            try {
                this->module.reload();
            } catch (const std::exception& e) {
                OpmLog::error(fmt::format("Exception thrown in python module {}: {}", this->module_name, e.what()));
                throw e;
            } catch(...) {
                OPM_THROW(std::runtime_error, fmt::format("General exception thrown in python module {}", this->module_name));
            }
        }
    }

    // The return value of this function is true; unless there is a run function, then the return value is the return value of that run function.
    return true;
}

}
