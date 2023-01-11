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

#include <config.h>
#include <opm/material/fluidsystems/blackoilpvt/OilPvtMultiplexer.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

namespace Opm {

template <class Scalar, bool enableThermal>
void OilPvtMultiplexer<Scalar,enableThermal>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    if (!eclState.runspec().phases().active(Phase::OIL))
        return;

    // The co2Storage option both works with oil + gas
    // and water/brine + gas
    if (eclState.runspec().co2Storage())
        setApproach(OilPvtApproach::BrineCo2);
    else if (eclState.runspec().h2Storage())
        setApproach(OilPvtApproach::BrineH2);
    else if (enableThermal && eclState.getSimulationConfig().isThermal())
        setApproach(OilPvtApproach::ThermalOil);
    else if (!eclState.getTableManager().getPvcdoTable().empty())
        setApproach(OilPvtApproach::ConstantCompressibilityOil);
    else if (eclState.getTableManager().hasTables("PVDO"))
        setApproach(OilPvtApproach::DeadOil);
    else if (!eclState.getTableManager().getPvtoTables().empty())
        setApproach(OilPvtApproach::LiveOil);

    OPM_OIL_PVT_MULTIPLEXER_CALL(pvtImpl.initFromState(eclState, schedule));
}

template class OilPvtMultiplexer<double,false>;
template class OilPvtMultiplexer<double,true>;
template class OilPvtMultiplexer<float,false>;
template class OilPvtMultiplexer<float,true>;

} // namespace Opm
