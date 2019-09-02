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

#ifndef GUIDE_RATE_MODEL_HPP
#define GUIDE_RATE_MODEL_HPP

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

namespace Opm {

class GuideRateModel {
public:

    enum class Target {
        OIL = 0,
        LIQ = 1,
        GAS = 2,
        RES = 3,
        COMB = 4,
        NONE = 5
    };

    static Target TargetFromString(const std::string& s);

    GuideRateModel(double time_interval_arg,
                   Target target_arg,
                   double A_arg,
                   double B_arg,
                   double C_arg,
                   double D_arg,
                   double E_arg,
                   double F_arg,
                   bool allow_increase_arg,
                   double damping_factor_arg,
                   bool use_free_gas_arg);

    GuideRateModel() = default;
    double eval(double pot, double R1, double R2) const;
    double update_delay() const;
    bool operator==(const GuideRateModel& other) const;
    bool operator!=(const GuideRateModel& other) const;

private:
    /*
      Unfortunately the default values will give a GuideRateModel which can not
      be evaluated, due to a division by zero problem.
    */
    double time_interval = 0;
    Target target = Target::NONE;
    double A = 0;
    double B = 0;
    double C = 0;
    double D = 0;
    double E = 0;
    double F = 0;
    bool allow_increase = true;
    double damping_factor = 1.0;
    bool use_free_gas = false;
    bool default_model = true;
};

}

#endif
