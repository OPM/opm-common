/*
Copyright (C) 2024 SINTEF Digital, Mathematics and Cybernetics.

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

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <fmt/format.h>

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Opm {

CompositionalConfig::CompositionalConfig(const Deck& deck, const Runspec& runspec) {
    if (!DeckSection::hasPROPS(deck)) return;

    const PROPSSection props_section {deck};
    const bool comp_mode_runspec = runspec.compositionalMode(); // TODO: the way to use comp_mode_runspec should be refactored
    // the following code goes to a function
    if (!comp_mode_runspec) {
        warningForExistingCompKeywords(props_section);
        return; // not processing compositional props keywords
    }

    // we are under compositional mode now
    this->num_comps = runspec.numComps();

    if (props_section.hasKeyword<ParserKeywords::NCOMPS>()) {
        // NCOMPS might be present within multiple included files
        // We check all the input NCOMPS until testing proves that we can not have multiple of them
        const auto& keywords = props_section.get<ParserKeywords::NCOMPS>();
        for (const auto& kw : keywords) {
            const auto& item = kw.getRecord(0).getItem<ParserKeywords::NCOMPS::NUM_COMPS>();
            const auto ncomps = item.get<int>(0);
            if (size_t(ncomps) != this->num_comps) {
                const std::string msg = fmt::format("NCOMPS is specified with {}, which is different from the number specified in COMPS {}",
                                                    ncomps, this->num_comps);
                throw OpmInputError(msg, kw.location());
            }
        }
    }

    if (props_section.hasKeyword<ParserKeywords::STCOND>()) {
        const auto& keywords = props_section.get<ParserKeywords::STCOND>();
        for (const auto& kw : keywords) {
            const auto& temp_item = kw.getRecord(0).getItem<ParserKeywords::STCOND::TEMPERATURE>();
            this->standard_temperature = temp_item.getSIDouble(0);
            const auto& pres_item = kw.getRecord(0).getItem<ParserKeywords::STCOND::PRESSURE>();
            this->standard_pressure = pres_item.getSIDouble(0);
        }
    }

    const Tabdims tabdims{deck};
    const size_t num_eos_res = tabdims.getNumEosRes();
    // TODO: EOS keyword can also be in RUNSPEC section
    eos_types.resize(num_eos_res, EOSType::PR);
    if (props_section.hasKeyword<ParserKeywords::EOS>()) {
        // we do not allow multiple input of the keyword EOS unless proven otherwise
        const auto keywords = props_section.get<ParserKeywords::EOS>();
        if (keywords.size() > 1) {
            throw OpmInputError("there are multiple EOS keyword specification", keywords.begin()->location());
        }
        const auto& kw = keywords.back();
        for (size_t i = 0; i < kw.size(); ++i) {
            const auto& item = kw.getRecord(i).getItem<ParserKeywords::EOS::EQUATION>();
            const auto& equ_str = item.getTrimmedString(0);
            eos_types[i] = eosTypeFromString(equ_str);
        }
    }

    acentric_factors.resize(num_eos_res, std::vector<double>(this->num_comps, 0.));
    if (props_section.hasKeyword<ParserKeywords::ACF>()) {
        // we do not allow multiple input of the keyword ACF unless proven otherwise
        const auto& keywords = props_section.get<ParserKeywords::ACF>();
        if (keywords.size() > 1) {
            throw OpmInputError("there are multiple ACF keyword specification", keywords.begin()->location());
        }
        const auto& kw = keywords.back();
        for (size_t i = 0; i < kw.size(); ++i) {
            const auto& item = kw.getRecord(i).getItem<ParserKeywords::ACF::DATA>();
            const auto data = item.getData<double>();
            if (data.size() > this->num_comps) { // size_t vs int
                const auto msg = fmt::format("in keyword ACF, {} values are specified, which is bigger than the number of components {}",
                                                        data.size(), this->num_comps);
               throw OpmInputError(msg, kw.location());
            }
            // ACF has default values for 0. so we only overwrite when values are provided
            std::copy(data.begin(), data.end(), acentric_factors[i].begin());
        }
    }

    const size_t bic_size = this->num_comps * (this->num_comps - 1) / 2;
    binary_interaction_coefficient.resize(num_eos_res, std::vector<double>(bic_size,  0.)); // default value 0.
    if (props_section.hasKeyword<ParserKeywords::BIC>()) {
        // we do not allow multiple input of the keyword ACF unless proven otherwise
        const auto& keywords = props_section.get<ParserKeywords::BIC>();
        if (keywords.size() > 1) {
            throw OpmInputError("there are multiple BIC keyword specification", keywords.begin()->location());
        }
        const auto& kw = keywords.back();
        for (size_t i = 0; i < kw.size(); ++i) {
            const auto& item = kw.getRecord(i).getItem<ParserKeywords::BIC::DATA>();
            const auto data = item.getData<double>();
            if (data.size() > bic_size) { // size_t vs int
                const auto msg = fmt::format("in keyword BIC, {} values are specified, which is bigger than the number"
                                             "({} X {} = {})should be specified ",
                                             data.size(), this->num_comps, this->num_comps-1, bic_size);
                throw OpmInputError(msg, kw.location());
            }
            // BIC has default values for 0. so we only overwrite when values are provided
            std::copy(data.begin(), data.end(), binary_interaction_coefficient[i].begin());
        }
    }

    CompositionalConfig::processKeyword<ParserKeywords::PCRIT>(props_section, this->critical_pressure,
                                                              num_eos_res, this->num_comps, "PCRIT");
    CompositionalConfig::processKeyword<ParserKeywords::TCRIT>(props_section, this->critical_temperature,
                                                              num_eos_res, this->num_comps, "TCRIT");
    CompositionalConfig::processKeyword<ParserKeywords::VCRIT>(props_section, this->critical_volume,
                                                              num_eos_res, this->num_comps, "VCRIT");
}

bool CompositionalConfig::operator==(const CompositionalConfig& other) const {
    return this->num_comps == other.num_comps &&
           this->standard_temperature == other.standard_temperature &&
           this->standard_pressure == other.standard_pressure &&
           this->eos_types == other.eos_types &&
           this->acentric_factors == other.acentric_factors &&
           this->critical_pressure == other.critical_pressure &&
           this->critical_temperature == other.critical_temperature &&
           this->critical_volume == other.critical_volume &&
           this->binary_interaction_coefficient == other.binary_interaction_coefficient;
}


CompositionalConfig CompositionalConfig::serializationTestObject() {
    CompositionalConfig result;

    result.num_comps = 3;
    result.standard_temperature = 5.;
    result.standard_pressure = 1e5;
    result.eos_types = {2, EOSType::SRK};
    result.acentric_factors = {2,  std::vector(result.num_comps, 1.)};
    result.critical_pressure = {2, std::vector(result.num_comps, 2.)};
    result.critical_temperature = {2, std::vector(result.num_comps, 3.)};
    result.critical_volume = {2, std::vector(result.num_comps, 5.)};
    result.binary_interaction_coefficient = {2, std::vector(result.num_comps * (result.num_comps - 1) / 2, 6.)};

    return result;
}

CompositionalConfig::EOSType CompositionalConfig::eosTypeFromString(const std::string& str) {
    if (str == "PR") return EOSType::PR;
    if (str == "RK") return EOSType::RK;
    if (str == "SRK") return EOSType::SRK;
    if (str == "ZJ") return EOSType::ZJ;
    throw std::invalid_argument("Unknown string for EOSType");
}

std::string CompositionalConfig::eosTypeToString(Opm::CompositionalConfig::EOSType eos) {
    switch (eos) {
        case EOSType::PR: return "PR";
        case EOSType::RK: return "RK";
        case EOSType::SRK: return "SRK";
        case EOSType::ZJ: return "ZJ";
        default: throw std::invalid_argument("Unknown EOSType");
    }
}

void CompositionalConfig::warningForExistingCompKeywords(const PROPSSection& props_section) {
    bool any_comp_prop_kw = false;
    std::string msg {" COMPS is not specified, the following keywords related to compositional simulation in PROPS section will be ignored:\n"};

    static const std::unordered_map<std::string, std::function<bool(const PROPSSection&)>> keywordCheckers = {
        {"NCOMPS", [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::NCOMPS>(); }},
        {"EOS",    [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::EOS>(); }},
        {"STCOND", [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::STCOND>(); }},
        {"PCRIT",  [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::PCRIT>(); }},
        {"TCRIT",  [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::TCRIT>(); }},
        {"VCRIT",  [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::VCRIT>(); }},
        {"ACF",    [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::ACF>(); }},
        {"BIC",    [](const PROPSSection& section) -> bool { return section.hasKeyword<ParserKeywords::BIC>(); }},
    };

    for (const auto& [kwname, checker] : keywordCheckers) {
        if (checker(props_section)) {
            any_comp_prop_kw = true;
            fmt::format_to(std::back_inserter(msg),  " {}", kwname);
        }
    }

    if (any_comp_prop_kw) {
        OpmLog::warning(msg);
    }
}

double CompositionalConfig::standardTemperature() const {
    return this->standard_temperature;
}

double CompositionalConfig::standardPressure() const {
    return this->standard_pressure;
}

CompositionalConfig::EOSType CompositionalConfig::eosType(size_t eos_region) const {
    return this->eos_types[eos_region];
}

const std::vector<double>& CompositionalConfig::acentricFactors(size_t eos_region) const {
    return this->acentric_factors[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalPressure(size_t eos_region) const {
    return this->critical_pressure[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalTemperature(size_t eos_region) const {
    return this->critical_temperature[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalVolume(size_t eos_region) const {
    return this->critical_volume[eos_region];
}

const std::vector<double>& CompositionalConfig::binaryInteractionCoefficient(size_t eos_region) const {
    return this->binary_interaction_coefficient[eos_region];
}

}