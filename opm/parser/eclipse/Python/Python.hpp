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

#ifndef OPM_EMBEDDED_PYTHON
#define OPM_EMBEDDED_PYTHON

#include <memory>
#include <string>

#include <opm/parser/eclipse/EclipseState/Schedule/Action/PyAction.hpp>

namespace Opm {
class PythonInterp;
class Parser;
class Deck;
class SummaryState;
class Schedule;
class EclipseState;

/*
  This class is a thin wrapper around the PythonInterp class. The Python class
  can always be safely instantiated, but the actual PythonInterp implementation
  depends on whether Python support was enabled when this library instance was
  compiled.

  If one the methods actually invoking the Python interpreter is invoked without
  proper Python support a dummy PythinInterp instance will be used; and that
  will just throw std::logic_error. The operator bool can be used to check if
  this Python manager indeed has a valid Python runtime:


     Python python;

     if (python)
         python.exec("print('Hello world')")
     else
         OpmLog::Error("This version of opmcommon has been built with support for embedded Python");

*/

class Python {
public:
    Python();
    bool exec(const std::string& python_code) const;
    bool exec(const std::string& python_code, const Parser& parser, Deck& deck) const;
    bool exec(const Action::PyAction& py_action, EclipseState& ecl_state, Schedule& schedule, std::size_t report_step, SummaryState& st) const;
    static bool enabled();
    explicit operator bool() const;
private:
    std::shared_ptr<PythonInterp> interp;
};

std::unique_ptr<Python> PythonInstance();

}



#endif

