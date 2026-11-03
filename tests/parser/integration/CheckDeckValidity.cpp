/*
  Copyright 2014 Andreas Lauser

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

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/checkDeck.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <memory>
#include <sstream>
#include <string>


BOOST_AUTO_TEST_CASE( KeywordInCorrectSection ) {
    Opm::Parser parser;
    Opm::ParseContext parseContext;

    {
        Opm::ErrorGuard errorGuard;
        const char *correctDeckString =
            "RUNSPEC\n"
            "DIMENS\n"
            "3 3 3 /\n"
            "GRID\n"
            "DXV\n"
            "1 1 1 /\n"
            "DYV\n"
            "1 1 1 /\n"
            "DZV\n"
            "1 1 1 /\n"
            "BOX\n"
            "1 3 1 3 1 1 /\n"
            "TOPS\n"
            "9*100 /\n"
            "ENDBOX\n"
            "BOX\n"
            "1 3 1 3 1 3 /\n"
            "PROPS\n"
            "SOLUTION\n"
            "SCHEDULE\n";

        auto deck = parser.parseString(correctDeckString);
        BOOST_CHECK(Opm::checkDeck(deck, parser, parseContext, errorGuard));
        // prevent std::exit(1) in ErrorGuard's destructor!
        errorGuard.dump();
        errorGuard.clear();
    }

    {
        Opm::ErrorGuard errorGuard;
        // wrong section ordering
        const char *correctDeckString =
            "GRID\n"
            "RUNSPEC\n"
            "PROPS\n"
            "SOLUTION\n"
            "SCHEDULE\n";

        auto deck = parser.parseString(correctDeckString);
        BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
        BOOST_CHECK(Opm::checkDeck(deck, parser, parseContext, errorGuard, ~Opm::SectionTopology));
        // prevent std::exit(1) in ErrorGuard's destructor!
        errorGuard.dump();
        errorGuard.clear();
    }

    {
        Opm::ErrorGuard errorGuard;
        // the BOX keyword is in a section where it's not supposed to be
        const char *incorrectDeckString =
            "RUNSPEC\n"
            "BOX\n"
            "1 3 1 3 1 3 /\n"
            "DIMENS\n"
            "3 3 3 /\n"
            "GRID\n"
            "DXV\n"
            "1 1 1 /\n"
            "DYV\n"
            "1 1 1 /\n"
            "DZV\n"
            "1 1 1 /\n"
            "BOX\n"
            "1 3 1 3 1 1 /\n"
            "TOPS\n"
            "9*100 /\n"
            "ENDBOX\n"
            "PROPS\n"
            "SOLUTION\n"
            "SCHEDULE\n";

        auto deck = parser.parseString(incorrectDeckString);
        BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));

        // this is supposed to succeed as we don't ensure that all keywords are in the
        // correct section
        BOOST_CHECK(Opm::checkDeck(deck, parser, parseContext, errorGuard, Opm::SectionTopology));

        // this fails because of the incorrect BOX keyword
        BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard, Opm::SectionTopology | Opm::KeywordSection));
        // prevent std::exit(1) in ErrorGuard's destructor!
        errorGuard.dump();
        errorGuard.clear();
    }

    {
        Opm::ErrorGuard errorGuard;
        // the MULTIPLY TRANX is in a section where it's not supposed to be
        const char *incorrectDeckString =
            "RUNSPEC\n"
            "DIMENS\n"
            "3 3 3 /\n"
            "GRID\n"
            "DXV\n"
            "1 1 1 /\n"
            "DYV\n"
            "1 1 1 /\n"
            "DZV\n"
            "1 1 1 /\n"
            "BOX\n"
            "1 3 1 3 1 1 /\n"
            "TOPS\n"
            "9*100 /\n"
            "ENDBOX\n"
            "MULTIPLY\n"
            "'TRANX' 20.0 /\n"
            "'TRANY' 20.0 /\n"
            "/\n"
            "PROPS\n"
            "SOLUTION\n"
            "SCHEDULE\n";

        auto deck = parser.parseString(incorrectDeckString);
        BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));

        // this is supposed to succeed as we don't ensure that all keywords are in the
        // correct section
        BOOST_CHECK(Opm::checkDeck(deck, parser, parseContext, errorGuard, Opm::SectionTopology));

        // this fails because of the incorrect TRANX keyword
        BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard, Opm::SectionTopology | Opm::KeywordSection));
        // prevent std::exit(1) in ErrorGuard's destructor!
        errorGuard.dump();
        errorGuard.clear();
    }
}
BOOST_AUTO_TEST_CASE(Ignore_Leading_Global_Keywords)
{

    const auto* input_deck = R"(
ECHO
SKIP
NOECHO
COLUMNS
 10 10 /
SKIP100
SKIP300
ENDSKIP
FORMFEED
 10 /
RUNSPEC
DIMENS
10 10 10 /
GRID
PROPS
SOLUTION
SCHEDULE
)";

    Opm::ErrorGuard errorGuard;
    Opm::Parser parser;
    Opm::ParseContext parseContext;
    const auto deck = parser.parseString( input_deck);
    BOOST_CHECK(Opm::checkDeck(deck, parser, parseContext, errorGuard, Opm::SectionTopology | Opm::KeywordSection));
    // prevent std::exit(1) in ErrorGuard's destructor!
    errorGuard.dump();
    errorGuard.clear();
}

BOOST_AUTO_TEST_CASE(RepeatedUnitKeywordUsesInvalidUnitSectionPolicy)
{
    using UnitType = Opm::UnitSystem::UnitType;
    const struct {
        const char* runspecKeyword;
        const char* keyword;
        UnitType type;
        double lengthInSI;
    } cases[] = {
        {"METRIC", "METRIC", UnitType::UNIT_TYPE_METRIC, 10.0},
        {"FIELD", "FIELD", UnitType::UNIT_TYPE_FIELD, 3.048},
        {"LAB", "LAB", UnitType::UNIT_TYPE_LAB, 0.1},
        {"PVT-M", "PVT-M", UnitType::UNIT_TYPE_PVT_M, 10.0},
        {"", "METRIC", UnitType::UNIT_TYPE_METRIC, 10.0},
    };

    const Opm::Parser parser;
    for (const auto& test : cases) {
        BOOST_TEST_CONTEXT("RUNSPEC unit '" << test.runspecKeyword << "', repeat " << test.keyword) {
            const std::string input = std::string{"RUNSPEC\n"} + test.runspecKeyword +
                "\nDIMENS\n1 1 1 /\nGRID\nDX\n10 /\nPROPS\n" + test.keyword +
                "\nSOLUTION\nSCHEDULE\n";
            const auto deck = parser.parseString(input);

            // A repeated unit declaration is preserved even after dimensional data.
            BOOST_CHECK_EQUAL(deck[test.keyword].size(), test.runspecKeyword[0] ? 2U : 1U);
            BOOST_CHECK(deck.getActiveUnitSystem().getType() == test.type);
            BOOST_CHECK_CLOSE(deck["DX"].back().getSIDoubleData().at(0),
                              test.lengthInSI, 1.0e-10);

            Opm::ParseContext parseContext;
            Opm::ErrorGuard errorGuard;
            BOOST_CHECK(parseContext.get(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION) ==
                        Opm::InputErrorAction::WARN);
            BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
            BOOST_CHECK(!errorGuard);
            errorGuard.clear();

            parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION,
                                Opm::InputErrorAction::THROW_EXCEPTION);
            BOOST_CHECK_THROW(Opm::checkDeck(deck, parser, parseContext, errorGuard),
                              Opm::OpmInputError);
            BOOST_CHECK(!errorGuard);
            BOOST_CHECK(Opm::checkDeck(deck, parser, parseContext, errorGuard,
                                      Opm::SectionTopology));

            for (const auto action : {Opm::InputErrorAction::WARN,
                                      Opm::InputErrorAction::IGNORE}) {
                parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION, action);
                // The placement is still invalid, but the policy permits continuing.
                BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
                BOOST_CHECK(!errorGuard);
                errorGuard.clear();
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(MisplacedKeywordAndOperationRemainFatal)
{
    const Opm::Parser parser;
    const std::string inputs[] = {
        "RUNSPEC\nBOX\n1 1 1 1 1 1 /\nGRID\nPROPS\nSOLUTION\nSCHEDULE\n",
        "RUNSPEC\nGRID\nMULTIPLY\n'TRANX' 2 /\n/\nPROPS\nSOLUTION\nSCHEDULE\n",
        "RUNSPEC\nGRID\nADD\n'TRANX' 2 /\n/\nPROPS\nSOLUTION\nSCHEDULE\n",
        "RUNSPEC\nGRID\nMULTIPLY\n'METRIC' 2 /\n/\nPROPS\nSOLUTION\nSCHEDULE\n",
    };

    for (const auto& input : inputs) {
        BOOST_TEST_CONTEXT(input) {
            const auto deck = parser.parseString(input);
            for (const auto action : {Opm::InputErrorAction::THROW_EXCEPTION,
                                      Opm::InputErrorAction::WARN,
                                      Opm::InputErrorAction::IGNORE}) {
                Opm::ParseContext parseContext;
                Opm::ErrorGuard errorGuard;
                parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION, action);
                // Only actual, matching unit declarations use the configurable policy.
                BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
                BOOST_CHECK(errorGuard);
                errorGuard.clear();
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(SectionStructureErrorsRemainFatalUnderWarningPolicy)
{
    const Opm::Parser parser;
    Opm::ParseContext parseContext;
    parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION,
                        Opm::InputErrorAction::WARN);

    for (const auto* input : {
             "RUNSPEC\nPROPS\nSOLUTION\nSCHEDULE\n", // Missing GRID.
             "RUNSPEC\nGRID\nSOLUTION\nPROPS\nSCHEDULE\n", // Wrong order.
             "RUNSPEC\nGRID\nPROPS\nSOLUTION\n", // Missing SCHEDULE.
         }) {
        BOOST_TEST_CONTEXT(input) {
            const auto deck = parser.parseString(input);
            Opm::ErrorGuard errorGuard;
            BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
            BOOST_CHECK(errorGuard);
            errorGuard.clear();
        }
    }
}

BOOST_AUTO_TEST_CASE(RepeatedUnitKeywordReportsLocation)
{
    const auto* input = R"(
RUNSPEC
METRIC
DIMENS
1 1 1 /
TABDIMS
1 1 /
GRID
PROPS
METRIC
DENSITY
1 1 1 /
SOLUTION
SCHEDULE
)";
    const Opm::Parser parser;
    Opm::ParseContext parseContext;
    Opm::ErrorGuard errorGuard;
    const auto deck = parser.parseString(input, parseContext, errorGuard);

    // The repeated declaration stays in the deck for the section check.
    BOOST_CHECK_EQUAL(deck["METRIC"].size(), 2U);
    BOOST_CHECK(deck.getActiveUnitSystem().getType() ==
                Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC);
    BOOST_CHECK_CLOSE(deck["DENSITY"].back().getRecord(0).getItem(0).getSIDouble(0),
                      1.0, 1.0e-10);

    std::ostringstream warnings;
    const struct WarningLog {
        explicit WarningLog(std::ostream& stream)
        {
            Opm::OpmLog::addBackend("CHECK_DECK_VALIDITY",
                std::make_shared<Opm::StreamLog>(stream, Opm::Log::MessageType::Warning));
        }

        ~WarningLog()
        {
            Opm::OpmLog::removeBackend("CHECK_DECK_VALIDITY");
        }
    } warningLog(warnings);

    BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
    BOOST_CHECK(!errorGuard);
    const auto& location = deck["METRIC"].back().location();
    const auto warning = warnings.str();
    BOOST_CHECK(warning.find("METRIC") != std::string::npos);
    BOOST_CHECK(warning.find("PROPS") != std::string::npos);
    BOOST_CHECK(warning.find(location.filename) != std::string::npos);
    BOOST_CHECK(warning.find("line " + std::to_string(location.lineno)) != std::string::npos);

    warnings.str("");
    parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION,
                        Opm::InputErrorAction::IGNORE);
    BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
    BOOST_CHECK(!errorGuard);
    BOOST_CHECK(warnings.str().empty());
    errorGuard.clear();
}

BOOST_AUTO_TEST_CASE(ConflictingUnitKeywordsRemainFatal)
{
    using UnitType = Opm::UnitSystem::UnitType;
    const struct {
        const char* runspecKeyword;
        const char* misplacedKeyword;
        UnitType type;
        double lengthInSI;
    } cases[] = {
        {"METRIC", "FIELD", UnitType::UNIT_TYPE_METRIC, 10.0},
        {"FIELD", "METRIC", UnitType::UNIT_TYPE_FIELD, 3.048},
    };

    const Opm::Parser parser;
    for (const auto& test : cases) {
        BOOST_TEST_CONTEXT(test.runspecKeyword << " followed by " << test.misplacedKeyword) {
            const std::string input = std::string{"RUNSPEC\n"} + test.runspecKeyword +
                "\nDIMENS\n1 1 1 /\nGRID\n" + test.misplacedKeyword +
                "\nDX\n10 /\nPROPS\nSOLUTION\nSCHEDULE\n";
            const auto deck = parser.parseString(input);
            BOOST_CHECK_EQUAL(deck[test.misplacedKeyword].size(), 1U);
            BOOST_CHECK(deck.getActiveUnitSystem().getType() == test.type);
            BOOST_CHECK_CLOSE(deck["DX"].back().getSIDoubleData().at(0),
                              test.lengthInSI, 1.0e-10);

            for (const auto action : {Opm::InputErrorAction::THROW_EXCEPTION,
                                      Opm::InputErrorAction::WARN,
                                      Opm::InputErrorAction::IGNORE}) {
                Opm::ParseContext parseContext;
                Opm::ErrorGuard errorGuard;
                parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION, action);
                BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
                BOOST_CHECK(errorGuard);
                errorGuard.clear();
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(MisplacedUnitKeywordAfterDimensionalDataDoesNotChangeUnits)
{
    const auto* input = R"(
RUNSPEC
METRIC
DIMENS
1 1 1 /
GRID
DX
10 /
PROPS
FIELD
SOLUTION
SCHEDULE
)";
    const Opm::Parser parser;
    for (const auto action : {Opm::InputErrorAction::THROW_EXCEPTION,
                              Opm::InputErrorAction::WARN,
                              Opm::InputErrorAction::IGNORE}) {
        BOOST_TEST_CONTEXT("Action " << static_cast<int>(action)) {
            Opm::ParseContext parseContext;
            Opm::ErrorGuard errorGuard;
            parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION, action);

            const auto deck = parser.parseString(input, parseContext, errorGuard);
            BOOST_CHECK(deck.getActiveUnitSystem().getType() ==
                        Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC);
            BOOST_CHECK_CLOSE(deck["DX"].back().getSIDoubleData().at(0), 10.0, 1.0e-10);

            BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
            BOOST_CHECK(errorGuard);
            errorGuard.clear();
        }
    }
}

BOOST_AUTO_TEST_CASE(RepeatedUnitKeywordCanBeReportedAsDelayedError)
{
    const Opm::Parser parser;
    const auto deck = parser.parseString(
        "RUNSPEC\nMETRIC\nGRID\nPROPS\nMETRIC\nSOLUTION\nSCHEDULE\n");
    Opm::ParseContext parseContext;
    Opm::ErrorGuard errorGuard;
    parseContext.update(Opm::ParseContext::PARSE_INVALID_UNIT_SECTION,
                        Opm::InputErrorAction::DELAYED_EXIT1);

    BOOST_CHECK(!Opm::checkDeck(deck, parser, parseContext, errorGuard));
    BOOST_CHECK(errorGuard);
    errorGuard.clear();
}
