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
#include <fstream>

#ifdef EMBEDDED_PYTHON
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;
#else
namespace py {
using dict = int;
}
#endif


#include <opm/parser/eclipse/Utility/String.hpp>
#include <opm/common/utility/FileSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/PyAction.hpp>

namespace Opm {
namespace Action {

PyAction::RunCount PyAction::from_string(std::string run_count) {
    run_count = uppercase(run_count);

    if (run_count == "SINGLE")
        return RunCount::single;

    if (run_count == "UNLIMITED")
        return RunCount::unlimited;

    if (run_count == "FIRST_TRUE")
        return RunCount::first_true;

    throw std::invalid_argument("RunCount string: " + run_count + " not recognized ");
}


std::string PyAction::load(const std::string& input_path, const std::string& fname) {
    namespace fs = Opm::filesystem;
    fs::path code_path = fs::path(input_path) / fs::path(fname);
    if (fs::exists(code_path)) {
        std::ifstream ifs(code_path);
        return std::string{ std::istreambuf_iterator<char>{ifs}, {} };
    } else
        throw std::invalid_argument("No such file: " + fname);
}



PyAction::PyAction(const std::string& name, RunCount run_count, const std::string& code) :
    m_name(name),
    m_run_count(run_count),
    input_code(code),
    m_storage( new py::dict() )
{}


const std::string& PyAction::code() const {
    return this->input_code;
}

const std::string& PyAction::name() const {
    return this->m_name;
}

PyAction::RunCount PyAction::run_count() const {
    return this->m_run_count;
}

/*
  The python variables are reference counted and when the Python dictionary
  stored in this->m_storage is destroyed the Python runtime system is involved.
  This will fail hard id the Python runtime system has not been initialized. If
  the python runtime has not been initialized the Python dictionary object will
  leak - the leakage is quite harmless, using the PyAction object without a
  Python runtime system does not make any sense after all.
*/

PyAction::~PyAction() {
    auto dict = static_cast<py::dict *>(this->m_storage);
#ifdef EMBEDDED_PYTHON
    if (Py_IsInitialized())
        delete dict;
#else
    delete dict;
#endif
}

bool PyAction::operator==(const PyAction& other) const {
    return this->m_name == other.m_name &&
           this->m_run_count == other.m_run_count &&
           this->input_code == other.input_code;
}



void * PyAction::storage() const {
    return this->m_storage;
}

}
}
