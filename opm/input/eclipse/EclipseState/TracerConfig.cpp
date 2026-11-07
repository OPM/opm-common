/*
  Copyright (C) 2020 Equinor

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

#include <config.h>

#include <opm/input/eclipse/EclipseState/TracerConfig.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/EclipseState/Tables/TracerVdTable.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

Opm::Phase phase_from_string(const std::string& phase_string)
{
    if (phase_string == "WAT") {
        return Opm::Phase::WATER;
    }

    if (phase_string == "OIL") {
        return Opm::Phase::OIL;
    }

    if (phase_string == "GAS") {
        return Opm::Phase::GAS;
    }

    throw std::invalid_argument("Tracer: invalid fluid name " + phase_string);
}

std::vector<double> blockConcentration(const Opm::DeckKeyword& keyword,
                                       const double inv_volume,
                                       Opm::InfoLogger& logger)
{
    auto concentration = keyword.getRecord(0).getItem(0).getData<double>();
    logger(keyword.location().format(
        "Loading tracer concentration from {keyword} in {file} line {line}"));

    std::ranges::transform(concentration, concentration.begin(),
                           [inv_volume](const auto& c) { return c * inv_volume; });

    return concentration;
}

// One table per tracer region
std::vector<Opm::TracerVdTable> depthTables(const Opm::DeckKeyword& keyword,
                                            const double inv_volume,
                                            Opm::InfoLogger& logger)
{
    logger(keyword.location().format(
        "Loading tracer concentration from {keyword} in {file} line {line}"));

    auto tables = std::vector<Opm::TracerVdTable>{};
    for (std::size_t table = 0; table < keyword.size(); ++table) {
        tables.emplace_back(keyword.getRecord(table).getItem(0), inv_volume,
                            static_cast<int>(table));
    }

    return tables;
}

} // Anonymous namespace

namespace Opm {

TracerConfig::TracerConfig([[maybe_unused]] const UnitSystem& unit_system,
                           const Deck&                        deck,
                           const bool                         compositional)
{
    using TR = ParserKeywords::TRACER;

    this->supportsSolutionGasTracer_ = deck.hasKeyword<ParserKeywords::DISGAS>();
    this->supportsVaporisedOilTracer_ = deck.hasKeyword<ParserKeywords::VAPOIL>();

    if (! deck.hasKeyword<TR>()) {
        return;
    }

    const auto& keyword = deck.get<TR>().back();
    OpmLog::info(
        keyword.location().format("\nInitializing tracers from {keyword} in {file} line {line}"));

    // A compositional run associates a tracer with a component and names its
    // concentration keywords after the tracer, e.g. TVDPT1. A black-oil run
    // associates it with a phase and has free and solution keywords, e.g.
    // TVDPFT1 and TVDPST1.
    const auto components = (compositional && deck.hasKeyword<ParserKeywords::CNAMES>())
        ? deck.get<ParserKeywords::CNAMES>().back().getStringData()
        : std::vector<std::string>{};

    InfoLogger logger("Tracer tables", 3);
    for (const auto& record : keyword) {
        const auto& name = record.getItem<TR::NAME>().get<std::string>(0);
        const auto& fluid = record.getItem<TR::FLUID>().get<std::string>(0);

        auto phase = Phase::OIL;
        auto component = std::optional<std::string>{};
        if (compositional) {
            if (fluid == "WATER") {
                phase = Phase::WATER;
            }
            else if (std::ranges::find(components, fluid) == components.end()) {
                throw OpmInputError {
                    fmt::format("Tracer {} is associated with '{}', which is neither "
                                "a component in CNAMES nor WATER.", name, fluid),
                    keyword.location()
                };
            }
            component = fluid;
        }
        else {
            phase = phase_from_string(fluid);
        }
        const auto free_suffix = compositional ? name : "F" + name;

        double inv_volume = 1.0; // Default scaling (vol/vol).
        auto unit_string = std::string {};
        if (!record.getItem<TR::UNIT>().defaultApplied(0)) {
            // Non-default tracer units can be "anything".  For now we just
            // keep it as a "tag", and make no attempt to relate it to
            // physical quantities recognized by the simulator.  We also
            // leave the denominator of the concentration fraction as is,
            // and do not convert it to simulator-internal volume.  Thus
            // concentrations will be _wrong_ in simulator units but
            // _correct_ in io-units during computations.  Since the passive
            // tracers currently considered have no physical impact, this
            // should be ok. TODO: However, for tracers where correct
            // quantity matters (e.g. chemical reactions) proper scaling
            // also for non-default tracer units will be needed.
            //
            // Correct unit names for non-default tracer units are generated
            // in the method 'get_unit_string' below, and is output by
            // "hijacking" the normal unit-name generating procedure.

            unit_string = uppercase(record.getItem<TR::UNIT>()
                                    .getTrimmedString(0));

            logger(keyword.location().format("Non-default tracer unit [" + unit_string
                                             + "] from {keyword} in {file} line {line}"));

            // Towards "proper" scaling:
            // if (phase == Phase::GAS)
            //    inv_volume =
            //    unit_system.getDimension(UnitSystem::measure::gas_surface_volume).getSIScaling();
            // else
            //    inv_volume =
            //    unit_system.getDimension(UnitSystem::measure::liquid_surface_volume).getSIScaling();
        }

        auto tracer = TracerEntry{name, unit_string, phase};
        tracer.component = component;

        if (const auto tracer_field = "TBLK" + free_suffix; deck.hasKeyword(tracer_field)) {
            tracer.free_concentration
                = blockConcentration(deck[tracer_field].back(), inv_volume, logger);

            if (const auto tracer_field_solution = "TBLKS" + name;
                !compositional && deck.hasKeyword(tracer_field_solution))
            {
                tracer.solution_concentration
                    = blockConcentration(deck[tracer_field_solution].back(), inv_volume, logger);
            }
        }
        else if (const auto tracer_table = "TVDP" + free_suffix; deck.hasKeyword(tracer_table)) {
            tracer.free_tvdp = depthTables(deck[tracer_table].back(), inv_volume, logger);

            if (const auto tracer_table_solution = "TVDPS" + name;
                !compositional && deck.hasKeyword(tracer_table_solution))
            {
                tracer.solution_tvdp
                    = depthTables(deck[tracer_table_solution].back(), inv_volume, logger);
            }
        }

        this->tracers.push_back(std::move(tracer));
    }
}

TracerConfig TracerConfig::serializationTestObject()
{
    TracerConfig result;

    result.tracers = {{"test", "", Phase::OIL, {1.0}}};
    result.tracers.front().component = "C1";
    result.supportsSolutionGasTracer_ = true;
    result.supportsVaporisedOilTracer_ = true;

    return result;
}

std::size_t TracerConfig::size() const
{
    return this->tracers.size();
}

bool TracerConfig::empty() const
{
    return this->tracers.empty();
}

const TracerConfig::TracerEntry& TracerConfig::operator[](std::size_t index) const
{
    return this->tracers.at(index);
}

const TracerConfig::TracerEntry& TracerConfig::operator[](const std::string& name) const
{
    const auto iter = std::ranges::find_if(this->tracers,
                                           [&name](const TracerEntry& tracer)
                                           { return tracer.name == name; });

    if (iter == this->tracers.end()) {
        throw std::logic_error(fmt::format("No such tracer: {}", name));
    }

    return *iter;
}

bool TracerConfig::operator==(const TracerConfig& other) const
{
    return (this->tracers == other.tracers)
        && (this->supportsSolutionGasTracer_ == other.supportsSolutionGasTracer_)
        && (this->supportsVaporisedOilTracer_ == other.supportsVaporisedOilTracer_)
        ;
}

std::string TracerConfig::get_unit_string(const UnitSystem& unit_system,
                                         const std::string& tracer_kw) const
{
    if (tracer_kw.length() > 4 ) {
        // Check two tracer names strings to allow for tracers starting with S or F
        std::string tracer_name = tracer_kw.substr(4);
        std::string tracer_name2 = tracer_kw.substr(5);
        for (const auto& tracer : tracers) {
            if (tracer.name == tracer_name ||
                (tracer.name == tracer_name2 && (tracer_kw[4] == 'F' || tracer_kw[4] == 'S'))) {
                std::string unit_string(tracer.unit_string);
                if (tracer.unit_string != "") {
                    if (tracer_kw[3] == 'R') {
                       std::string rateName = unit_system.name(Opm::UnitSystem::measure::rate);
                       std::size_t found = rateName.find('/');
                       unit_string += rateName.substr(found);
                    }
                    else if (tracer_kw[3] == 'T') {
                    }
                    else if (tracer_kw[3] == 'C') {
                        unit_string += "/";
                        if (tracer.phase == Phase::GAS )
                            unit_string += unit_system.name(Opm::UnitSystem::measure::gas_surface_volume);
                        else /* OIL or WAT */
                            unit_string += unit_system.name(Opm::UnitSystem::measure::liquid_surface_volume);
                    }
                    else {
                        throw std::runtime_error("Tracer summary kw not recognized: " + tracer_kw);
                    }
                }
                return unit_string;
            }
        }
    }
    return std::string("");
}

} // namespace Opm
