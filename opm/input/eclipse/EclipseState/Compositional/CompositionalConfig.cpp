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

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Tabdims.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/Z.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <vector>

namespace {

    template <typename Keyword>
    const Opm::DeckKeyword* getSinglePropsKeyword(const Opm::PROPSSection& props_section)
    {
        if (!props_section.hasKeyword<Keyword>()) {
            return nullptr;
        }

        const auto& keywords = props_section.get<Keyword>();
        if (keywords.size() > 1) {
            throw Opm::OpmInputError {
                fmt::format("there are multiple {} keyword specifications", Keyword::keywordName),
                keywords.begin()->location()
            };
        }

        return &keywords.back();
    }

    void validateKeywordRegionCount(const Opm::DeckKeyword& kw,
                                    const std::size_t       num_regions,
                                    const std::string_view  region_description,
                                    const bool              has_default_value)
    {
        const auto& kw_name = kw.name();
        if (kw.size() > num_regions) {
            throw Opm::OpmInputError {
                fmt::format("there are {} {}, while {} "
                            "regions are specified in {}",
                            num_regions, region_description, kw.size(), kw_name),
                kw.location()
            };
        }

        if (!has_default_value && kw.size() != num_regions) {
            throw Opm::OpmInputError {
                fmt::format("there are {} {}, while only {} "
                            "regions are specified in {}",
                            num_regions, region_description, kw.size(), kw_name),
                kw.location()
            };
        }
    }

    template <typename Keyword>
    void parsingKeywordValues(const Opm::DeckKeyword&        kw,
                              std::vector<std::vector<double>>& target,
                              const std::size_t              num_values,
                              const std::optional<double>    default_value)
    {
        const auto& kw_name = Keyword::keywordName;

        for (std::size_t i = 0; i < kw.size(); ++i) {
            const auto& item = kw.getRecord(i).template getItem<typename Keyword::DATA>();
            const auto& data = item.getSIDoubleData();

            if (!default_value.has_value()) {
                if (data.size() != num_values) {
                    const auto msg = fmt::format("in keyword {}, {} values are specified, "
                                                 "but {} values are expected",
                                                 kw_name, data.size(), num_values);
                    throw Opm::OpmInputError(msg, kw.location());
                }
            }
            else if (data.size() > num_values) {
                const auto msg = fmt::format("in keyword {}, {} values are specified, "
                                             "which is bigger than the number "
                                             "{} should be specified",
                                             kw_name, data.size(), num_values);

                throw Opm::OpmInputError(msg, kw.location());
            }

            std::ranges::copy(data, target[i].begin());
        }
    }

    std::string_view reservoirKeywordName(std::string_view surface_keyword_name)
    {
        static const std::unordered_map<std::string_view, std::string_view> mapping {
            {Opm::ParserKeywords::MWS::keywordName,    Opm::ParserKeywords::MW::keywordName},
            {Opm::ParserKeywords::ACFS::keywordName,   Opm::ParserKeywords::ACF::keywordName},
            {Opm::ParserKeywords::PCRITS::keywordName, Opm::ParserKeywords::PCRIT::keywordName},
            {Opm::ParserKeywords::TCRITS::keywordName, Opm::ParserKeywords::TCRIT::keywordName},
            {Opm::ParserKeywords::VCRITS::keywordName, Opm::ParserKeywords::VCRIT::keywordName},
            {Opm::ParserKeywords::ZCRITS::keywordName, Opm::ParserKeywords::ZCRIT::keywordName},
            {Opm::ParserKeywords::SSHIFTS::keywordName, Opm::ParserKeywords::SSHIFT::keywordName},
            {Opm::ParserKeywords::BICS::keywordName,   Opm::ParserKeywords::BIC::keywordName},
        };

        return mapping.at(surface_keyword_name);
    }

    // The following function is used to parse compositional related keywords:
    // MW, ACF, BIC, PCRIT, TCRIT, VCRIT and SSHIFT, and so on.
    template <typename Keyword>
    void processKeyword(const Opm::PROPSSection& props_section,
                        std::vector<std::vector<double>>& target,
                        const std::size_t num_eos_res,
                        const std::size_t num_values,
                        const std::optional<double> default_value = std::nullopt)
    {
        const auto* kw = getSinglePropsKeyword<Keyword>(props_section);
        if (!kw) {
            if (default_value.has_value()) {
                target.assign(num_eos_res,std::vector(num_values, default_value.value()));
            }

            return;
        }

        target.assign(num_eos_res,std::vector(num_values, default_value.value_or(0.0)));

        validateKeywordRegionCount(*kw, num_eos_res, "EOS regions",
                                   default_value.has_value());

        parsingKeywordValues<Keyword>(*kw, target, num_values, default_value);
    }

    // Parse one surface keyword. If absent, inherit per-region reservoir values.
    // If present, use only keyword data/defaults (no reservoir fallback).
    template <typename Keyword>
    void processSurfaceKeyword(const Opm::PROPSSection& props_section,
                               std::vector<std::vector<double>>& target,
                               const std::vector<std::vector<double>>& res_source,
                               const std::size_t num_eos_sur,
                               const std::size_t num_eos_res,
                               const std::size_t num_values,
                               const std::optional<double> default_value = std::nullopt)
    {
        const auto* kw = getSinglePropsKeyword<Keyword>(props_section);

        if (!kw) {
            // Missing surface keyword: inherit from reservoir regions.
            if (!res_source.empty()) {
                target.resize(num_eos_sur);
                for (std::size_t i = 0; i < num_eos_sur; ++i) {
                    const std::size_t res_idx = std::min(i, num_eos_res - 1);
                    target[i] = res_source[res_idx];
                }
            }
            return;
        }

        if (res_source.empty()) {
            throw Opm::OpmInputError(
                fmt::format("surface keyword {} is specified, but reservoir keyword {} is not specified",
                            Keyword::keywordName, reservoirKeywordName(Keyword::keywordName)),
                kw->location());
        }

        // Specified surface keyword: require full region coverage if no item default exists.
        validateKeywordRegionCount(*kw, num_eos_sur, "surface EOS regions",
                                   default_value.has_value());

        target.assign(num_eos_sur,
                      std::vector<double>(num_values, default_value.value_or(0.0)));

        parsingKeywordValues<Keyword>(*kw, target, num_values, default_value);
    }

    // Resolve EOS/EOSS from PROPS or RUNSPEC (exclusive), with at most one instance.
    template <typename Keyword>
    const Opm::DeckKeyword*
    resolveEosKeyword(const Opm::PROPSSection&   props_section,
                      const Opm::RUNSPECSection& runspec_section)
    {
        const auto& kw_name    = Keyword::keywordName;
        const bool  has_props   = props_section  .hasKeyword<Keyword>();
        const bool  has_runspec = runspec_section.hasKeyword<Keyword>();

        if (!has_props && !has_runspec) {
            return nullptr;
        }

        if (has_props && has_runspec) {
            throw Opm::OpmInputError(
                fmt::format("{} is specified in both RUNSPEC and PROPS sections", kw_name),
                props_section.get<Keyword>().begin()->location());
        }

        const auto& keywords = has_props
            ? props_section  .get<Keyword>()
            : runspec_section.get<Keyword>();

        if (keywords.size() > 1) {
            throw Opm::OpmInputError(
                fmt::format("there are multiple {} keyword specifications", kw_name),
                keywords.begin()->location());
        }

        return &keywords.back();
    }

    // Parse EOS/EOSS records into pre-sized output.
    // Defaulted EQUATION items are skipped, keeping the caller-provided default.
    template <typename Keyword>
    void parseEosRecords(const Opm::DeckKeyword&                         kw,
                         std::vector<Opm::CompositionalConfig::EOSType>& out,
                         const std::size_t                               max_regions,
                         const std::string_view                          region_description)
    {
        const auto& kw_name = Keyword::keywordName;

        if (kw.size() > max_regions) {
            throw Opm::OpmInputError(
                fmt::format("{} equations of state are specified in keyword {}, "
                            "which is more than the number of {} regions of {}.",
                            kw.size(), kw_name, region_description, max_regions),
                kw.location());
        }

        for (std::size_t i = 0; i < kw.size(); ++i) {
            const auto& item = kw.getRecord(i).template getItem<typename Keyword::EQUATION>();
            if (item.defaultApplied(0)) {
                continue;
            }
            out[i] = Opm::CompositionalConfig::eosTypeFromString(item.getTrimmedString(0));
        }
    }

    void warningForExistingCompKeywords(const Opm::PROPSSection& section)
    {
        using namespace std::string_view_literals;

        const auto keywordCheckers = std::array {
            std::pair {"NCOMPS"sv, section.hasKeyword<Opm::ParserKeywords::NCOMPS>() },
            std::pair {"CNAMES"sv, section.hasKeyword<Opm::ParserKeywords::CNAMES>() },
            std::pair {"EOS"sv,    section.hasKeyword<Opm::ParserKeywords::EOS>() },
            std::pair {"EOSS"sv,   section.hasKeyword<Opm::ParserKeywords::EOSS>() },
            std::pair {"STCOND"sv, section.hasKeyword<Opm::ParserKeywords::STCOND>() },
            std::pair {"PCRIT"sv,  section.hasKeyword<Opm::ParserKeywords::PCRIT>() },
            std::pair {"PCRITS"sv, section.hasKeyword<Opm::ParserKeywords::PCRITS>() },
            std::pair {"TCRIT"sv,  section.hasKeyword<Opm::ParserKeywords::TCRIT>() },
            std::pair {"TCRITS"sv, section.hasKeyword<Opm::ParserKeywords::TCRITS>() },
            std::pair {"VCRIT"sv,  section.hasKeyword<Opm::ParserKeywords::VCRIT>() },
            std::pair {"SSHIFT"sv, section.hasKeyword<Opm::ParserKeywords::SSHIFT>() },
            std::pair {"SSHIFTS"sv, section.hasKeyword<Opm::ParserKeywords::SSHIFTS>() },
            std::pair {"VCRITS"sv, section.hasKeyword<Opm::ParserKeywords::VCRITS>() },
            std::pair {"ZCRIT"sv,  section.hasKeyword<Opm::ParserKeywords::ZCRIT>() },
            std::pair {"ZCRITS"sv, section.hasKeyword<Opm::ParserKeywords::ZCRITS>() },
            std::pair {"MW"sv,     section.hasKeyword<Opm::ParserKeywords::MW>() },
            std::pair {"MWS"sv,    section.hasKeyword<Opm::ParserKeywords::MWS>() },
            std::pair {"ACF"sv,    section.hasKeyword<Opm::ParserKeywords::ACF>() },
            std::pair {"ACFS"sv,   section.hasKeyword<Opm::ParserKeywords::ACFS>() },
            std::pair {"BIC"sv,    section.hasKeyword<Opm::ParserKeywords::BIC>() },
            std::pair {"PRCORR"sv, section.hasKeyword<Opm::ParserKeywords::PRCORR>() },
            std::pair {"BICS"sv,   section.hasKeyword<Opm::ParserKeywords::BICS>() },
        };

        bool any_comp_prop_kw = false;
        std::string msg {" COMPS is not specified, the following keywords related to compositional simulation in PROPS section will be ignored:\n"};

        for (const auto& [kwname, hasKw] : keywordCheckers) {
            if (hasKw) {
                any_comp_prop_kw = true;
                fmt::format_to(std::back_inserter(msg), " {}", kwname);
            }
        }

        if (any_comp_prop_kw) {
            Opm::OpmLog::warning(msg);
        }
    }
}

namespace Opm {

CompositionalConfig::CompositionalConfig(const Deck& deck, const Runspec& runspec) {
    if (!DeckSection::hasPROPS(deck)) return;

    // Return if CO2STORE is active with compositional keywords
    if (deck.hasKeyword("CO2STORE"))
        return;

    const PROPSSection props_section {deck};
    const bool comp_mode_runspec = runspec.compositionalMode(); // TODO: the way to use comp_mode_runspec should be refactored
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
            if (std::size_t(ncomps) != this->num_comps) {
                const std::string msg = fmt::format("NCOMPS is specified with {}, which is different from the number specified in COMPS {}",
                                                    ncomps, this->num_comps);
                throw OpmInputError(msg, kw.location());
            }
        }
    }

    if ( !props_section.hasKeyword<ParserKeywords::CNAMES>() ) {
        throw std::logic_error("CNAMES is not specified for compositional simulation");
    } else {
        comp_names.resize(num_comps);
        const auto& keywords = props_section.get<ParserKeywords::CNAMES>();
        if (keywords.size() > 1) {
            throw OpmInputError("there are multiple CNAMES keyword specification", keywords.begin()->location());
        }
        const auto& kw = keywords.back();
        const auto& item = kw.getRecord(0).getItem<ParserKeywords::CNAMES::data>();
        const auto names_size = item.getData<std::string>().size();
        if (names_size != num_comps) {
            const auto msg = fmt::format("in keyword CNAMES, {} values are specified, which is different from the number of components {}",
                                         names_size, num_comps);
            throw OpmInputError(msg, kw.location());
        }
        for (std::size_t c = 0; c < num_comps; ++c) {
            comp_names[c] = item.getTrimmedString(c);
        }
    }


    if (props_section.hasKeyword<ParserKeywords::STCOND>()) {
        const auto& keywords = props_section.get<ParserKeywords::STCOND>();
        if (keywords.size() > 1) {
            throw OpmInputError("there are multiple STCOND keyword specification", keywords.begin()->location());
        }
        const auto& kw = keywords.back();
        const auto& temp_item = kw.getRecord(0).getItem<ParserKeywords::STCOND::TEMPERATURE>();
        this->standard_temperature = temp_item.getSIDouble(0);
        const auto& pres_item = kw.getRecord(0).getItem<ParserKeywords::STCOND::PRESSURE>();
        this->standard_pressure = pres_item.getSIDouble(0);
    }

    const Tabdims tabdims{deck};
    const std::size_t num_eos_res = tabdims.getNumEosRes();

    // EOS may live in RUNSPEC or PROPS; parse it here.
    eos_types.resize(num_eos_res, EOSType::PR);
    {
        const RUNSPECSection runspec_section{deck};
        if (const auto* kw = resolveEosKeyword<ParserKeywords::EOS>(
                props_section, runspec_section))
        {
            parseEosRecords<ParserKeywords::EOS>(
                *kw, eos_types, num_eos_res,
                "reservoir-condition equation of state");
        }
    }

    // EOSS may be in RUNSPEC or PROPS.
    // If absent, surface EOS follows reservoir EOS per region (clipped by last reservoir region).
    const std::size_t num_eos_sur = tabdims.getNumEosSur();
    {
        const RUNSPECSection runspec_section{deck};
        if (const auto* kw = resolveEosKeyword<ParserKeywords::EOSS>(
                props_section, runspec_section))
        {
            eos_types_surf.assign(num_eos_sur, EOSType::PR);
            parseEosRecords<ParserKeywords::EOSS>(
                *kw, eos_types_surf, num_eos_sur,
                "surface-condition equation of state");
        }
        else {
            // No EOSS: inherit surface EOS from reservoir EOS.
            eos_types_surf.resize(num_eos_sur);
            for (std::size_t i = 0; i < num_eos_sur; ++i) {
                eos_types_surf[i] = eos_types[std::min(i, num_eos_res - 1)];
            }
        }
    }

    // PRCORR promotes PR -> PRCORR in reservoir and surface EOS regions.
    if (props_section.hasKeyword<ParserKeywords::PRCORR>()) {
        const auto& keywords = props_section.get<ParserKeywords::PRCORR>();
        if (keywords.size() > 1) {
            throw OpmInputError("there are multiple PRCORR keyword specifications",
                                keywords.begin()->location());
        }

        auto promote = [](std::vector<EOSType>& types) {
            std::string unaffected;
            for (std::size_t i = 0; i < types.size(); ++i) {
                if (types[i] == EOSType::PR) {
                    types[i] = EOSType::PRCORR;
                } else if (types[i] != EOSType::PRCORR) {
                    if (!unaffected.empty()) {
                        unaffected += ", ";
                    }
                    fmt::format_to(std::back_inserter(unaffected),
                                   "{} ({})", i + 1, eosTypeToString(types[i]));
                }
            }
            return unaffected;
        };

        const std::string unaffected_res = promote(eos_types);
        const std::string unaffected_surf = promote(eos_types_surf);

        const auto& location = keywords.back().location();
        if (!unaffected_res.empty()) {
            const auto msg = fmt::format(
                "PRCORR is ignored for reservoir EOS region(s) {} because their EOS is not PR.",
                unaffected_res);
            OpmLog::info(Opm::Log::fileMessage(location, msg));
        }
        if (!unaffected_surf.empty()) {
            const auto msg = fmt::format(
                "PRCORR is ignored for surface EOS region(s) {} because their EOS is not PR.",
                unaffected_surf);
            OpmLog::info(Opm::Log::fileMessage(location, msg));
        }
    }

    processKeyword<ParserKeywords::MW>(props_section, this->molecular_weights,
                                       num_eos_res, this->num_comps);
    processKeyword<ParserKeywords::ACF>(props_section, this->acentric_factors,
                                        num_eos_res, this->num_comps);
    processKeyword<ParserKeywords::PCRIT>(props_section, this->critical_pressure,
                                          num_eos_res, this->num_comps);
    processKeyword<ParserKeywords::TCRIT>(props_section, this->critical_temperature,
                                          num_eos_res, this->num_comps);
    processKeyword<ParserKeywords::VCRIT>(props_section, this->critical_volume,
                                          num_eos_res, this->num_comps);
    processKeyword<ParserKeywords::SSHIFT>(props_section, this->volume_shifts,
                                           num_eos_res, this->num_comps, 0.);
    processKeyword<ParserKeywords::ZCRIT>(props_section, this->critical_z_factor,
                                          num_eos_res, this->num_comps);

    const std::size_t bic_size = this->num_comps * (this->num_comps - 1) / 2;
    processKeyword<ParserKeywords::BIC>(props_section, this->binary_interaction_coefficient,
                                        num_eos_res, bic_size, 0.);

    // Surface keywords are sized by NMEOSS and handled independently per keyword.
    processSurfaceKeyword<ParserKeywords::MWS>(props_section, this->molecular_weights_surf,
                                               this->molecular_weights,
                                               num_eos_sur, num_eos_res, this->num_comps);
    processSurfaceKeyword<ParserKeywords::ACFS>(props_section, this->acentric_factors_surf,
                                                this->acentric_factors,
                                                num_eos_sur, num_eos_res, this->num_comps);
    processSurfaceKeyword<ParserKeywords::PCRITS>(props_section, this->critical_pressure_surf,
                                                  this->critical_pressure,
                                                  num_eos_sur, num_eos_res, this->num_comps);
    processSurfaceKeyword<ParserKeywords::TCRITS>(props_section, this->critical_temperature_surf,
                                                  this->critical_temperature,
                                                  num_eos_sur, num_eos_res, this->num_comps);
    processSurfaceKeyword<ParserKeywords::VCRITS>(props_section, this->critical_volume_surf,
                                                  this->critical_volume,
                                                  num_eos_sur, num_eos_res, this->num_comps);
    processSurfaceKeyword<ParserKeywords::ZCRITS>(props_section, this->critical_z_factor_surf,
                                                  this->critical_z_factor,
                                                  num_eos_sur, num_eos_res, this->num_comps);
    processSurfaceKeyword<ParserKeywords::SSHIFTS>(props_section, this->volume_shifts_surf,
                                                   this->volume_shifts,
                                                   num_eos_sur, num_eos_res, this->num_comps, 0.);
    processSurfaceKeyword<ParserKeywords::BICS>(props_section, this->binary_interaction_coefficient_surf,
                                                this->binary_interaction_coefficient,
                                                num_eos_sur, num_eos_res, bic_size, 0.);
}

bool CompositionalConfig::operator==(const CompositionalConfig& other) const {
    return this->num_comps == other.num_comps &&
           this->standard_temperature == other.standard_temperature &&
           this->standard_pressure == other.standard_pressure &&
           this->comp_names ==other.comp_names &&
           this->eos_types == other.eos_types &&
           this->molecular_weights == other.molecular_weights &&
           this->acentric_factors == other.acentric_factors &&
           this->critical_pressure == other.critical_pressure &&
           this->critical_temperature == other.critical_temperature &&
           this->critical_volume == other.critical_volume &&
           this->volume_shifts == other.volume_shifts &&
           this->critical_z_factor == other.critical_z_factor &&
           this->binary_interaction_coefficient == other.binary_interaction_coefficient &&
           this->eos_types_surf == other.eos_types_surf &&
           this->molecular_weights_surf == other.molecular_weights_surf &&
           this->acentric_factors_surf == other.acentric_factors_surf &&
           this->critical_pressure_surf == other.critical_pressure_surf &&
           this->critical_temperature_surf == other.critical_temperature_surf &&
           this->critical_volume_surf == other.critical_volume_surf &&
           this->critical_z_factor_surf == other.critical_z_factor_surf &&
           this->volume_shifts_surf == other.volume_shifts_surf &&
           this->binary_interaction_coefficient_surf == other.binary_interaction_coefficient_surf;
}


CompositionalConfig CompositionalConfig::serializationTestObject() {
    CompositionalConfig result;

    result.num_comps = 3;
    result.standard_temperature = 5.;
    result.standard_pressure = 1e5;
    result.comp_names = {"C1", "C10"};
    result.eos_types = {2, EOSType::SRK};
    result.molecular_weights = {2, std::vector<double>(result.num_comps, 16.)};
    result.acentric_factors = {2,  std::vector<double>(result.num_comps, 1.)};
    result.critical_pressure = {2, std::vector<double>(result.num_comps, 2.)};
    result.critical_temperature = {2, std::vector<double>(result.num_comps, 3.)};
    result.critical_volume = {2, std::vector<double>(result.num_comps, 5.)};
    result.volume_shifts = {2, std::vector<double>(result.num_comps, 0.1)};
    result.critical_z_factor = {2, std::vector<double>(result.num_comps, 0.29)};
    result.binary_interaction_coefficient = {2, std::vector<double>(result.num_comps * (result.num_comps - 1) / 2, 6.)};
    result.eos_types_surf = {3, EOSType::PR};
    result.molecular_weights_surf = {3, std::vector<double>(result.num_comps, 17.)};
    result.acentric_factors_surf = {3, std::vector<double>(result.num_comps, 1.1)};
    result.critical_pressure_surf = {3, std::vector<double>(result.num_comps, 2.1)};
    result.critical_temperature_surf = {3, std::vector<double>(result.num_comps, 3.1)};
    result.critical_volume_surf = {3, std::vector<double>(result.num_comps, 5.1)};
    result.critical_z_factor_surf = {3, std::vector<double>(result.num_comps, 0.3)};
    result.volume_shifts_surf = {3, std::vector<double>(result.num_comps, 0.11)};
    result.binary_interaction_coefficient_surf = {3, std::vector<double>(result.num_comps * (result.num_comps - 1) / 2, 6.1)};

    return result;
}

CompositionalConfig::EOSType CompositionalConfig::eosTypeFromString(const std::string& str) {
    if (str == "PR") return EOSType::PR;
    if (str == "PRCORR") return EOSType::PRCORR;
    if (str == "RK") return EOSType::RK;
    if (str == "SRK") return EOSType::SRK;
    if (str == "ZJ") return EOSType::ZJ;
    throw std::invalid_argument("Unknown string for EOSType");
}

std::string CompositionalConfig::eosTypeToString(Opm::CompositionalConfig::EOSType eos) {
    switch (eos) {
        case EOSType::PR: return "PR";
        case EOSType::PRCORR: return "PRCORR";
        case EOSType::RK: return "RK";
        case EOSType::SRK: return "SRK";
        case EOSType::ZJ: return "ZJ";
        default: throw std::invalid_argument("Unknown EOSType");
    }
}

double CompositionalConfig::standardTemperature() const {
    return this->standard_temperature;
}

double CompositionalConfig::standardPressure() const {
    return this->standard_pressure;
}

const std::vector<std::string>& CompositionalConfig::compName() const {
    return this->comp_names;
}

CompositionalConfig::EOSType CompositionalConfig::eosType(std::size_t eos_region) const {
    return this->eos_types[eos_region];
}

const std::vector<double>& CompositionalConfig::molecularWeights(std::size_t eos_region) const {
    return this->molecular_weights[eos_region];
}

const std::vector<double>& CompositionalConfig::acentricFactors(std::size_t eos_region) const {
    return this->acentric_factors[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalPressure(std::size_t eos_region) const {
    return this->critical_pressure[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalTemperature(std::size_t eos_region) const {
    return this->critical_temperature[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalVolume(std::size_t eos_region) const {
    return this->critical_volume[eos_region];
}

const std::vector<double>& CompositionalConfig::volumeShifts(std::size_t eos_region) const {
    return this->volume_shifts[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalZFactor(std::size_t eos_region) const {
    return this->critical_z_factor[eos_region];
}

const std::vector<double>& CompositionalConfig::binaryInteractionCoefficient(std::size_t eos_region) const {
    return this->binary_interaction_coefficient[eos_region];
}

CompositionalConfig::EOSType CompositionalConfig::eosTypeSurf(size_t eos_region) const {
    return this->eos_types_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::molecularWeightsSurf(std::size_t eos_region) const {
    return this->molecular_weights_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::acentricFactorsSurf(size_t eos_region) const {
    return this->acentric_factors_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalPressureSurf(size_t eos_region) const {
    return this->critical_pressure_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalTemperatureSurf(size_t eos_region) const {
    return this->critical_temperature_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalVolumeSurf(size_t eos_region) const {
    return this->critical_volume_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::criticalZFactorSurf(size_t eos_region) const {
    return this->critical_z_factor_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::volumeShiftsSurf(size_t eos_region) const {
    return this->volume_shifts_surf[eos_region];
}

const std::vector<double>& CompositionalConfig::binaryInteractionCoefficientSurf(size_t eos_region) const {
    return this->binary_interaction_coefficient_surf[eos_region];
}

std::size_t CompositionalConfig::numComps() const {
    return this->num_comps;
}

}
