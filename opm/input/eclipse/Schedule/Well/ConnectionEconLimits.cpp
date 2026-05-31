/*
  Copyright 2026 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/ConnectionEconLimits.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/UDAValue.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>

#include <stdexcept>
#include <string>

namespace {

// CECON defaulted MIN_OIL/MIN_GAS value (-1e+20) carries a rate dimension.
// any value bigger than that considered to be a limit
// TODO: since it is too small, not sure whether this matters at all
double minRateSentinel(const Opm::UDAValue& uda) {
    return uda.get_dim().convertRawToSi(-1e+20);
}

double minRateSI(const Opm::UDAValue& uda) {
    return uda.SI_value_or(minRateSentinel(uda));
}

} // Anonymous namespace

namespace Opm {

ConnectionEconLimits::ConnectionEconLimits(const DeckRecord& record)
    : max_water_cut      (record.getItem<ParserKeywords::CECON::MAX_WCUT>()
                                .get<UDAValue>(0).SI_value_or(0.0))
    , max_gas_oil_ratio  (record.getItem<ParserKeywords::CECON::MAX_GOR>()
                                .get<UDAValue>(0).SI_value_or(0.0))
    , max_water_gas_ratio(record.getItem<ParserKeywords::CECON::MAX_WGR>()
                                .get<UDAValue>(0).SI_value_or(0.0))
    , workover(ConnectionEconLimits::EconWorkoverFromString(
                   record.getItem<ParserKeywords::CECON::WORKOVER_PROCEDURE>()
                       .getTrimmedString(0)))
    , min_oil_rate(minRateSI(record.getItem<ParserKeywords::CECON::MIN_OIL>()
                                 .get<UDAValue>(0)))
    , min_gas_rate(minRateSI(record.getItem<ParserKeywords::CECON::MIN_GAS>()
                                 .get<UDAValue>(0)))
    , min_oil_rate_sentinel(minRateSentinel(record.getItem<ParserKeywords::CECON::MIN_OIL>()
                                                .get<UDAValue>(0)))
    , min_gas_rate_sentinel(minRateSentinel(record.getItem<ParserKeywords::CECON::MIN_GAS>()
                                                .get<UDAValue>(0)))
    , followon_well(record.getItem<ParserKeywords::CECON::FOLLOW_ON_WELL>()
                        .getTrimmedString(0))
{
    const auto check_str = record
        .getItem<ParserKeywords::CECON::CHECK_STOPPED>()
        .getTrimmedString(0);
    this->check_stopped_wells = DeckItem::to_bool(check_str);
}

bool ConnectionEconLimits::operator==(const ConnectionEconLimits& other) const
{
    return (this->max_water_cut       == other.max_water_cut)
        && (this->max_gas_oil_ratio   == other.max_gas_oil_ratio)
        && (this->max_water_gas_ratio == other.max_water_gas_ratio)
        && (this->workover            == other.workover)
        && (this->check_stopped_wells == other.check_stopped_wells)
        && (this->min_oil_rate         == other.min_oil_rate)
        && (this->min_gas_rate         == other.min_gas_rate)
        && (this->min_oil_rate_sentinel == other.min_oil_rate_sentinel)
        && (this->min_gas_rate_sentinel == other.min_gas_rate_sentinel)
        && (this->followon_well       == other.followon_well)
        ;
}

bool ConnectionEconLimits::onAnyEffectiveLimit() const
{
    return this->onMaxWaterCut()
        || this->onMaxGasOilRatio()
        || this->onMaxWaterGasRatio()
        || this->onMinOilRate()
        || this->onMinGasRate();
}

ConnectionEconLimits ConnectionEconLimits::serializationTestObject()
{
    ConnectionEconLimits limits;
    limits.max_water_cut       = 0.7;
    limits.max_gas_oil_ratio   = 3.5;
    limits.max_water_gas_ratio = 0.1;
    limits.workover            = EconWorkover::WELL;
    limits.check_stopped_wells = true;
    limits.min_oil_rate        = 1.0;
    limits.min_gas_rate        = 2.0;
    limits.followon_well       = "P2";

    return limits;
}

std::string ConnectionEconLimits::EconWorkover2String(EconWorkover enumValue)
{
    switch (enumValue) {
    case EconWorkover::CON:
        return "CON";
    case EconWorkover::CONP:
        return "+CON";
    case EconWorkover::WELL:
        return "WELL";
    case EconWorkover::PLUG:
        return "PLUG";
    default:
        throw std::invalid_argument("unhandled CECON EconWorkover value");
    }
}

ConnectionEconLimits::EconWorkover
ConnectionEconLimits::EconWorkoverFromString(const std::string& stringValue)
{
    if (stringValue == "CON")
        return EconWorkover::CON;
    else if (stringValue == "+CON")
        return EconWorkover::CONP;
    else if (stringValue == "WELL")
        return EconWorkover::WELL;
    else if (stringValue == "PLUG")
        return EconWorkover::PLUG;
    else
        throw std::invalid_argument("Unknown enum stringValue: " + stringValue +
                                    " for CECON EconWorkover");
}

} // namespace Opm
