/*
  Copyright (C) 2023 Equinor

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

#include <fmt/format.h>
#include <algorithm>
#include <iostream>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>
#include <opm/input/eclipse/EclipseState/WagHysteresisConfig.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

namespace Opm {

    const WagHysteresisConfig::WagHysteresisConfigRecord& WagHysteresisConfig::operator[](std::size_t index) const {
        return this->wagrecords.at(index);
    }

    WagHysteresisConfig::WagHysteresisConfig()
    {}

    WagHysteresisConfig::WagHysteresisConfig(const Deck& deck)
    {
        using WAG = ParserKeywords::WAGHYSTR;
        if (deck.hasKeyword<WAG>()) {
            const auto& keyword = deck.get<WAG>().back();
            OpmLog::info( keyword.location().format("\nInitializing WAG hystersis parameters from {keyword} in {file} line {line}") );
            for (const auto& record : keyword) {
                //this->wagrecords.emplace_back(kw.getRecord(0));
                this->wagrecords.emplace_back(record);
            }
        }
    }

    size_t WagHysteresisConfig::size() const {
        return this->wagrecords.size();
    }

    bool WagHysteresisConfig::empty() const {
        return this->wagrecords.empty();
    }

    const std::vector<WagHysteresisConfig::WagHysteresisConfigRecord>::const_iterator WagHysteresisConfig::begin() const {
        return this->wagrecords.begin();
    }

    const std::vector<WagHysteresisConfig::WagHysteresisConfigRecord>::const_iterator WagHysteresisConfig::end() const {
        return this->wagrecords.end();
    }

    bool WagHysteresisConfig::operator==(const WagHysteresisConfig& other) const {
        return this->wagrecords == other.wagrecords;
    }

    WagHysteresisConfig::WagHysteresisConfigRecord::WagHysteresisConfigRecord(const DeckRecord& record)
        : wagLandsParamValue(record.getItem<ParserKeywords::WAGHYSTR::LANDS_PARAMETER>().get<double>(0))
        , wagSecondaryDrainageReductionValue(record.getItem<ParserKeywords::WAGHYSTR::SECONDARY_DRAINAGE_REDUCTION>().get<double>(0))
        , wagGasFlagValue(DeckItem::to_bool(record.getItem<ParserKeywords::WAGHYSTR::GAS_MODEL>().get<std::string>(0)))
        , wagResidualOilFlagValue(DeckItem::to_bool(record.getItem<ParserKeywords::WAGHYSTR::RES_OIL>().get<std::string>(0)))
        , wagWaterFlagValue(DeckItem::to_bool(record.getItem<ParserKeywords::WAGHYSTR::WATER_MODEL>().get<std::string>(0)))
        , wagImbCurveLinearFractionValue(record.getItem<ParserKeywords::WAGHYSTR::IMB_LINEAR_FRACTION>().get<double>(0))
        , wagWaterThresholdSaturationValue(record.getItem<ParserKeywords::WAGHYSTR::THREEPHASE_SAT_LIMIT>().get<double>(0))
    {}

} // namespace Opm
