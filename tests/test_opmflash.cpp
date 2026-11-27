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
 * \brief Tests for the opmflash program (examples/opmflash.hpp).
 *
 * The round trips flash at a temperature, give the enthalpy of the result to
 * the isenthalpic flash and require the temperature back. The other tests
 * cover the fluid of the built-in components and of a deck, the command line
 * and what it refuses (each refusal by its message, one case per check), the
 * heat-capacity file, the output and the examples of the help, which have to
 * be valid command lines.
 */
#include "config.h"

#define BOOST_TEST_MODULE OpmFlash
#include <boost/test/unit_test.hpp>

#include <examples/opmflash.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Opm::OpmFlash;

namespace {

using Args = std::vector<std::string>;

Cli cliOf(const Args& args)
{
    return parseCli(args).cli;
}

Run runOf(const Args& args, const Opm::Deck* deck = nullptr)
{
    return prepare(cliOf(args), deck);
}

// The methane/decane pair, half and half, with the given options.
Args halfAndHalf(const Args& extra)
{
    Args args{"--z", "C1=0.5,C10=0.5"};
    args.insert(args.end(), extra.begin(), extra.end());
    return args;
}

// The first state of a run, flashed.
template <int N>
typename FlashRunner<N>::Result flashFirst(const Run& run)
{
    FlashRunner<N>::setup(run.mixture);
    return FlashRunner<N>::runState(run, run.states.front());
}

// The isothermal flash at tAnchor, then the isenthalpic flash over the whole
// fit range at the enthalpy of the result: the temperature found, or nothing
// when either flash fails.
template <int N>
std::optional<double> roundTrip(const Run& run, double tAnchor)
{
    using Runner = FlashRunner<N>;
    Runner::setup(run.mixture);
    const State anchor{run.states.front().p, tAnchor, run.mixture.z};
    const auto pt = Runner::runPt(run, anchor);
    if (!pt.ok || !pt.hDeparture) {
        return std::nullopt;
    }
    const State target{anchor.p, *pt.hDeparture, anchor.z};
    const auto ph = Runner::runPh(run, target, *run.fit);
    return ph.ok ? std::optional(ph.temperature) : std::nullopt;
}

// What the program prints for a run.
template <int N>
std::string outputOf(const Run& run)
{
    std::ostringstream out;
    runCases<N>(out, run);
    return out.str();
}

// The methane/decane pair with the parameters of the built-in components, as
// a deck in METRIC units; c10MolarMass [g/mol] replaces the molar mass of
// decane.
Opm::Deck builtinPairDeck(std::optional<double> c10MolarMass = std::nullopt,
                          const std::string& eos = "PR")
{
    const auto& c1 = *findBuiltin("C1");
    const auto& c10 = *findBuiltin("C10");
    std::ostringstream deck;
    deck << std::setprecision(17)
         << "RUNSPEC\nMETRIC\nOIL\nGAS\nTABDIMS\n/\nCOMPS\n2 /\nPROPS\n"
         << "CNAMES\nC1 C10 /\nEOS\n" << eos << " /\n"
         << "MW\n" << c1.molarMass * 1e3 << ' '
         << c10MolarMass.value_or(c10.molarMass * 1e3) << " /\n"
         << "TCRIT\n" << c1.criticalTemperature << ' ' << c10.criticalTemperature << " /\n"
         << "PCRIT\n" << c1.criticalPressure / 1e5 << ' ' << c10.criticalPressure / 1e5
         << " /\n"
         << "VCRIT\n" << c1.criticalVolume << ' ' << c10.criticalVolume << " /\n"
         << "ACF\n" << c1.acentricFactor << ' ' << c10.acentricFactor << " /\n"
         << "BIC\n0.0411 /\n";
    return Opm::Parser{}.parseString(deck.str());
}

// The message of the error that the command line gives, or nothing when it
// is accepted.
std::string refusal(const Args& args, const Opm::Deck* deck = nullptr)
{
    try {
        prepare(parseCli(args).cli, deck);
    }
    catch (const std::runtime_error& e) {
        return e.what();
    }
    return {};
}

// The command line is refused, for the reason that part of the message names.
void checkRefused(const Args& args, const std::string& part, const Opm::Deck* deck = nullptr)
{
    const std::string message = refusal(args, deck);
    BOOST_CHECK_MESSAGE(message.find(part) != std::string::npos,
                        "expected an error containing '" << part << "', got '" << message
                                                         << "'");
}

bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

std::size_t countOf(const std::string& text, const std::string& part)
{
    std::size_t count = 0;
    for (auto at = text.find(part); at != std::string::npos; at = text.find(part, at + 1)) {
        ++count;
    }
    return count;
}

// The example command lines of a help text that need no file.
std::vector<Args> examplesOf(const std::string& help)
{
    std::vector<Args> examples;
    std::istringstream lines(help);
    for (std::string line; std::getline(lines, line);) {
        if (!line.starts_with("  opmflash -P") || contains(line, "<") ||
            contains(line, "--deck")) {
            continue;
        }
        std::istringstream words(line.substr(line.find("opmflash") + 8));
        Args args{std::istream_iterator<std::string>(words), {}};
        // the comment of the short usage
        if (const auto comment = std::ranges::find_if(args, [](const std::string& w) {
                return w.starts_with("the");
            });
            comment != args.end()) {
            args.erase(comment, args.end());
        }
        examples.push_back(std::move(args));
    }
    return examples;
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(RoundTrips)

BOOST_AUTO_TEST_CASE(TwoComponents)
{
    const Run run = runOf(halfAndHalf({"-P", "50", "-T", "300", "--kij", "C1:C10=0.0411"}));
    const auto t = roundTrip<2>(run, 300.0);
    BOOST_REQUIRE(t);
    BOOST_CHECK_SMALL(*t - 300.0, 1e-4);
}

BOOST_AUTO_TEST_CASE(ThreeComponents)
{
    const Run run = runOf({"-P", "50", "-T", "320", "--z", "CO2=0.4,C1=0.2,C10=0.4",
                           "--kij", "C1:C10=0.0411"});
    const auto t = roundTrip<3>(run, 320.0);
    BOOST_REQUIRE(t);
    BOOST_CHECK_SMALL(*t - 320.0, 1e-4);
}

BOOST_AUTO_TEST_CASE(DeckFluid)
{
    const Opm::Deck deck = builtinPairDeck();
    const Run run = runOf({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5"}, &deck);
    const auto t = roundTrip<2>(run, 300.0);
    BOOST_REQUIRE(t);
    BOOST_CHECK_SMALL(*t - 300.0, 1e-4);
}

BOOST_AUTO_TEST_SUITE_END() // RoundTrips

BOOST_AUTO_TEST_SUITE(Fluid)

// The program has to come back from a flash that throws, as it does for this
// lean feed at 1 bar. Whether the flash converges is not checked, so the case
// stays valid if the flash gets more robust.
BOOST_AUTO_TEST_CASE(FlashThatThrowsIsReported)
{
    const auto r = flashFirst<2>(runOf({"-P", "1", "-T", "500", "--z", "C1=0.1,C10=0.9"}));
    BOOST_CHECK_EQUAL(r.temperature, 500.0);
    BOOST_CHECK(r.ok || !r.hDeparture);
}

// A deck with the parameters of the built-in components flashes like them,
// and its state is read in bar and K whatever the units of the deck.
BOOST_AUTO_TEST_CASE(DeckFlashesLikeBuiltinComponents)
{
    const Opm::Deck deck = builtinPairDeck();
    const Run builtin = runOf(halfAndHalf({"-P", "50", "-T", "300", "--kij", "C1:C10=0.0411"}));
    const Run fromDeck = runOf({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5"},
                               &deck);
    const auto a = flashFirst<2>(builtin);
    const auto b = flashFirst<2>(fromDeck);
    BOOST_REQUIRE(a.ok && b.ok);
    BOOST_CHECK(!a.singlePhase);
    double largest = std::abs(a.L - b.L);
    for (int c = 0; c < 2; ++c) {
        largest = std::max({largest, std::abs(a.x[c] - b.x[c]), std::abs(a.y[c] - b.y[c])});
    }
    BOOST_CHECK_SMALL(largest, 1e-10);
    BOOST_CHECK_EQUAL(fromDeck.states.front().p, 5e6);
    BOOST_CHECK_EQUAL(fromDeck.states.front().value, 300.0);
}

// A deck component under a built-in name has the built-in heat capacity only
// with the built-in molar mass.
BOOST_AUTO_TEST_CASE(DeckComponentHeatCapacity)
{
    const Opm::Deck lumped = builtinPairDeck(200.0);
    const Run run = runOf({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5"},
                          &lumped);
    BOOST_CHECK(run.mixture.components[0].heatCapacity);
    BOOST_CHECK(!run.mixture.components[1].heatCapacity);
}

// A gas at 1 bar is nearly ideal, and its density is p M / (Z R T) with the
// reported Z (no volume shift here).
BOOST_AUTO_TEST_CASE(GasAtOneBar)
{
    const auto r = flashFirst<2>(runOf({"-P", "1", "-T", "300", "--z", "C1=0.5,N2=0.5"}));
    BOOST_REQUIRE(r.ok && r.singlePhase && r.vapour && !r.liquid);
    const double molarMass = 0.5 * (findBuiltin("C1")->molarMass + findBuiltin("N2")->molarMass);
    const double zFactor = r.vapour->compressibility;
    BOOST_CHECK(zFactor > 0.99 && zFactor <= 1.0);
    BOOST_CHECK_CLOSE(r.vapour->density,
                      1e5 * molarMass / (zFactor * Opm::Constants<double>::R * 300.0),
                      1e-7); // [%]
    BOOST_CHECK(r.vapour->viscosity > 5e-6 && r.vapour->viscosity < 5e-5);
}

// --z by name: the names of built-in components, or their aliases, in any
// case, select the fluid, in the order of --z; with a deck, the names are
// matched in any order and case.
BOOST_AUTO_TEST_CASE(FeedByName)
{
    const Run alone = runOf({"-P", "50", "-T", "300", "--z", "decane=0.3,c1=0.7"});
    BOOST_REQUIRE_EQUAL(alone.mixture.components.size(), 2u);
    BOOST_CHECK_EQUAL(alone.mixture.components[0].name, "C10");
    BOOST_CHECK_CLOSE(alone.mixture.z[0], 0.3, 1e-12);

    const Opm::Deck deck = builtinPairDeck();
    const Run fromDeck = runOf({"--deck", "(test)", "-P", "50", "-T", "300",
                                "--z", "c10=0.3,C1=0.7"}, &deck);
    BOOST_CHECK_CLOSE(fromDeck.mixture.z[0], 0.7, 1e-12);
    BOOST_CHECK_CLOSE(fromDeck.mixture.z[1], 0.3, 1e-12);
}

// The errors about the feed name the components, in their order.
BOOST_AUTO_TEST_CASE(FeedErrorsNameTheComponents)
{
    const Opm::Deck deck = builtinPairDeck();
    checkRefused({"--deck", "(test)", "-P", "50", "-T", "300"},
                 "--z is required: one mole fraction per component, by name "
                 "(--z C1=<z>,C10=<z>)", &deck);
    checkRefused({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=1"},
                 "--z gives no mole fraction for C10", &deck);
    checkRefused({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,CO2=0.5"},
                 "--z names CO2, which is not among the components: C1, C10", &deck);
    checkRefused({"-P", "50", "-T", "300", "--z", "C1=0.5,METHANE=0.5"},
                 "--z names C1 twice ('METHANE')");
    checkRefused({"-P", "50", "-T", "300", "--z", "C1=0.5,XX=0.5"},
                 "--z names XX, which is not a built-in component");
    checkRefused({"-P", "50", "-T", "300", "--z", "C1=0.5,0.5"},
                 "--z expects NAME=fraction, got '0.5'");
}

BOOST_AUTO_TEST_CASE(EquationOfState)
{
    BOOST_CHECK(runOf(halfAndHalf({"-P", "50", "-T", "300", "--eos", "srk"})).mixture.eos ==
                EOSType::SRK);
    // ZJ is a deck keyword value that the flash does not have.
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "--eos", "ZJ"}),
                 "the flash has no ZJ equation of state");
    const Opm::Deck zj = builtinPairDeck(std::nullopt, "ZJ");
    checkRefused({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5"},
                 "the deck's equation of state is ZJ", &zj);
    // A deck's fluid comes with its equation of state.
    const Opm::Deck deck = builtinPairDeck();
    checkRefused({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5", "--eos", "SRK"},
                 "--eos does not apply with --deck", &deck);
}

BOOST_AUTO_TEST_SUITE_END() // Fluid

BOOST_AUTO_TEST_SUITE(CommandLine)

// -P, -p and --pressure are one option, -T and -H have long names too, and -h
// is only the help.
BOOST_AUTO_TEST_CASE(StateOptions)
{
    for (const std::string form : {"-P", "-p", "--pressure"}) {
        BOOST_CHECK(cliOf({form, "50"}).p == std::vector<double>{50.0});
    }
    BOOST_CHECK(cliOf({"--pressure=50"}).p == std::vector<double>{50.0});
    BOOST_CHECK(cliOf({"-T", "300"}).t == std::vector<double>{300.0});
    BOOST_CHECK(cliOf({"--temperature", "300"}).t == std::vector<double>{300.0});
    BOOST_CHECK(cliOf({"-H", "-25000"}).h == std::vector<double>{-25000.0});
    BOOST_CHECK(cliOf({"--enthalpy", "-25000"}).h == std::vector<double>{-25000.0});
    BOOST_CHECK(parseCli({"-h"}).action == Action::Help);
    BOOST_CHECK(parseCli({"--help"}).action == Action::Help);
    checkRefused({"-P", "50", "-p", "60", "-T", "300"}, "-p gives --pressure a second time");
    checkRefused({"-T", "300", "--z", "C1=0.5,C10=0.5"}, "the pressure is required: -P");
}

// Option values are read in any case.
BOOST_AUTO_TEST_CASE(ValuesInAnyCase)
{
    const Cli cli = cliOf({"--format", "CSV", "--eos", "pr", "--flash-two-phase-method", "SSI",
                           "--enthalpy-model", "Caloric"});
    BOOST_CHECK(cli.settings.format == Format::Csv);
    BOOST_CHECK(*cli.eos == EOSType::PR);
    BOOST_CHECK(cli.settings.twoPhaseMethod == Opm::PTFlashMethod::Ssi);
    BOOST_CHECK(cli.settings.enthalpyModel == Opm::EnthalpyModel::caloric);
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "--format", "xml"}),
                 "--format expects table or csv");
}

// A state option takes a number, a list or a range start:stop[:step], whose
// points include stop and whose step is 1 unless given.
BOOST_AUTO_TEST_CASE(ListsAndRanges)
{
    BOOST_CHECK(cliOf({"-T", "300,310,320"}).t == (std::vector<double>{300.0, 310.0, 320.0}));
    const auto unitStep = cliOf({"-T", "280:300"}).t;
    BOOST_CHECK_EQUAL(unitStep.size(), 21u);
    BOOST_CHECK_EQUAL(unitStep.back(), 300.0);
    const auto fractional = cliOf({"-T", "300:301:0.1"}).t;
    BOOST_CHECK_EQUAL(fractional.size(), 11u);
    BOOST_CHECK_SMALL(fractional.back() - 301.0, 1e-9);
    const auto falling = cliOf({"-H", "-5000:-30000:-1000"}).h;
    BOOST_CHECK_EQUAL(falling.size(), 26u);
    BOOST_CHECK_EQUAL(falling.back(), -30000.0);
    checkRefused(halfAndHalf({"-P", "50", "-T", "300:280"}),
                 "a falling range needs a negative step");
    checkRefused(halfAndHalf({"-P", "50", "-T", "1:2:3:4"}), "a list a,b,c or a range");
}

// Values in both p and T give every pressure with every temperature, the
// pressure in the outer loop.
BOOST_AUTO_TEST_CASE(Grid)
{
    const Run run = runOf(halfAndHalf({"-P", "10,20", "-T", "300:320:10"}));
    BOOST_REQUIRE_EQUAL(run.states.size(), 6u);
    BOOST_CHECK_EQUAL(run.states[1].p, 1e6);
    BOOST_CHECK_EQUAL(run.states[1].value, 310.0);
    BOOST_CHECK_EQUAL(run.states[3].p, 2e6);
    BOOST_CHECK_EQUAL(run.states[3].value, 300.0);
}

// nan and inf are refused as input, not run.
BOOST_AUTO_TEST_CASE(NumbersAreFinite)
{
    checkRefused(halfAndHalf({"-P", "nan", "-T", "300"}), "-P expects a number, got 'nan'");
}

// The options of the isenthalpic flash share the prefix --ph- and apply to it
// only.
BOOST_AUTO_TEST_CASE(IsenthalpicOptions)
{
    const Run whole = runOf(halfAndHalf({"-P", "50", "-H", "-25000"}));
    BOOST_CHECK_EQUAL(whole.search.min, 250.0);
    BOOST_CHECK_EQUAL(whole.search.max, 600.0);
    const Run part = runOf(halfAndHalf({"-P", "50", "-H", "-25000", "--ph-tmin", "260",
                                        "--ph-tmax", "500"}));
    BOOST_CHECK_EQUAL(part.search.min, 260.0);
    BOOST_CHECK_EQUAL(part.search.max, 500.0);
    const Cli cli = cliOf({"--ph-temperature-tolerance", "1e-7", "--ph-enthalpy-tolerance",
                           "0.5", "--ph-max-iterations", "50"});
    BOOST_CHECK_EQUAL(cli.settings.temperatureTolerance, 1e-7);
    BOOST_CHECK_EQUAL(cli.settings.enthalpyTolerance, 0.5);
    BOOST_CHECK_EQUAL(cli.settings.maxIterations, 50);
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "--ph-tmin", "260"}),
                 "--ph-tmin applies to the isenthalpic flash (-H) only");
    checkRefused(halfAndHalf({"-P", "50", "-H", "-25000", "--ph-tmax", "900"}),
                 "inside the heat-capacity fit range");
}

// The command line refuses what it cannot run, and every option given has to
// take part in the run.
BOOST_AUTO_TEST_CASE(Refusals)
{
    checkRefused({"-P", "50", "-T", "300"}, "give the fluid");
    checkRefused(halfAndHalf({"-P", "50"}), "give -T [K] for the isothermal flash");
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "-H", "1000"}), "not both");
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "--eos-region", "1"}),
                 "--eos-region needs --deck");
    checkRefused({"--list-components=x"}, "--list-components takes no value");
    checkRefused({"50"}, "unexpected '50'");
    checkRefused({"-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.49"}, "sums to 0.99, not 1");
    checkRefused({"-P", "50", "-T", "300", "--z", "C1=1.2,C10=-0.2"}, "must lie in [0, 1]");
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "--eos", "XYZ"}),
                 "--eos expects PR, PRCORR, SRK or RK");
    checkRefused(halfAndHalf({"-P", "50", "-T", "300", "--flash-two-phase-method", "xyz"}),
                 "--flash-two-phase-method expects ssi, newton or ssi+newton");
    checkRefused(halfAndHalf({"-P", "50", "-T", "300K"}), "-T expects a number, got '300K'");
    // the enthalpies are zero at 298.15 K, outside a fit from 300 K
    checkRefused(halfAndHalf({"-P", "50", "-T", "350", "--cp", "C1=20:0.05:0:0:300:600"}),
                 "the reference temperature 298.15 K lies outside the heat-capacity fit range "
                 "300 to 600 K");
    checkRefused(halfAndHalf({"-P", "50:60:10", "-T", "300", "--details", "--format", "csv"}),
                 "--details applies to --format table");

    const Opm::Deck deck = builtinPairDeck();
    checkRefused({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5",
                  "--eos-region", "2"},
                 "--eos-region expects 1 to 1, got 2", &deck);
}

BOOST_AUTO_TEST_SUITE_END() // CommandLine

BOOST_AUTO_TEST_SUITE(Files)

BOOST_AUTO_TEST_CASE(HeatCapacityFile)
{
    std::istringstream text("# name c0 c1 c2 c3 tmin tmax\nC10 1 2 3 4 250 600\n");
    const auto entries = readCpFile(text, "(test)");
    BOOST_REQUIRE_EQUAL(entries.size(), 1u);
    BOOST_CHECK_EQUAL(entries[0].heatCapacity.c1, 2.0);
    BOOST_CHECK_EQUAL(entries[0].source, "--cp-file");
}

BOOST_AUTO_TEST_SUITE_END() // Files

BOOST_AUTO_TEST_SUITE(Output)

// A single state: the full block, with the enthalpy labelled by its model and
// its reference; the header neither repeats the command line nor lists units.
BOOST_AUTO_TEST_CASE(SingleState)
{
    const std::string text =
        outputOf<2>(runOf(halfAndHalf({"-P", "50", "-T", "300", "--kij", "C1:C10=0.0411"})));
    BOOST_CHECK(contains(text, "  p = 50 bar   T = 300 K\n"));
    BOOST_CHECK_EQUAL(countOf(text, "    component"), 1u);
    BOOST_CHECK(contains(text, " J/mol   (eos_departure; ideal gas only: "));
    BOOST_CHECK(contains(text, "; zero for the ideal gas at 298.15 K)\n"));
    BOOST_CHECK(contains(text, "built-in, 250 to 600 K"));
    BOOST_CHECK(!contains(text, "z from"));
    BOOST_CHECK(!contains(text, "units:"));
}

BOOST_AUTO_TEST_CASE(SolvedTemperature)
{
    const std::string text =
        outputOf<2>(runOf(halfAndHalf({"-P", "50", "-H", "-25000", "--kij", "C1:C10=0.0411"})));
    BOOST_CHECK(contains(text, " K (solved for h = -25000 J/mol)\n"));
}

// Several states: one line each, which --details turns into the full blocks.
BOOST_AUTO_TEST_CASE(SeveralStatesOneLineEach)
{
    const Args args = halfAndHalf({"-P", "50:300:50", "-T", "300", "--kij", "C1:C10=0.0411"});
    const std::string text = outputOf<2>(runOf(args));
    // the header (5 lines), the head of the table (3) and the 6 states
    BOOST_CHECK_EQUAL(countOf(text, "\n"), 14u);
    BOOST_CHECK_EQUAL(countOf(text, "    component"), 0u);
    BOOST_CHECK(contains(text, "  two "));
    BOOST_CHECK(contains(text, "  liquid "));

    Args detailed = args;
    detailed.push_back("--details");
    BOOST_CHECK_EQUAL(countOf(outputOf<2>(runOf(detailed)), "    component"), 6u);
}

// Without a heat capacity, the enthalpy line says why and what to give; the
// component lines of a deck say where each heat capacity comes from.
BOOST_AUTO_TEST_CASE(DeckComponentsAndMissingEnthalpy)
{
    const Opm::Deck lumped = builtinPairDeck(200.0);
    const std::string text = outputOf<2>(
        runOf({"--deck", "(test)", "-P", "50", "-T", "300", "--z", "C1=0.5,C10=0.5"}, &lumped));
    BOOST_CHECK(contains(text, "H: none, no ideal-gas heat capacity for C10 (give it with --cp "
                               "or --cp-file)"));
    BOOST_CHECK(contains(text, "components of (test)"));
    BOOST_CHECK(contains(text, "as built-in C1, 250 to 600 K"));
    BOOST_CHECK(contains(text, "none (give it with --cp or --cp-file)"));
    BOOST_CHECK(contains(text, "everything else is the deck's"));
}

BOOST_AUTO_TEST_CASE(NormalisedFeed)
{
    const Run run = runOf({"-P", "50", "-T", "300", "--z", "CO2=0.3333,C1=0.3333,C10=0.3333"});
    BOOST_CHECK_CLOSE(std::reduce(run.mixture.z.begin(), run.mixture.z.end()), 1.0, 1e-12);
    BOOST_CHECK(contains(outputOf<3>(run), "z normalised from a sum of 0.9999\n"));
}

BOOST_AUTO_TEST_SUITE_END() // Output

BOOST_AUTO_TEST_SUITE(Help)

// The examples of the help and of the short usage are valid command lines.
BOOST_AUTO_TEST_CASE(ExamplesAreValid)
{
    std::ostringstream usage;
    printUsage(usage);
    std::ostringstream shortUsage;
    printShortUsage(shortUsage);
    const auto examples = examplesOf(usage.str());
    const auto shortExamples = examplesOf(shortUsage.str());
    BOOST_CHECK_EQUAL(examples.size(), 5u);
    BOOST_CHECK_EQUAL(shortExamples.size(), 2u);
    for (const auto* list : {&examples, &shortExamples}) {
        for (const auto& args : *list) {
            const std::string message = refusal(args);
            BOOST_CHECK_MESSAGE(message.empty(), "an example is refused: " << message);
        }
    }

    std::ostringstream components;
    listComponents(components);
    BOOST_CHECK(contains(components.str(), "--z CO2=0.4,C1=0.2,C10=0.4"));
    BOOST_CHECK(refusal({"-P", "50", "-T", "300", "--z", "CO2=0.4,C1=0.2,C10=0.4"}).empty());
}

BOOST_AUTO_TEST_SUITE_END() // Help
