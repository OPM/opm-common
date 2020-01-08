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
namespace py = pybind11;
#else
namespace py {
using dict = int;
}
#endif


#include <opm/parser/eclipse/EclipseState/Schedule/Action/PyAction.hpp>

namespace Opm {

PyAction::PyAction(const std::string& code_arg) :
    input_code(code_arg),
    m_storage( new py::dict() )
{}


const std::string& PyAction::code() const {
    return this->input_code;
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



void * PyAction::storage() const {
    return this->m_storage;
}


}
