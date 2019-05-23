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


#ifndef PRODUCTION_CONTROLS_HPP
#define PRODUCTION_CONTROLS_HPP

namespace Opm {
struct ProductionControls {
public:
    ProductionControls(int controls) :
        controls(controls)
    {
    }

    WellProducer::ControlModeEnum cmode;
    double oil_rate;
    double water_rate;
    double gas_rate;
    double liquid_rate;
    double resv_rate;
    double bhp_history;
    double thp_history;
    double bhp_limit;
    double thp_limit;
    double alq_value;
    int    vfp_table_number;
    bool   prediction_mode;

    bool hasControl(WellProducer::ControlModeEnum cmode) const {
        return (this->controls & cmode) != 0;
    }

private:
    int controls;
};
}
#endif
