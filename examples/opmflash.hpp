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
/*!
 * \file
 *
 * \brief The parts of the opmflash program: the command line, the fluid,
 *        the flash and the output.
 *
 * parseCli() reads the command line, prepare() checks the request and builds
 * a Run, and runCases() flashes its states and prints them. Nothing after
 * prepare() reads the command line: the Run carries the settings. main() is
 * in examples/opmflash.cpp, the tests are in tests/test_opmflash.cpp, and the
 * options, their units and examples are in printUsage(), which is what
 * opmflash --help prints.
 */
#ifndef OPM_OPMFLASH_HPP
#define OPM_OPMFLASH_HPP

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
#include <istream>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <ostream>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace Opm::OpmFlash {

using EOSType = CompositionalConfig::EOSType;
using HeatCapacity = ComponentCp<double>;

// A component of the flashed mixture: the equation-of-state parameters in the
// units of the fluid system (molar mass kg/mol as from a deck, critical volume
// m3/kmol) and, when known, the ideal-gas heat-capacity polynomial that the
// enthalpies need.
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
    // where the heat capacity comes from, as the header prints it: built-in,
    // as built-in C1 (a deck component), --cp or --cp-file
    std::string heatCapacitySource;
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
inline const std::vector<ComponentEntry>& componentDatabase()
{
    static const std::vector<ComponentEntry> db{
        makeEntry<C1<double>>({"CH4", "METHANE"}),
        makeEntry<C10<double>>({"NC10", "DECANE", "N-DECANE"}),
        makeEntry<SimpleCO2<double>>({"CARBONDIOXIDE"}),
        makeEntry<N2<double>>({"NITROGEN"}),
        makeEntry<H2O<double>>({"WATER"}),
        makeEntry<H2<double>>({"HYDROGEN"}),
    };
    return db;
}

inline std::string upper(std::string_view s)
{
    std::string out(s);
    std::ranges::transform(out, out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

inline std::string lower(std::string_view s)
{
    std::string out(s);
    std::ranges::transform(out, out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

inline std::string_view trim(std::string_view s)
{
    const auto space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && space(s.front())) { s.remove_prefix(1); }
    while (!s.empty() && space(s.back())) { s.remove_suffix(1); }
    return s;
}

// A number with six significant digits, as printf's %g.
inline std::string num(double v)
{
    std::ostringstream os;
    os << std::setprecision(6) << v;
    return os.str();
}

// Whether name, in any case, is the component's name or one of its aliases.
inline bool isNamed(const ComponentEntry& entry, std::string_view name)
{
    const std::string key = upper(name);
    return upper(entry.name) == key ||
           std::ranges::find(entry.aliases, key) != entry.aliases.end();
}

// The built-in component that name denotes, by its name or an alias.
inline const ComponentEntry* findBuiltin(std::string_view name)
{
    const auto& db = componentDatabase();
    const auto it = std::ranges::find_if(db, [name](const ComponentEntry& e) {
        return isNamed(e, name);
    });
    return it != db.end() ? &*it : nullptr;
}

// ---- command line ----------------------------------------------------------
enum class Action { Run, Help, ListComponents };
enum class Spec { PT, PH };               // a temperature or an enthalpy
enum class Format { Table, Csv };

struct KijEntry {
    std::string first, second;            // as given
    double value;
};

struct CpEntry {
    std::string name;                     // as given
    HeatCapacity heatCapacity;
    std::string source;                   // --cp or --cp-file
};

// How the flash runs and what the output shows. prepare() copies these into
// the Run, so that the flash and the output do not need the command line.
struct Settings {
    // The isothermal flash, with the option names and defaults of flow_comp.
    PTFlashMethod twoPhaseMethod = PTFlashMethod::SsiNewton;
    double flashTolerance = 1e-8;
    int flashVerbosity = 0;

    // The isenthalpic flash.
    EnthalpyModel enthalpyModel = EnthalpyModel::eos_departure;
    double temperatureTolerance = 1e-6;   // [K]
    double enthalpyTolerance = 1.0;       // [J/mol]
    int maxIterations = 100;

    // The output.
    Format format = Format::Table;
    bool details = false;                 // the full block for each of several states
};

struct Cli {
    std::set<std::string, std::less<>> given;   // the options on the command line, long names

    // The state: each a value, a list or the points of a range.
    std::vector<double> p;                // [bar]
    std::vector<double> t;                // [K]
    std::vector<double> h;                // [J/mol]

    // The fluid.
    std::optional<std::string> deck;
    std::optional<std::vector<double>> z;
    std::vector<std::string> zNames;      // --z NAME=value: the names, in the order of z
    int eosRegion = 1;
    std::vector<KijEntry> kij;
    std::vector<CpEntry> cp;              // from --cp and --cp-file
    std::optional<EOSType> eos;           // PR unless given; not with --deck

    // The isenthalpic flash's temperature interval.
    std::optional<double> tMin, tMax;     // [K]; the heat-capacity fit range unless given

    Settings settings;

    bool has(std::string_view option) const { return given.contains(option); }
};

struct Command {
    Action action = Action::Run;
    Cli cli;
};

inline std::vector<std::string> split(std::string_view s, char separator)
{
    std::vector<std::string> out;
    for (const auto part : std::views::split(s, separator)) {
        out.emplace_back(part.begin(), part.end());
    }
    return out;
}

// The whole of s as a finite number; the error names the option.
template <class Number>
Number toNumber(std::string_view option, std::string_view s)
{
    Number value{};
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    bool ok = !s.empty() && ec == std::errc{} && end == s.data() + s.size();
    if constexpr (std::is_floating_point_v<Number>) {
        // from_chars reads nan and inf, which no option takes
        ok = ok && std::isfinite(value);
    }
    if (!ok) {
        throw std::runtime_error(std::string(option) + " expects a number, got '" +
                                 std::string(s) + "'");
    }
    return value;
}

inline double toDouble(std::string_view option, std::string_view s)
{
    return toNumber<double>(option, s);
}

inline int toInt(std::string_view option, std::string_view s)
{
    return toNumber<int>(option, s);
}

inline std::vector<double> toDoubles(std::string_view option, std::string_view s)
{
    std::vector<double> values;
    for (const auto& item : split(s, ',')) {
        values.push_back(toDouble(option, item));
    }
    return values;
}

// The values of a state option: a number, a list a,b,c, or a range
// start:stop[:step] whose points include stop; the step is 1 unless given.
// The points of a range are counted once, so a fractional step does not miss
// stop by rounding.
inline std::vector<double> stateValues(const std::string& option, std::string_view s)
{
    if (s.find(':') == std::string_view::npos) {
        return toDoubles(option, s);
    }
    const auto fields = split(s, ':');
    if (fields.size() != 2 && fields.size() != 3) {
        throw std::runtime_error(option + " expects a number, a list a,b,c or a range "
                                 "start:stop[:step], got '" + std::string(s) + "'");
    }
    const double start = toDouble(option, fields[0]);
    const double stop = toDouble(option, fields[1]);
    const double step = fields.size() == 3 ? toDouble(option, fields[2]) : 1.0;
    if (step == 0.0 || (stop - start) * step < 0.0) {
        std::string message = option + " " + std::string(s) + ": the step " + num(step) +
                              " does not lead from " + num(start) + " to " + num(stop);
        if (step > 0.0 && stop < start) {
            message += "; a falling range needs a negative step, e.g. " + num(start) + ":" +
                       num(stop) + ":-1";
        }
        throw std::runtime_error(message);
    }
    const auto n = static_cast<long>(std::floor((stop - start) / step + 1e-9));
    std::vector<double> points;
    for (long i = 0; i <= n; ++i) {
        points.push_back(start + i * step);
    }
    return points;
}

// --z: NAME=fraction for each component.
inline void parseZ(std::string_view s, Cli& cli)
{
    std::vector<double> z;
    for (const auto& item : split(s, ',')) {
        const auto eq = item.find('=');
        if (eq == 0 || eq == std::string::npos) {
            throw std::runtime_error("--z expects NAME=fraction, got '" + item + "'");
        }
        cli.zNames.push_back(item.substr(0, eq));
        z.push_back(toDouble("--z", std::string_view(item).substr(eq + 1)));
    }
    cli.z = std::move(z);
}

// Option values are read in any case: --format CSV, --eos pr.
inline Format formatFromName(std::string_view s)
{
    const std::string key = lower(s);
    if (key == "table") { return Format::Table; }
    if (key == "csv") { return Format::Csv; }
    throw std::runtime_error("--format expects table or csv, got '" + std::string(s) + "'");
}

// The equations of state that the flash has. ZJ is a deck keyword value, but
// the cubic equation of state of the flash does not implement it.
inline EOSType eosFromName(std::string_view s)
{
    EOSType eos{};
    try {
        eos = CompositionalConfig::eosTypeFromString(upper(s));
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error("--eos expects PR, PRCORR, SRK or RK, got '" +
                                 std::string(s) + "'");
    }
    if (eos == EOSType::ZJ) {
        throw std::runtime_error("--eos ZJ: the flash has no ZJ equation of state; "
                                 "use PR, PRCORR, SRK or RK");
    }
    return eos;
}

inline PTFlashMethod methodFromName(std::string_view s)
{
    try {
        return ptFlashMethodFromString(lower(s));
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error("--flash-two-phase-method expects ssi, newton or ssi+newton, "
                                 "got '" + std::string(s) + "'");
    }
}

inline EnthalpyModel enthalpyModelFromName(std::string_view s)
{
    for (const auto model : {EnthalpyModel::caloric, EnthalpyModel::eos_departure}) {
        if (lower(s) == enthalpyModelToString(model)) {
            return model;
        }
    }
    throw std::runtime_error("--enthalpy-model expects caloric or eos_departure, got '" +
                             std::string(s) + "'");
}

inline std::vector<KijEntry> parseKij(std::string_view s)
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
                         toDouble("--kij", std::string_view(item).substr(eq + 1))});
    }
    return table;
}

// A heat capacity from its six numbers c0, c1, c2, c3, tmin, tmax.
inline CpEntry makeCpEntry(const std::string& where, const std::string& name,
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

inline std::vector<CpEntry> parseCp(std::string_view s)
{
    // NAME=c0:c1:c2:c3:tmin:tmax[,NAME=...]
    std::vector<CpEntry> table;
    for (const auto& item : split(s, ',')) {
        const auto eq = item.find('=');
        const auto fields = eq != std::string::npos
                                ? split(std::string_view(item).substr(eq + 1), ':')
                                : std::vector<std::string>{};
        if (eq == 0 || fields.size() != 6) {
            throw std::runtime_error("--cp expects NAME=c0:c1:c2:c3:tmin:tmax[,NAME=...], "
                                     "got '" + item + "'");
        }
        table.push_back(makeCpEntry("--cp", item.substr(0, eq), fields, "--cp"));
    }
    return table;
}

// The non-blank lines of a text file, without '#' comments, with their numbers.
inline std::vector<std::pair<int, std::string>> contentLines(std::istream& in)
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
inline std::vector<CpEntry> readCpFile(std::istream& in, const std::string& name)
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
        table.push_back(makeCpEntry(where, words[0], {words.begin() + 1, words.end()},
                                    "--cp-file"));
    }
    return table;
}

// The canonical long name of a short option, or empty.
inline std::string_view longName(std::string_view shortOption)
{
    static constexpr std::array<std::pair<std::string_view, std::string_view>, 5> forms{{
        {"-h", "--help"},
        {"-P", "--pressure"},
        {"-p", "--pressure"},
        {"-T", "--temperature"},
        {"-H", "--enthalpy"},
    }};
    const auto it = std::ranges::find(forms, shortOption,
                                      &std::pair<std::string_view, std::string_view>::first);
    return it != forms.end() ? it->second : std::string_view{};
}

inline Command parseCli(const std::vector<std::string>& args)
{
    Command command;
    Cli& cli = command.cli;
    for (std::size_t i = 0; i < args.size(); ++i) {
        // --option value, --option=value, or a short form: -P value
        std::string arg = args[i];
        std::optional<std::string> attached;
        if (arg.starts_with("--")) {
            if (const auto eq = arg.find('='); eq != std::string::npos) {
                attached = arg.substr(eq + 1);
                arg.resize(eq);
            }
        }
        else if (!arg.starts_with('-')) {
            throw std::runtime_error("unexpected '" + arg + "': options start with - or -- "
                                     "(see --help)");
        }
        const std::string typed = arg;   // for the messages
        if (const auto name = longName(arg); !name.empty()) {
            arg = name;
        }
        if (!cli.given.insert(arg).second) {
            throw std::runtime_error(typed == arg ? arg + " is given twice"
                                                  : typed + " gives " + arg + " a second time");
        }
        const auto value = [&]() -> std::string {
            if (attached) {
                return *std::exchange(attached, std::nullopt);
            }
            if (i + 1 >= args.size()) {
                throw std::runtime_error("missing value after " + typed);
            }
            return args[++i];
        };
        const auto flag = [&]() {
            if (attached) {
                throw std::runtime_error(typed + " takes no value");
            }
        };
        if (arg == "--help") {
            flag();
            command.action = Action::Help;
            return command;
        }
        else if (arg == "--list-components") {
            flag();
            command.action = Action::ListComponents;
            return command;
        }
        else if (arg == "--pressure") { cli.p = stateValues(typed, value()); }
        else if (arg == "--temperature") { cli.t = stateValues(typed, value()); }
        else if (arg == "--enthalpy") { cli.h = stateValues(typed, value()); }
        else if (arg == "--deck") { cli.deck = value(); }
        else if (arg == "--z") { parseZ(value(), cli); }
        else if (arg == "--eos-region") { cli.eosRegion = toInt(arg, value()); }
        else if (arg == "--kij") { cli.kij = parseKij(value()); }
        else if (arg == "--cp") {
            std::ranges::copy(parseCp(value()), std::back_inserter(cli.cp));
        }
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
            cli.settings.enthalpyModel = enthalpyModelFromName(value());
        }
        else if (arg == "--ph-tmin") { cli.tMin = toDouble(arg, value()); }
        else if (arg == "--ph-tmax") { cli.tMax = toDouble(arg, value()); }
        else if (arg == "--ph-temperature-tolerance") {
            cli.settings.temperatureTolerance = toDouble(arg, value());
        }
        else if (arg == "--ph-enthalpy-tolerance") {
            cli.settings.enthalpyTolerance = toDouble(arg, value());
        }
        else if (arg == "--ph-max-iterations") {
            cli.settings.maxIterations = toInt(arg, value());
        }
        else if (arg == "--flash-two-phase-method") {
            cli.settings.twoPhaseMethod = methodFromName(value());
        }
        else if (arg == "--flash-tolerance") {
            cli.settings.flashTolerance = toDouble(arg, value());
        }
        else if (arg == "--flash-verbosity") {
            cli.settings.flashVerbosity = toInt(arg, value());
        }
        else if (arg == "--format") { cli.settings.format = formatFromName(value()); }
        else if (arg == "--details") { flag(); cli.settings.details = true; }
        else {
            throw std::runtime_error("unknown option '" + typed + "' (see --help)");
        }
        if (attached) {
            throw std::runtime_error(typed + " takes no value");
        }
    }
    return command;
}

// The flash that the request asks for, and the checks that need no fluid. A
// temperature asks for the isothermal flash and an enthalpy for the
// isenthalpic one; every option given has to take part in the run.
inline Spec checkRequest(const Cli& cli)
{
    if (cli.p.empty()) {
        throw std::runtime_error("the pressure is required: -P (or -p, --pressure) [bar]");
    }
    const bool temperature = !cli.t.empty();
    const bool enthalpy = !cli.h.empty();
    if (temperature && enthalpy) {
        throw std::runtime_error("give a temperature (-T, the isothermal flash) or an "
                                 "enthalpy (-H, the isenthalpic flash), not both");
    }
    if (!temperature && !enthalpy) {
        throw std::runtime_error("give -T [K] for the isothermal flash or -H [J/mol] for "
                                 "the isenthalpic flash");
    }
    const Spec spec = enthalpy ? Spec::PH : Spec::PT;
    if (!cli.deck && cli.zNames.empty()) {
        throw std::runtime_error("give the fluid: --z by name (e.g. --z C1=0.5,C10=0.5, see "
                                 "--list-components) or --deck");
    }
    if (cli.deck && cli.eos) {
        throw std::runtime_error("--eos does not apply with --deck: the fluid is the deck's, "
                                 "whose OMEGAA, OMEGAB, BIC and SSHIFT belong to its equation "
                                 "of state");
    }
    if (!cli.deck && cli.has("--eos-region")) {
        throw std::runtime_error("--eos-region needs --deck");
    }
    if (spec == Spec::PT) {
        for (const auto* option : {"--ph-tmin", "--ph-tmax", "--ph-temperature-tolerance",
                                   "--ph-enthalpy-tolerance", "--ph-max-iterations",
                                   "--enthalpy-model"}) {
            if (cli.has(option)) {
                throw std::runtime_error(std::string(option) + " applies to the isenthalpic "
                                         "flash (-H) only");
            }
        }
    }
    if (cli.settings.details && cli.settings.format == Format::Csv) {
        throw std::runtime_error("--details applies to --format table; the CSV has every "
                                 "column");
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
    std::vector<double> z;                // in the order of the components
    std::string zNote;                    // set when z was normalised
    EOSType eos;
    std::string eosSource;
    std::array<double, 5> lbc = CompositionalConfig::defaultLBCCoefficients();
    std::optional<std::string> deck;      // the file of the fluid, for the header
};

// The names of the components, in their order: CO2, C1, C10.
inline std::string componentList(const Mixture& mixture)
{
    std::string names;
    for (const auto& e : mixture.components) {
        names += (names.empty() ? "" : ", ") + e.name;
    }
    return names;
}

// The built-in components that --z names, in its order.
inline Mixture builtinMixture(const Cli& cli)
{
    Mixture mixture;
    for (const auto& name : cli.zNames) {
        const ComponentEntry* entry = findBuiltin(name);
        if (!entry) {
            throw std::runtime_error("--z names " + name + ", which is not a built-in "
                                     "component (see --list-components; other fluids come "
                                     "from a deck with --deck)");
        }
        if (std::ranges::find(mixture.components, entry->name, &ComponentEntry::name) !=
            mixture.components.end()) {
            throw std::runtime_error("--z names " + entry->name + " twice ('" + name + "')");
        }
        mixture.components.push_back(*entry);
    }
    const auto n = mixture.components.size();
    mixture.bic.assign(n * (n - 1) / 2, 0.0);
    mixture.eos = cli.eos.value_or(EOSType::PR);
    mixture.eosSource = cli.eos ? "--eos" : "default";
    return mixture;
}

// The fluid of one equation-of-state region of the deck, as the compositional
// simulator builds it (compositionalRegionParams). A component with the name
// and the molar mass of a built-in one has its heat capacity; a lumped
// component of another molar mass under the same name has none.
inline Mixture deckMixture(const Deck& deck, const Runspec& runspec, const Cli& cli)
{
    const CompositionalConfig config(deck, runspec);
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
    const std::string regionText = "EOS region " + std::to_string(cli.eosRegion) + " of " +
                                   std::to_string(regions);
    const EOSType eos = config.eosType(region);
    if (eos == EOSType::ZJ) {
        throw std::runtime_error("the deck's equation of state is ZJ (" + regionText + "), "
                                 "which the flash does not have; it has PR, PRCORR, SRK and RK");
    }
    const auto params = compositionalRegionParams<double>(config, region);
    Mixture mixture;
    for (const auto& c : params.components) {
        const ComponentEntry* builtin = findBuiltin(c.name);
        const bool sameComponent =
            builtin && std::abs(c.molar_mass - builtin->molarMass) <= 0.01 * builtin->molarMass;
        mixture.components.push_back(
            {c.name, {}, c.molar_mass, c.critic_temp, c.critic_pres, c.critic_vol,
             c.acentric_factor, c.volume_shift, c.omega_a, c.omega_b,
             sameComponent ? builtin->heatCapacity : std::nullopt,
             sameComponent ? "as built-in " + builtin->name : ""});
    }
    const auto n = mixture.components.size();
    mixture.bic = params.interaction_coefficients;
    if (mixture.bic.empty()) {
        mixture.bic.assign(n * (n - 1) / 2, 0.0);
    }
    mixture.eos = eos;
    mixture.eosSource = regionText;
    mixture.lbc = params.lbc_coefficients;
    mixture.deck = cli.deck;
    return mixture;
}

inline Deck readDeck(const std::string& path)
{
    ParseContext parseContext(InputErrorAction::WARN);
    ErrorGuard errors;
    auto deck = Parser{}.parseFile(path, parseContext, errors);
    if (errors) {
        errors.dump();
        errors.clear();
        throw std::runtime_error("the deck " + path + " has input errors");
    }
    return deck;
}

inline std::size_t componentIndex(const Mixture& mixture, std::string_view option,
                                  std::string_view name)
{
    const auto it = std::ranges::find_if(mixture.components, [name](const ComponentEntry& e) {
        return isNamed(e, name);
    });
    if (it == mixture.components.end()) {
        throw std::runtime_error(std::string(option) + " names " + std::string(name) +
                                 ", which is not among the components: " +
                                 componentList(mixture));
    }
    return static_cast<std::size_t>(it - mixture.components.begin());
}

// A feed: one mole fraction per component, in [0, 1], whose sum is 1 within
// 1e-3 (fractions rounded to four decimals), normalised. The note says when
// the sum was not 1.
inline std::vector<double> normalisedFeed(const std::string& where, std::vector<double> z,
                                          std::string* note = nullptr)
{
    if (std::ranges::any_of(z, [](double v) { return !(v >= 0.0 && v <= 1.0); })) {
        throw std::runtime_error(where + " values must lie in [0, 1]");
    }
    const double sum = std::reduce(z.begin(), z.end());
    if (std::abs(sum - 1.0) > 1e-3) {
        throw std::runtime_error(where + " sums to " + num(sum) + ", not 1");
    }
    std::ranges::transform(z, z.begin(), [sum](double v) { return v / sum; });
    if (note && std::abs(sum - 1.0) > 1e-12) {
        *note = "z normalised from a sum of " + num(sum);
    }
    return z;
}

// The fractions of --z NAME=fraction in the order of the components; every
// component once.
inline std::vector<double> namedFeed(const Mixture& mixture, const Cli& cli)
{
    std::vector<std::optional<double>> byComponent(mixture.components.size());
    for (std::size_t i = 0; i < cli.zNames.size(); ++i) {
        const auto c = componentIndex(mixture, "--z", cli.zNames[i]);
        if (byComponent[c]) {
            throw std::runtime_error("--z names " + mixture.components[c].name + " twice ('" +
                                     cli.zNames[i] + "')");
        }
        byComponent[c] = (*cli.z)[i];
    }
    std::vector<double> z;
    for (std::size_t c = 0; c < byComponent.size(); ++c) {
        if (!byComponent[c]) {
            throw std::runtime_error("--z gives no mole fraction for " +
                                     mixture.components[c].name + "; it needs one for each "
                                     "component: " + componentList(mixture));
        }
        z.push_back(*byComponent[c]);
    }
    return z;
}

// The command line's interaction coefficients, heat capacities and feed on
// the mixture.
inline void applyCommandLine(Mixture& mixture, const Cli& cli)
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

    if (!cli.z) {
        std::string example;
        for (const auto& e : mixture.components) {
            example += (example.empty() ? "" : ",") + e.name + "=<z>";
        }
        throw std::runtime_error("--z is required: one mole fraction per component, by name "
                                 "(--z " + example + ")");
    }
    mixture.z = normalisedFeed("--z", namedFeed(mixture, cli), &mixture.zNote);
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

// Everything that the flash and the output need.
struct Run {
    Spec spec;
    Mixture mixture;
    std::vector<State> states;
    std::optional<FitRange> fit;          // none without a heat capacity for every component
    FitRange search{};                    // the isenthalpic flash's temperature interval [K]
    Settings settings;
};

inline std::string componentsWithoutHeatCapacity(const Mixture& mixture)
{
    std::string names;
    for (const auto& e : mixture.components) {
        if (!e.heatCapacity) {
            names += (names.empty() ? "" : ", ") + e.name;
        }
    }
    return names;
}

inline std::string rangeText(const FitRange& range)
{
    return num(range.min) + " to " + num(range.max) + " K";
}

inline std::optional<FitRange> fitRange(const Mixture& mixture)
{
    FitRange range{-std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity()};
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

// The states of the run: every pressure with every temperature or enthalpy,
// the pressure in the outer loop.
inline std::vector<State> statesOf(const Cli& cli, Spec spec, const Mixture& mixture)
{
    std::vector<State> states;
    for (const double p : cli.p) {
        for (const double value : spec == Spec::PH ? cli.h : cli.t) {
            states.push_back({p * pascalPerBar, value, mixture.z});
        }
    }
    return states;
}

// Everything that can be checked before a flash: the request, the fluid, the
// states, and the temperatures of the enthalpies. The deck, when given, is
// read here unless the caller passes it.
inline Run prepare(const Cli& cli, const Deck* deck = nullptr)
{
    Run run;
    run.spec = checkRequest(cli);
    run.settings = cli.settings;
    std::optional<Deck> parsed;
    if (cli.deck && !deck) {
        parsed = readDeck(*cli.deck);
        deck = &*parsed;
    }
    if (deck) {
        const Runspec runspec(*deck);
        run.mixture = deckMixture(*deck, runspec, cli);
    }
    else {
        run.mixture = builtinMixture(cli);
    }
    applyCommandLine(run.mixture, cli);
    run.states = statesOf(cli, run.spec, run.mixture);

    run.fit = fitRange(run.mixture);
    if (!run.fit && run.spec == Spec::PH) {
        throw std::runtime_error("the isenthalpic flash needs the ideal-gas heat capacity of "
                                 "every component; give it with --cp or --cp-file for " +
                                 componentsWithoutHeatCapacity(run.mixture));
    }
    // The enthalpies are zero for the ideal gas at the reference temperature,
    // so the heat-capacity fit of every component has to hold there.
    constexpr double tRef = HeatCapacity::referenceTemperature();
    if (run.fit && (tRef < run.fit->min || tRef > run.fit->max)) {
        throw std::runtime_error("the reference temperature " + num(tRef) + " K lies outside "
                                 "the heat-capacity fit range " + rangeText(*run.fit) +
                                 ": the fit coefficients are not meant for it");
    }
    if (run.spec == Spec::PH) {
        run.search = {cli.tMin.value_or(run.fit->min), cli.tMax.value_or(run.fit->max)};
        if (run.search.min < run.fit->min - 1e-9 || run.search.max > run.fit->max + 1e-9) {
            throw std::runtime_error("--ph-tmin and --ph-tmax must lie inside the "
                                     "heat-capacity fit range " + rangeText(*run.fit));
        }
        if (!(run.search.min < run.search.max)) {
            throw std::runtime_error("--ph-tmin must lie below --ph-tmax");
        }
    }
    return run;
}

// ---- the flash, for a compile-time number of components --------------------
template <int N>
struct FlashRunner {
    using FluidSystem = GenericOilGasWaterFluidSystem<double, N, false>;
    using FlashEval = DenseAd::Evaluation<double, N + 1>;
    using FluidState = CompositionalFluidState<FlashEval, FluidSystem>;
    using ParameterCache = typename FluidSystem::template ParameterCache<FlashEval>;
    using PtFlash = PTFlash<double, FluidSystem>;
    using PhFlash = PHFlash<double, FluidSystem>;
    using Enthalpy = MixtureEnthalpy<double, FluidSystem>;

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
    static inline std::optional<CpTable<double, N>> cpTable_;

    static void setup(const Mixture& mixture)
    {
        FluidSystem::init();
        for (const auto& e : mixture.components) {
            FluidSystem::addComponent(typename FluidSystem::ComponentParam{
                e.name, e.molarMass, e.criticalTemperature, e.criticalPressure,
                e.criticalVolume, e.acentricFactor, e.volumeShift, e.omegaA, e.omegaB});
        }
        FluidSystem::setEosType(mixture.eos);
        FluidSystem::setInteractionCoefficients(mixture.bic);
        FluidSystem::setLbcCoefficients(mixture.lbc);
        cpTable_.reset();
        if (std::ranges::all_of(mixture.components, [](const ComponentEntry& e) {
                return e.heatCapacity.has_value();
            })) {
            CpTable<double, N> table;
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

    static typename PhFlash::Config phConfig(const Settings& settings, const FitRange& search)
    {
        typename PhFlash::Config cfg;
        cfg.cpTable = *cpTable_;
        cfg.model = settings.enthalpyModel;
        cfg.tempMin = search.min;
        cfg.tempMax = search.max;
        cfg.temperatureTolerance = settings.temperatureTolerance;
        cfg.enthalpyTolerance = settings.enthalpyTolerance;
        cfg.maxIterations = settings.maxIterations;
        return cfg;
    }

    static std::optional<Phase> phase(const FluidState& fs, unsigned phaseIdx)
    {
        try {
            ParameterCache cache(fs.eosRegionIndex());
            cache.updatePhase(fs, phaseIdx);
            const double p = getValue(fs.pressure(phaseIdx));
            const double t = getValue(fs.temperature(phaseIdx));
            return Phase{getValue(FluidSystem::density(fs, cache, phaseIdx)),
                         getValue(FluidSystem::viscosity(fs, cache, phaseIdx)),
                         p * getValue(cache.molarVolume(phaseIdx)) / (Constants<double>::R * t)};
        }
        catch (const std::runtime_error&) {
            return std::nullopt;
        }
    }

    // The enthalpies need a converged state, the heat capacities (fit) and a
    // temperature inside their fit range.
    static Result collect(const FluidState& fs, bool singlePhase, bool ok,
                          const std::optional<FitRange>& fit)
    {
        Result r{};
        r.ok = ok;
        r.singlePhase = singlePhase;
        r.temperature = getValue(fs.temperature(0));
        r.L = getValue(fs.L());
        for (int c = 0; c < N; ++c) {
            r.x[c] = getValue(fs.moleFraction(FluidSystem::oilPhaseIdx, c));
            r.y[c] = getValue(fs.moleFraction(FluidSystem::gasPhaseIdx, c));
        }
        if (r.hasLiquid()) { r.liquid = phase(fs, FluidSystem::oilPhaseIdx); }
        if (r.hasVapour()) { r.vapour = phase(fs, FluidSystem::gasPhaseIdx); }
        if (ok && fit && r.temperature >= fit->min && r.temperature <= fit->max) {
            r.hCaloric = getValue(Enthalpy::mixtureEnthalpy(fs, *cpTable_,
                                                            EnthalpyModel::caloric));
            r.hDeparture = getValue(Enthalpy::mixtureEnthalpy(fs, *cpTable_,
                                                              EnthalpyModel::eos_departure));
        }
        return r;
    }

    static Result runPt(const Run& run, const State& state)
    {
        const auto& settings = run.settings;
        FluidState fs = makeState(state.p, state.value, state.z);
        // A state that the flash cannot solve is reported with ok = 0, as for
        // the isenthalpic flash, and the run goes on to the next one.
        bool ok = true;
        bool singlePhase = false;
        try {
            singlePhase = PtFlash::solve(fs, settings.twoPhaseMethod, settings.flashTolerance,
                                         settings.flashVerbosity);
        }
        catch (const std::runtime_error&) {
            ok = false;
        }
        catch (const Dune::FMatrixError&) {
            ok = false;
        }
        return collect(fs, singlePhase, ok, run.fit);
    }

    static Result runPh(const Run& run, const State& state, const FitRange& search)
    {
        const auto& settings = run.settings;
        // the initial temperature only seeds the state; PHFlash searches over T
        FluidState fs = makeState(state.p, 0.5 * (search.min + search.max), state.z);
        const bool ok = PhFlash::solve(fs, state.value, phConfig(settings, search),
                                       settings.twoPhaseMethod, settings.flashTolerance,
                                       settings.flashVerbosity);
        const bool singlePhase = !(getValue(fs.L()) > 0.0 && getValue(fs.L()) < 1.0);
        return collect(fs, singlePhase, ok, run.fit);
    }

    static Result runState(const Run& run, const State& state)
    {
        return run.spec == Spec::PT ? runPt(run, state) : runPh(run, state, run.search);
    }
};

// ---- output ----------------------------------------------------------------
inline std::string phaseLabel(bool singlePhase, double L)
{
    return !singlePhase ? "two-phase" : L > 0.5 ? "single-phase liquid" : "single-phase vapour";
}

// The enthalpy model that a state's H is reported with: the one that --enthalpy
// is matched against, and eos_departure for the isothermal flash.
inline EnthalpyModel reportedModel(const Run& run)
{
    return run.spec == Spec::PH ? run.settings.enthalpyModel : EnthalpyModel::eos_departure;
}

inline std::string referenceText()
{
    return "zero for the ideal gas at " + num(HeatCapacity::referenceTemperature()) + " K";
}

inline std::string noHeatCapacityText(const Run& run)
{
    return "H: none, no ideal-gas heat capacity for " +
           componentsWithoutHeatCapacity(run.mixture) + " (give it with --cp or --cp-file)";
}

// The line of a state's enthalpy: the reported model first, then the other
// one, and the reference; or why there is none.
inline std::string enthalpyLine(const Run& run, std::optional<double> caloric,
                                std::optional<double> departure)
{
    if (!run.fit) {
        return noHeatCapacityText(run);
    }
    if (!caloric || !departure) {
        return "H: none, T is outside the heat-capacity fit range " + rangeText(*run.fit);
    }
    if (reportedModel(run) == EnthalpyModel::eos_departure) {
        return "H = " + num(*departure) + " J/mol   (eos_departure; ideal gas only: " +
               num(*caloric) + "; " + referenceText() + ")";
    }
    return "H = " + num(*caloric) + " J/mol   (caloric, the ideal gas only; eos_departure: " +
           num(*departure) + "; " + referenceText() + ")";
}

inline std::string heatCapacityText(const ComponentEntry& e)
{
    if (!e.heatCapacity) {
        return "none (give it with --cp or --cp-file)";
    }
    return e.heatCapacitySource + ", " + num(e.heatCapacity->minTemperature) + " to " +
           num(e.heatCapacity->maxTemperature) + " K";
}

inline void printHeader(std::ostream& out, const Run& run)
{
    const auto& settings = run.settings;
    const bool csv = settings.format == Format::Csv;
    const std::string prefix = csv ? "# " : "";
    const auto& m = run.mixture;
    out << prefix << "opmflash: "
        << (run.spec == Spec::PT ? "isothermal flash (P-T)" : "isenthalpic flash (P-H)")
        << ", equation of state " << CompositionalConfig::eosTypeToString(m.eos) << " ("
        << m.eosSource << "), " << ptFlashMethodToString(settings.twoPhaseMethod)
        << ", flash tolerance " << num(settings.flashTolerance) << '\n';
    if (run.spec == Spec::PH) {
        out << prefix << "temperature search " << rangeText(run.search) << ", enthalpy model "
            << enthalpyModelToString(settings.enthalpyModel) << '\n';
    }
    if (csv) {
        out << prefix << "units: p bar, T K, h and H J/mol ("
            << (run.fit ? referenceText() : "no enthalpies") << "), density kg/m3, "
            << "viscosity cP\n";
    }

    // one line per component: molar mass and ideal-gas heat capacity
    const std::string title = m.deck ? "components of " + *m.deck : "components";
    std::size_t width = title.size();
    for (const auto& e : m.components) {
        width = std::max(width, e.name.size() + 2);
    }
    const int nameWidth = static_cast<int>(width);
    out << prefix << "  " << std::left << std::setw(nameWidth) << title << std::right
        << "  molar mass [g/mol]   ideal-gas heat capacity\n";
    for (const auto& e : m.components) {
        out << prefix << "  " << std::left << std::setw(nameWidth) << e.name << std::right
            << std::setw(20) << num(e.molarMass * 1e3) << "   " << heatCapacityText(e) << '\n';
    }
    if (m.deck) {
        out << prefix << "The ideal-gas heat capacity is that of the built-in component with "
                         "the same\n"
            << prefix << "name or alias and molar mass (within 1 %); everything else is the "
                         "deck's.\n";
    }
    if (!m.zNote.empty()) {
        out << prefix << m.zNote << '\n';
    }

    if (csv) {
        const auto columns = [&](std::string_view head) {
            for (const auto& e : m.components) {
                out << ',' << head << '_' << e.name;
            }
        };
        out << "p,T,h_spec";
        columns("z");
        out << ",phases,L";
        columns("x");
        columns("y");
        columns("K");
        out << ",density_liquid,density_vapour,viscosity_liquid,viscosity_vapour,"
               "Z_liquid,Z_vapour,H_caloric,H_departure,ok\n";
    }
    else {
        out << '\n';
    }
}

// A failed state leaves every result field empty in the CSV, including the
// temperature of the isenthalpic flash, which is then the last trial; so does
// a phase that is not there.
template <int N>
void printCsvRow(std::ostream& out, const Run& run, const State& state,
                 const typename FlashRunner<N>::Result& r)
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
    out << field(state.p / pascalPerBar) << ',' << field(when(validT, r.temperature)) << ','
        << field(when(run.spec == Spec::PH, state.value));
    for (const double v : state.z) {
        out << ',' << field(v);
    }
    out << ',' << (r.ok ? phaseLabel(r.singlePhase, r.L) : "") << ',' << field(when(r.ok, r.L));
    for (int c = 0; c < N; ++c) { out << ',' << field(when(r.hasLiquid(), r.x[c])); }
    for (int c = 0; c < N; ++c) { out << ',' << field(when(r.hasVapour(), r.y[c])); }
    for (int c = 0; c < N; ++c) {
        const bool both = r.ok && !r.singlePhase && r.x[c] > 0.0;
        out << ',' << field(when(both, both ? r.y[c] / r.x[c] : 0.0));
    }
    // a property of a phase in the output units, empty without the phase
    using Phase = typename FlashRunner<N>::Phase;
    const auto property = [&](const std::optional<Phase>& phase, double Phase::*member,
                              double unit) {
        return field(phase ? std::optional((*phase).*member / unit) : std::nullopt);
    };
    out << ',' << property(r.liquid, &Phase::density, 1.0)
        << ',' << property(r.vapour, &Phase::density, 1.0)
        << ',' << property(r.liquid, &Phase::viscosity, pascalSecondPerCentipoise)
        << ',' << property(r.vapour, &Phase::viscosity, pascalSecondPerCentipoise)
        << ',' << property(r.liquid, &Phase::compressibility, 1.0)
        << ',' << property(r.vapour, &Phase::compressibility, 1.0)
        << ',' << field(r.hCaloric) << ',' << field(r.hDeparture)
        << ',' << (r.ok ? 1 : 0) << '\n';
}

inline std::string noSolutionText(const Run& run, const State& state, double temperature)
{
    const std::string pText = num(state.p / pascalPerBar) + " bar";
    if (run.spec == Spec::PT) {
        return "NO SOLUTION at p = " + pText + ", T = " + num(temperature) +
               " K: the isothermal flash did not converge";
    }
    std::string text = "NO SOLUTION: no temperature in " + rangeText(run.search) +
                       " gives h = " + num(state.value) + " J/mol at p = " + pText;
    if (run.fit && (run.search.min > run.fit->min || run.search.max < run.fit->max)) {
        text += "; --ph-tmin and --ph-tmax may extend to " + rangeText(*run.fit);
    }
    return text;
}

// The full block of a state: the phase split, the compositions, the phase
// properties and the enthalpy.
template <int N>
void printDetails(std::ostream& out, const Run& run, const State& state,
                  const typename FlashRunner<N>::Result& r)
{
    const auto& m = run.mixture;
    if (!r.ok) {
        out << "  " << noSolutionText(run, state, r.temperature) << "\n\n";
        return;
    }
    out << "  p = " << num(state.p / pascalPerBar) << " bar   T = " << num(r.temperature) << " K";
    if (run.spec == Spec::PH) {
        out << " (solved for h = " << num(state.value) << " J/mol)";
    }
    out << "\n  " << phaseLabel(r.singlePhase, r.L) << "   L = " << num(r.L) << '\n';

    // one row per component: z, and the composition of each phase present
    std::size_t width = std::string_view("component").size();
    for (const auto& e : m.components) {
        width = std::max(width, e.name.size());
    }
    const int nameWidth = static_cast<int>(width) + 2;
    out << "    " << std::left << std::setw(nameWidth) << "component" << std::right
        << std::setw(13) << 'z';
    if (r.hasLiquid()) { out << std::setw(13) << 'x'; }
    if (r.hasVapour()) { out << std::setw(13) << 'y'; }
    if (!r.singlePhase) { out << std::setw(13) << 'K'; }
    out << '\n';
    for (int c = 0; c < N; ++c) {
        out << "    " << std::left << std::setw(nameWidth) << m.components[c].name
            << std::right << std::setw(13) << num(state.z[c]);
        if (r.hasLiquid()) { out << std::setw(13) << num(r.x[c]); }
        if (r.hasVapour()) { out << std::setw(13) << num(r.y[c]); }
        if (!r.singlePhase) {
            out << std::setw(13) << (r.x[c] > 0.0 ? num(r.y[c] / r.x[c]) : "-");
        }
        out << '\n';
    }

    // one row per phase present: density, viscosity, Z
    out << "    " << std::left << std::setw(nameWidth) << "phase" << std::right
        << std::setw(17) << "density [kg/m3]" << std::setw(17) << "viscosity [cP]"
        << std::setw(10) << 'Z' << '\n';
    const auto phaseRow = [&](const char* name, const auto& phase) {
        out << "    " << std::left << std::setw(nameWidth) << name << std::right;
        if (phase) {
            out << std::setw(17) << num(phase->density)
                << std::setw(17) << num(phase->viscosity / pascalSecondPerCentipoise)
                << std::setw(10) << num(phase->compressibility) << '\n';
        }
        else {
            out << "  (no properties: the phase volume is not positive)\n";
        }
    };
    if (r.hasLiquid()) { phaseRow("liquid", r.liquid); }
    if (r.hasVapour()) { phaseRow("vapour", r.vapour); }

    out << "  " << enthalpyLine(run, r.hCaloric, r.hDeparture) << "\n\n";
}

// The compact table: one line per state. For the isothermal flash the columns
// are p, T, the phases, L, the densities and viscosities of the phases and H;
// for the isenthalpic flash p, h, the solved T and the same, without H.
inline void printTableHead(std::ostream& out, const Run& run)
{
    const bool ph = run.spec == Spec::PH;
    if (ph) {
        out << "h: " << enthalpyModelToString(run.settings.enthalpyModel) << ", "
            << referenceText() << "\n";
    }
    else if (run.fit) {
        out << "H: eos_departure, " << referenceText() << "; none outside "
            << rangeText(*run.fit) << "\n";
    }
    else {
        out << noHeatCapacityText(run) << "\n";
    }
    // the widths of printTableLine
    out << std::setw(10) << 'p';
    if (ph) { out << std::setw(11) << 'h'; }
    out << std::setw(10) << 'T' << "  " << std::left << std::setw(8) << "phases" << std::right
        << std::setw(11) << 'L' << std::setw(20) << "density [kg/m3]" << std::setw(22)
        << "viscosity [cP]";
    if (!ph) { out << std::setw(12) << 'H'; }
    out << '\n' << std::setw(10) << "[bar]";
    if (ph) { out << std::setw(11) << "[J/mol]"; }
    out << std::setw(10) << "[K]" << std::string(21, ' ') << std::setw(10) << "liquid"
        << std::setw(10) << "vapour" << std::setw(11) << "liquid" << std::setw(11) << "vapour";
    if (!ph) { out << std::setw(12) << "[J/mol]"; }
    out << '\n';
}

template <int N>
void printTableLine(std::ostream& out, const Run& run, const State& state,
                    const typename FlashRunner<N>::Result& r)
{
    const auto cell = [&out](std::optional<double> v, int width) {
        out << std::setw(width) << (v ? num(*v) : std::string("-"));
    };
    const auto when = [](bool present, double v) {
        return present ? std::optional(v) : std::nullopt;
    };
    const bool ph = run.spec == Spec::PH;
    cell(state.p / pascalPerBar, 10);
    if (ph) {
        cell(state.value, 11);
    }
    cell(when(r.ok || !ph, r.temperature), 10);
    const std::string phases = !r.ok ? "failed" : !r.singlePhase ? "two" : r.L > 0.5 ? "liquid"
                                                                                      : "vapour";
    out << "  " << std::left << std::setw(8) << phases << std::right;
    cell(when(r.ok, r.L), 11);
    cell(r.liquid ? std::optional(r.liquid->density) : std::nullopt, 10);
    cell(r.vapour ? std::optional(r.vapour->density) : std::nullopt, 10);
    cell(r.liquid ? std::optional(r.liquid->viscosity / pascalSecondPerCentipoise)
                  : std::nullopt, 11);
    cell(r.vapour ? std::optional(r.vapour->viscosity / pascalSecondPerCentipoise)
                  : std::nullopt, 11);
    if (!ph) {
        cell(r.hDeparture, 12);
    }
    out << '\n';
}

inline void printFailures(std::ostream& out, const Run& run)
{
    if (run.spec == Spec::PT) {
        out << "\nfailed: the isothermal flash did not converge\n";
        return;
    }
    out << "\nfailed: no temperature in " << rangeText(run.search) << " gives h";
    if (run.fit && (run.search.min > run.fit->min || run.search.max < run.fit->max)) {
        out << "; --ph-tmin and --ph-tmax may extend to " << rangeText(*run.fit);
    }
    out << '\n';
}

// Flashes the states of the run and prints them: the full block of a single
// state (or of each with --details), one line per state for several, or the
// CSV. The exit status is EXIT_FAILURE when a state has no solution.
template <int N>
int runCases(std::ostream& out, const Run& run)
{
    using Runner = FlashRunner<N>;
    Runner::setup(run.mixture);
    printHeader(out, run);
    const bool csv = run.settings.format == Format::Csv;
    const bool compact = !csv && !run.settings.details && run.states.size() > 1;
    if (compact) {
        printTableHead(out, run);
    }
    bool allOk = true;
    for (const auto& state : run.states) {
        const auto r = Runner::runState(run, state);
        if (csv) {
            printCsvRow<N>(out, run, state, r);
        }
        else if (compact) {
            printTableLine<N>(out, run, state, r);
        }
        else {
            printDetails<N>(out, run, state, r);
        }
        allOk = allOk && r.ok;
    }
    if (compact && !allOk) {
        printFailures(out, run);
    }
    return allOk ? EXIT_SUCCESS : EXIT_FAILURE;
}

// ---- help --------------------------------------------------------------------
// The few lines that opmflash prints without arguments.
inline void printShortUsage(std::ostream& out)
{
    out <<
        "opmflash: the isothermal (P-T) or isenthalpic (P-H) flash of a mixture\n"
        "\n"
        "  opmflash -P 50 -T 300 --z C1=0.5,C10=0.5       the state at 50 bar and 300 K\n"
        "  opmflash -P 50 -H -25000 --z C1=0.5,C10=0.5    the temperature of -25000 J/mol\n"
        "  opmflash -P 100 -T 350 --deck CASE.DATA --z CO2=0.5,METHANE=0.3,DECANE=0.2\n"
        "\n"
        "Units: -P bar, -T K, -H J/mol (per mole). All options: opmflash --help\n";
}

inline void printUsage(std::ostream& out)
{
    out <<
        "opmflash: flash a compositional state from the command line\n"
        "\n"
        "Runs the isothermal flash (P-T, give -T) or the isenthalpic flash (P-H,\n"
        "give -H, which finds the temperature) with the fluid system and the\n"
        "flash solvers of the compositional simulator, on built-in components or\n"
        "on the fluid of a deck.\n"
        "\n"
        "usage:\n"
        "  opmflash -P <bar> -T <K>     --z <NAME=z,...> [options]\n"
        "  opmflash -P <bar> -H <J/mol> --z <NAME=z,...> [options]\n"
        "  opmflash -P <bar> -T <K> | -H <J/mol> --deck <file> --z <NAME=z,...> [options]\n"
        "  opmflash --list-components\n"
        "  opmflash -h | --help\n"
        "\n"
        "examples:\n"
        "  # phase split, phase properties and enthalpy at 50 bar and 300 K\n"
        "  opmflash -P 50 -T 300 --z C1=0.5,C10=0.5 --kij C1:C10=0.0411\n"
        "\n"
        "  # the temperature at which the mixture has the enthalpy -25000 J/mol\n"
        "  opmflash -P 50 -H -25000 --z C1=0.5,C10=0.5 --kij C1:C10=0.0411\n"
        "\n"
        "  # several states: one line each; --format csv for a spreadsheet\n"
        "  opmflash -P 50:300:50 -T 300 --z C1=0.5,C10=0.5\n"
        "  opmflash -P 50 -H -30000:5000:1000 --z C1=0.5,C10=0.5 --format csv\n"
        "\n"
        "  # a map: every pressure with every temperature\n"
        "  opmflash -P 10:200:10 -T 250:450:10 --z C1=0.5,C10=0.5 --format csv\n"
        "\n"
        "  # the fluid of a deck; the mole fractions by the deck's names\n"
        "  opmflash -P 100 -T 350 --deck CASE.DATA --z CO2=0.5,METHANE=0.3,DECANE=0.2\n"
        "\n"
        "Units, always, also with --deck: pressure bar (not Pa), temperature K\n"
        "(not deg C), enthalpy J/mol (per mole, not per kg), density kg/m3,\n"
        "viscosity cP.\n"
        "\n"
        "the state; each takes a number, a list a,b,c or a range start:stop[:step]\n"
        "(the step is 1 unless given; the range includes stop); values in both p\n"
        "and T (or h) give every pressure with every temperature (or enthalpy):\n"
        "  -P, -p, --pressure      pressure [bar]                        [required]\n"
        "  -T, --temperature       temperature [K]: the isothermal flash\n"
        "  -H, --enthalpy          molar enthalpy [J/mol]: the isenthalpic flash,\n"
        "                          which finds the temperature with this enthalpy\n"
        "\n"
        "the fluid:\n"
        "  --z NAME=z[,...]        feed mole fractions in [0, 1], one per component,\n"
        "                          by name; a sum within 1e-3 of 1 is normalised.\n"
        "                          Without --deck the names select the built-in\n"
        "                          components, 2 to 10 (see --list-components)\n"
        "  --deck <file>           the fluid of a compositional deck: its components,\n"
        "                          equation of state with its parameters, interaction\n"
        "                          and viscosity coefficients. The state still comes\n"
        "                          from the command line, in the units above\n"
        "  --eos-region <n>        with --deck: the equation-of-state region      [1]\n"
        "  --eos                   PR | PRCORR | SRK | RK; not with --deck,\n"
        "                          which has its own                            [PR]\n"
        "  --kij A:B=v[,C:D=v...]  binary interaction coefficients; the others are\n"
        "                          zero, or the deck's\n"
        "  --cp NAME=c0:c1:c2:c3:tmin:tmax[,...]\n"
        "                          ideal-gas heat capacity c0 + c1 T + c2 T^2 + c3 T^3\n"
        "                          [J/(mol K)], valid from tmin to tmax [K]\n"
        "  --cp-file <file>        the same, one component per line:\n"
        "                          NAME c0 c1 c2 c3 tmin tmax\n"
        "\n"
        "Enthalpies need the ideal-gas heat capacity of every component. Built-in\n"
        "components have it, and so does a deck component with the name (or an\n"
        "alias) and the molar mass (within 1 %) of a built-in one; the header shows\n"
        "where each comes from. Enthalpies are zero for the ideal gas at 298.15 K,\n"
        "and are only reported inside the fit range of every heat capacity\n"
        "(built-in components: 250 to 600 K).\n"
        "\n"
        "the isothermal flash, with the options of flow_comp:\n"
        "  --flash-two-phase-method  ssi | newton | ssi+newton          [ssi+newton]\n"
        "  --flash-tolerance         tolerance                                [1e-8]\n"
        "  --flash-verbosity         0..5                                        [0]\n"
        "\n"
        "the enthalpies and the isenthalpic flash:\n"
        "  --enthalpy-model          with -H: caloric | eos_departure, the model -H\n"
        "                            is matched against             [eos_departure]\n"
        "                            caloric = ideal-gas heat capacity only;\n"
        "                            eos_departure adds the residual enthalpy\n"
        "  --ph-tmin, --ph-tmax <K>  the temperature interval of the search, inside\n"
        "                            the heat-capacity fit range     [the fit range]\n"
        "  --ph-temperature-tolerance <K>                                     [1e-6]\n"
        "  --ph-enthalpy-tolerance <J/mol>                                       [1]\n"
        "  --ph-max-iterations <n>                                             [100]\n"
        "\n"
        "output:\n"
        "  --format table | csv      table: the full block of a single state, one\n"
        "                            line per state for several; csv: one row per\n"
        "                            state with every column, the settings in '#'\n"
        "                            lines                                  [table]\n"
        "  --details                 with several states: the full block of each\n"
        "  --list-components         the built-in components and their parameters\n"
        "  -h, --help                this help\n"
        "\n"
        "Option names are case sensitive (-h is the help, -H the enthalpy); their\n"
        "values are not (--eos pr, --format CSV). Long options also take the form\n"
        "--option=value. Z is the compressibility factor p Vm / (R T) of the\n"
        "equation-of-state root; the density includes the volume shift (SSHIFT).\n"
        "The exit status is 0 when every state is solved, 1 otherwise.\n";
}

inline void listComponents(std::ostream& out)
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
    out << "Built-in components (parameters and ideal-gas heat capacity from the\n"
           "component classes of this library; the heat-capacity polynomial is valid\n"
           "inside its fit range):\n\n"
           "  name   " << std::left << std::setw(static_cast<int>(width) + 2) << "aliases"
        << std::right << "MW[g/mol]    Tc[K]  Pc[bar]  Vc[m3/kmol]   omega  cp fit[K]\n";
    const auto flags = out.flags();
    const auto precision = out.precision();
    out << std::fixed;
    for (std::size_t i = 0; i < componentDatabase().size(); ++i) {
        const auto& e = componentDatabase()[i];
        out << "  " << std::left << std::setw(7) << e.name
            << std::setw(static_cast<int>(width) + 2) << aliases[i] << std::right
            << std::setw(9) << std::setprecision(3) << e.molarMass * 1e3
            << std::setw(9) << std::setprecision(2) << e.criticalTemperature
            << std::setw(9) << e.criticalPressure / 1e5
            << std::setw(13) << std::setprecision(4) << e.criticalVolume
            << std::setw(8) << std::setprecision(3) << e.acentricFactor
            << std::setprecision(0) << "  " << e.heatCapacity->minTemperature
            << '-' << e.heatCapacity->maxTemperature << '\n';
    }
    out.flags(flags);
    out.precision(precision);
    out << "\nSelect them by name in --z, e.g. --z CO2=0.4,C1=0.2,C10=0.4 (2 to 10\n"
           "components, each once).\n"
           "Binary interaction coefficients are zero unless given with --kij,\n"
           "e.g. --kij C1:C10=0.0411,CO2:C1=0.1. Water is a plain Peng-Robinson\n"
           "component here (no association term). Other fluids come from a deck\n"
           "with --deck.\n";
}

} // namespace Opm::OpmFlash

#endif // OPM_OPMFLASH_HPP
