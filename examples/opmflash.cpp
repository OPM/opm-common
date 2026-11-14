/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*
  opmflash: flash a compositional state from the command line.

  Runs the isothermal (P-T) or the isenthalpic (P-H) flash with
  GenericOilGasWaterFluidSystem, PTFlash and PHFlash, on a mixture of the
  built-in components of this library or on the fluid of a compositional deck
  (--deck). A temperature (--t) asks for the isothermal flash, an enthalpy
  (--h) for the isenthalpic one. For quick checks and for curves (--sweep,
  --input, --format csv) without a simulator run.

    opmflash --p 50 --t 300 --components C1,C10 --z 0.5,0.5 --kij C1:C10=0.0411
    opmflash --p 50 --h -25000 --components C1,C10 --z 0.5,0.5
    opmflash --p 50 --components C1,C10 --z 0.5,0.5 --sweep h=-30000:5000:1000 --format csv
    opmflash --deck CASE.DATA --p 100 --t 350 --z 0.5,0.3,0.2
    opmflash --list-components

  Units: --p bar, --t K, --h J/mol (molar enthalpy, zero for the ideal gas at
  298.15 K unless --tref gives another temperature), with or without --deck.
*/

#include "config.h"

#include <opm/common/Exceptions.hpp>
#include <opm/material/Constants.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/components/H2.hpp>
#include <opm/material/components/H2O.hpp>
#include <opm/material/components/N2.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/constraintsolvers/MixtureEnthalpy.hpp>
#include <opm/material/constraintsolvers/PHFlash.hpp>
#include <opm/material/constraintsolvers/PTFlash.hpp>
#include <opm/material/constraintsolvers/PTFlashMethod.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidsystems/GenericOilGasWaterFluidSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <dune/common/fmatrix.hh>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

using EOSType = Opm::CompositionalConfig::EOSType;
using HeatCapacity = Opm::ComponentCp<double>;

// A component of the flashed mixture: the equation-of-state parameters in the
// units of the fluid system (molar mass kg/mol as from a deck, critical volume
// m3/kmol) and,
// when known, the ideal-gas heat-capacity polynomial that the enthalpies need.
struct ComponentEntry {
    std::string name;
    std::vector<std::string> aliases;     // other accepted names, upper case
    double molarMass;                     // [kg/mol]
    double criticalTemperature;
    double criticalPressure;
    double criticalVolume;                // [m3/kmol]
    double acentricFactor;
    double volumeShift = 0.0;
    std::optional<double> omegaA;         // unset: the constant of the EOS
    std::optional<double> omegaB;
    std::optional<HeatCapacity> heatCapacity;
    std::string heatCapacitySource;       // built-in, --cp or --cp-file; empty without one
};

template <class Comp>
ComponentEntry makeEntry(std::vector<std::string> aliases)
{
    return {std::string(Comp::name()), std::move(aliases), Comp::molarMass(),
            Comp::criticalTemperature(), Comp::criticalPressure(), Comp::criticalVolume(),
            Comp::acentricFactor(), 0.0, std::nullopt, std::nullopt,
            Comp::idealGasHeatCapacityPolynomial(), "built-in"};
}

// The built-in components, from the component classes of this library. Water
// is flashed as a plain Peng-Robinson component (no association term): good
// enough for quick checks, not an aqueous-phase model.
const std::vector<ComponentEntry>& componentDatabase()
{
    static const std::vector<ComponentEntry> db{
        makeEntry<Opm::C1<double>>({"CH4", "METHANE"}),
        makeEntry<Opm::C10<double>>({"NC10", "DECANE", "N-DECANE"}),
        makeEntry<Opm::SimpleCO2<double>>({"CARBONDIOXIDE"}),
        makeEntry<Opm::N2<double>>({"NITROGEN"}),
        makeEntry<Opm::H2O<double>>({"WATER"}),
        makeEntry<Opm::H2<double>>({"HYDROGEN"}),
    };
    return db;
}

std::string upper(std::string_view s)
{
    std::string out(s);
    std::ranges::transform(out, out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

std::string_view trim(std::string_view s)
{
    const auto space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && space(s.front())) { s.remove_prefix(1); }
    while (!s.empty() && space(s.back())) { s.remove_suffix(1); }
    return s;
}

// A number with six significant digits, as printf's %g.
std::string num(double v)
{
    std::ostringstream os;
    os << std::setprecision(6) << v;
    return os.str();
}

// Whether name, in any case, is the component's name or one of its aliases.
bool isNamed(const ComponentEntry& entry, std::string_view name)
{
    const std::string key = upper(name);
    return upper(entry.name) == key || std::ranges::find(entry.aliases, key) != entry.aliases.end();
}

// The built-in component that name denotes, by its name or an alias.
const ComponentEntry* findBuiltin(std::string_view name)
{
    const auto& db = componentDatabase();
    const auto it = std::ranges::find_if(db, [name](const ComponentEntry& e) {
        return isNamed(e, name);
    });
    return it != db.end() ? &*it : nullptr;
}

// ---- command line ----------------------------------------------------------
enum class Action { Run, Help, ListComponents, Selftest };
enum class Spec { PT, PH };
enum class Format { Table, Csv };
enum class Variable { P, T, H };

struct Sweep {
    Variable variable;
    char letter;                          // p | t | h, for messages
    double start, stop, step;             // [bar], [K] or [J/mol]
};

struct KijEntry {
    std::string first, second;            // as given
    double value;
};

struct CpEntry {
    std::string name;                     // as given
    HeatCapacity heatCapacity;
    std::string source;                   // --cp or --cp-file
};

struct Cli {
    std::set<std::string, std::less<>> given;   // the options on the command line

    // The state.
    std::optional<double> p;              // [bar]
    std::optional<double> t;              // [K]
    std::optional<double> h;              // [J/mol]
    std::optional<Sweep> sweep;
    std::optional<std::string> input;     // a file of states

    // The fluid.
    std::optional<std::string> deck;
    std::optional<std::vector<std::string>> components;
    std::optional<std::vector<double>> z;
    int eosRegion = 1;
    std::vector<KijEntry> kij;
    std::vector<CpEntry> cp;              // from --cp and --cp-file
    std::optional<EOSType> eos;           // PR, or the deck's

    // The enthalpies and the isenthalpic flash.
    std::string enthalpyModelName = "eos_departure";
    Opm::EnthalpyModel enthalpyModel = Opm::EnthalpyModel::eos_departure;
    std::optional<double> tRef;           // [K]; 298.15 K unless given
    std::optional<double> tMin, tMax;     // [K]; the heat-capacity fit range unless given
    double temperatureTolerance = 1e-6;   // [K]
    double enthalpyTolerance = 1.0;       // [J/mol]
    int maxIterations = 100;

    // The isothermal flash, with the option names and defaults of flow_comp.
    Opm::PTFlashMethod twoPhaseMethod = Opm::PTFlashMethod::SsiNewton;
    double flashTolerance = 1e-8;
    int flashVerbosity = 0;

    Format format = Format::Table;

    bool has(std::string_view option) const { return given.contains(option); }
};

struct Command {
    Action action = Action::Run;
    Cli cli;
};

std::vector<std::string> split(std::string_view s, char separator)
{
    std::vector<std::string> out;
    for (const auto part : std::views::split(s, separator)) {
        out.emplace_back(part.begin(), part.end());
    }
    return out;
}

// The whole of s as a number; the error names the option.
template <class Number>
Number toNumber(std::string_view option, std::string_view s)
{
    Number value{};
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (s.empty() || ec != std::errc{} || end != s.data() + s.size()) {
        throw std::runtime_error(std::string(option) + " expects a number, got '" +
                                 std::string(s) + "'");
    }
    return value;
}

double toDouble(std::string_view option, std::string_view s)
{
    return toNumber<double>(option, s);
}

int toInt(std::string_view option, std::string_view s)
{
    return toNumber<int>(option, s);
}

std::vector<double> toDoubles(std::string_view option, std::string_view s)
{
    std::vector<double> values;
    for (const auto& item : split(s, ',')) {
        values.push_back(toDouble(option, item));
    }
    return values;
}

Format formatFromName(std::string_view s)
{
    if (s == "table") { return Format::Table; }
    if (s == "csv") { return Format::Csv; }
    throw std::runtime_error("--format expects table or csv, got '" + std::string(s) + "'");
}

EOSType eosFromName(std::string_view s)
{
    try {
        return Opm::CompositionalConfig::eosTypeFromString(std::string(s));
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error("--eos expects PR, PRCORR, SRK, ZJ or RK, got '" +
                                 std::string(s) + "'");
    }
}

Opm::PTFlashMethod methodFromName(std::string_view s)
{
    try {
        return Opm::ptFlashMethodFromString(s);
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error("--flash-two-phase-method expects ssi, newton or ssi+newton, "
                                 "got '" + std::string(s) + "'");
    }
}

Opm::EnthalpyModel enthalpyModelFromName(std::string_view s)
{
    if (s == "caloric") { return Opm::EnthalpyModel::caloric; }
    if (s == "eos_departure") { return Opm::EnthalpyModel::eos_departure; }
    throw std::runtime_error("--enthalpy-model expects caloric or eos_departure, got '" +
                             std::string(s) + "'");
}

Sweep parseSweep(std::string_view s)
{
    // var=start:stop:step
    const auto fields = s.size() > 2 && s[1] == '=' ? split(s.substr(2), ':')
                                                     : std::vector<std::string>{};
    if (fields.size() != 3 || (s[0] != 'p' && s[0] != 't' && s[0] != 'h')) {
        throw std::runtime_error("--sweep expects p|t|h=start:stop:step, got '" +
                                 std::string(s) + "'");
    }
    const auto variable = s[0] == 'p' ? Variable::P : s[0] == 't' ? Variable::T : Variable::H;
    return {variable, s[0], toDouble("--sweep", fields[0]), toDouble("--sweep", fields[1]),
            toDouble("--sweep", fields[2])};
}

// The points start, start + step, ..., stop of a sweep. They are counted once,
// so a fractional step does not miss stop by rounding.
std::vector<double> sweepPoints(const Sweep& sweep)
{
    if ((sweep.stop - sweep.start) * sweep.step <= 0.0) {
        throw std::runtime_error("--sweep step does not advance from start to stop");
    }
    const auto n = static_cast<long>(std::floor((sweep.stop - sweep.start) / sweep.step + 1e-9));
    std::vector<double> points;
    for (long i = 0; i <= n; ++i) {
        points.push_back(sweep.start + i * sweep.step);
    }
    return points;
}

std::vector<KijEntry> parseKij(std::string_view s)
{
    // A:B=value[,C:D=value...]
    std::vector<KijEntry> table;
    for (const auto& item : split(s, ',')) {
        const auto colon = item.find(':');
        const auto eq = item.find('=');
        if (colon == std::string::npos || eq == std::string::npos || colon > eq) {
            throw std::runtime_error("--kij expects A:B=value[,C:D=value...], got '" + item + "'");
        }
        table.push_back({item.substr(0, colon), item.substr(colon + 1, eq - colon - 1),
                         toDouble("--kij", item.substr(eq + 1))});
    }
    return table;
}

// A heat capacity from its six numbers c0, c1, c2, c3, tmin, tmax.
CpEntry makeCpEntry(const std::string& where, const std::string& name,
                    const std::vector<std::string>& fields, const std::string& source)
{
    const HeatCapacity cp{toDouble(where, fields[0]), toDouble(where, fields[1]),
                          toDouble(where, fields[2]), toDouble(where, fields[3]),
                          toDouble(where, fields[4]), toDouble(where, fields[5])};
    if (!(cp.minTemperature < cp.maxTemperature)) {
        throw std::runtime_error(where + ": the fit range of " + name + " is empty");
    }
    return {name, cp, source};
}

std::vector<CpEntry> parseCp(std::string_view s)
{
    // NAME=c0:c1:c2:c3:tmin:tmax[,NAME=...]
    std::vector<CpEntry> table;
    for (const auto& item : split(s, ',')) {
        const auto eq = item.find('=');
        const auto fields = eq != std::string::npos ? split(std::string_view(item).substr(eq + 1), ':')
                                                    : std::vector<std::string>{};
        if (eq == 0 || fields.size() != 6) {
            throw std::runtime_error("--cp expects NAME=c0:c1:c2:c3:tmin:tmax[,NAME=...], got '" +
                                     item + "'");
        }
        table.push_back(makeCpEntry("--cp", item.substr(0, eq), fields, "--cp"));
    }
    return table;
}

// The non-blank lines of a text file, without '#' comments, with their numbers.
std::vector<std::pair<int, std::string>> contentLines(std::istream& in)
{
    std::vector<std::pair<int, std::string>> lines;
    std::string line;
    for (int number = 1; std::getline(in, line); ++number) {
        const auto content = trim(std::string_view(line).substr(0, line.find('#')));
        if (!content.empty()) {
            lines.emplace_back(number, std::string(content));
        }
    }
    return lines;
}

// --cp-file: one component per line, NAME c0 c1 c2 c3 tmin tmax, separated by
// blanks or commas.
std::vector<CpEntry> readCpFile(std::istream& in, const std::string& name)
{
    std::vector<CpEntry> table;
    for (auto [number, line] : contentLines(in)) {
        std::ranges::replace(line, ',', ' ');
        std::istringstream fields(line);
        const std::vector<std::string> words{std::istream_iterator<std::string>(fields), {}};
        const std::string where = "--cp-file " + name + " line " + std::to_string(number);
        if (words.size() != 7) {
            throw std::runtime_error(where + ": expects NAME c0 c1 c2 c3 tmin tmax");
        }
        table.push_back(makeCpEntry(where, words[0], {words.begin() + 1, words.end()}, "--cp-file"));
    }
    return table;
}

// --input: a header line naming the columns, then one state per line, comma
// separated. The columns are p [bar], and T [K] or h [J/mol], and
// optionally z_<component> for every component.
struct StateFile {
    struct Row {
        int line;
        double p, value;                  // value: T or h
        std::vector<double> z;            // in the order of zNames
    };
    Variable variable;                    // T or H
    std::vector<std::string> zNames;      // the components of the z_ columns, as given
    std::vector<Row> rows;
};

StateFile readStateFile(std::istream& in, const std::string& name)
{
    const auto lines = contentLines(in);
    if (lines.empty()) {
        throw std::runtime_error("--input " + name + " has no header line");
    }
    StateFile states;
    std::optional<std::size_t> pColumn, valueColumn;
    std::vector<std::size_t> zColumns;
    const auto header = split(lines.front().second, ',');
    const std::string where = "--input " + name + " line " + std::to_string(lines.front().first);
    for (std::size_t c = 0; c < header.size(); ++c) {
        const std::string column(trim(header[c]));
        const std::string key = upper(column);
        if (key == "P" && !pColumn) {
            pColumn = c;
        }
        else if ((key == "T" || key == "H") && !valueColumn) {
            valueColumn = c;
            states.variable = key == "T" ? Variable::T : Variable::H;
        }
        else if (key.starts_with("Z_") && key.size() > 2) {
            zColumns.push_back(c);
            states.zNames.push_back(column.substr(2));
        }
        else {
            throw std::runtime_error(where + ": unexpected column '" + column +
                                     "'; the columns are p, T or h, and z_<component>");
        }
    }
    if (!pColumn || !valueColumn) {
        throw std::runtime_error(where + ": the header names the columns p, and T or h");
    }
    for (const auto& [number, line] : lines | std::views::drop(1)) {
        const auto cells = split(line, ',');
        const std::string at = "--input " + name + " line " + std::to_string(number);
        if (cells.size() != header.size()) {
            throw std::runtime_error(at + ": " + std::to_string(cells.size()) + " fields for " +
                                     std::to_string(header.size()) + " columns");
        }
        StateFile::Row row{number, toDouble(at, trim(cells[*pColumn])),
                           toDouble(at, trim(cells[*valueColumn])), {}};
        for (const auto c : zColumns) {
            row.z.push_back(toDouble(at, trim(cells[c])));
        }
        states.rows.push_back(std::move(row));
    }
    if (states.rows.empty()) {
        throw std::runtime_error("--input " + name + " has no states");
    }
    return states;
}

void listComponents()
{
    std::size_t width = std::string_view("aliases").size();
    std::vector<std::string> aliases;
    for (const auto& e : componentDatabase()) {
        std::string joined;
        for (const auto& alias : e.aliases) {
            joined += (joined.empty() ? "" : ", ") + alias;
        }
        width = std::max(width, joined.size());
        aliases.push_back(std::move(joined));
    }
    std::cout <<
        "Built-in components (parameters and ideal-gas heat capacity from the\n"
        "component classes of this library; the heat-capacity polynomial is valid\n"
        "inside its fit range):\n\n"
        "  name   " << std::left << std::setw(static_cast<int>(width) + 2) << "aliases"
              << std::right << "MW[g/mol]    Tc[K]  Pc[bar]  Vc[m3/kmol]   omega  cp fit[K]\n";
    std::cout << std::fixed;
    for (std::size_t i = 0; i < componentDatabase().size(); ++i) {
        const auto& e = componentDatabase()[i];
        std::cout << "  " << std::left << std::setw(7) << e.name
                  << std::setw(static_cast<int>(width) + 2) << aliases[i] << std::right
                  << std::setw(9) << std::setprecision(3) << e.molarMass * 1e3
                  << std::setw(9) << std::setprecision(2) << e.criticalTemperature
                  << std::setw(9) << e.criticalPressure / 1e5
                  << std::setw(13) << std::setprecision(4) << e.criticalVolume
                  << std::setw(8) << std::setprecision(3) << e.acentricFactor
                  << std::setprecision(0) << "  " << e.heatCapacity->minTemperature
                  << '-' << e.heatCapacity->maxTemperature << '\n';
    }
    std::cout <<
        "\nBinary interaction coefficients are zero unless given with --kij,\n"
        "e.g. --kij C1:C10=0.0411,CO2:C1=0.1.\n"
        "Select with --components, e.g. --components C1,C10 or --components CO2,C1,C10\n"
        "(2 to 10 components, each once; --z must match). Water is a plain\n"
        "Peng-Robinson component here (no association term). Other fluids come\n"
        "from a deck with --deck.\n";
}

void printUsage()
{
    std::cout <<
        "opmflash: flash a compositional state from the command line\n"
        "\n"
        "Runs the isothermal (P-T) or the isenthalpic (P-H) flash on a mixture\n"
        "with the fluid system and the flash solvers of the compositional path.\n"
        "A temperature asks for the isothermal flash, an enthalpy for the\n"
        "isenthalpic one.\n"
        "\n"
        "usage:\n"
        "  opmflash --p <p> --t <T>     --components <c,c[,...]> --z <z,z[,...]> [options]\n"
        "  opmflash --p <p> --h <J/mol> --components <c,c[,...]> --z <z,z[,...]> [options]\n"
        "  opmflash --p <p> --t <T> | --h <J/mol> --deck <file> --z <z,z[,...]> [options]\n"
        "  opmflash --input <file> --components ... | --deck ... [options]\n"
        "  opmflash --list-components\n"
        "\n"
        "Options take their value as '--p 50' or '--p=50'.\n"
        "\n"
        "the state:\n"
        "  --p               pressure [bar]                          [required]\n"
        "  --t               temperature [K]: the isothermal flash, which reports the\n"
        "                    phase split, the phase properties and the molar\n"
        "                    enthalpy with both enthalpy models\n"
        "  --h               molar enthalpy [J/mol]: the isenthalpic flash, which\n"
        "                    finds the temperature with this enthalpy, then\n"
        "                    reports the same\n"
        "  --sweep           p|t|h=start:stop:step instead of --p, --t or --h\n"
        "  --input           a CSV file of states, one per line, after a header\n"
        "                    naming the columns: p, T or h, and optionally\n"
        "                    z_<component> for every component\n"
        "\n"
        "the fluid:\n"
        "  --components      built-in components, comma list, each once;\n"
        "                    see --list-components\n"
        "  --deck            the fluid of a compositional deck: the components,\n"
        "                    their equation-of-state parameters, interaction\n"
        "                    coefficients, equation of state and viscosity\n"
        "                    coefficients; excludes --components. The state\n"
        "                    still comes from the command line\n"
        "  --z               feed mole fractions in [0, 1]; a sum within 1e-3\n"
        "                    of 1 is normalised\n"
        "  --eos-region      with --deck: the equation-of-state region   [1]\n"
        "  --kij             A:B=value[,C:D=value...] binary interaction\n"
        "                    coefficients; others zero, or the deck's\n"
        "  --cp              NAME=c0:c1:c2:c3:tmin:tmax[,...] ideal-gas heat\n"
        "                    capacity c0 + c1 T + c2 T^2 + c3 T^3 [J/(mol K)],\n"
        "                    valid in [tmin, tmax] K\n"
        "  --cp-file         the same, one component per line:\n"
        "                    NAME c0 c1 c2 c3 tmin tmax\n"
        "  --eos             PR | PRCORR | SRK | ZJ | RK             [PR, or the deck's]\n"
        "\n"
        "Built-in components have their heat capacity, and so does a deck\n"
        "component with the name and the molar mass (within 1 %) of one. The\n"
        "header shows the source of each. Without a heat capacity for every\n"
        "component, the isothermal flash reports no enthalpies and the\n"
        "isenthalpic flash cannot run.\n"
        "\n"
        "the isothermal flash (as flow_comp):\n"
        "  --flash-two-phase-method  ssi | newton | ssi+newton      [ssi+newton]\n"
        "  --flash-tolerance         tolerance                      [1e-8]\n"
        "  --flash-verbosity         0..5                           [0]\n"
        "\n"
        "the enthalpies and the isenthalpic flash:\n"
        "  --enthalpy-model  caloric | eos_departure, for --h        [eos_departure]\n"
        "                    caloric = ideal-gas heat capacity only;\n"
        "                    eos_departure adds the residual enthalpy\n"
        "  --tref            reference temperature: enthalpies are zero for\n"
        "                    the ideal gas at tref                    [298.15 K]\n"
        "  --tmin, --tmax    temperature search interval, inside the\n"
        "                    heat-capacity fit range      [the fit range]\n"
        "  --tol             temperature tolerance [K]               [1e-6]\n"
        "  --enthalpy-tol    enthalpy acceptance [J/mol]             [1]\n"
        "  --maxiter         iteration limit                         [100]\n"
        "\n"
        "output:\n"
        "  --format          table | csv (csv: one row per state, the\n"
        "                    settings in '#' header lines)           [table]\n"
        "\n"
        "Units: bar, K, J/mol, kg/m3 and cP, for the input and the output, also\n"
        "with --deck. Enthalpies are molar [J/mol]. Enthalpies are only reported\n"
        "inside the heat-capacity fit range of every component (built-in\n"
        "components: 250-600 K). Z is the compressibility factor of the\n"
        "equation-of-state root, p Vm / (R T); the density includes the\n"
        "volume shift (SSHIFT).\n"
        "\n"
        "examples:\n"
        "  # phase split, phase properties and enthalpy of a state\n"
        "  opmflash --p 50 --t 300 --components C1,C10 --z 0.5,0.5 --kij C1:C10=0.0411\n"
        "\n"
        "  # the temperature at which the mixture has a given enthalpy\n"
        "  opmflash --p 50 --h -25000 --components C1,C10 --z 0.5,0.5 --kij C1:C10=0.0411\n"
        "\n"
        "  # the curve H -> T as CSV\n"
        "  opmflash --p 50 --components C1,C10 --z 0.5,0.5 \\\n"
        "           --sweep h=-30000:5000:1000 --format csv > curve.csv\n"
        "\n"
        "  # the fluid of a deck\n"
        "  opmflash --p 100 --t 350 --deck CASE.DATA --z 0.5,0.3,0.2\n";
}

Command parseCli(const std::vector<std::string>& args)
{
    Command command;
    Cli& cli = command.cli;
    for (std::size_t i = 0; i < args.size(); ++i) {
        // --option value, or --option=value
        std::string arg = args[i];
        std::optional<std::string> attached;
        if (arg.starts_with("--")) {
            if (const auto eq = arg.find('='); eq != std::string::npos) {
                attached = arg.substr(eq + 1);
                arg.resize(eq);
            }
            if (!cli.given.insert(arg).second) {
                throw std::runtime_error(arg + " is given twice");
            }
        }
        const auto value = [&]() -> std::string {
            if (attached) {
                return *std::exchange(attached, std::nullopt);
            }
            if (i + 1 >= args.size()) {
                throw std::runtime_error("missing value after " + arg);
            }
            return args[++i];
        };
        const auto flag = [&]() {
            if (attached) {
                throw std::runtime_error(arg + " takes no value");
            }
        };
        if (arg == "--help" || arg == "-h") {
            flag();
            command.action = Action::Help;
            return command;
        }
        else if (arg == "--list-components") {
            flag();
            command.action = Action::ListComponents;
            return command;
        }
        else if (arg == "--selftest") { flag(); command.action = Action::Selftest; }
        else if (arg == "--p") { cli.p = toDouble(arg, value()); }
        else if (arg == "--t") { cli.t = toDouble(arg, value()); }
        else if (arg == "--h") { cli.h = toDouble(arg, value()); }
        else if (arg == "--sweep") { cli.sweep = parseSweep(value()); }
        else if (arg == "--input") { cli.input = value(); }
        else if (arg == "--deck") { cli.deck = value(); }
        else if (arg == "--components") { cli.components = split(value(), ','); }
        else if (arg == "--z") { cli.z = toDoubles(arg, value()); }
        else if (arg == "--eos-region") { cli.eosRegion = toInt(arg, value()); }
        else if (arg == "--kij") { cli.kij = parseKij(value()); }
        else if (arg == "--cp") { std::ranges::copy(parseCp(value()), std::back_inserter(cli.cp)); }
        else if (arg == "--cp-file") {
            const std::string path = value();
            std::ifstream file(path);
            if (!file) {
                throw std::runtime_error("--cp-file: cannot read " + path);
            }
            std::ranges::copy(readCpFile(file, path), std::back_inserter(cli.cp));
        }
        else if (arg == "--eos") { cli.eos = eosFromName(value()); }
        else if (arg == "--enthalpy-model") {
            cli.enthalpyModelName = value();
            cli.enthalpyModel = enthalpyModelFromName(cli.enthalpyModelName);
        }
        else if (arg == "--tref") { cli.tRef = toDouble(arg, value()); }
        else if (arg == "--tmin") { cli.tMin = toDouble(arg, value()); }
        else if (arg == "--tmax") { cli.tMax = toDouble(arg, value()); }
        else if (arg == "--tol") { cli.temperatureTolerance = toDouble(arg, value()); }
        else if (arg == "--enthalpy-tol") { cli.enthalpyTolerance = toDouble(arg, value()); }
        else if (arg == "--maxiter") { cli.maxIterations = toInt(arg, value()); }
        else if (arg == "--flash-two-phase-method") { cli.twoPhaseMethod = methodFromName(value()); }
        else if (arg == "--flash-tolerance") { cli.flashTolerance = toDouble(arg, value()); }
        else if (arg == "--flash-verbosity") { cli.flashVerbosity = toInt(arg, value()); }
        else if (arg == "--format") { cli.format = formatFromName(value()); }
        else {
            throw std::runtime_error("unknown option '" + arg + "' (see --help)");
        }
        if (attached) {
            throw std::runtime_error(arg + " takes no value");
        }
    }
    return command;
}

// The flash that the request asks for, and the checks that need no fluid. A
// temperature asks for the isothermal flash and an enthalpy for the
// isenthalpic one; every option given has to take part in the run.
Spec checkRequest(const Cli& cli, const StateFile* states)
{
    const auto sweeping = [&](Variable v) { return cli.sweep && cli.sweep->variable == v; };
    Spec spec = Spec::PT;
    if (states) {
        for (const auto* option : {"--p", "--t", "--h", "--sweep"}) {
            if (cli.has(option)) {
                throw std::runtime_error(std::string(option) + " does not apply with --input, "
                                         "whose states give p, and T or h");
            }
        }
        if (!states->zNames.empty() && cli.z) {
            throw std::runtime_error("--input gives z, so --z does not apply");
        }
        spec = states->variable == Variable::T ? Spec::PT : Spec::PH;
    }
    else {
        if (cli.p && sweeping(Variable::P)) {
            throw std::runtime_error("--p and a p-sweep both give the pressure");
        }
        if (cli.t && sweeping(Variable::T)) {
            throw std::runtime_error("--t and a t-sweep both give the temperature");
        }
        if (cli.h && sweeping(Variable::H)) {
            throw std::runtime_error("--h and an h-sweep both give the enthalpy");
        }
        if (!cli.p && !sweeping(Variable::P)) {
            throw std::runtime_error("--p is required (or a p-sweep)");
        }
        const bool temperature = cli.t || sweeping(Variable::T);
        const bool enthalpy = cli.h || sweeping(Variable::H);
        if (temperature && enthalpy) {
            throw std::runtime_error("give a temperature (--t, the isothermal flash) or an "
                                     "enthalpy (--h, the isenthalpic flash), not both");
        }
        if (!temperature && !enthalpy) {
            throw std::runtime_error("give --t for the isothermal flash or --h for the "
                                     "isenthalpic flash");
        }
        spec = enthalpy ? Spec::PH : Spec::PT;
        if (cli.sweep) {
            sweepPoints(*cli.sweep);
        }
    }
    if (!cli.deck && !cli.components) {
        throw std::runtime_error("give the fluid: --components (see --list-components) or --deck");
    }
    if (cli.deck && cli.components) {
        throw std::runtime_error("--deck and --components exclude each other: the deck names "
                                 "the components");
    }
    if (!cli.deck && cli.has("--eos-region")) {
        throw std::runtime_error("--eos-region needs --deck");
    }
    if (spec == Spec::PT) {
        for (const auto* option : {"--tmin", "--tmax", "--tol", "--enthalpy-tol", "--maxiter",
                                   "--enthalpy-model"}) {
            if (cli.has(option)) {
                throw std::runtime_error(std::string(option) + " applies to the isenthalpic "
                                         "flash (--h) only");
            }
        }
    }
    return spec;
}

// ---- the mixture -----------------------------------------------------------
// The command line, the files and the output use bar, K, kg/m3 and cP; the
// flash works in SI units.
constexpr double pascalPerBar = 1e5;
constexpr double pascalSecondPerCentipoise = 1e-3;

// The flashed mixture, from the built-in components or from a deck, with the
// interaction coefficients, heat capacities and feed of the command line.
struct Mixture {
    std::vector<ComponentEntry> components;
    std::vector<double> bic;              // packed lower triangle: (1,0), (2,0), (2,1), ...
    std::vector<double> z;                // empty when each state of --input has its own
    std::string zSource;
    EOSType eos;
    std::string eosSource;
    std::array<double, 5> lbc = Opm::CompositionalConfig::defaultLBCCoefficients();
};

Mixture builtinMixture(const Cli& cli)
{
    Mixture mixture;
    for (const auto& name : *cli.components) {
        const ComponentEntry* entry = findBuiltin(name);
        if (!entry) {
            throw std::runtime_error("unknown component '" + name + "' (see --list-components)");
        }
        if (std::ranges::find(mixture.components, entry->name, &ComponentEntry::name) !=
            mixture.components.end()) {
            throw std::runtime_error("--components names " + entry->name + " twice ('" + name + "')");
        }
        mixture.components.push_back(*entry);
    }
    const auto n = mixture.components.size();
    mixture.bic.assign(n * (n - 1) / 2, 0.0);
    mixture.eos = cli.eos.value_or(EOSType::PR);
    mixture.eosSource = cli.eos ? "--eos" : "default";
    return mixture;
}

// Critical volume [m3/mol], from ZCRIT when given, as the fluid system takes it
// from a deck.
double criticalVolume(const Opm::CompositionalConfig::EOSProps& props, std::size_t c)
{
    if (!props.critical_z_factor.empty()) {
        return props.critical_z_factor[c] * Opm::Constants<double>::R *
               props.critical_temperature[c] / props.critical_pressure[c];
    }
    if (props.critical_volume.empty()) {
        throw std::runtime_error("the deck gives neither VCRIT nor ZCRIT");
    }
    return props.critical_volume[c];
}

// The fluid of one equation-of-state region of the deck, as the compositional
// simulator builds it. A component with the name and the molar mass of a
// built-in one has its heat capacity; a lumped component of another molar mass
// under the same name has none.
Mixture deckMixture(const Opm::Deck& deck, const Opm::Runspec& runspec, const Cli& cli)
{
    const Opm::CompositionalConfig config(deck, runspec);
    if (config.numComps() == 0) {
        throw std::runtime_error("the deck has no compositional fluid (COMPS, CNAMES and the "
                                 "equation-of-state keywords)");
    }
    const auto regions = runspec.tabdims().getNumEosRes();
    if (cli.eosRegion < 1 || static_cast<std::size_t>(cli.eosRegion) > regions) {
        throw std::runtime_error("--eos-region expects 1 to " + std::to_string(regions) +
                                 ", got " + std::to_string(cli.eosRegion));
    }
    const std::size_t region = cli.eosRegion - 1;
    const auto& names = config.compName();
    const auto& props = config.eosProps(region);
    const auto& omegaA = config.omegaA(region);
    const auto& omegaB = config.omegaB(region);
    Mixture mixture;
    for (std::size_t c = 0; c < config.numComps(); ++c) {
        const ComponentEntry* builtin = findBuiltin(names[c]);
        const bool sameComponent =
            builtin && std::abs(props.molecular_weights[c] - builtin->molarMass) <=
                           0.01 * builtin->molarMass;
        mixture.components.push_back(
            {names[c], {}, props.molecular_weights[c], props.critical_temperature[c],
             props.critical_pressure[c], criticalVolume(props, c) * 1e3,
             props.acentric_factors[c],
             c < props.volume_shifts.size() ? props.volume_shifts[c] : 0.0,
             omegaA[c], omegaB[c],
             sameComponent ? builtin->heatCapacity : std::nullopt,
             sameComponent ? "built-in " + builtin->name : ""});
    }
    const auto n = mixture.components.size();
    mixture.bic = props.binary_interaction_coefficient;
    if (mixture.bic.empty()) {
        mixture.bic.assign(n * (n - 1) / 2, 0.0);
    }
    mixture.eos = cli.eos.value_or(config.eosType(region));
    mixture.eosSource = (cli.eos ? "--eos, " : "") + std::string("EOS region ") +
                        std::to_string(cli.eosRegion) + " of " + std::to_string(regions);
    mixture.lbc = config.lbcCoefficients();
    return mixture;
}

Opm::Deck readDeck(const std::string& path)
{
    Opm::ParseContext parseContext(Opm::InputErrorAction::WARN);
    Opm::ErrorGuard errors;
    auto deck = Opm::Parser{}.parseFile(path, parseContext, errors);
    if (errors) {
        errors.dump();
        errors.clear();
        throw std::runtime_error("the deck " + path + " has input errors");
    }
    return deck;
}

std::size_t componentIndex(const Mixture& mixture, std::string_view option, std::string_view name)
{
    const auto it = std::ranges::find_if(mixture.components, [name](const ComponentEntry& e) {
        return isNamed(e, name);
    });
    if (it == mixture.components.end()) {
        throw std::runtime_error(std::string(option) + " names " + std::string(name) +
                                 ", which is not among the components");
    }
    return static_cast<std::size_t>(it - mixture.components.begin());
}

// A feed: n mole fractions in [0, 1] whose sum is 1 within 1e-3 (fractions
// rounded to four decimals), normalised. The note says when the sum was not 1.
std::vector<double> normalisedFeed(const std::string& where, std::vector<double> z,
                                   std::size_t n, std::string* note = nullptr)
{
    if (z.size() != n) {
        throw std::runtime_error(where + " has " + std::to_string(z.size()) +
                                 " mole fractions for " + std::to_string(n) + " components");
    }
    if (std::ranges::any_of(z, [](double v) { return !(v >= 0.0 && v <= 1.0); })) {
        throw std::runtime_error(where + " values must lie in [0, 1]");
    }
    const double sum = std::reduce(z.begin(), z.end());
    if (std::abs(sum - 1.0) > 1e-3) {
        throw std::runtime_error(where + " sums to " + num(sum) + ", not 1");
    }
    std::ranges::transform(z, z.begin(), [sum](double v) { return v / sum; });
    if (note && std::abs(sum - 1.0) > 1e-12) {
        *note = ", normalised from a sum of " + num(sum);
    }
    return z;
}

// The command line's interaction coefficients, heat capacities and feed on
// the mixture.
void applyCommandLine(Mixture& mixture, const Cli& cli, const StateFile* states)
{
    for (const auto& kij : cli.kij) {
        const auto a = componentIndex(mixture, "--kij", kij.first);
        const auto b = componentIndex(mixture, "--kij", kij.second);
        if (a == b) {
            throw std::runtime_error("--kij pair " + kij.first + ":" + kij.second +
                                     " names the same component twice");
        }
        const auto [row, col] = std::minmax(a, b);
        mixture.bic[col * (col - 1) / 2 + row] = kij.value;
    }

    const auto n = mixture.components.size();
    std::vector<bool> given(n, false);
    for (const auto& cp : cli.cp) {
        const auto c = componentIndex(mixture, cp.source, cp.name);
        if (given[c]) {
            throw std::runtime_error("the heat capacity of " + mixture.components[c].name +
                                     " is given twice (--cp, --cp-file)");
        }
        given[c] = true;
        mixture.components[c].heatCapacity = cp.heatCapacity;
        mixture.components[c].heatCapacitySource = cp.source;
    }

    if (cli.z) {
        std::string note;
        mixture.z = normalisedFeed("--z", *cli.z, n, &note);
        mixture.zSource = "--z" + note;
    }
    else if (states && !states->zNames.empty()) {
        mixture.zSource = "--input, normalised when the sum is within 1e-3 of 1";
    }
    else if (mixture.z.empty()) {
        throw std::runtime_error("--z is required (one mole fraction per component: " +
                                 std::to_string(n) + ")");
    }
}

// ---- the run ---------------------------------------------------------------
// A state to flash, in SI units: p [Pa], and T [K] or h [J/mol].
struct State {
    double p;
    double value;
    std::vector<double> z;
};

// The temperatures [K] inside the heat-capacity fit range of every component.
struct FitRange {
    double min, max;
};

struct Run {
    Spec spec;
    Mixture mixture;
    std::vector<State> states;
    std::optional<FitRange> fit;          // none without a heat capacity for every component
    double tRef;                          // [K]
    FitRange search{};                    // the isenthalpic flash's temperature interval [K]
};

std::string componentsWithoutHeatCapacity(const Mixture& mixture)
{
    std::string names;
    for (const auto& e : mixture.components) {
        if (!e.heatCapacity) {
            names += (names.empty() ? "" : ", ") + e.name;
        }
    }
    return names;
}

std::string rangeText(const FitRange& range)
{
    return num(range.min) + " to " + num(range.max) + " K";
}

std::optional<FitRange> fitRange(const Mixture& mixture)
{
    FitRange range{-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    for (const auto& e : mixture.components) {
        if (!e.heatCapacity) {
            return std::nullopt;
        }
        range.min = std::max(range.min, e.heatCapacity->minTemperature);
        range.max = std::min(range.max, e.heatCapacity->maxTemperature);
    }
    if (!(range.min < range.max)) {
        throw std::runtime_error("the heat-capacity fit ranges of the components do not overlap");
    }
    return range;
}

// The states of the run: one, the points of a sweep, or the lines of --input.
std::vector<State> statesOf(const Cli& cli, Spec spec, const Mixture& mixture,
                            const StateFile* file)
{
    const auto n = mixture.components.size();
    std::vector<State> states;
    if (file) {
        // the z_ column of each component
        std::vector<std::size_t> column;
        for (const auto& e : mixture.components) {
            if (file->zNames.empty()) {
                break;
            }
            const auto it = std::ranges::find_if(file->zNames, [&](const std::string& name) {
                return isNamed(e, name);
            });
            if (it == file->zNames.end()) {
                throw std::runtime_error("--input has no column z_" + e.name);
            }
            column.push_back(static_cast<std::size_t>(it - file->zNames.begin()));
        }
        if (!column.empty() && file->zNames.size() != n) {
            throw std::runtime_error("--input has " + std::to_string(file->zNames.size()) +
                                     " z_ columns for " + std::to_string(n) + " components");
        }
        for (const auto& row : file->rows) {
            std::vector<double> z = mixture.z;
            if (!column.empty()) {
                z.clear();
                for (const auto c : column) {
                    z.push_back(row.z[c]);
                }
                z = normalisedFeed("--input line " + std::to_string(row.line) + ": z", z, n);
            }
            states.push_back({row.p * pascalPerBar, row.value, z});
        }
        return states;
    }

    const double p = cli.p.value_or(0.0) * pascalPerBar;
    const double fixed = spec == Spec::PH ? cli.h.value_or(0.0) : cli.t.value_or(0.0);
    if (!cli.sweep) {
        return {{p, fixed, mixture.z}};
    }
    for (const double v : sweepPoints(*cli.sweep)) {
        switch (cli.sweep->variable) {
        case Variable::P: states.push_back({v * pascalPerBar, fixed, mixture.z}); break;
        case Variable::T: states.push_back({p, v, mixture.z}); break;
        case Variable::H: states.push_back({p, v, mixture.z}); break;
        }
    }
    return states;
}

// Everything that can be checked before a flash: the request, the fluid, the
// states, and the temperatures of the enthalpies.
Run prepare(const Cli& cli, const StateFile* stateFile, const Opm::Deck* deck = nullptr)
{
    Run run;
    run.spec = checkRequest(cli, stateFile);
    std::optional<Opm::Deck> parsed;
    if (cli.deck && !deck) {
        parsed = readDeck(*cli.deck);
        deck = &*parsed;
    }
    if (deck) {
        const Opm::Runspec runspec(*deck);
        run.mixture = deckMixture(*deck, runspec, cli);
    }
    else {
        run.mixture = builtinMixture(cli);
    }
    applyCommandLine(run.mixture, cli, stateFile);
    run.states = statesOf(cli, run.spec, run.mixture, stateFile);

    run.fit = fitRange(run.mixture);
    if (!run.fit && run.spec == Spec::PH) {
        throw std::runtime_error("the isenthalpic flash needs the ideal-gas heat capacity of "
                                 "every component; give it with --cp or --cp-file for " +
                                 componentsWithoutHeatCapacity(run.mixture));
    }
    if (!run.fit && cli.tRef) {
        throw std::runtime_error("--tref does not apply: there are no enthalpies without the "
                                 "heat capacity of " + componentsWithoutHeatCapacity(run.mixture));
    }
    run.tRef = cli.tRef.value_or(HeatCapacity::referenceTemperature());
    if (run.fit && (run.tRef < run.fit->min || run.tRef > run.fit->max)) {
        throw std::runtime_error("the reference temperature (--tref) " +
                                 num(run.tRef) + " K lies outside the heat-capacity fit range " +
                                 rangeText(*run.fit));
    }
    if (run.spec == Spec::PH) {
        run.search = {cli.tMin.value_or(run.fit->min), cli.tMax.value_or(run.fit->max)};
        if (run.search.min < run.fit->min - 1e-9 || run.search.max > run.fit->max + 1e-9) {
            throw std::runtime_error("--tmin and --tmax must lie inside the heat-capacity fit "
                                     "range " + rangeText(*run.fit));
        }
        if (!(run.search.min < run.search.max)) {
            throw std::runtime_error("--tmin must lie below --tmax");
        }
    }
    return run;
}

// ---- the flash, for a compile-time number of components --------------------
template <int N>
struct FlashRunner {
    using FluidSystem = Opm::GenericOilGasWaterFluidSystem<double, N, false>;
    using FlashEval = Opm::DenseAd::Evaluation<double, N + 1>;
    using FluidState = Opm::CompositionalFluidState<FlashEval, FluidSystem>;
    using ParameterCache = typename FluidSystem::template ParameterCache<FlashEval>;
    using PtFlash = Opm::PTFlash<double, FluidSystem>;
    using PhFlash = Opm::PHFlash<double, FluidSystem>;
    using Enthalpy = Opm::MixtureEnthalpy<double, FluidSystem>;

    // The properties of a phase, in SI units; Z of the equation-of-state root.
    struct Phase {
        double density, viscosity, compressibility;
    };

    // A failed state has no phase split and no enthalpies; its temperature is
    // the requested one for the isothermal flash.
    struct Result {
        bool ok;
        bool singlePhase;
        double temperature;
        double L;
        std::array<double, N> x, y;
        std::optional<double> hCaloric, hDeparture;
        std::optional<Phase> liquid, vapour;

        bool hasLiquid() const { return ok && (!singlePhase || L > 0.5); }
        bool hasVapour() const { return ok && (!singlePhase || L < 0.5); }
    };

    // The heat capacities, when every component has one.
    static inline std::optional<Opm::CpTable<double, N>> cpTable_;

    static void setup(const Mixture& mixture)
    {
        FluidSystem::init();
        for (const auto& e : mixture.components) {
            FluidSystem::addComponent(typename FluidSystem::ComponentParam{
                e.name, e.molarMass, e.criticalTemperature, e.criticalPressure,
                e.criticalVolume, e.acentricFactor, e.volumeShift, e.omegaA, e.omegaB});
        }
        FluidSystem::setInteractionCoefficients(mixture.bic);
        FluidSystem::setLbcCoefficients(mixture.lbc);
        cpTable_.reset();
        if (std::ranges::all_of(mixture.components,
                                [](const ComponentEntry& e) { return e.heatCapacity.has_value(); })) {
            Opm::CpTable<double, N> table;
            for (int c = 0; c < N; ++c) {
                table[c] = *mixture.components[c].heatCapacity;
            }
            cpTable_ = table;
        }
    }

    static FluidState makeState(double p, double t, const std::vector<double>& z)
    {
        FluidState fs;
        for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
            fs.setPressure(phaseIdx, p);
        }
        fs.setTemperature(t);
        for (int compIdx = 0; compIdx < N; ++compIdx) {
            fs.setMoleFraction(compIdx, z[compIdx]);
        }
        for (int compIdx = 0; compIdx < N; ++compIdx) {
            fs.setKvalue(compIdx, fs.wilsonK_(compIdx));
        }
        fs.setLvalue(-1.0);
        return fs;
    }

    static typename PhFlash::Config phConfig(const Cli& cli, const FitRange& search)
    {
        typename PhFlash::Config cfg;
        cfg.cpTable = *cpTable_;
        cfg.model = cli.enthalpyModel;
        cfg.tempMin = search.min;
        cfg.tempMax = search.max;
        cfg.temperatureTolerance = cli.temperatureTolerance;
        cfg.enthalpyTolerance = cli.enthalpyTolerance;
        cfg.maxIterations = cli.maxIterations;
        return cfg;
    }

    // The ideal-gas enthalpy of the feed at tRef on the library's basis (zero
    // for the ideal gas at ComponentCp::referenceTemperature()): an enthalpy
    // on the tRef basis is the library's minus this shift. Needs cpTable_.
    static double referenceShift(const std::vector<double>& z, double tRef)
    {
        double shift = 0.0;
        for (int compIdx = 0; compIdx < N; ++compIdx) {
            shift += z[compIdx] * (*cpTable_)[compIdx].enthalpyIntegral(tRef);
        }
        return shift;
    }

    static std::optional<Phase> phase(const FluidState& fs, EOSType eos, unsigned phaseIdx)
    {
        try {
            ParameterCache cache(eos);
            cache.updatePhase(fs, phaseIdx);
            const double p = Opm::getValue(fs.pressure(phaseIdx));
            const double t = Opm::getValue(fs.temperature(phaseIdx));
            return Phase{Opm::getValue(FluidSystem::density(fs, cache, phaseIdx)),
                         Opm::getValue(FluidSystem::viscosity(fs, cache, phaseIdx)),
                         p * Opm::getValue(cache.molarVolume(phaseIdx)) /
                             (Opm::Constants<double>::R * t)};
        }
        catch (const std::runtime_error&) {
            return std::nullopt;
        }
    }

    // The enthalpies need a converged state, the heat capacities (shift) and
    // a temperature inside their fit range.
    static Result collect(const FluidState& fs, bool singlePhase, bool ok, EOSType eos,
                          std::optional<double> shift, const std::optional<FitRange>& fit)
    {
        Result r{};
        r.ok = ok;
        r.singlePhase = singlePhase;
        r.temperature = Opm::getValue(fs.temperature(0));
        r.L = Opm::getValue(fs.L());
        for (int c = 0; c < N; ++c) {
            r.x[c] = Opm::getValue(fs.moleFraction(FluidSystem::oilPhaseIdx, c));
            r.y[c] = Opm::getValue(fs.moleFraction(FluidSystem::gasPhaseIdx, c));
        }
        if (r.hasLiquid()) { r.liquid = phase(fs, eos, FluidSystem::oilPhaseIdx); }
        if (r.hasVapour()) { r.vapour = phase(fs, eos, FluidSystem::gasPhaseIdx); }
        if (ok && shift && fit && r.temperature >= fit->min && r.temperature <= fit->max) {
            r.hCaloric = Opm::getValue(Enthalpy::mixtureEnthalpy(
                fs, *cpTable_, eos, Opm::EnthalpyModel::caloric)) - *shift;
            r.hDeparture = Opm::getValue(Enthalpy::mixtureEnthalpy(
                fs, *cpTable_, eos, Opm::EnthalpyModel::eos_departure)) - *shift;
        }
        return r;
    }

    static Result runPt(const Cli& cli, const Run& run, const State& state,
                        std::optional<double> shift)
    {
        FluidState fs = makeState(state.p, state.value, state.z);
        // A state that the flash cannot solve is reported with ok = 0, as for
        // the isenthalpic flash, and the run goes on to the next one.
        bool ok = true;
        bool singlePhase = false;
        try {
            singlePhase = PtFlash::solve(fs, cli.twoPhaseMethod, cli.flashTolerance,
                                         run.mixture.eos, cli.flashVerbosity);
        }
        catch (const std::runtime_error&) {
            ok = false;
        }
        catch (const Dune::FMatrixError&) {
            ok = false;
        }
        return collect(fs, singlePhase, ok, run.mixture.eos, shift, run.fit);
    }

    static Result runPh(const Cli& cli, const Run& run, const State& state, double shift,
                        const FitRange& search)
    {
        // the initial temperature only seeds the state; PHFlash searches over T
        FluidState fs = makeState(state.p, 0.5 * (search.min + search.max), state.z);
        // h is on the --tref basis; the flash works on the library's.
        const bool ok = PhFlash::solve(fs, state.value + shift, phConfig(cli, search),
                                       cli.twoPhaseMethod, cli.flashTolerance, run.mixture.eos,
                                       cli.flashVerbosity);
        const bool singlePhase = !(Opm::getValue(fs.L()) > 0.0 && Opm::getValue(fs.L()) < 1.0);
        return collect(fs, singlePhase, ok, run.mixture.eos, shift, run.fit);
    }

    static Result runState(const Cli& cli, const Run& run, const State& state)
    {
        const std::optional<double> shift =
            run.fit ? std::optional(referenceShift(state.z, run.tRef)) : std::nullopt;
        return run.spec == Spec::PT ? runPt(cli, run, state, shift)
                                    : runPh(cli, run, state, *shift, run.search);
    }
};

// ---- output ----------------------------------------------------------------
std::string phaseLabel(bool singlePhase, double L)
{
    return !singlePhase ? "two-phase" : L > 0.5 ? "single-phase liquid" : "single-phase vapour";
}

void printHeader(const Cli& cli, const Run& run)
{
    const std::string prefix = cli.format == Format::Csv ? "# " : "";
    const auto& m = run.mixture;
    std::cout << prefix << "opmflash: "
              << (run.spec == Spec::PT ? "isothermal flash (P-T)" : "isenthalpic flash (P-H)")
              << ", eos " << Opm::CompositionalConfig::eosTypeToString(m.eos) << " ("
              << m.eosSource << "), " << Opm::ptFlashMethodToString(cli.twoPhaseMethod)
              << ", flash tolerance " << num(cli.flashTolerance) << '\n';
    if (run.spec == Spec::PH) {
        std::cout << prefix << "temperature search " << rangeText(run.search)
                  << ", enthalpy model " << cli.enthalpyModelName << '\n';
    }
    std::cout << prefix << "units: p bar, T K, density kg/m3, viscosity cP; enthalpies J/mol";
    if (run.fit) {
        std::cout << ", zero for the ideal gas at " << num(run.tRef) << " K"
                  << ", reported in the heat-capacity fit range " << rangeText(*run.fit);
    }
    else {
        std::cout << ", none without the heat capacity of " << componentsWithoutHeatCapacity(m)
                  << " (see --cp, --cp-file)";
    }
    std::cout << '\n' << prefix << "components";
    if (cli.deck) {
        std::cout << " of the deck " << *cli.deck;
    }
    std::cout << ": name, molar mass [g/mol], heat capacity\n";
    for (const auto& e : m.components) {
        std::cout << prefix << "  " << std::left << std::setw(10) << e.name << std::right
                  << std::setw(10) << num(e.molarMass * 1e3) << "   ";
        if (e.heatCapacity) {
            std::cout << e.heatCapacitySource << ", " << num(e.heatCapacity->minTemperature)
                      << " to " << num(e.heatCapacity->maxTemperature) << " K\n";
        }
        else {
            std::cout << "none\n";
        }
    }
    std::cout << prefix << "z from " << m.zSource << '\n';

    if (cli.format == Format::Csv) {
        const auto columns = [&](std::string_view head) {
            for (const auto& e : m.components) {
                std::cout << ',' << head << '_' << e.name;
            }
        };
        std::cout << "p,T,h_spec";
        columns("z");
        std::cout << ",phases,L";
        columns("x");
        columns("y");
        columns("K");
        std::cout << ",density_liquid,density_vapour,viscosity_liquid,viscosity_vapour,"
                     "Z_liquid,Z_vapour,H_caloric,H_departure,ok\n";
    }
    else {
        std::cout << '\n';
    }
}

// A failed state leaves every result field empty in the CSV, including the
// temperature of the isenthalpic flash, which is then the last trial; so does
// a phase that is not there.
template <int N>
void printCsvRow(const Run& run, const State& state, const typename FlashRunner<N>::Result& r)
{
    const auto field = [](std::optional<double> v) {
        std::ostringstream os;
        if (v) {
            os << std::setprecision(10) << *v;
        }
        return os.str();
    };
    const auto when = [](bool present, double v) {
        return present ? std::optional(v) : std::nullopt;
    };
    const bool validT = r.ok || run.spec == Spec::PT;
    std::cout << field(state.p / pascalPerBar) << ','
              << field(when(validT, r.temperature)) << ','
              << field(when(run.spec == Spec::PH, state.value));
    for (const double v : state.z) {
        std::cout << ',' << field(v);
    }
    std::cout << ',' << (r.ok ? phaseLabel(r.singlePhase, r.L) : "") << ','
              << field(when(r.ok, r.L));
    for (int c = 0; c < N; ++c) { std::cout << ',' << field(when(r.hasLiquid(), r.x[c])); }
    for (int c = 0; c < N; ++c) { std::cout << ',' << field(when(r.hasVapour(), r.y[c])); }
    for (int c = 0; c < N; ++c) {
        const bool both = r.ok && !r.singlePhase && r.x[c] > 0.0;
        std::cout << ',' << field(when(both, both ? r.y[c] / r.x[c] : 0.0));
    }
    // a property of a phase in the output units, empty without the phase
    using Phase = typename FlashRunner<N>::Phase;
    const auto property = [&](const std::optional<Phase>& phase, double Phase::*member,
                              double unit) {
        return field(phase ? std::optional((*phase).*member / unit) : std::nullopt);
    };
    std::cout << ',' << property(r.liquid, &Phase::density, 1.0)
              << ',' << property(r.vapour, &Phase::density, 1.0)
              << ',' << property(r.liquid, &Phase::viscosity, pascalSecondPerCentipoise)
              << ',' << property(r.vapour, &Phase::viscosity, pascalSecondPerCentipoise)
              << ',' << property(r.liquid, &Phase::compressibility, 1.0)
              << ',' << property(r.vapour, &Phase::compressibility, 1.0)
              << ',' << field(r.hCaloric) << ',' << field(r.hDeparture)
              << ',' << (r.ok ? 1 : 0) << '\n';
}

template <int N>
void printTableRow(const Run& run, const State& state, const typename FlashRunner<N>::Result& r)
{
    const auto& m = run.mixture;
    const std::string pText = num(state.p / pascalPerBar) + " bar";
    const auto tText = [](double t) { return num(t) + " K"; };
    if (!r.ok) {
        if (run.spec == Spec::PH) {
            std::cout << "  NO SOLUTION: no temperature in " << rangeText(run.search)
                      << " gives h = " << num(state.value) << " J/mol at p = " << pText;
            if (run.fit && (run.search.min > run.fit->min || run.search.max < run.fit->max)) {
                std::cout << "; --tmin and --tmax may extend to " << rangeText(*run.fit);
            }
            std::cout << "\n\n";
        }
        else {
            std::cout << "  NO SOLUTION at p = " << pText << ", T = " << tText(r.temperature)
                      << ": the isothermal flash did not converge\n\n";
        }
        return;
    }
    std::cout << "  p = " << pText << "   T = " << tText(r.temperature);
    if (run.spec == Spec::PH) {
        std::cout << "   h = " << num(state.value) << " J/mol";
    }
    std::cout << "\n  " << phaseLabel(r.singlePhase, r.L) << "   L = " << num(r.L) << '\n';

    // one row per component: z, and the composition of each phase present
    std::size_t width = std::string_view("component").size();
    for (const auto& e : m.components) {
        width = std::max(width, e.name.size());
    }
    const int nameWidth = static_cast<int>(width) + 2;
    std::cout << "    " << std::left << std::setw(nameWidth) << "component" << std::right
              << std::setw(13) << 'z';
    if (r.hasLiquid()) { std::cout << std::setw(13) << 'x'; }
    if (r.hasVapour()) { std::cout << std::setw(13) << 'y'; }
    if (!r.singlePhase) { std::cout << std::setw(13) << 'K'; }
    std::cout << '\n';
    for (int c = 0; c < N; ++c) {
        std::cout << "    " << std::left << std::setw(nameWidth) << m.components[c].name
                  << std::right << std::setw(13) << num(state.z[c]);
        if (r.hasLiquid()) { std::cout << std::setw(13) << num(r.x[c]); }
        if (r.hasVapour()) { std::cout << std::setw(13) << num(r.y[c]); }
        if (!r.singlePhase) {
            std::cout << std::setw(13) << (r.x[c] > 0.0 ? num(r.y[c] / r.x[c]) : "-");
        }
        std::cout << '\n';
    }

    // one row per phase present: density, viscosity, Z
    std::cout << "    " << std::left << std::setw(nameWidth) << "phase" << std::right
              << std::setw(17) << "density [kg/m3]"
              << std::setw(17) << "viscosity [cP]"
              << std::setw(10) << 'Z' << '\n';
    const auto phaseRow = [&](const char* name, const auto& phase) {
        std::cout << "    " << std::left << std::setw(nameWidth) << name << std::right;
        if (phase) {
            std::cout << std::setw(17) << num(phase->density)
                      << std::setw(17) << num(phase->viscosity / pascalSecondPerCentipoise)
                      << std::setw(10) << num(phase->compressibility) << '\n';
        }
        else {
            std::cout << "  (no properties: the phase volume is not positive)\n";
        }
    };
    if (r.hasLiquid()) { phaseRow("liquid", r.liquid); }
    if (r.hasVapour()) { phaseRow("vapour", r.vapour); }

    if (r.hCaloric && r.hDeparture) {
        std::cout << "  H caloric = " << num(*r.hCaloric)
                  << "   H eos_departure = " << num(*r.hDeparture) << "  [J/mol]\n";
    }
    else if (run.fit) {
        std::cout << "  no enthalpies: T is outside the heat-capacity fit range "
                  << rangeText(*run.fit) << '\n';
    }
    std::cout << '\n';
}

// ---- drivers ---------------------------------------------------------------
template <int N>
int runCases(const Cli& cli, const Run& run)
{
    using Runner = FlashRunner<N>;
    Runner::setup(run.mixture);
    printHeader(cli, run);
    bool allOk = true;
    for (const auto& state : run.states) {
        const auto r = Runner::runState(cli, run, state);
        if (cli.format == Format::Csv) {
            printCsvRow<N>(run, state, r);
        }
        else {
            printTableRow<N>(run, state, r);
        }
        allOk = allOk && r.ok;
    }
    return allOk ? EXIT_SUCCESS : EXIT_FAILURE;
}

// The numbers of components that the compositional simulator is built for.
using ComponentCounts = std::integer_sequence<int, 2, 3, 4, 5, 6, 7, 8, 9, 10>;

template <int... Ns>
int dispatch(const Cli& cli, const Run& run, std::integer_sequence<int, Ns...>)
{
    int result = EXIT_FAILURE;
    const auto n = run.mixture.components.size();
    const bool found = ((n == static_cast<std::size_t>(Ns) && (result = runCases<Ns>(cli, run), true)) || ...);
    if (!found) {
        throw std::runtime_error("supported numbers of components: 2 to 10, got " +
                                 std::to_string(n));
    }
    return result;
}

// ---- selftest --------------------------------------------------------------
// Isothermal flash at tAnchor, then the isenthalpic flash over the whole fit
// range at the enthalpy of the result: the temperature found, or nothing when
// either flash fails.
template <int N>
std::optional<double> roundTrip(const Cli& cli, const Run& run, double tAnchor)
{
    using Runner = FlashRunner<N>;
    Runner::setup(run.mixture);
    const State anchor{run.states.front().p, tAnchor, run.mixture.z};
    const double shift = Runner::referenceShift(anchor.z, run.tRef);
    const auto pt = Runner::runPt(cli, run, anchor, shift);
    if (!pt.ok || !pt.hDeparture) {
        return std::nullopt;
    }
    const State target{anchor.p, *pt.hDeparture, anchor.z};
    const auto ph = Runner::runPh(cli, run, target, shift, *run.fit);
    return ph.ok ? std::optional(ph.temperature) : std::nullopt;
}

// The methane/decane pair with the parameters of the built-in components, as a
// deck in METRIC units; c10MolarMass [g/mol] replaces the molar mass of decane.
std::string builtinPairDeck(std::optional<double> c10MolarMass = std::nullopt)
{
    const auto& c1 = *findBuiltin("C1");
    const auto& c10 = *findBuiltin("C10");
    std::ostringstream deck;
    deck << std::setprecision(17)
         << "RUNSPEC\nMETRIC\nOIL\nGAS\nTABDIMS\n/\nCOMPS\n2 /\nPROPS\n"
         << "CNAMES\nC1 C10 /\nEOS\nPR /\n"
         << "MW\n" << c1.molarMass * 1e3 << ' '
         << c10MolarMass.value_or(c10.molarMass * 1e3) << " /\n"
         << "TCRIT\n" << c1.criticalTemperature << ' ' << c10.criticalTemperature << " /\n"
         << "PCRIT\n" << c1.criticalPressure / 1e5 << ' ' << c10.criticalPressure / 1e5 << " /\n"
         << "VCRIT\n" << c1.criticalVolume << ' ' << c10.criticalVolume << " /\n"
         << "ACF\n" << c1.acentricFactor << ' ' << c10.acentricFactor << " /\n"
         << "BIC\n0.0411 /\n";
    return deck.str();
}

// Round trips through the whole program apart from the input files: flash at a
// temperature, give the enthalpy of the result to the isenthalpic flash, and
// require the temperature back within 1e-4 K; a deck with the parameters of
// the built-in components, in bar and K whatever its units; the phase
// properties; the checks of the command line.
int selftest()
{
    int failures = 0;
    const auto report = [&](bool pass, const std::string& what) {
        std::cout << (pass ? "  PASS  " : "  FAIL  ") << what << '\n';
        if (!pass) { ++failures; }
    };
    const auto args = [](std::vector<std::string> extra) {
        std::vector<std::string> all{"--components", "C1,C10", "--z", "0.5,0.5"};
        all.insert(all.end(), extra.begin(), extra.end());
        return all;
    };
    const auto checkRoundTrip = [&](const std::string& name, const std::vector<std::string>& line,
                                    double tAnchor, const Opm::Deck* deck = nullptr) {
        const Cli cli = parseCli(line).cli;
        const Run run = prepare(cli, nullptr, deck);
        const auto tBack = run.mixture.components.size() == 2 ? roundTrip<2>(cli, run, tAnchor)
                                                              : roundTrip<3>(cli, run, tAnchor);
        std::ostringstream what;
        what << name << "  T = " << tAnchor << " -> ";
        if (tBack) { what << *tBack << " K"; } else { what << "failed"; }
        report(tBack && std::abs(*tBack - tAnchor) < 1e-4, what.str());
    };
    // the first state of a run, flashed
    const auto flashFirst = [](const Cli& cli, const Run& run) {
        FlashRunner<2>::setup(run.mixture);
        return FlashRunner<2>::runState(cli, run, run.states.front());
    };

    checkRoundTrip("C1/C10, 50 bar, 300 K", args({"--p", "50", "--t", "300", "--kij", "C1:C10=0.0411"}),
                   300.0);
    checkRoundTrip("CO2/C1/C10, 50 bar, 320 K",
                   {"--p", "50", "--t", "320", "--components", "CO2,C1,C10", "--z", "0.4,0.2,0.4",
                    "--kij", "C1:C10=0.0411"},
                   320.0);

    // Another reference temperature moves every enthalpy by the ideal-gas
    // enthalpy of the feed between the two references, and the isenthalpic
    // flash on that basis finds the same temperature.
    checkRoundTrip("C1/C10, 50 bar, 300 K, --tref 273.15",
                   args({"--p", "50", "--t", "300", "--tref", "273.15"}), 300.0);
    {
        const Cli onDefault = parseCli(args({"--p", "50", "--t", "300"})).cli;
        const Cli onOther = parseCli(args({"--p", "50", "--t", "300", "--tref", "273.15"})).cli;
        const Run a = prepare(onDefault, nullptr);
        const Run b = prepare(onOther, nullptr);
        const double moved = *flashFirst(onOther, b).hDeparture - *flashFirst(onDefault, a).hDeparture;
        FlashRunner<2>::setup(b.mixture);
        const double expected = -FlashRunner<2>::referenceShift(b.mixture.z, b.tRef);
        std::ostringstream what;
        what << "--tref 273.15 moves H by " << moved << " J/mol (ideal-gas term " << expected << ")";
        report(expected > 0.0 && std::abs(moved - expected) <= 1e-9 * std::abs(expected),
               what.str());
    }

    // The program has to come back from a flash that throws, as it does for
    // this lean feed at 1 bar. Whether the flash converges is not checked, so
    // the case stays valid if the flash gets more robust.
    {
        const Cli cli = parseCli({"--p", "1", "--t", "500", "--components", "C1,C10",
                                  "--z", "0.1,0.9"}).cli;
        const auto r = flashFirst(cli, prepare(cli, nullptr));
        report(r.temperature == 500.0 && (r.ok || !r.hDeparture),
               "C1/C10 0.1/0.9, 1 bar, 500 K is reported");
    }

    // A deck with the parameters of the built-in components flashes like them,
    // and its components, named like built-in ones with their molar masses,
    // have their heat capacities.
    {
        const Opm::Deck deck = Opm::Parser{}.parseString(builtinPairDeck());
        const Cli viaBuiltin = parseCli(args({"--p", "50", "--t", "300", "--kij", "C1:C10=0.0411"})).cli;
        const Cli viaDeck = parseCli({"--deck", "(selftest)", "--p", "50", "--t", "300",
                                      "--z", "0.5,0.5"}).cli;
        const Run builtinRun = prepare(viaBuiltin, nullptr);
        const Run deckRun = prepare(viaDeck, nullptr, &deck);
        const auto a = flashFirst(viaBuiltin, builtinRun);
        const auto b = flashFirst(viaDeck, deckRun);
        double largest = std::abs(a.L - b.L);
        for (int c = 0; c < 2; ++c) {
            largest = std::max({largest, std::abs(a.x[c] - b.x[c]), std::abs(a.y[c] - b.y[c])});
        }
        std::ostringstream what;
        what << "a METRIC deck with the built-in C1/C10, at 50 bar and 300 K, flashes like them "
             << "(largest difference " << largest << ")";
        report(a.ok && b.ok && !a.singlePhase && largest < 1e-10 &&
               deckRun.states.front().p == 5e6 && deckRun.states.front().value == 300.0,
               what.str());
        checkRoundTrip("C1/C10 from a deck, 50 bar, 300 K",
                       {"--deck", "(selftest)", "--p", "50", "--t", "300", "--z", "0.5,0.5"},
                       300.0, &deck);
    }

    // A deck component under a built-in name with another molar mass has no
    // heat capacity.
    {
        const Opm::Deck lumped = Opm::Parser{}.parseString(builtinPairDeck(200.0));
        const Run run2 = prepare(parseCli({"--deck", "(selftest)", "--p", "50", "--t", "30",
                                           "--z", "0.5,0.5"}).cli, nullptr, &lumped);
        report(run2.mixture.components[0].heatCapacity && !run2.mixture.components[1].heatCapacity,
               "a deck C10 of molar mass 200 has no built-in heat capacity");
    }

    // The phase properties: a gas at 1 bar is nearly ideal, and its density
    // is p M / (Z R T) with the reported Z (no volume shift here).
    {
        const Cli cli = parseCli({"--p", "1", "--t", "300", "--components", "C1,N2",
                                  "--z", "0.5,0.5"}).cli;
        const Run run = prepare(cli, nullptr);
        const auto r = flashFirst(cli, run);
        const double molarMass = 0.5 * (findBuiltin("C1")->molarMass + findBuiltin("N2")->molarMass);
        bool pass = r.ok && r.singlePhase && r.vapour && !r.liquid;
        if (pass) {
            const double zFactor = r.vapour->compressibility;
            const double expected = 1e5 * molarMass / (zFactor * Opm::Constants<double>::R * 300.0);
            pass = zFactor > 0.99 && zFactor <= 1.0 &&
                   std::abs(r.vapour->density - expected) <= 1e-9 * expected &&
                   r.vapour->viscosity > 5e-6 && r.vapour->viscosity < 5e-5;
        }
        report(pass, "C1/N2 at 1 bar, 300 K: single-phase vapour, Z near 1, density p M / (Z R T)");
    }

    // A sum rounded to four decimals is normalised; a larger error is not.
    {
        const Run run = prepare(parseCli({"--p", "50", "--t", "300", "--components", "CO2,C1,C10",
                                          "--z", "0.3333,0.3333,0.3333"}).cli, nullptr);
        report(std::abs(std::reduce(run.mixture.z.begin(), run.mixture.z.end()) - 1.0) < 1e-15 &&
               run.mixture.zSource.find("normalised") != std::string::npos,
               "--z 0.3333,0.3333,0.3333 is normalised");
    }

    // --key=value is read as --key value.
    {
        const Cli cli = parseCli(args({"--p=50", "--t=300"})).cli;
        report(cli.p == 50.0 && cli.t == 300.0, "--p=50 --t=300 read as --p 50 --t 300");
    }

    // The isenthalpic flash searches the heat-capacity fit range unless told otherwise.
    {
        const Run run = prepare(parseCli(args({"--p", "50", "--h", "-25000"})).cli, nullptr);
        report(run.search.min == 250.0 && run.search.max == 600.0,
               "the isenthalpic flash searches the fit range 250 to 600 K");
    }

    // The files of states and of heat capacities.
    {
        std::istringstream text("# states\np, T, z_C1, z_C10\n50, 300, 0.5, 0.5\n"
                                "60, 310, 0.3, 0.7\n");
        const StateFile file = readStateFile(text, "(selftest)");
        const Cli cli = parseCli({"--components", "C1,C10"}).cli;
        const Run run = prepare(cli, &file);
        report(run.spec == Spec::PT && run.states.size() == 2 && run.states[1].p == 6e6 &&
               run.states[1].value == 310.0 && std::abs(run.states[1].z[1] - 0.7) < 1e-15,
               "--input: two isothermal states with their own z");

        std::istringstream cpText("# name c0 c1 c2 c3 tmin tmax\nC10 1 2 3 4 250 600\n");
        const auto entries = readCpFile(cpText, "(selftest)");
        report(entries.size() == 1 && entries[0].heatCapacity.c1 == 2.0 &&
               entries[0].source == "--cp-file", "--cp-file: one heat capacity");
    }

    // The command line refuses what it cannot run, and every option given has
    // to take part in the run.
    const auto refused = [&](const std::string& what, const std::vector<std::string>& line,
                             const Opm::Deck* deck = nullptr) {
        bool threw = false;
        try {
            const Command command = parseCli(line);
            prepare(command.cli, nullptr, deck);
        }
        catch (const std::runtime_error&) {
            threw = true;
        }
        report(threw, "refused: " + what);
    };
    refused("no --p", args({"--t", "300"}));
    refused("no fluid", {"--p", "50", "--t", "300"});
    refused("no --z", {"--p", "50", "--t", "300", "--components", "C1,C10"});
    refused("neither --t nor --h", args({"--p", "50"}));
    refused("--t and --h", args({"--p", "50", "--t", "300", "--h", "1000"}));
    refused("--tmin in an isothermal flash", args({"--p", "50", "--t", "300", "--tmin", "260"}));
    refused("--p twice", args({"--p", "50", "--p", "60", "--t", "300"}));
    refused("--p and a p-sweep", args({"--p", "50", "--t", "300", "--sweep", "p=10:20:5"}));
    refused("--tmax above the heat-capacity fit range",
            args({"--p", "50", "--h", "-25000", "--tmax", "900"}));
    refused("--eos-region without --deck", args({"--p", "50", "--t", "300", "--eos-region", "1"}));
    refused("--list-components=x", {"--list-components=x"});
    refused("--z summing to 0.99",
            {"--p", "50", "--t", "300", "--components", "C1,C10", "--z", "0.5,0.49"});
    refused("--z outside [0, 1]",
            {"--p", "50", "--t", "300", "--components", "C1,C10", "--z", "1.2,-0.2"});
    refused("a component twice, through an alias",
            {"--p", "50", "--t", "300", "--components", "C1,METHANE", "--z", "0.5,0.5"});
    refused("--format CSV", args({"--p", "50", "--t", "300", "--format", "CSV"}));
    refused("--eos XYZ", args({"--p", "50", "--t", "300", "--eos", "XYZ"}));
    refused("--flash-two-phase-method xyz",
            args({"--p", "50", "--t", "300", "--flash-two-phase-method", "xyz"}));
    refused("--p abc", args({"--p", "abc", "--t", "300"}));
    refused("--t 300K", args({"--p", "50", "--t", "300K"}));
    refused("--sweep h=1:2:3:4", args({"--p", "50", "--sweep", "h=1:2:3:4"}));
    refused("--kij for a component that is not selected",
            args({"--p", "50", "--t", "300", "--kij", "C1:CO2=0.1"}));
    refused("--cp for a component that is not selected",
            args({"--p", "50", "--t", "300", "--cp", "CO2=1:0:0:0:250:600"}));
    {
        const Opm::Deck deck = Opm::Parser{}.parseString(builtinPairDeck());
        refused("a deck with neither --t nor --h",
                {"--deck", "(selftest)", "--p", "50", "--z", "0.5,0.5"}, &deck);
        refused("--eos-region 2 of 1",
                {"--deck", "(selftest)", "--p", "50", "--t", "30", "--z", "0.5,0.5",
                 "--eos-region", "2"}, &deck);
    }

    // A fractional step reaches the end point.
    {
        const auto points = sweepPoints(parseSweep("t=300:301:0.1"));
        report(points.size() == 11 && std::abs(points.back() - 301.0) < 1e-9,
               "--sweep t=300:301:0.1 ends at 301 K (11 points)");
    }

    std::cout << (failures == 0 ? "opmflash selftest: all passed\n"
                                : "opmflash selftest: failures\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // anonymous namespace

int main(int argc, char** argv)
{
    try {
        if (argc <= 1) {
            printUsage();
            return EXIT_FAILURE;
        }
        const Command command = parseCli(std::vector<std::string>(argv + 1, argv + argc));
        switch (command.action) {
        case Action::Help:
            printUsage();
            return EXIT_SUCCESS;
        case Action::ListComponents:
            listComponents();
            return EXIT_SUCCESS;
        case Action::Selftest:
            return selftest();
        case Action::Run:
            break;
        }
        std::optional<StateFile> states;
        if (command.cli.input) {
            std::ifstream file(*command.cli.input);
            if (!file) {
                throw std::runtime_error("--input: cannot read " + *command.cli.input);
            }
            states = readStateFile(file, *command.cli.input);
        }
        const Run run = prepare(command.cli, states ? &*states : nullptr);
        return dispatch(command.cli, run, ComponentCounts{});
    }
    catch (const std::exception& e) {
        std::cerr << "opmflash: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
