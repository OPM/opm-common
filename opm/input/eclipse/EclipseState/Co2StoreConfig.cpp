/*
  Copyright (C) 2024 Norce

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
#include <opm/input/eclipse/EclipseState/Co2StoreConfig.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>

namespace Opm {

    Co2StoreConfig::Co2StoreConfig()
    {
        this->brine_type = Co2StoreConfig::SaltMixingType::MICHAELIDES;
        this->liquid_type = Co2StoreConfig::LiquidMixingType::DUANSUN;
        this->gas_type = Co2StoreConfig::GasMixingType::NONE;
    }

    Co2StoreConfig::Co2StoreConfig(const Deck& deck)
    {
        using MIX = ParserKeywords::THCO2MIX;
        if (!deck.hasKeyword("THCO2MIX")) {
            this->brine_type = string2enumSalt(MIX::MIXING_MODEL_SALT::defaultValue);
            this->liquid_type = string2enumLiquid(MIX::MIXING_MODEL_LIQUID::defaultValue);
            this->gas_type = string2enumGas(MIX::MIXING_MODEL_GAS::defaultValue);
            return;
        }     
        this->brine_type = string2enumSalt(deck["THCO2MIX"].back().getRecord(0).getItem<MIX::MIXING_MODEL_SALT>().get<std::string>( 0 ));
        this->liquid_type = string2enumLiquid(deck["THCO2MIX"].back().getRecord(0).getItem<MIX::MIXING_MODEL_LIQUID>().get<std::string>( 0 ));
        this->gas_type = string2enumGas(deck["THCO2MIX"].back().getRecord(0).getItem<MIX::MIXING_MODEL_GAS>().get<std::string>( 0 ));
    }

    bool Co2StoreConfig::operator==(const Co2StoreConfig& other) const {
        return this->brine_type == other.brine_type 
                && this->liquid_type == other.liquid_type
                && this->gas_type == other.gas_type;
    }

    enum class SaltMixingType {
        NONE,        // Pure water
        MICHAELIDES, // MICHAELIDES 1971 (default)
    };
    
    enum class LiquidMixingType {
        NONE,     // Pure water
        IDEAL,    // Ideal mixing
        DUANSUN,  // Add heat of dissolution for CO2 according to Fig. 6 in Duan and Sun 2003. (kJ/kg) (default)
    };

    enum class GasMixingType {
        NONE,   // Pure co2 (default)
        IDEAL,  // Ideal mixing
    };

    Co2StoreConfig::SaltMixingType Co2StoreConfig::string2enumSalt(const std::string& input) const {
        if (input == "MICHAELIDES") return Co2StoreConfig::SaltMixingType::MICHAELIDES;
        else if (input == "NONE") return Co2StoreConfig::SaltMixingType::NONE;
        else {
            throw std::invalid_argument(input + " is not a valid Salte mixing type. See THCO2MIX item 1");
        }
    }
    Co2StoreConfig::LiquidMixingType Co2StoreConfig::string2enumLiquid(const std::string& input) const {
        if (input == "DUANSUN") return Co2StoreConfig::LiquidMixingType::DUANSUN;
        else if (input == "IDEAL") return Co2StoreConfig::LiquidMixingType::IDEAL;
        else if (input == "NONE") return Co2StoreConfig::LiquidMixingType::NONE;
        else {
            throw std::invalid_argument(input + " is not a valid Salte mixing type. See THCO2MIX item 2");
        }
    }
    Co2StoreConfig::GasMixingType Co2StoreConfig::string2enumGas(const std::string& input) const {
        if (input == "IDEAL") return Co2StoreConfig::GasMixingType::IDEAL;
        else if (input == "NONE") return Co2StoreConfig::GasMixingType::NONE;
        else {
            throw std::invalid_argument(input + " is not a valid Salte mixing type. See THCO2MIX item 3");
        }
    }


} // namespace Opm
