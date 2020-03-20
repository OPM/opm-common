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

#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/Python/Python.hpp>
#include "PythonInterp.hpp"

namespace Opm {

Python::Python() :
    interp(std::make_shared<PythonInterp>())
{
}

bool Python::exec(const std::string& python_code) const {
    this->interp->exec(python_code);
    return true;
}

bool Python::enabled() {
#ifdef EMBEDDED_PYTHON
    return true;
#else
    return false;
#endif
}

bool Python::exec(const std::string& python_code, const Parser& parser, Deck& deck) const {
    this->interp->exec(python_code, parser, deck);
    return true;
}

bool Python::exec(const Action::PyAction& py_action, EclipseState& ecl_state, Schedule& schedule, std::size_t report_step, SummaryState& st) const {
    this->interp->exec(py_action, ecl_state, schedule, report_step, st);
    return true;
}


Python::operator bool() const {
    if (this->interp)
        return bool( *this->interp );
    else
        return false;
}

std::unique_ptr<Python> PythonInstance() {
    #ifdef EMBEDDED_PYTHON
    if (Py_IsInitialized())
        return NULL;
    #endif

    return std::make_unique<Python>();
}


}
