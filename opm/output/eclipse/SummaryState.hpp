/*
  Copyright 2016 Statoil ASA.

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

#ifndef SUMMARY_STATE_H
#define SUMMARY_STATE_H

#include <cstddef>
#include <unordered_map>

namespace Opm{


/*
  The purpose of this class is to serve as a small container object for
  computed, ready to use summary values. The values will typically be used by
  the UDQ, WTEST and ACTIONX calculations. Observe that all value *have been
  converted to the correct output units*.
*/
class SummaryState {
public:
    typedef std::unordered_map<std::string, double>::const_iterator const_iterator;

    void add(const std::string& key, double value);
    double get(const std::string&) const;
    bool has(const std::string& key) const;

    const_iterator begin() const;
    const_iterator end() const;
private:
    std::unordered_map<std::string,double> values;
};

}
#endif
