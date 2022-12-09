// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::EclEpsConfig
 */

#include <config.h>
#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <stdexcept>

#include <cassert>

namespace Opm {

void EclEpsConfig::initFromState(const EclipseState& eclState,
                                 EclTwoPhaseSystemType twoPhaseSystemType,
                                const std::string& prefix,
                                const std::string& suffix)
{
    const auto& endscale = eclState.runspec().endpointScaling();
    // find out if endpoint scaling is used in the first place
    if (!endscale) {
        // it is not used, i.e., just set all enable$Foo attributes to 0 and be done
        // with it.
        enableSatScaling_ = false;
        enableThreePointKrSatScaling_ = false;
        enablePcScaling_ = false;
        enableLeverettScaling_ = false;
        enableKrwScaling_ = false;
        enableKrnScaling_ = false;
        return;
    }

    // endpoint scaling is used, i.e., at least saturation scaling needs to be enabled
    enableSatScaling_ = true;
    enableThreePointKrSatScaling_ = endscale.threepoint();

    if (eclState.getTableManager().useJFunc()) {
        const auto flag = eclState.getTableManager().getJFunc().flag();

        enableLeverettScaling_ = (flag == JFunc::Flag::BOTH)
            || ((twoPhaseSystemType == EclOilWaterSystem) &&
                (flag == JFunc::Flag::WATER))
            || ((twoPhaseSystemType == EclGasOilSystem) &&
                (flag == JFunc::Flag::GAS));
    }

    const auto& fp = eclState.fieldProps();
    auto hasKR = [&fp, &prefix, &suffix](const std::string& scaling)
    {
        return fp.has_double(prefix + "KR" + scaling + suffix);
    };
    auto hasPC = [&fp, &prefix](const std::string& scaling)
    {
        return fp.has_double(prefix + "PC" + scaling);
    };

    // check if we are supposed to scale the Y axis of the capillary pressure
    if (twoPhaseSystemType == EclOilWaterSystem) {
        this->setEnableThreePointKrwScaling(hasKR("WR"));
        this->setEnableThreePointKrnScaling(hasKR("ORW"));

        this->enableKrnScaling_ = hasKR("O") || this->enableThreePointKrnScaling();
        this->enableKrwScaling_ = hasKR("W") || this->enableThreePointKrwScaling();
        this->enablePcScaling_  = hasPC("W") || fp.has_double("SWATINIT");
    }
    else if (twoPhaseSystemType == EclGasOilSystem) {
        this->setEnableThreePointKrwScaling(hasKR("ORG"));
        this->setEnableThreePointKrnScaling(hasKR("GR"));

        this->enableKrnScaling_ = hasKR("G") || this->enableThreePointKrnScaling();
        this->enableKrwScaling_ = hasKR("O") || this->enableThreePointKrwScaling();
        this->enablePcScaling_  = hasPC("G");
    }
    else {
        assert(twoPhaseSystemType == EclGasWaterSystem);
        //TODO enable endpoint scaling for gaswater system
    }

    if (enablePcScaling_ && enableLeverettScaling_) {
        throw std::runtime_error {
            "Capillary pressure scaling and the Leverett scaling function "
            "are mutually exclusive. The deck contains the PCW/PCG property "
            "and the JFUNC keyword applies to the water phase."
        };
    }
}

} // namespace Opm
