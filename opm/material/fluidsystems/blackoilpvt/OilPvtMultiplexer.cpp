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

#include <stdexcept>

namespace Opm {

template <class Scalar, bool enableThermal>
OilPvtMultiplexer<Scalar,enableThermal>::
~OilPvtMultiplexer()
{
    releaseRealPvt_();
}

template <class Scalar, bool enableThermal>
void OilPvtMultiplexer<Scalar,enableThermal>::
releaseRealPvt_()
{
    switch (approach_) {
    case OilPvtApproach::LiveOil: {
        delete &getRealPvt<OilPvtApproach::LiveOil>();
        break;
    }
    case OilPvtApproach::DeadOil: {
        delete &getRealPvt<OilPvtApproach::DeadOil>();
        break;
    }
    case OilPvtApproach::ConstantCompressibilityOil: {
        delete &getRealPvt<OilPvtApproach::ConstantCompressibilityOil>();
        break;
    }
    case OilPvtApproach::ThermalOil: {
        delete &getRealPvt<OilPvtApproach::ThermalOil>();
        break;
    }
    case OilPvtApproach::BrineCo2: {
        delete &getRealPvt<OilPvtApproach::BrineCo2>();
        break;
    }
    case OilPvtApproach::BrineH2: {
        delete &getRealPvt<OilPvtApproach::BrineH2>();
        break;
    }
    case OilPvtApproach::ConstantRsDeadOil: {
        delete &getRealPvt<OilPvtApproach::ConstantRsDeadOil>();
        break;
    }
    case OilPvtApproach::NoOil:
        break;
    }

    realOilPvt_ = nullptr;
}

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
    else if (enableThermal && (eclState.getSimulationConfig().isTemp() || eclState.getSimulationConfig().isThermal()))
        setApproach(OilPvtApproach::ThermalOil);
    else if (!eclState.getTableManager().getPvcdoTable().empty())
        setApproach(OilPvtApproach::ConstantCompressibilityOil);
    else if (eclState.getTableManager().hasTables("RSCONST")
             && !eclState.getTableManager().getRsconstTables().empty()) {
        if (eclState.runspec().phases().active(Phase::GAS)
            || eclState.getSimulationConfig().hasDISGAS())
        {
            throw std::invalid_argument(
                "RSCONST requires dead-oil mode with GAS and DISGAS disabled.");
        }
        setApproach(OilPvtApproach::ConstantRsDeadOil);
    }
    else if (eclState.getTableManager().hasTables("PVDO"))
        setApproach(OilPvtApproach::DeadOil);
    else if (!eclState.getTableManager().getPvtoTables().empty())
        setApproach(OilPvtApproach::LiveOil);

    OPM_OIL_PVT_MULTIPLEXER_CALL(pvtImpl.initFromState(eclState, schedule), break);
}

template <class Scalar, bool enableThermal>
void OilPvtMultiplexer<Scalar,enableThermal>::
initEnd()
{
    OPM_OIL_PVT_MULTIPLEXER_CALL(pvtImpl.initEnd(), break);
}

template <class Scalar, bool enableThermal>
unsigned OilPvtMultiplexer<Scalar,enableThermal>::
numRegions() const
{
    OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.numRegions());
}

template <class Scalar, bool enableThermal>
void OilPvtMultiplexer<Scalar,enableThermal>::
setVapPars(const Scalar par1, const Scalar par2)
{
    OPM_OIL_PVT_MULTIPLEXER_CALL(pvtImpl.setVapPars(par1, par2), break);
}

template <class Scalar, bool enableThermal>
Scalar OilPvtMultiplexer<Scalar,enableThermal>::
oilReferenceDensity(unsigned regionIdx) const
{
    OPM_OIL_PVT_MULTIPLEXER_CALL(return pvtImpl.oilReferenceDensity(regionIdx));
}

template <class Scalar, bool enableThermal>
void OilPvtMultiplexer<Scalar,enableThermal>::
setApproach(OilPvtApproach appr)
{
    void* newPvt = nullptr;
    switch (appr) {
    case OilPvtApproach::LiveOil:
        newPvt = new LiveOilPvt<Scalar>;
        break;

    case OilPvtApproach::DeadOil:
        newPvt = new DeadOilPvt<Scalar>;
        break;

    case OilPvtApproach::ConstantCompressibilityOil:
        newPvt = new ConstantCompressibilityOilPvt<Scalar>;
        break;

    case OilPvtApproach::ThermalOil:
        newPvt = new OilPvtThermal<Scalar>;
        break;

    case OilPvtApproach::BrineCo2:
        newPvt = new BrineCo2Pvt<Scalar>;
        break;

    case OilPvtApproach::BrineH2:
        newPvt = new BrineH2Pvt<Scalar>;
        break;

    case OilPvtApproach::ConstantRsDeadOil:
        newPvt = new ConstantRsDeadOilPvt<Scalar>;
        break;

    case OilPvtApproach::NoOil:
        throw std::logic_error("Not implemented: Oil PVT of this deck!");
    }

    releaseRealPvt_();
    realOilPvt_ = newPvt;
    approach_ = appr;
}

template <class Scalar, bool enableThermal>
OilPvtMultiplexer<Scalar,enableThermal>&
OilPvtMultiplexer<Scalar,enableThermal>::
operator=(const OilPvtMultiplexer<Scalar,enableThermal>& data)
{
    if (this == &data) {
        return *this;
    }

    // Copy first, then release the current object using the current approach.
    void* newPvt = nullptr;
    switch (data.approach_) {
    case OilPvtApproach::ConstantCompressibilityOil:
        newPvt = new ConstantCompressibilityOilPvt<Scalar>(*static_cast<const ConstantCompressibilityOilPvt<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::DeadOil:
        newPvt = new DeadOilPvt<Scalar>(*static_cast<const DeadOilPvt<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::LiveOil:
        newPvt = new LiveOilPvt<Scalar>(*static_cast<const LiveOilPvt<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::ThermalOil:
        newPvt = new OilPvtThermal<Scalar>(*static_cast<const OilPvtThermal<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::BrineCo2:
        newPvt = new BrineCo2Pvt<Scalar>(*static_cast<const BrineCo2Pvt<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::BrineH2:
        newPvt = new BrineH2Pvt<Scalar>(*static_cast<const BrineH2Pvt<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::ConstantRsDeadOil:
        newPvt = new ConstantRsDeadOilPvt<Scalar>(*static_cast<const ConstantRsDeadOilPvt<Scalar>*>(data.realOilPvt_));
        break;
    case OilPvtApproach::NoOil:
        break;
    }

    releaseRealPvt_();
    realOilPvt_ = newPvt;
    approach_ = data.approach_;

    return *this;
}

template class OilPvtMultiplexer<double,false>;
template class OilPvtMultiplexer<double,true>;
template class OilPvtMultiplexer<float,false>;
template class OilPvtMultiplexer<float,true>;

} // namespace Opm
