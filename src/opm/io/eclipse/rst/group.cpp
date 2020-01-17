/*
  Copyright 2020 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/group.hpp>

#include <opm/output/eclipse/VectorItems/group.hpp>


namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace Opm {
namespace RestartIO {

Group::Group(const Header& header,
             const EclIO::PaddedOutputString<8>* zwel,
             const int *,
             const float * sgrp,
             const double * xgrp) :
    name(zwel[0].trim()),
    oil_rate_limit(sgrp[VI::SGroup::OilRateLimit]),
    water_rate_limit(sgrp[VI::SGroup::WatRateLimit]),
    gas_rate_limit(sgrp[VI::SGroup::GasRateLimit]),
    liquid_rate_limit(sgrp[VI::SGroup::LiqRateLimit]),
    water_surface_limit(sgrp[VI::SGroup::waterSurfRateLimit]),
    water_reservoir_limit(sgrp[VI::SGroup::waterResRateLimit]),
    water_reinject_limit(sgrp[VI::SGroup::waterReinjectionLimit]),
    water_voidage_limit(sgrp[VI::SGroup::waterVoidageLimit]),
    gas_surface_limit(sgrp[VI::SGroup::gasSurfRateLimit]),
    gas_reservoir_limit(sgrp[VI::SGroup::gasResRateLimit]),
    gas_reinject_limit(sgrp[VI::SGroup::gasReinjectionLimit]),
    gas_voidage_limit(sgrp[VI::SGroup::gasVoidageLimit]),
    oil_production_rate(xgrp[VI::XGroup::OilPrRate]),
    water_production_rate(xgrp[VI::XGroup::WatPrRate]),
    gas_production_rate(xgrp[VI::XGroup::GasPrRate]),
    liquid_production_rate(xgrp[VI::XGroup::LiqPrRate]),
    water_injection_rate(xgrp[VI::XGroup::WatInjRate]),
    gas_injection_rate(xgrp[VI::XGroup::GasInjRate]),
    wct(xgrp[VI::XGroup::WatCut]),
    gor(xgrp[VI::XGroup::GORatio]),
    oil_production_total(xgrp[VI::XGroup::OilPrTotal]),
    water_production_total(xgrp[VI::XGroup::WatPrTotal]),
    gas_production_total(xgrp[VI::XGroup::GasPrTotal]),
    voidage_production_total(xgrp[VI::XGroup::VoidPrTotal]),
    water_injection_total(xgrp[VI::XGroup::WatInjTotal]),
    gas_injection_total(xgrp[VI::XGroup::GasInjTotal]),
    oil_production_potential(xgrp[VI::XGroup::OilPrPot]),
    water_production_potential(xgrp[VI::XGroup::WatPrPot]),
    history_total_oil_production(xgrp[VI::XGroup::HistOilPrTotal]),
    history_total_water_production(xgrp[VI::XGroup::HistWatPrTotal]),
    history_total_water_injection(xgrp[VI::XGroup::HistWatInjTotal]),
    history_total_gas_production(xgrp[VI::XGroup::HistGasPrTotal]),
    history_total_gas_injection(xgrp[VI::XGroup::HistGasInjTotal])
{
}


}
}
