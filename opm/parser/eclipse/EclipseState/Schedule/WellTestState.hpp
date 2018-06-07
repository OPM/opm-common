/*
  Copyright 2018 Statoil ASA.

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
#ifndef WELLTEST_STATE_H
#define WELLTEST_STATE_H

#include <cstddef>
#include <string>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/WellTestConfig.hpp>

namespace Opm {

class WellTestState {
public:
    /*
      This class implements a small mutable state object which keeps track of
      which wells have been automatically closed by the simulator, and by
      checking with the WTEST configuration object the class can return a list
      (well_name,reason) pairs for wells which should be checked as candiates
      for opening.
    */



    struct ClosedWell {
        std::string name;
        WellTestConfig::Reason reason;
        double last_test;
        int num_attempt;
    };

    /*
      The simulator has decided to close a particular well; we then add it here
      as a closed well with a particualar reason.
    */
    void addClosedWell(const std::string& well_name, WellTestConfig::Reason reason, double sim_time);

    /*
      The update will consult the WellTestConfig object and return a list of
      wells which should be checked for possible reopening; observe that the
      update method will update the internal state of the object by counting up
      the openiing attempts, and also set the time for the last attempt to open.
    */
    std::vector<std::pair<std::string, WellTestConfig::Reason>> update(const WellTestConfig& config, double sim_time);

    /*
      If the simulator decides that a constraint is no longer met the drop()
      method should be called to indicate that this reason for keeping the well
      closed is no longer active.
    */
    void drop(const std::string& well_name, WellTestConfig::Reason reason);


    bool has(const std::string well_name, WellTestConfig::Reason reason) const;
    void openWell(const std::string& well_name);
    size_t size() const;
private:
    std::vector<ClosedWell> wells;
};


}

#endif

