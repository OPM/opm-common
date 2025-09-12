/*
  Copyright 2020 Equinor ASA.

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

#include <opm/io/eclipse/rst/group.hpp>

#include <opm/common/utility/String.hpp>

#include <opm/io/eclipse/rst/header.hpp>

#include <opm/output/eclipse/VectorItems/group.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;
using M = ::Opm::UnitSystem::measure;

Opm::RestartIO::RstGroup::RstGroup(const ::Opm::UnitSystem& unit_system,
                                   const RstHeader&         header,
                                   const std::string*       zwel,
                                   const int*               igrp,
                                   const float*             sgrp,
                                   const double*            xgrp) :
    // -------------------------------------------------------------------------

    name(trim_copy(zwel[0])),

    // -------------------------------------------------------------------------

    parent_group(igrp[header.nwgmax + VI::IGroup::ParentGroup]),
    prod_cmode(igrp[header.nwgmax + VI::IGroup::GConProdCMode]),
    winj_cmode(igrp[header.nwgmax + VI::IGroup::GConInjeWInjCMode]),
    ginj_cmode(igrp[header.nwgmax + VI::IGroup::GConInjeGInjCMode]),
    prod_guide_rate_def(igrp[header.nwgmax + VI::IGroup::GuideRateDef]),
    exceed_action(igrp[header.nwgmax + VI::IGroup::ExceedAction]),
    inj_water_guide_rate_def(igrp[header.nwgmax + VI::IGroup::GConInjeWaterGuideRateMode]),
    inj_gas_guide_rate_def(igrp[header.nwgmax + VI::IGroup::GConInjeGasGuideRateMode]),
    voidage_group_index(igrp[header.nwgmax + VI::IGroup::VoidageGroupIndex]),
    add_gas_lift_gas(igrp[header.nwgmax + VI::IGroup::AddGLiftGasAsProducedGas]),
    group_type(igrp[header.nwgmax + VI::IGroup::GroupType]),

    // -------------------------------------------------------------------------

    // The values oil_rate_limit -> gas_voidage_limit will be used in UDA
    // values. The UDA values are responsible for unit conversion and raw values
    // are internalized here.
    oil_rate_limit(                sgrp[VI::SGroup::OilRateLimit]),
    water_rate_limit(              sgrp[VI::SGroup::WatRateLimit]),
    gas_rate_limit(                sgrp[VI::SGroup::GasRateLimit]),
    liquid_rate_limit(             sgrp[VI::SGroup::LiqRateLimit]),
    resv_rate_limit(               sgrp[VI::SGroup::ResvRateLimit]),
    water_surface_limit(           sgrp[VI::SGroup::waterSurfRateLimit]),
    water_reservoir_limit(         sgrp[VI::SGroup::waterResRateLimit]),
    water_reinject_limit(          sgrp[VI::SGroup::waterReinjectionLimit]),
    water_voidage_limit(           sgrp[VI::SGroup::waterVoidageLimit]),
    gas_surface_limit(             sgrp[VI::SGroup::gasSurfRateLimit]),
    gas_reservoir_limit(           sgrp[VI::SGroup::gasResRateLimit]),
    gas_reinject_limit(            sgrp[VI::SGroup::gasReinjectionLimit]),
    gas_voidage_limit(             sgrp[VI::SGroup::gasVoidageLimit]),
    glift_max_supply(              unit_system.to_si(M::gas_surface_rate,      sgrp[VI::SGroup::GLOMaxSupply])),
    glift_max_rate(                unit_system.to_si(M::gas_surface_rate,      sgrp[VI::SGroup::GLOMaxRate])),
    efficiency_factor(             unit_system.to_si(M::identity,              sgrp[VI::SGroup::EfficiencyFactor])),
    inj_water_guide_rate(          sgrp[VI::SGroup::waterGuideRate]),
    inj_gas_guide_rate(            sgrp[VI::SGroup::gasGuideRate]),
    gas_consumption_rate(          sgrp[VI::SGroup::GasConsumptionRate]),      // UDA, stored in output units
    gas_import_rate(               sgrp[VI::SGroup::GasImportRate]),           // UDA, stored in output units

    // -------------------------------------------------------------------------

    oil_production_rate(           unit_system.to_si(M::liquid_surface_rate,   xgrp[VI::XGroup::OilPrRate])),
    water_production_rate(         unit_system.to_si(M::liquid_surface_rate,   xgrp[VI::XGroup::WatPrRate])),
    gas_production_rate(           unit_system.to_si(M::gas_surface_rate,      xgrp[VI::XGroup::GasPrRate])),
    liquid_production_rate(        unit_system.to_si(M::liquid_surface_rate,   xgrp[VI::XGroup::LiqPrRate])),
    water_injection_rate(          unit_system.to_si(M::liquid_surface_rate,   xgrp[VI::XGroup::WatInjRate])),
    gas_injection_rate(            unit_system.to_si(M::gas_surface_rate,      xgrp[VI::XGroup::GasInjRate])),
    wct(                           unit_system.to_si(M::water_cut,             xgrp[VI::XGroup::WatCut])),
    gor(                           unit_system.to_si(M::gas_oil_ratio,         xgrp[VI::XGroup::GORatio])),
    oil_production_total(          unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::OilPrTotal])),
    water_production_total(        unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::WatPrTotal])),
    gas_production_total(          unit_system.to_si(M::gas_surface_volume,    xgrp[VI::XGroup::GasPrTotal])),
    voidage_production_total(      unit_system.to_si(M::geometric_volume,      xgrp[VI::XGroup::VoidPrTotal])),
    water_injection_total(         unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::WatInjTotal])),
    gas_injection_total(           unit_system.to_si(M::gas_surface_volume,    xgrp[VI::XGroup::GasInjTotal])),
    voidage_injection_total(       unit_system.to_si(M::volume,                xgrp[VI::XGroup::VoidInjTotal])),
    oil_production_potential(      unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::OilPrPot])),
    water_production_potential(    unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::WatPrPot])),
    history_total_oil_production(  unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::HistOilPrTotal])),
    history_total_water_production(unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::HistWatPrTotal])),
    history_total_water_injection( unit_system.to_si(M::liquid_surface_volume, xgrp[VI::XGroup::HistWatInjTotal])),
    history_total_gas_production(  unit_system.to_si(M::gas_surface_volume,    xgrp[VI::XGroup::HistGasPrTotal])),
    history_total_gas_injection(   unit_system.to_si(M::gas_surface_volume,    xgrp[VI::XGroup::HistGasInjTotal])),
    gas_consumption_total(         unit_system.to_si(M::gas_surface_volume,    xgrp[VI::XGroup::GasConsumptionTotal])),
    gas_import_total(              unit_system.to_si(M::gas_surface_volume,    xgrp[VI::XGroup::GasImportTotal]))
{}
