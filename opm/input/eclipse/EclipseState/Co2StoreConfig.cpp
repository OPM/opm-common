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

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Tabdims.hpp>

namespace {
    template <class EzrokhiTable>
    void initEzrokhiTable(const Opm::Deck& deck,
                          const std::string& keyword_name,
                          const std::size_t num_eos_res,
                          const std::map<std::string, int>& cnames,
                          EzrokhiTable& ezrokhitable)
    {
        if (!deck.hasKeyword(keyword_name))
            return;

        if (cnames.empty()) {
            const auto msg = "CNAMES must be defined together with " + keyword_name;
            throw Opm::OpmInputError(msg, deck[keyword_name].begin()->location());
        }    
        
        ezrokhitable.resize(num_eos_res);
        const auto& keyword = deck[keyword_name].back();
        for (std::size_t tableIdx = 0; tableIdx < num_eos_res; ++tableIdx) {
            const auto& record = keyword.getRecord(tableIdx);
            if (record.getItem("DATA").data_size() < 3 * cnames.size()) {
                auto msg = "The " + keyword_name + " table does not have C0, C1 and C2 for all CNAMES: ";
                for (const auto& [cname, val]: cnames) {
                    msg += cname + " ";
                }
                throw Opm::OpmInputError(msg, keyword.location());
            }
            for (const auto& cname: cnames) {
                ezrokhitable[tableIdx].init(record, cname.first, cname.second);
            }
        }
    }
}

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
        }     
        else {
            this->brine_type = string2enumSalt(deck["THCO2MIX"].back().getRecord(0).getItem<MIX::MIXING_MODEL_SALT>().get<std::string>( 0 ));
            this->liquid_type = string2enumLiquid(deck["THCO2MIX"].back().getRecord(0).getItem<MIX::MIXING_MODEL_LIQUID>().get<std::string>( 0 ));
            this->gas_type = string2enumGas(deck["THCO2MIX"].back().getRecord(0).getItem<MIX::MIXING_MODEL_GAS>().get<std::string>( 0 ));
        }

        // //
        // Parse compositional keywords from PROPS section
        // //
        // CNAMES
        const PROPSSection props_section {deck};
        if (props_section.hasKeyword<ParserKeywords::CNAMES>())
        {
            const auto& cnames_keyword = props_section.get<ParserKeywords::CNAMES>();
            if (cnames_keyword.size() > 1){
                throw OpmInputError("Multiple CNAMES keywords defined in deck", cnames_keyword.begin()->location());
            }
            const auto& keyword = cnames_keyword.back();
            const auto& item = keyword.getRecord(0).getItem<ParserKeywords::CNAMES::data>();
            const auto num_comp = item.getData<std::string>().size();
            cnames.insert({{"H2O", -1}, {"CO2", -1}, {"NACL", -1}});
            for (size_t c = 0; c < num_comp; ++c) {
                const auto& name = item.getTrimmedString(c);
                if (cnames.find(name) != cnames.end()) {
                    cnames[name] = c;
                }
            }
            for (const auto& cname: cnames) {
                if (cname.second < 0) {
                    const auto msg = "Component " + cname.first + " is required in CNAMES when CO2STORE is active!";
                    throw OpmInputError(msg, keyword.location());
                }
            }
        }

        // DENAQA and VISCAQA
        const Tabdims tabdims{deck};
        const size_t num_eos_res = tabdims.getNumEosRes();
        if (props_section.hasKeyword<ParserKeywords::DENAQA>()) {
            initEzrokhiTable(deck, "DENAQA", num_eos_res, cnames, denaqa_tables);
        }
        if (props_section.hasKeyword<ParserKeywords::VISCAQA>()) {
            initEzrokhiTable(deck, "VISCAQA", num_eos_res, cnames, viscaqa_tables);
        }
    }

    const std::vector<EzrokhiTable>& Co2StoreConfig::getDenaqaTables() const {
        return denaqa_tables;
    }

    const std::vector<EzrokhiTable>& Co2StoreConfig::getViscaqaTables() const {
        return viscaqa_tables;
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
