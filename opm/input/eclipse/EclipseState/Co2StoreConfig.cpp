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
#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>
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
            for (const auto& [cname, index]: cnames) {
                // Check if CNAMES include H2O, CO2 and NACL
                if (index < 0) {
                    const auto msg = "CNAMES must include " + cname + " to use "
                                     + keyword_name + " in combination with CO2STORE";
                    throw Opm::OpmInputError(msg, keyword.location());
                }
                // Check if table has entries for queried cname
                if (3 * static_cast<std::size_t>(index) + 3 > record.getItem("DATA").data_size()) {
                    auto msg = keyword_name + " does not have C0, C1 and C2 entries for CNAMES = " + cname;
                    throw Opm::OpmInputError(msg, keyword.location());
                }
                ezrokhitable[tableIdx].init(record, cname, index);
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

            // Only read CNAMES for H2O, CO2 and NACL
            const auto& keyword = cnames_keyword.back();
            const auto& item = keyword.getRecord(0).getItem<ParserKeywords::CNAMES::data>();
            const auto num_comp = item.getData<std::string>().size();
            cnames.insert({{"H2O", -1}, {"CO2", -1}, {"NACL", -1}});
            for (size_t c = 0; c < num_comp; ++c) {
                const auto name = item.getTrimmedString(c);
                auto it = cnames.find(name);
                if (it != cnames.end()) {
                    it->second  = c;
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

        // SALINITY or SALTMF convert to mass fraction.
        if (props_section.hasKeyword<ParserKeywords::SALINITY>()) {
            const auto& molality = deck["SALINITY"].back().getRecord(0).getItem("MOLALITY").get<double>(0);
            salt = 1.0 / (1.0 + 1.0 / (molality * MmNaCl));
        }
        else if (props_section.hasKeyword<ParserKeywords::SALTMF>()) {
            const auto& mole_frac = deck["SALTMF"].back().getRecord(0).getItem("MOLE_FRACTION").get<double>(0);
            salt = mole_frac * MmNaCl / (mole_frac * (MmNaCl - MmH2O) + MmH2O);
        }

        // ACTCO2S
        if (props_section.hasKeyword<ParserKeywords::ACTCO2S>()) {
            activityModel = deck["ACTCO2S"].back().getRecord(0).getItem("ACTIVITY_MODEL").get<int>(0);
        }
        else {
            activityModel = ParserKeywords::ACTCO2S::ACTIVITY_MODEL::defaultValue;
        }
    }

    const std::vector<EzrokhiTable>& Co2StoreConfig::getDenaqaTables() const {
        return denaqa_tables;
    }

    const std::vector<EzrokhiTable>& Co2StoreConfig::getViscaqaTables() const {
        return viscaqa_tables;
    }

    double Co2StoreConfig::salinity() const {
        return salt;
    }

    int Co2StoreConfig::actco2s() const {
        return activityModel;
    }

    bool Co2StoreConfig::operator==(const Co2StoreConfig& other) const {
        return this->brine_type == other.brine_type
                && this->liquid_type == other.liquid_type
                && this->gas_type == other.gas_type
                && this->denaqa_tables == other.denaqa_tables
                && this->viscaqa_tables == other.viscaqa_tables
                && this->salt == other.salt
                && this->activityModel == other.activityModel
                && this->cnames == other.cnames;
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
