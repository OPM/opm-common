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

  Runs the isothermal (P-T) or the isenthalpic (P-H) flash on a mixture of
  components of this library, with GenericOilGasWaterFluidSystem, PTFlash and
  PHFlash. For quick checks and for curves (--sweep, --format csv) without a
  deck or a simulator run.

    opmflash --spec pt --p 50 --t 300 --z 0.5,0.5 --components C1,C10 --kij C1:C10=0.0411
    opmflash --spec ph --p 50 --h -25000 --z 0.5,0.5 --components C1,C10
    opmflash --spec ph --p 50 --z 0.5,0.5 --components C1,C10 \
             --sweep h=-30000:5000:1000 --format csv
    opmflash --list-components
    opmflash --selftest

  Units: --p bar, --t K, --h J/mol (molar enthalpy, zero for the ideal gas at
  298.15 K). Output enthalpies are J/mol.
*/

#include "config.h"

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

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using EOSType = Opm::CompositionalConfig::EOSType;

// The components: equation-of-state parameters and the ideal-gas heat-capacity
// polynomial from the component classes of this library, in the units of the
// fluid system (molar mass g/mol, critical volume m3/kmol).
struct ComponentEntry {
    std::string name;
    double molarMassGramPerMol;
    double criticalTemperature;
    double criticalPressure;
    double criticalVolume;     // [m3/kmol]
    double acentricFactor;
    Opm::ComponentCp<double> heatCapacity;
};

template <class Comp>
ComponentEntry makeEntry()
{
    return {std::string(Comp::name()), Comp::molarMass() * 1e3, Comp::criticalTemperature(),
            Comp::criticalPressure(), Comp::criticalVolume(), Comp::acentricFactor(),
            Comp::idealGasHeatCapacityPolynomial()};
}

// Water is flashed as a plain Peng-Robinson component (no association term):
// good enough for quick checks, not an aqueous-phase model.
const std::vector<ComponentEntry>& componentDatabase()
{
    static const std::vector<ComponentEntry> db{
        makeEntry<Opm::C1<double>>(),
        makeEntry<Opm::C10<double>>(),
        makeEntry<Opm::SimpleCO2<double>>(),
        makeEntry<Opm::N2<double>>(),
        makeEntry<Opm::H2O<double>>(),
        makeEntry<Opm::H2<double>>(),
    };
    return db;
}

// The component name as the component class reports it, from that name or an
// alias, in any case.
std::string canonicalName(const std::string& name)
{
    std::string upper;
    for (char c : name) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    if (upper == "C1" || upper == "CH4" || upper == "METHANE") { return "C1"; }
    if (upper == "C10" || upper == "NC10" || upper == "DECANE" || upper == "N-DECANE") {
        return "C10";
    }
    if (upper == "CO2" || upper == "CARBONDIOXIDE") { return "CO2"; }
    if (upper == "N2" || upper == "NITROGEN") { return "N2"; }
    if (upper == "H2O" || upper == "WATER") { return "H2O"; }
    if (upper == "H2" || upper == "HYDROGEN") { return "H2"; }
    throw std::runtime_error("unknown component '" + name + "' (see --list-components)");
}

const ComponentEntry& lookupComponent(const std::string& name)
{
    const std::string canonical = canonicalName(name);
    for (const auto& entry : componentDatabase()) {
        if (entry.name == canonical) {
            return entry;
        }
    }
    throw std::logic_error("component '" + canonical + "' has an alias but no entry");
}

// Binary interaction coefficients by component pair, in canonical names with
// the smaller name first. Pairs that are not given are zero.
using PairKey = std::pair<std::string, std::string>;
using KijTable = std::map<PairKey, double>;

PairKey pairKey(std::string a, std::string b)
{
    if (b < a) {
        std::swap(a, b);
    }
    return {std::move(a), std::move(b)};
}

// ---- command line ----------------------------------------------------------
struct Sweep {
    char var = '\0';          // 'p' | 't' | 'h'
    double start = 0.0, stop = 0.0, step = 0.0;
};

struct Cli {
    std::string spec = "pt";              // pt | ph
    double pBar = 50.0;
    std::optional<double> tKelvin;        // pt: required
    std::optional<double> hMolar;         // ph: required (unless sweeping h)
    std::vector<std::string> components{"C1", "C10"};
    std::vector<double> z{0.5, 0.5};
    KijTable kij;
    std::string eos = "PR";
    std::string enthalpyModelName = "eos_departure";
    Opm::EnthalpyModel enthalpyModel = Opm::EnthalpyModel::eos_departure;
    double tempMin = 270.0;
    double tempMax = 460.0;
    double temperatureTolerance = 1e-6;   // [K]
    double enthalpyTolerance = 1.0;       // [J/mol]
    int maxIterations = 100;
    double ptTolerance = 1e-8;
    Opm::PTFlashMethod twoPhaseMethod = Opm::PTFlashMethod::Ssi;
    std::optional<Sweep> sweep;
    std::string format = "table";         // table | csv
    int verbosity = 0;
    bool selftest = false;
};

std::vector<std::string> splitCommas(const std::string& s)
{
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) { out.push_back(item); }
    return out;
}

Sweep parseSweep(const std::string& s)
{
    // var=start:stop:step
    const auto eq = s.find('=');
    const auto c1 = s.find(':', eq);
    const auto c2 = s.find(':', c1 + 1);
    if (eq != 1 || c1 == std::string::npos || c2 == std::string::npos ||
        (s[0] != 'p' && s[0] != 't' && s[0] != 'h')) {
        throw std::runtime_error("--sweep expects p|t|h=start:stop:step, got '" + s + "'");
    }
    return {s[0],
            std::stod(s.substr(eq + 1, c1 - eq - 1)),
            std::stod(s.substr(c1 + 1, c2 - c1 - 1)),
            std::stod(s.substr(c2 + 1))};
}

KijTable parseKij(const std::string& s)
{
    // A:B=value[,C:D=value...]
    KijTable table;
    for (const auto& item : splitCommas(s)) {
        const auto colon = item.find(':');
        const auto eq = item.find('=');
        if (colon == std::string::npos || eq == std::string::npos || colon > eq) {
            throw std::runtime_error("--kij expects A:B=value[,C:D=value...], got '" + item + "'");
        }
        const std::string a = canonicalName(item.substr(0, colon));
        const std::string b = canonicalName(item.substr(colon + 1, eq - colon - 1));
        if (a == b) {
            throw std::runtime_error("--kij pair '" + item + "' names the same component twice");
        }
        table[pairKey(a, b)] = std::stod(item.substr(eq + 1));
    }
    return table;
}

Opm::EnthalpyModel enthalpyModelFromName(const std::string& name)
{
    if (name == "caloric") { return Opm::EnthalpyModel::caloric; }
    if (name == "eos_departure") { return Opm::EnthalpyModel::eos_departure; }
    throw std::runtime_error("--enthalpy-model expects caloric or eos_departure, got '" +
                             name + "'");
}

void listComponents()
{
    std::cout <<
        "Available components (parameters and ideal-gas heat capacity from the\n"
        "component classes of this library; the heat-capacity polynomials are\n"
        "valid from 250 K, 275 K for water, to 600 K):\n\n"
        "  name  aliases          MW[g/mol]   Tc[K]    Pc[bar]  Vc[m3/kmol]  omega\n";
    std::cout << std::fixed;
    for (const auto& e : componentDatabase()) {
        std::string aliases;
        if (e.name == "C1") { aliases = "CH4, METHANE"; }
        else if (e.name == "C10") { aliases = "nC10, DECANE"; }
        else if (e.name == "CO2") { aliases = "CARBONDIOXIDE"; }
        else if (e.name == "N2") { aliases = "NITROGEN"; }
        else if (e.name == "H2O") { aliases = "WATER"; }
        else if (e.name == "H2") { aliases = "HYDROGEN"; }
        std::cout << "  " << std::setw(5) << std::left << e.name
                  << std::setw(17) << aliases << std::right
                  << std::setw(9) << std::setprecision(3) << e.molarMassGramPerMol
                  << std::setw(9) << std::setprecision(2) << e.criticalTemperature
                  << std::setw(9) << e.criticalPressure / 1e5
                  << std::setw(11) << std::setprecision(4) << e.criticalVolume
                  << std::setw(8) << std::setprecision(3) << e.acentricFactor
                  << '\n';
    }
    std::cout <<
        "\nBinary interaction coefficients are zero unless given with --kij,\n"
        "e.g. --kij C1:C10=0.0411,CO2:C1=0.1.\n"
        "Select with --components, e.g. --components C1,C10 or --components CO2,C1,C10\n"
        "(2 to 4 components; --z must match). Water is a plain Peng-Robinson\n"
        "component here (no association term).\n";
}

void printUsage()
{
    std::cout <<
        "opmflash: flash a compositional state from the command line\n"
        "\n"
        "Runs the isothermal (P-T) or the isenthalpic (P-H) flash on a mixture\n"
        "with the fluid system and the flash solvers of the compositional path.\n"
        "\n"
        "usage:\n"
        "  opmflash --spec pt --p <bar> --t <K>     --z <z,z[,z]> --components <c,c[,c]> [options]\n"
        "  opmflash --spec ph --p <bar> --h <J/mol> --z <z,z[,z]> --components <c,c[,c]> [options]\n"
        "  opmflash --list-components\n"
        "  opmflash --selftest\n"
        "\n"
        "specifications:\n"
        "  pt   isothermal: flash at (p, T, z); reports the phase split and the\n"
        "       molar enthalpy of the flashed state with both enthalpy models\n"
        "       (the enthalpy to give a later ph run)\n"
        "  ph   isenthalpic: finds the temperature at which the flashed mixture\n"
        "       has the enthalpy --h, then reports the same quantities\n"
        "\n"
        "options:\n"
        "  --components      comma list; see --list-components       [C1,C10]\n"
        "  --z               feed mole fractions, must sum to 1      [0.5,0.5]\n"
        "  --kij             A:B=value[,C:D=value...] binary\n"
        "                    interaction coefficients; others zero   [none]\n"
        "  --eos             PR | PRCORR | SRK | ZJ | RK             [PR]\n"
        "  --enthalpy-model  caloric | eos_departure                 [eos_departure]\n"
        "                    caloric = ideal-gas heat capacity only;\n"
        "                    eos_departure adds the residual enthalpy\n"
        "  --tmin, --tmax    P-H temperature search interval [K]     [270, 460]\n"
        "  --tol             P-H temperature tolerance [K]           [1e-6]\n"
        "  --enthalpy-tol    P-H enthalpy acceptance [J/mol]         [1]\n"
        "  --maxiter         P-H iteration limit                     [100]\n"
        "  --pt-tol          isothermal flash tolerance              [1e-8]\n"
        "  --method          ssi | newton | ssi+newton               [ssi]\n"
        "  --sweep           p|t|h=start:stop:step, one variable;\n"
        "                    t-sweeps need --spec pt, h-sweeps --spec ph\n"
        "  --format          table | csv (csv: one row per state,\n"
        "                    settings in '#' header lines)           [table]\n"
        "  --verbosity       0..5, passed to the solvers             [0]\n"
        "\n"
        "units: --p bar, --t K, --h J/mol. Enthalpies are molar [J/mol], zero for\n"
        "the ideal gas at 298.15 K.\n"
        "\n"
        "examples:\n"
        "  # phase split and enthalpy of a state\n"
        "  opmflash --spec pt --p 50 --t 300 --z 0.5,0.5 --components C1,C10 --kij C1:C10=0.0411\n"
        "\n"
        "  # the temperature at which the mixture has a given enthalpy\n"
        "  opmflash --spec ph --p 50 --h -25000 --z 0.5,0.5 --components C1,C10 --kij C1:C10=0.0411\n"
        "\n"
        "  # the curve H -> T as CSV\n"
        "  opmflash --spec ph --p 50 --z 0.5,0.5 --components C1,C10 \\\n"
        "           --sweep h=-30000:5000:1000 --format csv > curve.csv\n"
        "\n"
        "  # ternary with CO2\n"
        "  opmflash --spec pt --p 50 --t 320 --z 0.4,0.2,0.4 --components CO2,C1,C10\n";
}

Cli parseCli(int argc, char** argv)
{
    Cli cli;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing value after " + arg);
            }
            return argv[++i];
        };
        if (arg == "--help" || arg == "-h") { printUsage(); std::exit(EXIT_SUCCESS); }
        else if (arg == "--list-components") { listComponents(); std::exit(EXIT_SUCCESS); }
        else if (arg == "--selftest") { cli.selftest = true; }
        else if (arg == "--spec") { cli.spec = value(); }
        else if (arg == "--p") { cli.pBar = std::stod(value()); }
        else if (arg == "--t") { cli.tKelvin = std::stod(value()); }
        else if (arg == "--h") { cli.hMolar = std::stod(value()); }
        else if (arg == "--components") { cli.components = splitCommas(value()); }
        else if (arg == "--z") {
            cli.z.clear();
            for (const auto& v : splitCommas(value())) { cli.z.push_back(std::stod(v)); }
        }
        else if (arg == "--kij") { cli.kij = parseKij(value()); }
        else if (arg == "--eos") { cli.eos = value(); }
        else if (arg == "--enthalpy-model") {
            cli.enthalpyModelName = value();
            cli.enthalpyModel = enthalpyModelFromName(cli.enthalpyModelName);
        }
        else if (arg == "--tmin") { cli.tempMin = std::stod(value()); }
        else if (arg == "--tmax") { cli.tempMax = std::stod(value()); }
        else if (arg == "--tol") { cli.temperatureTolerance = std::stod(value()); }
        else if (arg == "--enthalpy-tol") { cli.enthalpyTolerance = std::stod(value()); }
        else if (arg == "--maxiter") { cli.maxIterations = std::stoi(value()); }
        else if (arg == "--pt-tol") { cli.ptTolerance = std::stod(value()); }
        else if (arg == "--method") { cli.twoPhaseMethod = Opm::ptFlashMethodFromString(value()); }
        else if (arg == "--sweep") { cli.sweep = parseSweep(value()); }
        else if (arg == "--format") { cli.format = value(); }
        else if (arg == "--verbosity") { cli.verbosity = std::stoi(value()); }
        else {
            throw std::runtime_error("unknown option '" + arg + "' (see --help)");
        }
    }
    return cli;
}

// ---- the flash, for a compile-time number of components --------------------
template <int N>
struct FlashRunner {
    using FluidSystem = Opm::GenericOilGasWaterFluidSystem<double, N, false>;
    using FlashEval = Opm::DenseAd::Evaluation<double, N + 1>;
    using FluidState = Opm::CompositionalFluidState<FlashEval, FluidSystem>;
    using PtFlash = Opm::PTFlash<double, FluidSystem>;
    using PhFlash = Opm::PHFlash<double, FluidSystem>;
    using Enthalpy = Opm::MixtureEnthalpy<double, FluidSystem>;

    struct Result {
        bool ok;
        bool singlePhase;
        double temperature;
        double L;
        std::array<double, N> x, y, K;
        double hCaloric, hDeparture;
    };

    static void setup(const Cli& cli)
    {
        FluidSystem::init();
        std::vector<std::string> names;
        for (const auto& c : cli.components) {
            const auto& e = lookupComponent(c);
            names.push_back(e.name);
            FluidSystem::addComponent(typename FluidSystem::ComponentParam{
                e.name, e.molarMassGramPerMol, e.criticalTemperature,
                e.criticalPressure, e.criticalVolume, e.acentricFactor});
        }
        for (const auto& [pair, value] : cli.kij) {
            const auto selected = [&](const std::string& n) {
                return std::find(names.begin(), names.end(), n) != names.end();
            };
            if (!selected(pair.first) || !selected(pair.second)) {
                throw std::runtime_error("--kij pair " + pair.first + ":" + pair.second +
                                         " is not among --components");
            }
        }
        // packed lower triangle: (1,0), (2,0), (2,1), ...
        std::vector<double> bic;
        for (int row = 1; row < N; ++row) {
            for (int col = 0; col < row; ++col) {
                const auto it = cli.kij.find(pairKey(names[row], names[col]));
                bic.push_back(it != cli.kij.end() ? it->second : 0.0);
            }
        }
        FluidSystem::setInteractionCoefficients(std::move(bic));
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

    static Opm::CpTable<double, N> cpTable()
    {
        Opm::CpTable<double, N> table;
        for (int compIdx = 0; compIdx < N; ++compIdx) {
            table[compIdx] =
                lookupComponent(std::string(FluidSystem::componentName(compIdx))).heatCapacity;
        }
        return table;
    }

    static typename PhFlash::Config phConfig(const Cli& cli)
    {
        typename PhFlash::Config cfg;
        cfg.cpTable = cpTable();
        cfg.model = cli.enthalpyModel;
        cfg.tempMin = cli.tempMin;
        cfg.tempMax = cli.tempMax;
        cfg.temperatureTolerance = cli.temperatureTolerance;
        cfg.enthalpyTolerance = cli.enthalpyTolerance;
        cfg.maxIterations = cli.maxIterations;
        return cfg;
    }

    static Result collect(FluidState& fs, bool singlePhase, bool ok, const EOSType& eos)
    {
        Result r{};
        r.ok = ok;
        r.singlePhase = singlePhase;
        r.temperature = Opm::getValue(fs.temperature(0));
        r.L = Opm::getValue(fs.L());
        for (int c = 0; c < N; ++c) {
            r.x[c] = Opm::getValue(fs.moleFraction(FluidSystem::oilPhaseIdx, c));
            r.y[c] = Opm::getValue(fs.moleFraction(FluidSystem::gasPhaseIdx, c));
            r.K[c] = r.x[c] > 0.0 ? r.y[c] / r.x[c] : 0.0;
        }
        if (ok) {
            const auto table = cpTable();
            r.hCaloric = Opm::getValue(Enthalpy::mixtureEnthalpy(
                fs, table, eos, Opm::EnthalpyModel::caloric));
            r.hDeparture = Opm::getValue(Enthalpy::mixtureEnthalpy(
                fs, table, eos, Opm::EnthalpyModel::eos_departure));
        }
        return r;
    }

    static Result runPt(const Cli& cli, double pPa, double t)
    {
        const auto eos = Opm::CompositionalConfig::eosTypeFromString(cli.eos);
        FluidState fs = makeState(pPa, t, cli.z);
        const bool singlePhase =
            PtFlash::solve(fs, cli.twoPhaseMethod, cli.ptTolerance, eos, cli.verbosity);
        return collect(fs, singlePhase, true, eos);
    }

    static Result runPh(const Cli& cli, double pPa, double h)
    {
        const auto eos = Opm::CompositionalConfig::eosTypeFromString(cli.eos);
        // the initial temperature only seeds the state; PHFlash searches over T
        FluidState fs = makeState(pPa, 0.5 * (cli.tempMin + cli.tempMax), cli.z);
        const bool ok = PhFlash::solve(fs, h, phConfig(cli), cli.twoPhaseMethod,
                                       cli.ptTolerance, eos, cli.verbosity);
        const bool singlePhase = !(Opm::getValue(fs.L()) > 0.0 &&
                                   Opm::getValue(fs.L()) < 1.0);
        return collect(fs, singlePhase, ok, eos);
    }
};

// ---- output ----------------------------------------------------------------
template <int N>
void printHeader(const Cli& cli)
{
    const char* prefix = (cli.format == "csv") ? "# " : "";
    std::cout << prefix << "opmflash, spec " << cli.spec << ", eos " << cli.eos
              << ", enthalpy model " << cli.enthalpyModelName
              << ", enthalpies J/mol, zero for the ideal gas at "
              << Opm::ComponentCp<double>::referenceTemperature() << " K\n";
    if (cli.format == "csv") {
        std::cout << "p_bar,T_K,h_spec";
        for (int c = 0; c < N; ++c) { std::cout << ",z" << c; }
        std::cout << ",single_phase,L";
        for (int c = 0; c < N; ++c) { std::cout << ",x" << c; }
        for (int c = 0; c < N; ++c) { std::cout << ",y" << c; }
        for (int c = 0; c < N; ++c) { std::cout << ",K" << c; }
        std::cout << ",H_caloric,H_departure,ok\n";
    }
}

template <int N>
void printRow(const Cli& cli, double pPa, std::optional<double> hSpec,
              const typename FlashRunner<N>::Result& r)
{
    if (cli.format == "csv") {
        std::cout << std::setprecision(10)
                  << pPa / 1e5 << ',' << r.temperature << ','
                  << (hSpec ? std::to_string(*hSpec) : "");
        for (int c = 0; c < N; ++c) { std::cout << ',' << cli.z[c]; }
        std::cout << ',' << (r.singlePhase ? 1 : 0) << ',' << r.L;
        for (int c = 0; c < N; ++c) { std::cout << ',' << r.x[c]; }
        for (int c = 0; c < N; ++c) { std::cout << ',' << r.y[c]; }
        for (int c = 0; c < N; ++c) { std::cout << ',' << r.K[c]; }
        std::cout << ',' << r.hCaloric << ',' << r.hDeparture
                  << ',' << (r.ok ? 1 : 0) << '\n';
        return;
    }

    std::cout << std::fixed << std::setprecision(4);
    if (!r.ok) {
        std::cout << "  NO SOLUTION in [" << cli.tempMin << ", " << cli.tempMax
                  << "] K for h = " << (hSpec ? *hSpec : 0.0) << " J/mol at p = "
                  << pPa / 1e5 << " bar; widen --tmin/--tmax or check the input\n";
        return;
    }
    std::cout << "  p = " << pPa / 1e5 << " bar";
    if (hSpec) { std::cout << "   h_spec = " << *hSpec << " J/mol"; }
    std::cout << "\n  T = " << r.temperature << " K   "
              << (r.singlePhase ? "single-phase" : "two-phase")
              << "   L = " << r.L << "\n  ";
    for (int c = 0; c < N; ++c) {
        std::cout << "x" << c << " = " << r.x[c] << "  ";
    }
    for (int c = 0; c < N; ++c) {
        std::cout << "y" << c << " = " << r.y[c] << "  ";
    }
    std::cout << "\n  H caloric = " << r.hCaloric
              << "   H eos_departure = " << r.hDeparture << "  [J/mol]\n\n";
}

// ---- drivers ---------------------------------------------------------------
template <int N>
int runCases(const Cli& cli)
{
    FlashRunner<N>::setup(cli);
    printHeader<N>(cli);

    const double pPa = cli.pBar * 1e5;
    bool allOk = true;

    const auto runOne = [&](double p, std::optional<double> t, std::optional<double> h) {
        if (cli.spec == "pt") {
            if (!t) { throw std::runtime_error("--spec pt requires --t (or a t-sweep)"); }
            const auto r = FlashRunner<N>::runPt(cli, p, *t);
            printRow<N>(cli, p, std::nullopt, r);
            allOk = allOk && r.ok;
        }
        else if (cli.spec == "ph") {
            if (!h) { throw std::runtime_error("--spec ph requires --h (or an h-sweep)"); }
            const auto r = FlashRunner<N>::runPh(cli, p, *h);
            printRow<N>(cli, p, h, r);
            allOk = allOk && r.ok;
        }
        else {
            throw std::runtime_error("--spec expects pt or ph, got '" + cli.spec + "'");
        }
    };

    if (!cli.sweep) {
        runOne(pPa, cli.tKelvin, cli.hMolar);
        return allOk ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    const auto& sw = *cli.sweep;
    if ((sw.stop - sw.start) * sw.step <= 0.0) {
        throw std::runtime_error("--sweep step does not advance from start to stop");
    }
    if ((cli.spec == "pt" && sw.var == 'h') || (cli.spec == "ph" && sw.var == 't')) {
        throw std::runtime_error(std::string("--sweep over '") + sw.var +
                                 "' does not apply to --spec " + cli.spec);
    }
    for (double v = sw.start; (sw.step > 0.0) ? v <= sw.stop : v >= sw.stop; v += sw.step) {
        switch (sw.var) {
        case 'p': runOne(v * 1e5, cli.tKelvin, cli.hMolar); break;
        case 't': runOne(pPa, v, cli.hMolar); break;
        case 'h': runOne(pPa, cli.tKelvin, v); break;
        }
    }
    return allOk ? EXIT_SUCCESS : EXIT_FAILURE;
}

int dispatchOnComponentCount(const Cli& cli)
{
    if (cli.components.size() != cli.z.size()) {
        throw std::runtime_error("--components and --z must have the same length");
    }
    double sum = 0.0;
    for (double v : cli.z) { sum += v; }
    if (std::abs(sum - 1.0) > 1e-9) {
        throw std::runtime_error("--z must sum to 1");
    }

    switch (cli.components.size()) {
    case 2: return runCases<2>(cli);
    case 3: return runCases<3>(cli);
    case 4: return runCases<4>(cli);
    default:
        throw std::runtime_error("supported numbers of components: 2, 3, 4");
    }
}

// Round trips through the whole program apart from the parsing: flash at a
// temperature, give the enthalpy of the result to the isenthalpic flash, and
// require the temperature back within 1e-4 K; for a binary and a ternary.
int selftest()
{
    int failures = 0;
    const auto check = [&](const char* name, const Cli& cli, double tAnchor) {
        const auto verdict = [&](bool pass, double tBack) {
            std::cout << (pass ? "  PASS  " : "  FAIL  ") << name
                      << "  T = " << tAnchor << " -> " << tBack << " K\n";
            if (!pass) { ++failures; }
        };
        if (cli.components.size() == 2) {
            FlashRunner<2>::setup(cli);
            const auto pt = FlashRunner<2>::runPt(cli, cli.pBar * 1e5, tAnchor);
            const auto ph = FlashRunner<2>::runPh(cli, cli.pBar * 1e5, pt.hDeparture);
            verdict(ph.ok && std::abs(ph.temperature - tAnchor) < 1e-4, ph.temperature);
        }
        else {
            FlashRunner<3>::setup(cli);
            const auto pt = FlashRunner<3>::runPt(cli, cli.pBar * 1e5, tAnchor);
            const auto ph = FlashRunner<3>::runPh(cli, cli.pBar * 1e5, pt.hDeparture);
            verdict(ph.ok && std::abs(ph.temperature - tAnchor) < 1e-4, ph.temperature);
        }
    };

    Cli binary;
    binary.components = {"C1", "C10"};
    binary.z = {0.5, 0.5};
    binary.kij = parseKij("C1:C10=0.0411");
    check("C1/C10, 50 bar, 300 K", binary, 300.0);

    Cli ternary;
    ternary.components = {"CO2", "C1", "C10"};
    ternary.z = {0.4, 0.2, 0.4};
    ternary.kij = parseKij("C1:C10=0.0411");
    check("CO2/C1/C10, 50 bar, 320 K", ternary, 320.0);

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
        const Cli cli = parseCli(argc, argv);
        return cli.selftest ? selftest() : dispatchOnComponentCount(cli);
    }
    catch (const std::exception& e) {
        std::cerr << "opmflash: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
