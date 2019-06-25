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


#ifndef INJECTION_CONTROLS_HPP
#define INJECTION_CONTROLS_HPP

namespace Opm {
struct InjectionControls {
public:
    InjectionControls(int controls_arg) :
        controls(controls_arg)
    {}

    double bhp_limit;
    double thp_limit;


    WellInjector::TypeEnum injector_type;
    WellInjector::ControlModeEnum cmode;
    double surface_rate;
    double reservoir_rate;
    double temperature;
    int    vfp_table_number;
    bool   prediction_mode;

    bool hasControl(WellInjector::ControlModeEnum cmode_arg) const {
        return (this->controls & cmode_arg) != 0;
    }
private:
    int controls;
};
}
#endif
