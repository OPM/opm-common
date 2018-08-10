/*
  Copyright 2013 Statoil ASA.

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


#ifndef ActionX_HPP_
#define ActionX_HPP_

#include <string>

namespace Opm {
/*
  The ActionX class internalizes the ACTIONX keyword. This keyword represents a
  small in-deck programming language for the SCHEDULE section. In the deck the
  ACTIONX keyword comes together with a 'ENDACTIO' kewyord and then a list of
  regular keywords in the between. The principle is then that ACTIONX represents
  a condition, and when that condition is satisfied the keywords are applied. In
  the example below the ACTIONX keyword defines a condition whether well OPX has
  watercut above 0.75, when the condition is met the WELOPEN keyword is applied
  - and the well is shut.

  ACTIONX
     'NAME'  /
     WWCT OPX > 0.50 /
  /

  WELOPEN
     'OPX'  OPEN /
  /

  ENDACTION


*/

class DeckKeyword;

class ActionX {
public:
    ActionX(const std::string& name, size_t max_run, double max_wait);
    explicit ActionX(const DeckKeyword& kw);

    void addKeyword(const DeckKeyword& kw);

    const std::string& name() const;
private:
    std::string m_name;
    size_t max_run;
    double max_wait;

    std::vector<DeckKeyword> keywords;
};

}
#endif /* WELL_HPP_ */
