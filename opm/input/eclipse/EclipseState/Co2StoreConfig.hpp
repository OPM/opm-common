/*
  Copyright 2024 Norce.

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
#ifndef OPM_PARSER_CO2STORECONFIG_HPP
#define	OPM_PARSER_CO2STORECONFIG_HPP

#include <cstddef>
#include <vector>
#include <string>
#include <map>

#include <opm/input/eclipse/EclipseState/Tables/EzrokhiTable.hpp>

namespace Opm {

class Deck;

  class Co2StoreConfig {
  public:

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

    Co2StoreConfig();

    explicit Co2StoreConfig(const Deck& deck);

    const std::vector<EzrokhiTable>& getDenaqaTables() const;
    const std::vector<EzrokhiTable>& getViscaqaTables() const;
    
    double salinity() const;
    int actco2s() const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
       serializer(brine_type);
       serializer(liquid_type);
       serializer(gas_type);
    }
    bool operator==(const Co2StoreConfig& other) const;

    SaltMixingType brine_type;
    LiquidMixingType liquid_type;
    GasMixingType gas_type;

  private:

    SaltMixingType string2enumSalt(const std::string& input) const;
    LiquidMixingType string2enumLiquid(const std::string& input) const;
    GasMixingType string2enumGas(const std::string& input) const;
    
    // std::vector<std::pair<std::string, int> > cnames;
    std::map<std::string, int> cnames;
    std::vector<EzrokhiTable> denaqa_tables;
    std::vector<EzrokhiTable> viscaqa_tables;
    double salt {0.0};
    double MmNaCl = 58.44e-3;
    double MmH2O = 18e-3;
    int activityModel {3};
  };
}

#endif // OPM_PARSER_CO2STORECONFIG_HPP
