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


    struct ClosedCompletion {
        std::string wellName;
        int complnum;
        double last_test;
        int num_attempt;
    };

    /*
      The simulator has decided to close a particular well; we then add it here
      as a closed well with a particualar reason.
    */
    void addClosedWell(const std::string& well_name, WellTestConfig::Reason reason, double sim_time);

    /*
      The simulator has decided to close a particular completion in a well; we then add it here
      as a closed completions
    */
    void addClosedCompletion(const std::string& well_name, int complnum, double sim_time);

    /*
      The update will consult the WellTestConfig object and return a list of
      wells which should be checked for possible reopening; observe that the
      update method will update the internal state of the object by counting up
      the openiing attempts, and also set the time for the last attempt to open.
    */
    std::vector<std::pair<std::string, WellTestConfig::Reason>> updateWell(const WellTestConfig& config, double sim_time);

    /*
      The update will consult the WellTestConfig object and return a list of
      completions which should be checked for possible reopening; observe that the
      update method will update the internal state of the object by counting up
      the openiing attempts, and also set the time for the last attempt to open.
    */
    std::vector<std::pair<std::string, int>> updateCompletion(const WellTestConfig& config, double sim_time);


    /*
      If the simulator decides that a constraint is no longer met the dropWell()
      method should be called to indicate that this reason for keeping the well
      closed is no longer active.
    */
    void dropWell(const std::string& well_name, WellTestConfig::Reason reason);

    /*
      If the simulator decides that a constraint is no longer met the dropCompletion()
      method should be called to indicate that this reason for keeping the well
      closed is no longer active.
    */
    void dropCompletion(const std::string& well_name, int complnum);

    bool hasWell(const std::string& well_name, WellTestConfig::Reason reason) const;
    void openWell(const std::string& well_name);

    bool hasCompletion(const std::string& well_name, const int complnum) const;

    size_t sizeWells() const;
    size_t sizeCompletions() const;

private:
    std::vector<ClosedWell> wells;
    std::vector<ClosedCompletion> completions;
};


}

#endif

