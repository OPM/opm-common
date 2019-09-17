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

#ifndef GUIDE_RATE_HPP
#define GUIDE_RATE_HPP

#include <string>
#include <cstddef>
#include <ctime>
#include <unordered_map>

namespace Opm {

class Schedule;
class GuideRate {

struct GuideRateValue {
    GuideRateValue() = default;
    GuideRateValue(std::time_t t, double v):
        sim_time(t),
        value(v)
    {}

    bool operator==(const GuideRateValue& other) const {
        return (this->sim_time == other.sim_time &&
                this->value == other.value);
    }

    bool operator!=(const GuideRateValue& other) const {
        return !(*this == other);
    }

    std::time_t sim_time;
    double value;
};


public:
    GuideRate(const Schedule& schedule);
    double update(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot);
private:
    void well_update(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot);
    void group_update(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot);
    double get(const std::string& wgname, size_t report_step) const;
    double eval_form(const GuideRateModel& model, double oil_pot, double gas_pot, double wat_pot, const GuideRateValue * prev) const;
    double eval_group_pot() const;
    double eval_group_resvinj() const;

    std::unordered_map<std::string,GuideRateValue> values;
    const Schedule& schedule;
};

}

#endif
