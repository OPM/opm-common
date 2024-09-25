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


#ifndef WELL_INJECTION_CONTROLS_HPP
#define WELL_INJECTION_CONTROLS_HPP

#include <opm/input/eclipse/Schedule/ScheduleTypes.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEnums.hpp>

#include <cmath>

namespace Opm {

struct WellInjectionControls {
public:
    explicit WellInjectionControls(int controls_arg) :
        controls(controls_arg)
    {}

    bool hasControl(WellInjectorCMode cmode_arg) const
    {
        return (this->controls & static_cast<int>(cmode_arg)) != 0;
    }

    void skipControl(WellInjectorCMode cmode_arg) {
        auto int_arg = static_cast<int>(cmode_arg);
        if ((this->controls & int_arg) != 0)
            this->controls -= int_arg;
    }

    void addControl(WellInjectorCMode cmode_arg) {
        auto int_arg = static_cast<int>(cmode_arg);
        if ((this->controls & int_arg) == 0)
            this->controls += int_arg;
    }

    void clearControls(){
        this->controls = 0;
    }

    bool anyZeroRateConstraint() const {
        auto is_zero = [](const double x)
        {
            return std::isfinite(x) && !std::isnormal(x);
        };

        if (this->hasControl(WellInjectorCMode::RATE) && is_zero(this->surface_rate) ) {
            return true;
        }

        if (this->hasControl(WellInjectorCMode::RESV) && is_zero(this->reservoir_rate) ) {
            return true;
        }

        return false;
    }

    double bhp_limit{};
    double thp_limit{};

    InjectorType injector_type{InjectorType::GAS};
    WellInjectorCMode cmode = WellInjectorCMode::CMODE_UNDEFINED;
    double surface_rate{};
    double reservoir_rate{};
    int    vfp_table_number{};
    bool   prediction_mode{false};
    double rs_rv_inj{};

private:
    int controls{};
};

}

#endif
