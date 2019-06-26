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
#ifndef WELLTEST_CONFIG_H
#define WELLTEST_CONFIG_H

#include <cstddef>
#include <string>
#include <vector>


namespace Opm {

class WellTestConfig {
public:
    enum Reason {
        PHYSICAL = 1,
        ECONOMIC = 2,
        GROUP = 4,
        THP_DESIGN=8,
        COMPLETION=16,
    };

    struct WTESTWell {
        std::string name;
        Reason shut_reason;
        double test_interval;
        int num_test;
        double startup_time;
    };

    WellTestConfig();
    void add_well(const std::string& well, Reason reason, double test_interval, int num_test, double startup_time);
    void add_well(const std::string& well, const std::string& reasons, double test_interval, int num_test, double startup_time);
    void drop_well(const std::string& well);
    bool has(const std::string& well) const;
    bool has(const std::string& well, Reason reason) const;
    const WTESTWell& get(const std::string& well, Reason reason) const;
    size_t size() const;

    static std::string reasonToString(const Reason reason);

private:
    std::vector<WTESTWell> wells;
};


}

#endif

