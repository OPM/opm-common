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

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/EndpointScaling.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string_view relpermValueMode(bool enabled, bool threePoint)
{
    if (!enabled) {
        return "None";
    }
    return threePoint ? "Three Point (Residual + Maximum)" : "Maximum Ratio";
}

std::string_view pcValueMode(const Opm::EclEpsConfig& config)
{
    if (config.enableLeverettScaling()) {
        return "Leverett J-function";
    }
    return config.enablePcScaling() ? "Maximum Pressure Ratio" : "None";
}

std::string endpointProperties(const Opm::FieldPropsManager& properties,
                               Opm::EclTwoPhaseSystemType type,
                               std::string_view prefix)
{
    const auto present = [&properties, prefix](std::initializer_list<std::string_view> names)
    {
        std::string result;
        for (const auto name : names) {
            const auto keyword = std::string(prefix) + std::string(name);
            if (properties.has_double(keyword)) {
                if (!result.empty()) {
                    result += ", ";
                }
                result += keyword;
            }
        }
        return result.empty() ? std::string{"None"} : result;
    };

    if (type == Opm::EclTwoPhaseSystemType::OilWater) {
        return present({"SWL", "SWCR", "SOWCR", "SWU", "PCW",
                        "KRW", "KRWR", "KRO", "KRORW"});
    }
    if (type == Opm::EclTwoPhaseSystemType::GasOil) {
        return present({"SGL", "SGCR", "SOGCR", "SGU", "PCG",
                        "KRO", "KRORG", "KRG", "KRGR"});
    }
    return present({"SWL", "SGL", "SWCR", "SGCR", "SWU", "SGU",
                    "PCG", "KRW", "KRWR", "KRG", "KRGR"});
}

struct TableRow {
    std::string label;
    std::string drainage;
    std::string imbibition;
};

void printTable(const std::vector<TableRow>& rows, bool hysteresis)
{
    std::size_t labelWidth = 0;
    std::size_t drainageWidth = std::string_view{"Drainage"}.size();
    std::size_t imbibitionWidth = std::string_view{"Imbibition"}.size();
    for (const auto& row : rows) {
        labelWidth = std::max(labelWidth, row.label.size());
        drainageWidth = std::max(drainageWidth, row.drainage.size());
        imbibitionWidth = std::max(imbibitionWidth, row.imbibition.size());
    }

    const auto printCell = [](std::string_view value, std::size_t width)
    {
        std::cout << ' ' << value << std::string(width - value.size(), ' ') << " |";
    };
    const auto printRow = [&](std::string_view label,
                              std::string_view drainage,
                              std::string_view imbibition)
    {
        std::cout << '|';
        printCell(label, labelWidth);
        printCell(drainage, drainageWidth);
        if (hysteresis) {
            printCell(imbibition, imbibitionWidth);
        }
        std::cout << '\n';
    };

    printRow("", "Drainage", "Imbibition");
    std::cout << '|' << std::string(labelWidth + 2, '-') << '|'
              << std::string(drainageWidth + 2, '-') << '|';
    if (hysteresis) {
        std::cout << std::string(imbibitionWidth + 2, '-') << '|';
    }
    std::cout << '\n';
    for (const auto& row : rows) {
        printRow(row.label, row.drainage, row.imbibition);
    }
}

void printPhasePair(const Opm::EclipseState& state,
                    Opm::EclTwoPhaseSystemType type,
                    std::string_view name,
                    bool hysteresis)
{
    Opm::EclEpsConfig config;
    config.initFromState(state, type);

    std::string_view nonWettingKr;
    std::string_view wettingKr;
    std::string_view pc;
    if (type == Opm::EclTwoPhaseSystemType::OilWater) {
        nonWettingKr = "Kro";
        wettingKr = "Krw";
        pc = "Pcow";
    }
    else if (type == Opm::EclTwoPhaseSystemType::GasOil) {
        nonWettingKr = "Krg";
        wettingKr = "Kro";
        pc = "Pcgo";
    }
    else {
        nonWettingKr = "Krg";
        wettingKr = "Krw";
        pc = "Pcgw";
    }

    std::cout << "\n### " << name << "\n\n";
    std::vector<TableRow> rows;

    const auto sharedRow = [&rows](std::string_view label, std::string_view value)
    {
        rows.push_back({std::string(label), std::string(value), std::string(value)});
    };

    rows.push_back({"Region", "SATNUM", "IMBNUM"});
    sharedRow("Horizontal Scaling (Kr)",
              config.enableSatScaling()
                  ? (config.enableThreePointKrSatScaling() ? "Three Point" : "Two Point")
                  : "None");
    sharedRow("Horizontal Scaling (Pc)", config.enableSatScaling() ? "Two Point" : "None");
    sharedRow(std::string("Vertical Scaling (") + std::string(nonWettingKr) + ')',
              relpermValueMode(config.enableKrnScaling(), config.enableThreePointKrnScaling()));
    sharedRow(std::string("Vertical Scaling (") + std::string(wettingKr) + ')',
              relpermValueMode(config.enableKrwScaling(), config.enableThreePointKrwScaling()));
    sharedRow(std::string("Vertical Scaling (") + std::string(pc) + ')', pcValueMode(config));
    rows.push_back({"End Points", endpointProperties(state.fieldProps(), type, ""),
                    hysteresis ? endpointProperties(state.fieldProps(), type, "I") : std::string{}});
    if (type != Opm::EclTwoPhaseSystemType::GasOil &&
        state.fieldProps().has_double("SWATINIT")) {
        rows.push_back({std::string(pc) + " Initialisation", "SWATINIT", ""});
    }
    printTable(rows, hysteresis);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: inspect-eps <deck file>\n";
        return EXIT_FAILURE;
    }

    try {
        Opm::ParseContext parseContext { Opm::InputErrorAction::WARN };
        parseContext.update(Opm::ParseContext::PARSE_MISSING_INCLUDE,
                            Opm::InputErrorAction::THROW_EXCEPTION);
        Opm::ErrorGuard errors;
        const auto deck = Opm::Parser{}.parseFile(argv[1], parseContext, errors);
        errors.dump();
        const Opm::EndpointScaling scaling { deck };
        const Opm::Runspec runspec { deck };
        const bool hysteresis = runspec.hysterPar().active();

        std::cout << "End point scaling: " << (scaling ? "enabled" : "disabled") << '\n';
        if (scaling) {
            std::cout << "Relative permeability saturation scaling: "
                      << (scaling.threepoint() ? "three-point" : "two-point") << '\n'
                      << "Direction: "
                      << (scaling.directional() ? "directional" : "non-directional") << '\n'
                      << "Reversibility: "
                      << (scaling.reversible() ? "reversible" : "irreversible") << '\n';
        }
        if (hysteresis) {
            std::cout << "Hysteresis: active\n"
                      << "Scaling configuration: shared by drainage and imbibition\n";
        }

        if (scaling || hysteresis) {
            const auto& phases = runspec.phases();
            const bool oil = phases.active(Opm::Phase::OIL);
            const bool gas = phases.active(Opm::Phase::GAS);
            const bool water = phases.active(Opm::Phase::WATER);

            if ((oil && water) || (gas && oil) || (gas && water)) {
                const Opm::EclipseState state { deck };
                if (gas && oil) {
                    printPhasePair(state, Opm::EclTwoPhaseSystemType::GasOil,
                                   "Gas-Oil", hysteresis);
                }
                if (oil && water) {
                    printPhasePair(state, Opm::EclTwoPhaseSystemType::OilWater,
                                   "Oil-Water", hysteresis);
                }
                if (gas && water && !oil) {
                    printPhasePair(state, Opm::EclTwoPhaseSystemType::GasWater,
                                   "Gas-Water", hysteresis);
                }
            }
        }
    }
    catch (const std::exception& error) {
        std::cerr << "inspect-eps: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
