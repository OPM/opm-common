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


#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include "src/opm/input/eclipse/Python/PyRunModule.hpp"

#include <filesystem>

#include <pybind11/stl.h>
namespace Opm {

namespace fs = std::filesystem;

PyRunModule::PyRunModule(std::shared_ptr<const Python> python, const std::string& fname) {
    if (python->enabled())
        this->python_handle = python;
    else
        throw std::logic_error("Tried to make a PYACTION object with an invalid Python handle");


    fs::path file(fname);
    if (!fs::is_regular_file(file))
        throw std::invalid_argument("No such module: " + fname);

    std::string module_name = file.filename().stem();
    std::string module_path = file.parent_path().string();
    if (!module_path.empty()) {
        py::module sys = py::module::import("sys");
        py::list sys_path = sys.attr("path");
        {
            bool have_path = false;
            for (const auto& elm : sys_path) {
                const std::string& path_elm = static_cast<py::str>(elm);
                if (path_elm == module_path)
                    have_path = true;
            }
            if (!have_path)
                sys_path.append(py::str(module_path));
        }
    }
    this->opm_embedded = py::module::import("opm_embedded");
    this->module = py::module::import(module_name.c_str());
    if (this->module.is_none())
        throw std::runtime_error("Syntax error when loading Python module: " + fname);

    bool module_possibly_ok = false;
    
    if (py::hasattr(this->module, "run")) {
        if (py::hasattr(this->module, "run_post_step")) {
            throw std::runtime_error("Python module: " + fname + " has both run() and run_post_step() methods. Cannot decide which one to use..");
        }
        module_possibly_ok = true;    
        this->run_post_step_function = this->module.attr("run");
    }

    if (py::hasattr(this->module, "run_post_step")) {
        module_possibly_ok = true;
        this->run_post_step_function = this->module.attr("run_post_step");
    }
    if (py::hasattr(this->module, "run_pre_step")) {
        module_possibly_ok = true;
        this->run_pre_step_function = this->module.attr("run_pre_step");
    }
    if (py::hasattr(this->module, "run_post_newton")) {
        module_possibly_ok = true;
        this->run_post_newton_function = this->module.attr("run_post_newton");
    }
    if (py::hasattr(this->module, "run_pre_newton")) {
        module_possibly_ok = true;
        this->run_pre_newton_function = this->module.attr("run_pre_newton");
    }    
    if (py::hasattr(this->module, "run_post_report")) {
        module_possibly_ok = true;
        this->run_post_report_function = this->module.attr("run_post_report");
    }        
    if (py::hasattr(this->module, "run_pre_report")) {
        module_possibly_ok = true;
        this->run_pre_report_function = this->module.attr("run_pre_report");
    }
            
    
    if (!module_possibly_ok)
        throw std::runtime_error("Python module: " + fname + " has no suitable run* method");

    this->module.attr("storage") = this->storage;
}

namespace {

py::cpp_function py_actionx_callback(const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    return py::cpp_function( actionx_callback );
}

}

bool PyRunModule::run(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    return run_post_step(ecl_state, sched, report_step, st, actionx_callback);
}

bool PyRunModule::run_post_step(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    if (this->run_post_step_function.is_none())
        return false;
        
    auto cpp_callback = py_actionx_callback(actionx_callback);
    py::object result = this->run_post_step_function(&ecl_state, &sched, report_step, &st, cpp_callback);
    return result.cast<bool>();
}

bool PyRunModule::run_pre_step(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    if (this->run_pre_step_function.is_none())
        return false;
        
    auto cpp_callback = py_actionx_callback(actionx_callback);
    py::object result = this->run_pre_step_function(&ecl_state, &sched, report_step, &st, cpp_callback);
    return result.cast<bool>();
}

bool PyRunModule::run_post_newton(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    if (this->run_post_newton_function.is_none())
        return false;
        
    auto cpp_callback = py_actionx_callback(actionx_callback);
    py::object result = this->run_post_newton_function(&ecl_state, &sched, report_step, &st, cpp_callback);
    return result.cast<bool>();
}

bool PyRunModule::run_pre_newton(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    if (this->run_pre_newton_function.is_none())
        return false;
        
    auto cpp_callback = py_actionx_callback(actionx_callback);
    py::object result = this->run_pre_newton_function(&ecl_state, &sched, report_step, &st, cpp_callback);
    return result.cast<bool>();
}

bool PyRunModule::run_pre_report(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    if (this->run_pre_report_function.is_none())
        return false;
        
    auto cpp_callback = py_actionx_callback(actionx_callback);
    py::object result = this->run_pre_report_function(&ecl_state, &sched, report_step, &st, cpp_callback);
    return result.cast<bool>();
}

bool PyRunModule::run_post_report(EclipseState& ecl_state, Schedule& sched, std::size_t report_step, SummaryState& st, const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) {
    if (this->run_post_report_function.is_none())
        return false;
        
    auto cpp_callback = py_actionx_callback(actionx_callback);
    py::object result = this->run_post_report_function(&ecl_state, &sched, report_step, &st, cpp_callback);
    return result.cast<bool>();
}



}
