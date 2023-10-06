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
#include <memory>

#ifdef EMBEDDED_PYTHON
#include "src/opm/input/eclipse/Python/PyRunModule.hpp"
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;
#endif


#include <opm/common/utility/String.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Action/PyAction.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>


namespace Opm {
namespace Action {

PyAction::RunCount PyAction::count_from_string(std::string run_count) {
    run_count = uppercase(run_count);

    if (run_count == "SINGLE")
        return RunCount::single;

    if (run_count == "UNLIMITED")
        return RunCount::unlimited;

    if (run_count == "FIRST_TRUE")
        return RunCount::first_true;

    throw std::invalid_argument("RunCount string: " + run_count + " not recognized ");
}

std::string PyAction::count_to_string(RunCount run_count)
{
    switch (run_count) {
        case RunCount::single: return "SINGLE";
        case RunCount::unlimited: return "UNLIMITED";
        case RunCount::first_true: return "FIRST_TRUE";
        default: throw std::invalid_argument("Invalid RunCount value - should never happen");
    }
    return "UNKNOWN_RUN_COUNT";
}

PyAction::RunWhen PyAction::when_from_string(std::string run_when) {
    run_when = uppercase(run_when);

    if (run_when == "POST_STEP")
        return RunWhen::post_step;

    if (run_when == "PRE_STEP")
        return RunWhen::pre_step;

    if (run_when == "POST_NEWTON")
        return RunWhen::post_newton;

    if (run_when == "PRE_NEWTON")
        return RunWhen::pre_newton;

    if (run_when == "POST_REPORT")
        return RunWhen::post_report;

    if (run_when == "PRE_REPORT")
        return RunWhen::pre_report;
    
    throw std::invalid_argument("RunWhen string: " + run_when + " not recognized ");
}

std::string PyAction::when_to_string(RunWhen run_when)
{
    switch (run_when) {
        case RunWhen::pre_step: return "PRE_STEP";
        case RunWhen::post_step: return "POST_STEP";
        case RunWhen::pre_newton: return "PRE_NEWTON";
        case RunWhen::post_newton: return "POST_NEWTON";        
        case RunWhen::pre_report: return "PRE_REPORT";
        case RunWhen::post_report: return "POST_REPORT";
        default: throw std::invalid_argument("Invalid RunWhen value - should never happen");
    }
    return "UNKNOWN_RUN_WHEN";
}


PyAction PyAction::serializationTestObject()
{
    PyAction result;

    result.m_name = "name";
    result.m_run_count = RunCount::first_true;
    result.m_run_when = RunWhen::post_step;
    result.m_active = false;
    result.module_file = "no.such.file.py";

    return result;
}

bool PyAction::ready(const State& state) const {
    if (this->m_run_count == RunCount::unlimited)
        return true;

    auto last_result = state.python_result(this->m_name);
    if (!last_result.has_value())
        return true;

    if (this->m_run_count == RunCount::first_true && last_result.value() == false)
        return true;

    return false;
}


const std::string& PyAction::name() const {
    return this->m_name;
}

const std::string PyAction::when() const {
    return PyAction::when_to_string(this->m_run_when);
}

bool PyAction::operator==(const PyAction& other) const {
    return this->m_name == other.m_name &&
        this->m_run_count == other.m_run_count &&
        this->m_run_when == other.m_run_when &&
        this->m_active == other.m_active &&
        this->module_file == other.module_file;
}


#ifndef EMBEDDED_PYTHON

bool PyAction::run(EclipseState&, Schedule&, std::size_t, SummaryState&,
                   const std::function<void(const std::string&, const std::vector<std::string>&)>&) const
{
    return false;
}

PyAction::PyAction(std::shared_ptr<const Python>, const std::string& name, RunCount run_count, RunWhen run_when, const std::string& fname) :
    m_name(name),
    m_run_count(run_count),
    m_run_when(run_when),
    module_file(fname)
{}

#else

PyAction::PyAction(std::shared_ptr<const Python> python, const std::string& name, RunCount run_count, RunWhen run_when, const std::string& fname) :
    run_module( std::make_shared<Opm::PyRunModule>(python, fname)),
    m_name(name),
    m_run_count(run_count),
    m_run_when(run_when),
    module_file(fname)
{
}


bool PyAction::run(EclipseState& ecl_state, Schedule& schedule, std::size_t report_step, SummaryState& st,
                   const std::function<void(const std::string&, const std::vector<std::string>&)>& actionx_callback) const
{
    /*
      For PyAction instances which have been constructed the 'normal' way
      through the four argument constructor the run_module member variable has
      already been correctly initialized, however if this instance lives on a
      rank != 0 process and has been created through deserialization it was
      created without access to a Python handle and we must import the module
      now.
     */
    if (!this->run_module)
        this->run_module = std::make_shared<Opm::PyRunModule>(schedule.python(), this->module_file);

    if (this->m_run_when == RunWhen::post_step)
        return this->run_module->run(ecl_state, schedule, report_step, st, actionx_callback);

    if (this->m_run_when == RunWhen::pre_report)
        return this->run_module->run_pre_report(ecl_state, schedule, report_step, st, actionx_callback);
        
    if (this->m_run_when == RunWhen::post_newton)
        // TODO: Also check for run_post_step method (?)
        return this->run_module->run_post_newton(ecl_state, schedule, report_step, st, actionx_callback);

    if (this->m_run_when == RunWhen::pre_newton)
        return this->run_module->run_pre_newton(ecl_state, schedule, report_step, st, actionx_callback);

    if (this->m_run_when == RunWhen::post_report)
        return this->run_module->run_post_report(ecl_state, schedule, report_step, st, actionx_callback);
        
    if (this->m_run_when == RunWhen::pre_step)
        return this->run_module->run_pre_step(ecl_state, schedule, report_step, st, actionx_callback);
    
    throw std::runtime_error("Invalid run_when vaue:" + when_to_string(this->m_run_when));
    
    // Silence warning
    return false;
}





#endif

}
}
