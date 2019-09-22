/*
  Copyright (c) 2019 Equinor ASA

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

#define BOOST_TEST_MODULE SummaryParameterTests

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/Summary/BlockParameter.hpp>
#include <opm/output/eclipse/Summary/EvaluateQuantity.hpp>
#include <opm/output/eclipse/Summary/SummaryParameter.hpp>
#include <opm/output/eclipse/Summary/WellParameter.hpp>

#include <opm/output/eclipse/RegionCache.hpp>
#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
    struct Setup
    {
        explicit Setup(const std::string& fname)
            : Setup{ ::Opm::Parser{}.parseFile(fname) }
        {}

        explicit Setup(const Opm::Deck& deck)
            : ctxt {}
            , err  {}
            , es   { deck,     ctxt, err }
            , sched{ deck, es, ctxt, err }
        {}

        Opm::ParseContext ctxt;
        Opm::ErrorGuard   err;
        Opm::EclipseState es;
        Opm::Schedule     sched;
    };

    double sm3_pr_day()
    {
        using namespace Opm::unit;

        return cubic(meter) / day;
    }

    double rm3_pr_day()
    {
        using namespace Opm::unit;

        return cubic(meter) / day;
    }

    double sm3_pr_day_barsa()
    {
        using namespace Opm::unit;

        return cubic(meter) / day / barsa;
    }
} // Anonymous

BOOST_AUTO_TEST_SUITE(Block_Parameters)

namespace {
    Opm::data::WellRates wellResults()
    {
        return {};
    }

    std::map<std::string, double> singleResults()
    {
        return {};
    }

    std::map<std::string, std::vector<double>> regionResults()
    {
        return {};
    }

    std::map<std::pair<std::string, int>, double> blockResults()
    {
        using Key = std::map<std::pair<std::string, int>, double>::key_type;

        return {
            { Key{ "BPR"  , 1234 }, 123.4*Opm::unit::barsa },
            { Key{ "BOKR" ,   10 }, 0.128 },
            { Key{ "BGVIS",  512 }, 0.256*Opm::prefix::centi*Opm::unit::Poise },
        };
    }
} // Anonymous

BOOST_AUTO_TEST_SUITE(Construct)

BOOST_AUTO_TEST_CASE(Pressure)
{
    const auto bpr_1234 = ::Opm::BlockParameter {
        1234, Opm::UnitSystem::measure::pressure, "BPR"
    };

    BOOST_CHECK_EQUAL(bpr_1234.summaryKey(), "BPR:1234");
    BOOST_CHECK_EQUAL(bpr_1234.keyword(), "BPR");
    BOOST_CHECK_EQUAL(bpr_1234.name(), ":+:+:+:+");
    BOOST_CHECK_EQUAL(bpr_1234.num(), 1234);

    {
        const auto usys = ::Opm::UnitSystem::newMETRIC();
        BOOST_CHECK_EQUAL(bpr_1234.unit(usys), "BARSA");
    }

    {
        const auto usys = ::Opm::UnitSystem::newFIELD();
        BOOST_CHECK_EQUAL(bpr_1234.unit(usys), "PSIA");
    }

    {
        const auto usys = ::Opm::UnitSystem::newLAB();
        BOOST_CHECK_EQUAL(bpr_1234.unit(usys), "ATM");
    }

    {
        const auto usys = ::Opm::UnitSystem::newPVT_M();
        BOOST_CHECK_EQUAL(bpr_1234.unit(usys), "ATM");
    }
}

BOOST_AUTO_TEST_CASE(Oil_Kr)
{
    const auto bokr_10 = ::Opm::BlockParameter {
        10, Opm::UnitSystem::measure::identity, "BOKR"
    };

    BOOST_CHECK_EQUAL(bokr_10.summaryKey(), "BOKR:10");
    BOOST_CHECK_EQUAL(bokr_10.keyword(), "BOKR");
    BOOST_CHECK_EQUAL(bokr_10.name(), ":+:+:+:+");
    BOOST_CHECK_EQUAL(bokr_10.num(), 10);

    const auto expect_unit = std::string{ "" };

    {
        const auto usys = ::Opm::UnitSystem::newMETRIC();
        BOOST_CHECK_EQUAL(bokr_10.unit(usys), expect_unit);
    }

    {
        const auto usys = ::Opm::UnitSystem::newFIELD();
        BOOST_CHECK_EQUAL(bokr_10.unit(usys), expect_unit);
    }

    {
        const auto usys = ::Opm::UnitSystem::newLAB();
        BOOST_CHECK_EQUAL(bokr_10.unit(usys), expect_unit);
    }

    {
        const auto usys = ::Opm::UnitSystem::newPVT_M();
        BOOST_CHECK_EQUAL(bokr_10.unit(usys), expect_unit);
    }
}

BOOST_AUTO_TEST_CASE(Gas_Viscosity)
{
    const auto bgvis_512 = ::Opm::BlockParameter {
        512, Opm::UnitSystem::measure::viscosity, "BGVIS"
    };

    BOOST_CHECK_EQUAL(bgvis_512.summaryKey(), "BGVIS:512");
    BOOST_CHECK_EQUAL(bgvis_512.keyword(), "BGVIS");
    BOOST_CHECK_EQUAL(bgvis_512.name(), ":+:+:+:+");
    BOOST_CHECK_EQUAL(bgvis_512.num(), 512);

    const auto expect_unit = std::string{ "CP" };

    {
        const auto usys = ::Opm::UnitSystem::newMETRIC();
        BOOST_CHECK_EQUAL(bgvis_512.unit(usys), expect_unit);
    }

    {
        const auto usys = ::Opm::UnitSystem::newFIELD();
        BOOST_CHECK_EQUAL(bgvis_512.unit(usys), expect_unit);
    }

    {
        const auto usys = ::Opm::UnitSystem::newLAB();
        BOOST_CHECK_EQUAL(bgvis_512.unit(usys), expect_unit);
    }

    {
        const auto usys = ::Opm::UnitSystem::newPVT_M();
        BOOST_CHECK_EQUAL(bgvis_512.unit(usys), expect_unit);
    }
}

BOOST_AUTO_TEST_SUITE_END() // Construct

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Values)

BOOST_AUTO_TEST_CASE(Pressure)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    const auto bpr_1234 = ::Opm::BlockParameter {
        1234, Opm::UnitSystem::measure::pressure, "BPR"
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    bpr_1234.update(1, 86400.0, input, simRes, st);
    BOOST_CHECK(st.has(bpr_1234.summaryKey()));

    BOOST_CHECK_CLOSE(st.get("BPR:1234"), 123.4, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Oil_Kr)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    const auto bokr_10 = ::Opm::BlockParameter {
        10, Opm::UnitSystem::measure::identity, "BOKR"
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    bokr_10.update(1, 86400.0, input, simRes, st);
    BOOST_CHECK(st.has(bokr_10.summaryKey()));

    BOOST_CHECK_CLOSE(st.get("BOKR:10"), 0.128, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Gas_Viscosity)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    const auto bgvis_512 = ::Opm::BlockParameter {
        512, Opm::UnitSystem::measure::viscosity, "BGVIS"
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    bgvis_512.update(1, 86400.0, input, simRes, st);
    BOOST_CHECK(st.has(bgvis_512.summaryKey()));

    BOOST_CHECK_CLOSE(st.get("BGVIS:512"), 0.256, 1.0e-10);
}

BOOST_AUTO_TEST_SUITE_END() // Values

BOOST_AUTO_TEST_SUITE_END() // Block_Parameters

// =====================================================================

BOOST_AUTO_TEST_SUITE(FunctionHandlerTable)

namespace {
    std::vector<std::string> supportedVectors()
    {
        return {
#if 0
            // ---------------------------------------------------------
            // Connection quantities
            "CCIR", "CCIT", "CGIR", "CGIT", "CGPR", "CGPT", "CNFR", "CNIT",
            "CNPT", "COPR", "COPT", "CTFAC", "CWIR", "CWIT", "CWPR", "CWPT",
#endif
            // ---------------------------------------------------------
            // Field quantities
            "FCIR", "FCIT",
            "FGIR", "FGIRH", "FGIT", "FGITH",
            "FGLR", "FGLRH", "FGOR", "FGORH",
            "FGPI", "FGPP",
            "FGPR", "FGPRF", "FGPRH", "FGPRS",
            "FGPT", "FGPTF", "FGPTH", "FGPTS",
            "FGVIR", "FGVIT", "FGVPR", "FGVPT",
            "FLIR", "FLIT", "FLPR", "FLPRH", "FLPT", "FLPTH",
            "FMWIN", "FMWIT", "FMWPR", "FMWPT",
            "FNIR", "FNIT", "FNPR", "FNPT",
            "FOIR", "FOIRH", "FOIT", "FOITH",
            "FOPI", "FOPP",
            "FOPR", "FOPRF", "FOPRH", "FOPRS",
            "FOPT", "FOPTF", "FOPTH", "FOPTS",
            "FOVIR", "FOVIT", "FOVPR", "FOVPT",
            "FVIR", "FVIT", "FVPR", "FVPRT", "FVPT",
            "FWCT", "FWCTH",
            "FWIR", "FWIRH", "FWIT", "FWITH",
            "FWPI", "FWPP",
            "FWPR", "FWPRH", "FWPT", "FWPTH",
            "FWVIR", "FWVIT", "FWVPR", "FWVPT",

            // ---------------------------------------------------------
            // Group quantities
            "GCIR", "GCIT",
            "GGIR", "GGIRH", "GGIT", "GGITH",
            "GGLR", "GGLRH", "GGOR", "GGORH",
            "GGPI", "GGPP",
            "GGPR", "GGPRF", "GGPRH", "GGPRS",
            "GGPT", "GGPTF", "GGPTH", "GGPTS",
            "GGVIR", "GGVIT", "GGVPR", "GGVPT",
            "GLIR", "GLIT", "GLPR", "GLPRH", "GLPT", "GLPTH",
            "GMWIN", "GMWIT", "GMWPR", "GMWPT",
            "GNIR", "GNIT", "GNPR", "GNPT",
            "GOIR", "GOIRH", "GOIT", "GOITH",
            "GOPI", "GOPP",
            "GOPR", "GOPRF", "GOPRH", "GOPRS",
            "GOPT", "GOPTF", "GOPTH", "GOPTS",
            "GOVIR", "GOVIT", "GOVPR", "GOVPT",
            "GVIR", "GVIT", "GVPR", "GVPRT", "GVPT",
            "GWCT", "GWCTH",
            "GWIR", "GWIRH", "GWIT", "GWITH",
            "GWPI", "GWPP",
            "GWPR", "GWPRH", "GWPT", "GWPTH",
            "GWVIR", "GWVIT", "GWVPR", "GWVPT",
#if 0
            // ---------------------------------------------------------
            // Region quantities
            "RGIR",
            "RGIT", "RGPR", "RGPT", "ROIR", "ROIT", "ROPR", "ROPT", "RWIR", "RWIT", "RWPR",
            "RWPT",
#endif
            // ---------------------------------------------------------
            // Segment quantities
            "SGFR", "SOFR", "SPR", "SWFR",

            // ---------------------------------------------------------
            // Well quantities
            "WBHP", "WBHPH",                 // Well only
            "WCIR", "WCIT",
            "WGIR", "WGIRH", "WGIT", "WGITH",
            "WGLR", "WGLRH", "WGOR", "WGORH",
            "WGPI", "WGPP",
            "WGPR", "WGPRF", "WGPRH", "WGPRS",
            "WGPT", "WGPTF", "WGPTH", "WGPTS",
            "WGVIR", "WGVIT", "WGVPR", "WGVPT",
            "WLIR", "WLIT", "WLPR", "WLPRH", "WLPT", "WLPTH",
            "WNIR", "WNIT", "WNPR", "WNPT",
            "WOIR", "WOIRH", "WOIT", "WOITH",
            "WOPI", "WOPP",
            "WOPR", "WOPRF", "WOPRH", "WOPRS",
            "WOPT", "WOPTF", "WOPTH", "WOPTS",
            "WOVIR", "WOVIT", "WOVPR", "WOVPT",
            "WPIG", "WPIL", "WPIO", "WPIW",  // Well only
            "WTHP", "WTHPH",                 // Well only
            "WVIR", "WVIT", "WVPR", "WVPRT", "WVPT",
            "WWCT", "WWCTH",
            "WWIR", "WWIRH", "WWIT", "WWITH",
            "WWPI", "WWPP",
            "WWPR", "WWPRH", "WWPT", "WWPTH",
            "WWVIR", "WWVIT", "WWVPR", "WWVPT",
        };
    }
}

BOOST_AUTO_TEST_CASE(Supported_Vectors)
{
    const auto ref = supportedVectors();
    auto supp = ::Opm::SummaryHelpers::supportedKeywords();

    std::sort(supp.begin(), supp.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(supp.begin(), supp.end(),
                                  ref.begin(), ref.end());
}

BOOST_AUTO_TEST_CASE(WBHP)
{
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WBHP");
    BOOST_CHECK_MESSAGE(eval != nullptr, "WBHP Evalulator must not be null");
}

BOOST_AUTO_TEST_SUITE_END() // FunctionHandlerTable

// =====================================================================

BOOST_AUTO_TEST_SUITE(Well_Parameters)

BOOST_AUTO_TEST_SUITE(Construct)

BOOST_AUTO_TEST_CASE(WBHP)
{
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WBHP");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WBHP" },
        Opm::WellParameter::UnitString { "BARSA" },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);

    prm.pressure(Opm::WellParameter::Pressure::BHP);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::THP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Rate),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Ratio),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Total),
                      std::invalid_argument);

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "BARSA");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "BARSA"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "BARSA");   // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "BARSA"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_1");
    BOOST_CHECK_EQUAL(prm2->keyword(), "WBHP");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "WBHP:OP_1");
}

BOOST_AUTO_TEST_CASE(WTHP)
{
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WTHP");

    BOOST_REQUIRE_MESSAGE(eval != nullptr,
                          "Invalid evaluator function table for THP");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_2" },
        Opm::WellParameter::Keyword    { "WTHP" },
        Opm::WellParameter::UnitString { "ATM" },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);

    prm.pressure(Opm::WellParameter::Pressure::THP);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::BHP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Rate),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Ratio),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Total),
                      std::invalid_argument);

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "ATM"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "ATM"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "ATM");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "ATM");

    BOOST_CHECK_EQUAL(prm2->name(), "OP_2");
    BOOST_CHECK_EQUAL(prm2->keyword(), "WTHP");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "WTHP:OP_2");
}

BOOST_AUTO_TEST_CASE(WOPR)
{
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPR");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_3" },
        Opm::WellParameter::Keyword    { "WOPR" },
        Opm::WellParameter::UnitString { "SCC/HR" },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);

    prm.flowType(Opm::WellParameter::FlowType::Rate);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::BHP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::THP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Ratio),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Total),
                      std::invalid_argument);

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "SCC/HR"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "SCC/HR"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "SCC/HR");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "SCC/HR"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_3");
    BOOST_CHECK_EQUAL(prm2->keyword(), "WOPR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "WOPR:OP_3");
}

BOOST_AUTO_TEST_CASE(WLPT)
{
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WLPT");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_2" },
        Opm::WellParameter::Keyword    { "WLPT" },
        Opm::WellParameter::UnitString { "SM3" },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);

    prm.flowType(Opm::WellParameter::FlowType::Total);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::BHP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::THP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Rate),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Ratio),
                      std::invalid_argument);

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "SM3"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "SM3"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "SM3");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "SM3"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_2");
    BOOST_CHECK_EQUAL(prm2->keyword(), "WLPT");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "WLPT:OP_2");
}

BOOST_AUTO_TEST_CASE(WGLR)
{
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGLR");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WGLR" },
        Opm::WellParameter::UnitString { "STB/STB" },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);

    prm.flowType(Opm::WellParameter::FlowType::Ratio);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::BHP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.pressure(Opm::WellParameter::Pressure::THP),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Rate),
                      std::invalid_argument);
    BOOST_CHECK_THROW(prm.flowType(Opm::WellParameter::FlowType::Total),
                      std::invalid_argument);

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "STB/STB"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "STB/STB"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "STB/STB");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "STB/STB"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_1");
    BOOST_CHECK_EQUAL(prm2->keyword(), "WGLR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "WGLR:OP_1");
}

BOOST_AUTO_TEST_SUITE_END() // Construct

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Dynamic_Simulator_Values)

namespace {
    Opm::data::Well OP_1()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for producers
        xw.rates.set(r::oil, - 10.0e3*sm3_pr_day());
        xw.rates.set(r::gas, -100.0e3*sm3_pr_day());
        xw.rates.set(r::wat, - 50.0e3*sm3_pr_day());

        xw.rates.set(r::polymer, - 123.4*sm3_pr_day());
        xw.rates.set(r::solvent, -5432.1*sm3_pr_day());

        xw.rates.set(r::dissolved_gas, -82.15e3*sm3_pr_day());
        xw.rates.set(r::vaporized_oil, -1000.0*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  , -30.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  , - 4.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, -49.0e3*rm3_pr_day());

        xw.rates.set(r::productivity_index_water, 876.5*sm3_pr_day_barsa());
        xw.rates.set(r::productivity_index_oil, 654.32*sm3_pr_day_barsa());
        xw.rates.set(r::productivity_index_gas, 432.1*sm3_pr_day_barsa());

        xw.rates.set(r::well_potential_water, 65.43e3*sm3_pr_day());
        xw.rates.set(r::well_potential_oil, 37.92e3*sm3_pr_day());
        xw.rates.set(r::well_potential_gas, 123.45e3*sm3_pr_day());

        xw.bhp = 256.512*Opm::unit::barsa;
        xw.thp = 128.123*Opm::unit::barsa;

        return xw;
    }

    Opm::data::Well OP_2()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Positive rate signs for injectors
        xw.rates.set(r::oil,  3.0*sm3_pr_day());
        xw.rates.set(r::gas, 80.0e3*sm3_pr_day());
        xw.rates.set(r::wat, 20.0e3*sm3_pr_day());

        xw.rates.set(r::polymer, 128.256*sm3_pr_day());
        xw.rates.set(r::solvent, 25.75*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  ,  2.9*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  ,  4.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, 19.0e3*rm3_pr_day());

        xw.rates.set(r::well_potential_water, 543.21e3*sm3_pr_day());
        xw.rates.set(r::well_potential_oil, 12345.6*sm3_pr_day());
        xw.rates.set(r::well_potential_gas, 86420.8*sm3_pr_day());

        xw.bhp = 512.1*Opm::unit::barsa;
        xw.thp = 150.0*Opm::unit::barsa;

        return xw;
    }

    Opm::data::WellRates wellResults()
    {
        auto xw = Opm::data::WellRates{};

        xw["OP_1"] = OP_1();
        xw["OP_2"] = OP_2();

        return xw;
    }

    std::map<std::string, double> singleResults()
    {
        return {};
    }

    std::map<std::string, std::vector<double>> regionResults()
    {
        return {};
    }

    std::map<std::pair<std::string, int>, double> blockResults()
    {
        return {};
    }
} // Anonymous

BOOST_AUTO_TEST_CASE(WBHP)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WBHP");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WBHP" },
        Opm::WellParameter::UnitString { "BARSA" },
        *eval
    }.pressure(Opm::WellParameter::Pressure::BHP).validate();

    auto sprm = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm) }
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    sprm->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(sprm->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WBHP:OP_1"), 256.512, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WTHP)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WTHP");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WTHP" },
        Opm::WellParameter::UnitString { "BARSA" },
        *eval
    }.pressure(Opm::WellParameter::Pressure::THP).validate();

    auto sprm = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm) }
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    sprm->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(sprm->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WTHP:OP_1"), 128.123, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WPIG)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WPIG");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WPIG" },
        Opm::WellParameter::UnitString { "SM3/DAY" },
        *eval
    }.flowType(Opm::WellParameter::FlowType::Rate).validate();

    auto sprm = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm) }
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    sprm->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(sprm->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WPIG:OP_1"), 432.1, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WPIO)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WPIO");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WPIO" },
        Opm::WellParameter::UnitString { "SM3/DAY" },
        *eval
    }.flowType(Opm::WellParameter::FlowType::Rate).validate();

    auto sprm = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm) }
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    sprm->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(sprm->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WPIO:OP_1"), 654.32, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WPIW)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WPIW");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WPIW" },
        Opm::WellParameter::UnitString { "SM3/DAY" },
        *eval
    }.flowType(Opm::WellParameter::FlowType::Rate).validate();

    auto sprm = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm) }
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    sprm->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(sprm->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WPIW:OP_1"), 876.5, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WPIL)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WPIL");

    auto prm = ::Opm::WellParameter {
        Opm::WellParameter::WellName   { "OP_1" },
        Opm::WellParameter::Keyword    { "WPIL" },
        Opm::WellParameter::UnitString { "SM3/DAY" },
        *eval
    }.flowType(Opm::WellParameter::FlowType::Rate).validate();

    auto sprm = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::WellParameter { std::move(prm) }
    };

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    sprm->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(sprm->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WPIL:OP_1"), 1530.82, 1.0e-10); // W+O
}

BOOST_AUTO_TEST_CASE(WOPx)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wopr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wovpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto wovpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto woprs = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopts = std::unique_ptr<Opm::SummaryParameter>{};
    auto woprf = std::unique_ptr<Opm::SummaryParameter>{};
    auto woptf = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wopr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wopt.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOVPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOVPR" },
            Opm::WellParameter::UnitString { "RM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wovpr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOVPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOVPT" },
            Opm::WellParameter::UnitString { "RM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wovpt.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPRS");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPRS" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        woprs.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPTS");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPTS" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wopts.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPRF");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPRF" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        woprf.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPTF");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPTF" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        woptf.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPP");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WOPP" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wopp.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    wopr ->update(1, cse.sched.seconds(1), input, simRes, st);
    wopt ->update(1, cse.sched.seconds(1), input, simRes, st);
    wovpr->update(1, cse.sched.seconds(1), input, simRes, st);
    wovpt->update(1, cse.sched.seconds(1), input, simRes, st);
    woprs->update(1, cse.sched.seconds(1), input, simRes, st);
    wopts->update(1, cse.sched.seconds(1), input, simRes, st);
    woprf->update(1, cse.sched.seconds(1), input, simRes, st);
    woptf->update(1, cse.sched.seconds(1), input, simRes, st);
    wopp ->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(wopr ->summaryKey()));
    BOOST_CHECK(st.has(wopt ->summaryKey()));
    BOOST_CHECK(st.has(wovpr->summaryKey()));
    BOOST_CHECK(st.has(wovpt->summaryKey()));
    BOOST_CHECK(st.has(woprs->summaryKey()));
    BOOST_CHECK(st.has(wopts->summaryKey()));
    BOOST_CHECK(st.has(woprf->summaryKey()));
    BOOST_CHECK(st.has(woptf->summaryKey()));
    BOOST_CHECK(st.has(wopp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WOPR:OP_1"), 10.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOVPR:OP_1"), 30.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPRS:OP_1"), 1.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPRF:OP_1"), 9.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPP:OP_1"), 37.92e3, 1.0e-10);

    // Constant rates for each of 11,403 days
    BOOST_CHECK_CLOSE(st.get("WOPT:OP_1"), 114.03e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOVPT:OP_1"), 342.09e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPTS:OP_1"), 11.403e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPTF:OP_1"), 102.627e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WGPx)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wgpr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgpt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgvpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgvpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgprs = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgpts = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgprf = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgptf = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgpp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgpr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgpt.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGVPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGVPR" },
            Opm::WellParameter::UnitString { "RM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgvpr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGVPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGVPT" },
            Opm::WellParameter::UnitString { "RM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgvpt.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPRS");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPRS" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgprs.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPTS");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPTS" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgpts.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPRF");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPRF" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgprf.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPTF");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPTF" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgptf.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPP");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGPP" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgpp.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    wgpr ->update(1, cse.sched.seconds(1), input, simRes, st);
    wgpt ->update(1, cse.sched.seconds(1), input, simRes, st);
    wgvpr->update(1, cse.sched.seconds(1), input, simRes, st);
    wgvpt->update(1, cse.sched.seconds(1), input, simRes, st);
    wgprs->update(1, cse.sched.seconds(1), input, simRes, st);
    wgpts->update(1, cse.sched.seconds(1), input, simRes, st);
    wgprf->update(1, cse.sched.seconds(1), input, simRes, st);
    wgptf->update(1, cse.sched.seconds(1), input, simRes, st);
    wgpp ->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(wgpr ->summaryKey()));
    BOOST_CHECK(st.has(wgpt ->summaryKey()));
    BOOST_CHECK(st.has(wgvpr->summaryKey()));
    BOOST_CHECK(st.has(wgvpt->summaryKey()));
    BOOST_CHECK(st.has(wgprs->summaryKey()));
    BOOST_CHECK(st.has(wgpts->summaryKey()));
    BOOST_CHECK(st.has(wgprf->summaryKey()));
    BOOST_CHECK(st.has(wgptf->summaryKey()));
    BOOST_CHECK(st.has(wgpp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WGPR:OP_1"), 100.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGVPR:OP_1"), 4.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGPRS:OP_1"), 82.15e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGPRF:OP_1"), 17.85e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGPP:OP_1"), 123.45e3, 1.0e-10);

    // Constant rates for each of 11,403 days
    BOOST_CHECK_CLOSE(st.get("WGPT:OP_1"), 1140.3e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGVPT:OP_1"), 45.612e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGPTS:OP_1"), 936.75645e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGPTF:OP_1"), 203.54355e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WWPx)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wwpr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwpt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwvpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwvpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwpp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WWPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwpr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WWPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wwpt.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWVPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WWVPR" },
            Opm::WellParameter::UnitString { "RM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwvpr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWVPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WWVPT" },
            Opm::WellParameter::UnitString { "RM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wwvpt.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWPP");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WWPP" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwpp.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    wwpr ->update(1, cse.sched.seconds(1), input, simRes, st);
    wwpt ->update(1, cse.sched.seconds(1), input, simRes, st);
    wwvpr->update(1, cse.sched.seconds(1), input, simRes, st);
    wwvpt->update(1, cse.sched.seconds(1), input, simRes, st);
    wwpp ->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(wwpr ->summaryKey()));
    BOOST_CHECK(st.has(wwpt ->summaryKey()));
    BOOST_CHECK(st.has(wwvpr->summaryKey()));
    BOOST_CHECK(st.has(wwvpt->summaryKey()));
    BOOST_CHECK(st.has(wwpp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WWPR:OP_1"), 50.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWVPR:OP_1"), 49.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWPP:OP_1"), 65.43e3, 1.0e-10);

    // Constant rates for each of 11,403 days
    BOOST_CHECK_CLOSE(st.get("WWPT:OP_1"), 570.15e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWVPT:OP_1"), 558.747e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WOIx)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto woir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto woit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wovir = std::unique_ptr<Opm::SummaryParameter>{};
    auto wovit = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopi  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WOIR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        woir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WOIT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        woit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOVIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WOVIR" },
            Opm::WellParameter::UnitString { "RM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wovir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOVIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WOVIT" },
            Opm::WellParameter::UnitString { "RM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wovit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPI");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WOPI" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wopi.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    woir ->update(2, dt, input, simRes, st);
    woit ->update(2, dt, input, simRes, st);
    wovir->update(2, dt, input, simRes, st);
    wovit->update(2, dt, input, simRes, st);
    wopi ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(woir ->summaryKey()));
    BOOST_CHECK(st.has(woit ->summaryKey()));
    BOOST_CHECK(st.has(wovir->summaryKey()));
    BOOST_CHECK(st.has(wovit->summaryKey()));
    BOOST_CHECK(st.has(wopi ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WOIR:OP_2"), 3.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOVIR:OP_2"), 2.9, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPI:OP_2"), 12345.6, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("WOIT:OP_2"), 2631.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOVIT:OP_2"), 2543.3, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WGIx)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wgir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgvir = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgvit = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgpi  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGIR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGIT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGVIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGVIR" },
            Opm::WellParameter::UnitString { "RM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgvir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGVIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGVIT" },
            Opm::WellParameter::UnitString { "RM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgvit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPI");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGPI" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgpi.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    wgir ->update(2, dt, input, simRes, st);
    wgit ->update(2, dt, input, simRes, st);
    wgvir->update(2, dt, input, simRes, st);
    wgvit->update(2, dt, input, simRes, st);
    wgpi ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(wgir ->summaryKey()));
    BOOST_CHECK(st.has(wgit ->summaryKey()));
    BOOST_CHECK(st.has(wgvir->summaryKey()));
    BOOST_CHECK(st.has(wgvit->summaryKey()));
    BOOST_CHECK(st.has(wgpi ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WGIR:OP_2"), 80.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGVIR:OP_2"), 4.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGPI:OP_2"), 86420.8, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("WGIT:OP_2"), 70.16e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGVIT:OP_2"), 3.508e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WWIx)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wwir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwvir = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwvit = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwpi  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wlir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wlit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wvir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto wvit  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WWIR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WWIT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wwit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWVIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WWVIR" },
            Opm::WellParameter::UnitString { "RM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwvir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWVIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WWVIT" },
            Opm::WellParameter::UnitString { "RM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wwvit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWPI");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WWPI" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwpi.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WLIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WLIR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wlir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WLIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WLIT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wlit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WVIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WVIR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wvir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WVIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WVIT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wvit.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    wwir ->update(2, dt, input, simRes, st);
    wwit ->update(2, dt, input, simRes, st);
    wwvir->update(2, dt, input, simRes, st);
    wwvit->update(2, dt, input, simRes, st);
    wwpi ->update(2, dt, input, simRes, st);
    wlir ->update(2, dt, input, simRes, st);
    wlit ->update(2, dt, input, simRes, st);
    wvir ->update(2, dt, input, simRes, st);
    wvit ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(wwir ->summaryKey()));
    BOOST_CHECK(st.has(wwit ->summaryKey()));
    BOOST_CHECK(st.has(wwvir->summaryKey()));
    BOOST_CHECK(st.has(wwvit->summaryKey()));
    BOOST_CHECK(st.has(wwpi ->summaryKey()));
    BOOST_CHECK(st.has(wlir ->summaryKey()));
    BOOST_CHECK(st.has(wlit ->summaryKey()));
    BOOST_CHECK(st.has(wvir ->summaryKey()));
    BOOST_CHECK(st.has(wvit ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WWIR:OP_2"), 20.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWVIR:OP_2"), 19.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWPI:OP_2"), 543.21e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WLIR:OP_2"), 20.003e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WVIR:OP_2"), 23.0029e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("WWIT:OP_2"), 17.54e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWVIT:OP_2"), 16.663e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WLIT:OP_2"), 17.542631e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WVIT:OP_2"), 20.1735433e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WxR)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wgor   = std::unique_ptr<Opm::SummaryParameter>{};
    auto wglr   = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwct   = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgor_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wglr_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwct_2 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGOR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGOR" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wgor.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGLR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WGLR" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wglr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWCT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WWCT" },
            Opm::WellParameter::UnitString { "" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wwct.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGOR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGOR" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wgor_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGLR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WGLR" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wglr_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWCT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WWCT" },
            Opm::WellParameter::UnitString { "" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wwct_2.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    wgor  ->update(2, dt, input, simRes, st);
    wglr  ->update(2, dt, input, simRes, st);
    wwct  ->update(2, dt, input, simRes, st);
    wgor_2->update(2, dt, input, simRes, st);
    wglr_2->update(2, dt, input, simRes, st);
    wwct_2->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(wgor  ->summaryKey()));
    BOOST_CHECK(st.has(wglr  ->summaryKey()));
    BOOST_CHECK(st.has(wwct  ->summaryKey()));
    BOOST_CHECK(st.has(wgor_2->summaryKey()));
    BOOST_CHECK(st.has(wglr_2->summaryKey()));
    BOOST_CHECK(st.has(wwct_2->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WGOR:OP_1"), 10.0, 1.0e-10);    // 100/10
    BOOST_CHECK_CLOSE(st.get("WGLR:OP_1"), 5.0/3.0, 1.0e-10); // 100/(50+10)
    BOOST_CHECK_CLOSE(st.get("WWCT:OP_1"), 5.0/6.0, 1.0e-10); //  50/(50+10)

    // All producing ratios should be zero for injectors
    BOOST_CHECK_CLOSE(st.get("WGOR:OP_2"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGLR:OP_2"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWCT:OP_2"), 0.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Solvent)
{
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wnir = std::unique_ptr<Opm::SummaryParameter>{};
    auto wnit = std::unique_ptr<Opm::SummaryParameter>{};
    auto wnpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto wnpt = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WNIR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WNIR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wnir.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WNIT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_2" },
            Opm::WellParameter::Keyword    { "WNIT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wnit.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WNPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WNPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wnpr.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WNPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "OP_1" },
            Opm::WellParameter::Keyword    { "WNPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wnpt.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    wnir->update(2, dt, input, simRes, st);
    wnit->update(2, dt, input, simRes, st);
    wnpr->update(2, dt, input, simRes, st);
    wnpt->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(wnir->summaryKey()));
    BOOST_CHECK(st.has(wnit->summaryKey()));
    BOOST_CHECK(st.has(wnpr->summaryKey()));
    BOOST_CHECK(st.has(wnpt->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WNIR:OP_2"), 25.75, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WNPR:OP_1"), 5432.1, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("WNIT:OP_2"), 22.58275e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WNPT:OP_1"), 4.7639517e6, 1.0e-10);
}

BOOST_AUTO_TEST_SUITE_END() // Dynamic_Simulator_Values

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Observed_Control_Values)

namespace {
    Opm::data::WellRates wellResults()
    {
        return {};
    }

    std::map<std::string, double> singleResults()
    {
        return {};
    }

    std::map<std::string, std::vector<double>> regionResults()
    {
        return {};
    }

    std::map<std::pair<std::string, int>, double> blockResults()
    {
        return {};
    }
}

BOOST_AUTO_TEST_CASE(WxHPH)
{
    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wbhph_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wthph_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wbhph_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wthph_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wbhph_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wthph_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WBHPH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_1" },
            Opm::WellParameter::Keyword    { "WBHPH" },
            Opm::WellParameter::UnitString { "BARSA" },
            *eval
        }.pressure(Opm::WellParameter::Pressure::BHP).validate();

        wbhph_1.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WTHPH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_1" },
            Opm::WellParameter::Keyword    { "WTHPH" },
            Opm::WellParameter::UnitString { "BARSA" },
            *eval
        }.pressure(Opm::WellParameter::Pressure::THP).validate();

        wthph_1.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WBHPH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WBHPH" },
            Opm::WellParameter::UnitString { "BARSA" },
            *eval
        }.pressure(Opm::WellParameter::Pressure::BHP).validate();

        wbhph_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WTHPH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WTHPH" },
            Opm::WellParameter::UnitString { "BARSA" },
            *eval
        }.pressure(Opm::WellParameter::Pressure::THP).validate();

        wthph_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WBHPH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WBHPH" },
            Opm::WellParameter::UnitString { "BARSA" },
            *eval
        }.pressure(Opm::WellParameter::Pressure::BHP).validate();

        wbhph_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WTHPH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WTHPH" },
            Opm::WellParameter::UnitString { "BARSA" },
            *eval
        }.pressure(Opm::WellParameter::Pressure::THP).validate();

        wthph_3.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    wbhph_1->update(1, cse.sched.seconds(1), input, simRes, st);
    wthph_1->update(1, cse.sched.seconds(1), input, simRes, st);
    wbhph_2->update(1, cse.sched.seconds(1), input, simRes, st);
    wthph_2->update(1, cse.sched.seconds(1), input, simRes, st);
    wbhph_3->update(1, cse.sched.seconds(1), input, simRes, st);
    wthph_3->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK(st.has(wbhph_1->summaryKey()));
    BOOST_CHECK(st.has(wthph_1->summaryKey()));
    BOOST_CHECK(st.has(wbhph_2->summaryKey()));
    BOOST_CHECK(st.has(wthph_2->summaryKey()));
    BOOST_CHECK(st.has(wbhph_3->summaryKey()));
    BOOST_CHECK(st.has(wthph_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WBHPH:W_1"), 0.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WTHPH:W_1"), 0.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WBHPH:W_2"), 1.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WTHPH:W_2"), 1.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WBHPH:W_3"), 2.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WTHPH:W_3"), 2.2, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WOxH)
{
    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto woprh_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopth_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto woirh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto woith_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_1" },
            Opm::WellParameter::Keyword    { "WOPRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        woprh_1.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPTH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_1" },
            Opm::WellParameter::Keyword    { "WOPTH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wopth_1.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOIRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WOIRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        woirh_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOITH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WOITH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        woith_3.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(3) - cse.sched.seconds(2);
    woprh_1->update(3, dt, input, simRes, st);
    wopth_1->update(3, dt, input, simRes, st);
    woirh_3->update(3, dt, input, simRes, st);
    woith_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(woprh_1->summaryKey()));
    BOOST_CHECK(st.has(wopth_1->summaryKey()));
    BOOST_CHECK(st.has(woirh_3->summaryKey()));
    BOOST_CHECK(st.has(woith_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WOPRH:W_1"), 10.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOIRH:W_3"),  0.0, 1.0e-10);

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("WOPTH:W_1"), 101.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOITH:W_3"),   0.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WGxH)
{
    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wgprh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgpth_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgirh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgith_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WGPRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgprh_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGPTH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WGPTH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgpth_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGIRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WGIRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wgirh_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGITH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WGITH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wgith_3.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(3) - cse.sched.seconds(2);
    wgprh_2->update(3, dt, input, simRes, st);
    wgpth_2->update(3, dt, input, simRes, st);
    wgirh_3->update(3, dt, input, simRes, st);
    wgith_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(wgprh_2->summaryKey()));
    BOOST_CHECK(st.has(wgpth_2->summaryKey()));
    BOOST_CHECK(st.has(wgirh_3->summaryKey()));
    BOOST_CHECK(st.has(wgith_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WGPRH:W_2"), 20.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGIRH:W_3"),  0.0, 1.0e-10);

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("WGPTH:W_2"), 202.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGITH:W_3"),   0.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WWxH)
{
    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wwprh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwpth_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wlprh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wlpth_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwirh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwith_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWPRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WWPRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwprh_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWPTH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WWPTH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wwpth_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WLPRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WLPRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wlprh_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WLPTH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WLPTH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wlpth_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWIRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WWIRH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wwirh_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWITH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WWITH" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wwith_3.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(3) - cse.sched.seconds(2);
    wwprh_2->update(3, dt, input, simRes, st);
    wwpth_2->update(3, dt, input, simRes, st);
    wlprh_2->update(3, dt, input, simRes, st);
    wlpth_2->update(3, dt, input, simRes, st);
    wwirh_3->update(3, dt, input, simRes, st);
    wwith_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(wwprh_2->summaryKey()));
    BOOST_CHECK(st.has(wwpth_2->summaryKey()));
    BOOST_CHECK(st.has(wlprh_2->summaryKey()));
    BOOST_CHECK(st.has(wlpth_2->summaryKey()));
    BOOST_CHECK(st.has(wwirh_3->summaryKey()));
    BOOST_CHECK(st.has(wwith_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WWPRH:W_2"), 20.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WLPRH:W_2"), 40.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWIRH:W_3"), 30.0, 1.0e-10);

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("WWPTH:W_2"), 200.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WLPTH:W_2"), 401.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWITH:W_3"), 300.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(WxRH)
{
    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wgorh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wglrh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwcth_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wgorh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wglrh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wwcth_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGORH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WGORH" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wgorh_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGLRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WGLRH" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wglrh_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWCTH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WWCTH" },
            Opm::WellParameter::UnitString { "" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wwcth_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGORH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WGORH" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wgorh_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WGLRH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WGLRH" },
            Opm::WellParameter::UnitString { "SM3/SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wglrh_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WWCTH");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WWCTH" },
            Opm::WellParameter::UnitString { "" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Ratio).validate();

        wwcth_3.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    const auto dt = cse.sched.seconds(3) - cse.sched.seconds(2);
    wgorh_2->update(3, dt, input, simRes, st);
    wglrh_2->update(3, dt, input, simRes, st);
    wwcth_2->update(3, dt, input, simRes, st);
    wgorh_3->update(3, dt, input, simRes, st);
    wglrh_3->update(3, dt, input, simRes, st);
    wwcth_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(wgorh_2->summaryKey()));
    BOOST_CHECK(st.has(wglrh_2->summaryKey()));
    BOOST_CHECK(st.has(wwcth_2->summaryKey()));
    BOOST_CHECK(st.has(wgorh_3->summaryKey()));
    BOOST_CHECK(st.has(wglrh_3->summaryKey()));
    BOOST_CHECK(st.has(wwcth_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("WGORH:W_2"), 20.2 / 20.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGLRH:W_2"), 20.2 / 40.1, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWCTH:W_2"), 20.0 / 40.1, 1.0e-10);

    // Flowing/producing ratios is zero in injectors
    BOOST_CHECK_CLOSE(st.get("WGORH:W_3"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WGLRH:W_3"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WWCTH:W_3"), 0.0, 1.0e-10);
}

BOOST_AUTO_TEST_SUITE_END() // Observed_Control_Values

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Efficiency_Factors)

namespace {
    Opm::data::Well W_1()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for producers
        xw.rates.set(r::oil, - 10.0e3*sm3_pr_day());
        xw.rates.set(r::gas, -100.0e3*sm3_pr_day());
        xw.rates.set(r::wat, - 50.0e3*sm3_pr_day());

        xw.rates.set(r::dissolved_gas, -82.15e3*sm3_pr_day());
        xw.rates.set(r::vaporized_oil, -1000.0*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  , -30.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  , - 4.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, -49.0e3*rm3_pr_day());

        xw.bhp = 256.512*Opm::unit::barsa;
        xw.thp = 128.123*Opm::unit::barsa;

        return xw;
    }

    Opm::data::Well W_2()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for injectors
        xw.rates.set(r::oil, -50.0e3*sm3_pr_day());
        xw.rates.set(r::gas, -20.0e3*sm3_pr_day());
        xw.rates.set(r::wat, -10.0e3*sm3_pr_day());

        xw.rates.set(r::dissolved_gas, -5.15e3*sm3_pr_day());
        xw.rates.set(r::vaporized_oil, -654.3*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  , -40.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  , - 6.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, - 9.5e3*rm3_pr_day());

        xw.bhp = 234.5*Opm::unit::barsa;
        xw.thp = 150.0*Opm::unit::barsa;

        return xw;
    }

    Opm::data::Well W_3()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Positive rate signs for injectors
        xw.rates.set(r::oil, - 25.0e3*sm3_pr_day());
        xw.rates.set(r::gas, - 80.0e3*sm3_pr_day());
        xw.rates.set(r::wat, -100.0e3*sm3_pr_day());

        xw.rates.set(r::dissolved_gas, -45.0e3*sm3_pr_day());
        xw.rates.set(r::vaporized_oil, -750.0*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  , -22.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  , -63.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, -92.8e3*rm3_pr_day());

        xw.bhp = 198.1*Opm::unit::barsa;
        xw.thp = 123.0*Opm::unit::barsa;

        return xw;
    }

    Opm::data::WellRates wellResults()
    {
        auto xw = Opm::data::WellRates{};

        xw["W_1"] = W_1();
        xw["W_2"] = W_2();
        xw["W_3"] = W_3();

        return xw;
    }

    std::map<std::string, double> singleResults()
    {
        return {};
    }

    std::map<std::string, std::vector<double>> regionResults()
    {
        return {};
    }

    std::map<std::pair<std::string, int>, double> blockResults()
    {
        return {};
    }
} // Anonymous

BOOST_AUTO_TEST_CASE(WOPT)
{
    const auto cse    = Setup{ "SUMMARY_EFF_FAC.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto wopr_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopt_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopr_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopt_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopr_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto wopt_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_1" },
            Opm::WellParameter::Keyword    { "WOPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wopr_1.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_1" },
            Opm::WellParameter::Keyword    { "WOPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wopt_1.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WOPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wopr_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_2" },
            Opm::WellParameter::Keyword    { "WOPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wopt_2.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPR");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WOPR" },
            Opm::WellParameter::UnitString { "SM3/DAY" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Rate).validate();

        wopr_3.reset(new Opm::WellParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("WOPT");

        auto prm = ::Opm::WellParameter {
            Opm::WellParameter::WellName   { "W_3" },
            Opm::WellParameter::Keyword    { "WOPT" },
            Opm::WellParameter::UnitString { "SM3" },
            *eval
        }.flowType(Opm::WellParameter::FlowType::Total).validate();

        wopt_3.reset(new Opm::WellParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };
    wopr_1->update(1, cse.sched.seconds(1), input, simRes, st);
    wopt_1->update(1, cse.sched.seconds(1), input, simRes, st);
    wopr_2->update(1, cse.sched.seconds(1), input, simRes, st);
    wopt_2->update(1, cse.sched.seconds(1), input, simRes, st);
    wopr_3->update(1, cse.sched.seconds(1), input, simRes, st);
    wopt_3->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK_CLOSE(st.get("WOPR:W_1"), 10.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPR:W_2"), 50.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("WOPR:W_3"), 25.0e3, 1.0e-10);

    // Cumulatives after 1st step
    {
        const auto ef_1 = 1.0;
        const auto ef_2 = 0.2 * 0.01;        // WEFAC W_2 * GEFAC G_2
        const auto ef_3 = 0.3 * 0.02 * 0.03; // WEFAC W_3 * GEFAC G_3 * GEFAC G_4

        BOOST_CHECK_CLOSE(st.get("WOPT:W_1"), ef_1 * 100.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("WOPT:W_2"), ef_2 * 500.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("WOPT:W_3"), ef_3 * 250.0e3, 1.0e-10);
    }

    auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    wopt_1->update(2, dt, input, simRes, st);
    wopt_2->update(2, dt, input, simRes, st);
    wopt_3->update(2, dt, input, simRes, st);

    // Cumulatives after 2nd step
    {
        const auto pt_1_init = 1.0               * 100.0e3;
        const auto pt_2_init = 0.2 * 0.01        * 500.0e3;
        const auto pt_3_init = 0.3 * 0.02 * 0.03 * 250.0e3;

        const auto ef_1 = 1.0;
        const auto ef_2 = 0.2 * 0.01;        // WEFAC W_2 * GEFAC G_2
        const auto ef_3 = 0.3 * 0.02 * 0.04; // WEFAC W_3 * GEFAC G_3 * GEFAC G_4

        BOOST_CHECK_CLOSE(st.get("WOPT:W_1"), pt_1_init + (ef_1 * 100.0e3), 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("WOPT:W_2"), pt_2_init + (ef_2 * 500.0e3), 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("WOPT:W_3"), pt_3_init + (ef_3 * 250.0e3), 1.0e-10);
    }
}

BOOST_AUTO_TEST_SUITE_END() // Efficiency_Factors

BOOST_AUTO_TEST_SUITE_END() // Well_Parameters
