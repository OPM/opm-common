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
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <functional>
namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
void WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    if (!eclState.runspec().phases().active(Phase::WATER))
        return;

    // The co2Storage option both works with oil + gas
    // and water/brine + gas
    if (eclState.runspec().co2Storage() || eclState.runspec().co2Sol())
        setApproach(WaterPvtApproach::BrineCo2);
    else if (eclState.runspec().h2Storage() || eclState.runspec().h2Sol())
        setApproach(WaterPvtApproach::BrineH2);
    else if (enableThermal && eclState.getSimulationConfig().isThermal())
        setApproach(WaterPvtApproach::ThermalWater);
    else if (!eclState.getTableManager().getPvtwTable().empty())
        setApproach(WaterPvtApproach::ConstantCompressibilityWater);
    else if (enableBrine && !eclState.getTableManager().getPvtwSaltTables().empty())
        setApproach(WaterPvtApproach::ConstantCompressibilityBrine);

    OPM_WATER_PVT_MULTIPLEXER_CALL(pvtImpl.initFromState(eclState, schedule), break);
}
#endif

template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
void WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::
initEnd()
{
    OPM_WATER_PVT_MULTIPLEXER_CALL(pvtImpl.initEnd(), break);
}


template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
unsigned WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::
numRegions() const
{
    OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.numRegions());
}

template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
void WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::
setVapPars(const Scalar par1, const Scalar par2)
{
    OPM_WATER_PVT_MULTIPLEXER_CALL(pvtImpl.setVapPars(par1, par2), break);
}

template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
Scalar WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::
hVap(unsigned regionIdx) const
{
    OPM_WATER_PVT_MULTIPLEXER_CALL(return pvtImpl.hVap(regionIdx));
}

// Helper function to keep the switch case tidy when constructing different pvts
template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
template <class ConcreteGasPvt>
typename WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::UniqueVoidPtrWithDeleter
WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::makeWaterPvt()
{
    return UniqueVoidPtrWithDeleter(
        new ConcreteGasPvt,
        [this](void* ptr) { deleter(ptr); }
    );
}

template<class Scalar, bool enableThermal, bool enableBrine, class ParamsContainer, class ContainerT, template <class...> class PtrType>
void WaterPvtMultiplexer<Scalar,enableThermal,enableBrine, ParamsContainer, ContainerT, PtrType>::
setApproach(WaterPvtApproach appr)
{
    switch (appr) {
    case WaterPvtApproach::ConstantCompressibilityWater:
        realWaterPvt_ = makeWaterPvt<ConstantCompressibilityWaterPvt<Scalar>>();
        break;

    case WaterPvtApproach::ConstantCompressibilityBrine:
        realWaterPvt_ = makeWaterPvt<ConstantCompressibilityBrinePvt<Scalar>>();
        break;

    case WaterPvtApproach::ThermalWater:
        realWaterPvt_ = makeWaterPvt<WaterPvtThermal<Scalar, enableBrine>>();
        break;

    case WaterPvtApproach::BrineCo2:
        realWaterPvt_ = makeWaterPvt<BrineCo2Pvt<Scalar>>();
        break;

    case WaterPvtApproach::BrineH2:
        realWaterPvt_ = makeWaterPvt<BrineH2Pvt<Scalar>>();
        break;

    case WaterPvtApproach::NoWater:
        throw std::logic_error("Not implemented: Water PVT of this deck!");
    }

    approach_ = appr;
}

template class WaterPvtMultiplexer<double,false,false>;
template class WaterPvtMultiplexer<double,true,false>;
template class WaterPvtMultiplexer<double,false,true>;
template class WaterPvtMultiplexer<double,true,true>;
template class WaterPvtMultiplexer<float,false,false>;
template class WaterPvtMultiplexer<float,true,false>;
template class WaterPvtMultiplexer<float,false,true>;
template class WaterPvtMultiplexer<float,true,true>;

} // namespace Opm
