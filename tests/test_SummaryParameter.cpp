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
#include <opm/output/eclipse/Summary/GroupParameter.hpp>
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

// =====================================================================

BOOST_AUTO_TEST_SUITE(Group_Parameters)

BOOST_AUTO_TEST_SUITE(Construct)

BOOST_AUTO_TEST_CASE(GMWPR_InvalidType)
{
    using Type = Opm::GroupParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GMWPR");

    auto prm = ::Opm::GroupParameter {
        Opm::GroupParameter::GroupName  { "OP_3" },
        Opm::GroupParameter::Keyword    { "GMWPR" },
        Opm::GroupParameter::UnitString { "" },
        Opm::GroupParameter::Type       { static_cast<Type>(1729) },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(GMWPR)
{
    using Type = Opm::GroupParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GMWPR");

    auto prm = ::Opm::GroupParameter {
        Opm::GroupParameter::GroupName  { "OP_3" },
        Opm::GroupParameter::Keyword    { "GMWPR" },
        Opm::GroupParameter::UnitString { "" },
        Opm::GroupParameter::Type       { Type::Count },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::GroupParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "");

    BOOST_CHECK_EQUAL(prm2->name(), "OP_3");
    BOOST_CHECK_EQUAL(prm2->keyword(), "GMWPR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "GMWPR:OP_3");
}

BOOST_AUTO_TEST_CASE(GOPR)
{
    using Type = Opm::GroupParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

    auto prm = ::Opm::GroupParameter {
        Opm::GroupParameter::GroupName  { "OP_3" },
        Opm::GroupParameter::Keyword    { "GOPR" },
        Opm::GroupParameter::UnitString { "SCC/HR" },
        Opm::GroupParameter::Type       { Type::Rate },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::GroupParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "SCC/HR"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "SCC/HR"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "SCC/HR");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "SCC/HR"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_3");
    BOOST_CHECK_EQUAL(prm2->keyword(), "GOPR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "GOPR:OP_3");
}

BOOST_AUTO_TEST_CASE(GLPT)
{
    using Type = Opm::GroupParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GLPT");

    auto prm = ::Opm::GroupParameter {
        Opm::GroupParameter::GroupName  { "OP_2" },
        Opm::GroupParameter::Keyword    { "GLPT" },
        Opm::GroupParameter::UnitString { "SM3" },
        Opm::GroupParameter::Type       { Type::Total },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::GroupParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "SM3"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "SM3"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "SM3");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "SM3"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_2");
    BOOST_CHECK_EQUAL(prm2->keyword(), "GLPT");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "GLPT:OP_2");
}

BOOST_AUTO_TEST_CASE(GGLR)
{
    using Type = Opm::GroupParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGLR");

    auto prm = ::Opm::GroupParameter {
        Opm::GroupParameter::GroupName  { "OP_1" },
        Opm::GroupParameter::Keyword    { "GGLR" },
        Opm::GroupParameter::UnitString { "STB/STB" },
        Opm::GroupParameter::Type       { Type::Ratio },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::GroupParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "STB/STB"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "STB/STB"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "STB/STB");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "STB/STB"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "OP_1");
    BOOST_CHECK_EQUAL(prm2->keyword(), "GGLR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "GGLR:OP_1");
}

BOOST_AUTO_TEST_CASE(FMWPR_InvalidType)
{
    using Type = Opm::FieldParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FMWPR");

    auto prm = ::Opm::FieldParameter {
        Opm::FieldParameter::Keyword    { "FMWPR" },
        Opm::FieldParameter::UnitString { "" },
        Opm::FieldParameter::Type       { static_cast<Type>(11) },
        *eval
    };

    BOOST_CHECK_THROW(prm.validate(), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(FMWPR)
{
    using Type = Opm::FieldParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FMWPR");

    auto prm = ::Opm::FieldParameter {
        Opm::FieldParameter::Keyword    { "FMWPR" },
        Opm::FieldParameter::UnitString { "" },
        Opm::FieldParameter::Type       { Type::Count },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::FieldParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "");

    BOOST_CHECK_EQUAL(prm2->name(), "FIELD");
    BOOST_CHECK_EQUAL(prm2->keyword(), "FMWPR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "FMWPR");
}

BOOST_AUTO_TEST_CASE(FOPR)
{
    using Type = Opm::FieldParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPR");

    auto prm = ::Opm::FieldParameter {
        Opm::FieldParameter::Keyword    { "FOPR" },
        Opm::FieldParameter::UnitString { "SCC/HR" },
        Opm::FieldParameter::Type       { Type::Rate },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::FieldParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "SCC/HR"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "SCC/HR"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "SCC/HR");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "SCC/HR"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "FIELD");
    BOOST_CHECK_EQUAL(prm2->keyword(), "FOPR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "FOPR");
}

BOOST_AUTO_TEST_CASE(FLPT)
{
    using Type = Opm::FieldParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FLPT");

    auto prm = ::Opm::FieldParameter {
        Opm::FieldParameter::Keyword    { "FLPT" },
        Opm::FieldParameter::UnitString { "SM3" },
        Opm::FieldParameter::Type       { Type::Total },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::FieldParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "SM3"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "SM3"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "SM3");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "SM3"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "FIELD");
    BOOST_CHECK_EQUAL(prm2->keyword(), "FLPT");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "FLPT");
}

BOOST_AUTO_TEST_CASE(FGLR)
{
    using Type = Opm::FieldParameter::Type;
    auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGLR");

    auto prm = ::Opm::FieldParameter {
        Opm::FieldParameter::Keyword    { "FGLR" },
        Opm::FieldParameter::UnitString { "STB/STB" },
        Opm::FieldParameter::Type       { Type::Ratio },
        *eval
    };

    auto prm2 = std::unique_ptr<Opm::SummaryParameter> {
        new Opm::FieldParameter { std::move(prm).validate() }
    };

    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newMETRIC()), "STB/STB"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newFIELD()), "STB/STB"); // (!)
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newLAB()), "STB/STB");
    BOOST_CHECK_EQUAL(prm2->unit(Opm::UnitSystem::newPVT_M()), "STB/STB"); // (!)

    BOOST_CHECK_EQUAL(prm2->name(), "FIELD");
    BOOST_CHECK_EQUAL(prm2->keyword(), "FGLR");
    BOOST_CHECK_EQUAL(prm2->num(), 0);
    BOOST_CHECK_EQUAL(prm2->summaryKey(), "FGLR");
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

    Opm::data::Well OP_3()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for producers
        xw.rates.set(r::oil, -50.0e3*sm3_pr_day());
        xw.rates.set(r::gas, -33.0e3*sm3_pr_day());
        xw.rates.set(r::wat, - 5.0e3*sm3_pr_day());

        xw.rates.set(r::polymer, -  12.34*sm3_pr_day());
        xw.rates.set(r::solvent, -1234.5*sm3_pr_day());

        xw.rates.set(r::dissolved_gas, -  30.0e3*sm3_pr_day());
        xw.rates.set(r::vaporized_oil, -1234.0*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  , -45.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  , - 1.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, - 4.9e3*rm3_pr_day());

        xw.rates.set(r::productivity_index_water, 20.0*sm3_pr_day_barsa());
        xw.rates.set(r::productivity_index_oil, 15.0*sm3_pr_day_barsa());
        xw.rates.set(r::productivity_index_gas, 12.5*sm3_pr_day_barsa());

        xw.rates.set(r::well_potential_water, 15.0e3*sm3_pr_day());
        xw.rates.set(r::well_potential_oil, 65.0e3*sm3_pr_day());
        xw.rates.set(r::well_potential_gas, 23.45e3*sm3_pr_day());

        xw.bhp = 75.57*Opm::unit::barsa;
        xw.thp = 45.67*Opm::unit::barsa;

        return xw;
    }

    Opm::data::Well OP_4()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for producers
        xw.rates.set(r::oil, -1.0e3*sm3_pr_day());

        return xw;
    }

    Opm::data::Well OP_5()
    {
        // Not Flowing
        return {};
    }

    Opm::data::WellRates wellResults()
    {
        auto xw = Opm::data::WellRates{};

        xw["OP_1"] = OP_1();
        xw["OP_2"] = OP_2();
        xw["OP_3"] = OP_3();
        xw["OP_4"] = OP_4();
        xw["OP_5"] = OP_5();

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

BOOST_AUTO_TEST_CASE(GOPx)
{
    using Type = Opm::GroupParameter::Type;
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gopr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto govpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto govpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto goprs = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopts = std::unique_ptr<Opm::SummaryParameter>{};
    auto goprf = std::unique_ptr<Opm::SummaryParameter>{};
    auto goptf = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopt.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOVPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOVPR" },
            Opm::GroupParameter::UnitString { "RM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        govpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOVPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOVPT" },
            Opm::GroupParameter::UnitString { "RM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        govpt.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPRS");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPRS" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        goprs.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPTS");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPTS" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopts.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPRF");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPRF" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        goprf.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPTF");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPTF" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        goptf.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPP");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPP" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopp.reset(new Opm::GroupParameter { std::move(prm) });
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
    gopr ->update(2, dt, input, simRes, st);
    gopt ->update(2, dt, input, simRes, st);
    govpr->update(2, dt, input, simRes, st);
    govpt->update(2, dt, input, simRes, st);
    goprs->update(2, dt, input, simRes, st);
    gopts->update(2, dt, input, simRes, st);
    goprf->update(2, dt, input, simRes, st);
    goptf->update(2, dt, input, simRes, st);
    gopp ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(gopr ->summaryKey()));
    BOOST_CHECK(st.has(gopt ->summaryKey()));
    BOOST_CHECK(st.has(govpr->summaryKey()));
    BOOST_CHECK(st.has(govpt->summaryKey()));
    BOOST_CHECK(st.has(goprs->summaryKey()));
    BOOST_CHECK(st.has(gopts->summaryKey()));
    BOOST_CHECK(st.has(goprf->summaryKey()));
    BOOST_CHECK(st.has(goptf->summaryKey()));
    BOOST_CHECK(st.has(gopp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GOPR:OP"), 60.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOVPR:OP"), 75.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPRS:OP"), 2.234e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPRF:OP"), 57.766e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPP:OP"), 102.92e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GOPT:OP"), 52.62e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOVPT:OP"), 65.775e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPTS:OP"), 1.959218e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPTF:OP"), 50.660782e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GGPx)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto ggpr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggpt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggvpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggvpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggprs = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggpts = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggprf = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggptf = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggpp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggpt.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGVPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGVPR" },
            Opm::GroupParameter::UnitString { "RM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggvpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGVPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGVPT" },
            Opm::GroupParameter::UnitString { "RM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggvpt.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPRS");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPRS" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggprs.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPTS");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPTS" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggpts.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPRF");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPRF" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggprf.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPTF");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPTF" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggptf.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPP");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPP" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggpp.reset(new Opm::GroupParameter { std::move(prm) });
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
    ggpr ->update(2, dt, input, simRes, st);
    ggpt ->update(2, dt, input, simRes, st);
    ggvpr->update(2, dt, input, simRes, st);
    ggvpt->update(2, dt, input, simRes, st);
    ggprs->update(2, dt, input, simRes, st);
    ggpts->update(2, dt, input, simRes, st);
    ggprf->update(2, dt, input, simRes, st);
    ggptf->update(2, dt, input, simRes, st);
    ggpp ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(ggpr ->summaryKey()));
    BOOST_CHECK(st.has(ggpt ->summaryKey()));
    BOOST_CHECK(st.has(ggvpr->summaryKey()));
    BOOST_CHECK(st.has(ggvpt->summaryKey()));
    BOOST_CHECK(st.has(ggprs->summaryKey()));
    BOOST_CHECK(st.has(ggpts->summaryKey()));
    BOOST_CHECK(st.has(ggprf->summaryKey()));
    BOOST_CHECK(st.has(ggptf->summaryKey()));
    BOOST_CHECK(st.has(ggpp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GGPR:OP"), 133.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGVPR:OP"), 5.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPRS:OP"), 112.15e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPRF:OP"), 20.85e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPP:OP"), 146.9e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GGPT:OP"), 116.641e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGVPT:OP"), 4.385e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPTS:OP"), 98.35555e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPTF:OP"), 18.28545e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GWPx)
{
    using Type = ::Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gwpr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwpt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwvpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwvpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwpp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwpt.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWVPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWVPR" },
            Opm::GroupParameter::UnitString { "RM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwvpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWVPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWVPT" },
            Opm::GroupParameter::UnitString { "RM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwvpt.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPP");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWPP" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwpp.reset(new Opm::GroupParameter { std::move(prm) });
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
    gwpr ->update(2, dt, input, simRes, st);
    gwpt ->update(2, dt, input, simRes, st);
    gwvpr->update(2, dt, input, simRes, st);
    gwvpt->update(2, dt, input, simRes, st);
    gwpp ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(gwpr ->summaryKey()));
    BOOST_CHECK(st.has(gwpt ->summaryKey()));
    BOOST_CHECK(st.has(gwvpr->summaryKey()));
    BOOST_CHECK(st.has(gwvpt->summaryKey()));
    BOOST_CHECK(st.has(gwpp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GWPR:OP"), 55.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWVPR:OP"), 53.9e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWPP:OP"), 80.43e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GWPT:OP"), 48.235e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWVPT:OP"), 47.2703e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GOIx)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto goir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto goit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto govir = std::unique_ptr<Opm::SummaryParameter>{};
    auto govit = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopi  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOIR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        goir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOIT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        goit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOVIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOVIR" },
            Opm::GroupParameter::UnitString { "RM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        govir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOVIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOVIT" },
            Opm::GroupParameter::UnitString { "RM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        govit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPI");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GOPI" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopi.reset(new Opm::GroupParameter { std::move(prm) });
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
    goir ->update(2, dt, input, simRes, st);
    goit ->update(2, dt, input, simRes, st);
    govir->update(2, dt, input, simRes, st);
    govit->update(2, dt, input, simRes, st);
    gopi ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(goir ->summaryKey()));
    BOOST_CHECK(st.has(goit ->summaryKey()));
    BOOST_CHECK(st.has(govir->summaryKey()));
    BOOST_CHECK(st.has(govit->summaryKey()));
    BOOST_CHECK(st.has(gopi ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GOIR:OP"), 3.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOVIR:OP"), 2.9, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPI:OP"), 12345.6, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GOIT:OP"), 2631.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOVIT:OP"), 2543.3, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GGIx)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto ggir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggvir = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggvit = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggpi  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGIR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGIT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGVIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGVIR" },
            Opm::GroupParameter::UnitString { "RM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggvir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGVIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGVIT" },
            Opm::GroupParameter::UnitString { "RM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggvit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPI");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGPI" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggpi.reset(new Opm::GroupParameter { std::move(prm) });
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
    ggir ->update(2, dt, input, simRes, st);
    ggit ->update(2, dt, input, simRes, st);
    ggvir->update(2, dt, input, simRes, st);
    ggvit->update(2, dt, input, simRes, st);
    ggpi ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(ggir ->summaryKey()));
    BOOST_CHECK(st.has(ggit ->summaryKey()));
    BOOST_CHECK(st.has(ggvir->summaryKey()));
    BOOST_CHECK(st.has(ggvit->summaryKey()));
    BOOST_CHECK(st.has(ggpi ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GGIR:OP"), 80.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGVIR:OP"), 4.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPI:OP"), 86420.8, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GGIT:OP"), 70.16e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGVIT:OP"), 3.508e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GWIx)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gwir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwvir = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwvit = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwpi  = std::unique_ptr<Opm::SummaryParameter>{};
    auto glir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto glit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gvir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto gvit  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWIR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWIT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWVIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWVIR" },
            Opm::GroupParameter::UnitString { "RM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwvir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWVIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWVIT" },
            Opm::GroupParameter::UnitString { "RM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwvit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPI");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWPI" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwpi.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GLIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GLIR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        glir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GLIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GLIT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        glit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GVIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GVIR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gvir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GVIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GVIT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gvit.reset(new Opm::GroupParameter { std::move(prm) });
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
    gwir ->update(2, dt, input, simRes, st);
    gwit ->update(2, dt, input, simRes, st);
    gwvir->update(2, dt, input, simRes, st);
    gwvit->update(2, dt, input, simRes, st);
    gwpi ->update(2, dt, input, simRes, st);
    glir ->update(2, dt, input, simRes, st);
    glit ->update(2, dt, input, simRes, st);
    gvir ->update(2, dt, input, simRes, st);
    gvit ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(gwir ->summaryKey()));
    BOOST_CHECK(st.has(gwit ->summaryKey()));
    BOOST_CHECK(st.has(gwvir->summaryKey()));
    BOOST_CHECK(st.has(gwvit->summaryKey()));
    BOOST_CHECK(st.has(gwpi ->summaryKey()));
    BOOST_CHECK(st.has(glir ->summaryKey()));
    BOOST_CHECK(st.has(glit ->summaryKey()));
    BOOST_CHECK(st.has(gvir ->summaryKey()));
    BOOST_CHECK(st.has(gvit ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GWIR:OP"), 20.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWVIR:OP"), 19.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWPI:OP"), 543.21e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GLIR:OP"), 20.003e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GVIR:OP"), 23.0029e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GWIT:OP"), 17.54e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWVIT:OP"), 16.663e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GLIT:OP"), 17.542631e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GVIT:OP"), 20.1735433e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GxR)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto ggor = std::unique_ptr<Opm::SummaryParameter>{};
    auto gglr = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwct = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGOR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGOR" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        ggor.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGLR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GGLR" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gglr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWCT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GWCT" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gwct.reset(new Opm::GroupParameter { std::move(prm) });
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
    ggor->update(2, dt, input, simRes, st);
    gglr->update(2, dt, input, simRes, st);
    gwct->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(ggor->summaryKey()));
    BOOST_CHECK(st.has(gglr->summaryKey()));
    BOOST_CHECK(st.has(gwct->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GGOR:OP"), (100.0 + 33.0) / (10.0 + 50.0), 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGLR:OP"), (100.0 + 33.0) / (60.0 + 55.0), 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWCT:OP"), 55.0 / (60.0 + 55.0), 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Solvent)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gnir = std::unique_ptr<Opm::SummaryParameter>{};
    auto gnit = std::unique_ptr<Opm::SummaryParameter>{};
    auto gnpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto gnpt = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GNIR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GNIR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gnir.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GNIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GNIT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gnit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GNPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GNPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gnpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GNPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GNPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gnpt.reset(new Opm::GroupParameter { std::move(prm) });
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
    gnir->update(2, dt, input, simRes, st);
    gnit->update(2, dt, input, simRes, st);
    gnpr->update(2, dt, input, simRes, st);
    gnpt->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(gnir->summaryKey()));
    BOOST_CHECK(st.has(gnit->summaryKey()));
    BOOST_CHECK(st.has(gnpr->summaryKey()));
    BOOST_CHECK(st.has(gnpt->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GNIR:OP"), 25.75, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GNPR:OP"), 6666.6, 1.0e-10);  // 5432.1 + 1234.5

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("GNIT:OP"), 22.58275e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GNPT:OP"), 5.8466082e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Active_Well_Types)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gmwin = std::unique_ptr<Opm::SummaryParameter>{};
    auto gmwit = std::unique_ptr<Opm::SummaryParameter>{};
    auto gmwpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto gmwpt = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GMWIN");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GMWIN" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Count },
            *eval
        }.validate();

        gmwin.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GMWIT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GMWIT" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Count },
            *eval
        }.validate();

        gmwit.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GMWPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GMWPR" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Count },
            *eval
        }.validate();

        gmwpr.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GMWPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "OP" },
            Opm::GroupParameter::Keyword    { "GMWPT" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Count },
            *eval
        }.validate();

        gmwpt.reset(new Opm::GroupParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    {
        const auto dt = cse.sched.seconds(1);

        gmwin->update(1, dt, input, simRes, st);
        gmwit->update(1, dt, input, simRes, st);
        gmwpr->update(1, dt, input, simRes, st);
        gmwpt->update(1, dt, input, simRes, st);
    }

    BOOST_CHECK(st.has(gmwin->summaryKey()));
    BOOST_CHECK(st.has(gmwit->summaryKey()));
    BOOST_CHECK(st.has(gmwpr->summaryKey()));
    BOOST_CHECK(st.has(gmwpt->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GMWIN:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWIT:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPR:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPT:OP"), 1.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);

        gmwin->update(2, dt, input, simRes, st);
        gmwit->update(2, dt, input, simRes, st);
        gmwpr->update(2, dt, input, simRes, st);
        gmwpt->update(2, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("GMWIN:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWIT:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPR:OP"), 2.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPT:OP"), 2.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(3) - cse.sched.seconds(2);

        gmwin->update(3, dt, input, simRes, st);
        gmwit->update(3, dt, input, simRes, st);
        gmwpr->update(3, dt, input, simRes, st);
        gmwpt->update(3, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("GMWIN:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWIT:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPR:OP"), 2.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPT:OP"), 2.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(4) - cse.sched.seconds(3);

        gmwin->update(4, dt, input, simRes, st);
        gmwit->update(4, dt, input, simRes, st);
        gmwpr->update(4, dt, input, simRes, st);
        gmwpt->update(4, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("GMWIN:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWIT:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPR:OP"), 3.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPT:OP"), 3.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(5) - cse.sched.seconds(4);

        gmwin->update(5, dt, input, simRes, st);
        gmwit->update(5, dt, input, simRes, st);
        gmwpr->update(5, dt, input, simRes, st);
        gmwpt->update(5, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("GMWIN:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWIT:OP"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GMWPR:OP"), 3.0, 1.0e-10);  // New well OP_5 not flowing
    BOOST_CHECK_CLOSE(st.get("GMWPT:OP"), 4.0, 1.0e-10);
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

//                      +-------+
//                      | FIELD |
//                      +-------+
//                          |
//            +-------------+-------------+
//            |             |             |
//         +-----+       +-----+       +-----+
//         | G_1 |       | G_2 |       | G_3 |
//         +-----+       +-----+       +-----+
//            |             |             |
//    +-------+         +-------+         +-------+
//    |       |         |       |         |       |
// +-----+ +-----+   +-----+ +-----+   +-----+ +-----+
// | W_1 | | W_2 |   | W_3 | | W_6 |   | W_4 | | W_5 |
// +-----+ +-----+   +-----+ +-----+   +-----+ +-----+

BOOST_AUTO_TEST_CASE(GOxH)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto goprh_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopth_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto goirh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto goith_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto goprh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopth_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GOPRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        goprh_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GOPTH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopth_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOIRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GOIRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        goirh_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOITH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GOITH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        goith_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GOPRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        goprh_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GOPTH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopth_3.reset(new Opm::GroupParameter { std::move(prm) });
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
    goprh_1->update(3, dt, input, simRes, st);
    gopth_1->update(3, dt, input, simRes, st);
    goirh_2->update(3, dt, input, simRes, st);
    goith_2->update(3, dt, input, simRes, st);
    goprh_3->update(3, dt, input, simRes, st);
    gopth_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(goprh_1->summaryKey()));
    BOOST_CHECK(st.has(gopth_1->summaryKey()));
    BOOST_CHECK(st.has(goirh_2->summaryKey()));
    BOOST_CHECK(st.has(goith_2->summaryKey()));
    BOOST_CHECK(st.has(goprh_3->summaryKey()));
    BOOST_CHECK(st.has(gopth_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GOPRH:G_1"), 30.2, 1.0e-10); // W_1 + W_2
    BOOST_CHECK_CLOSE(st.get("GOIRH:G_2"),  0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPRH:G_3"),  0.0, 1.0e-10); // WCONPROD only

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("GOPTH:G_1"), 302.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOITH:G_2"),   0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPTH:G_3"),   0.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GGxH)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto ggprh_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggpth_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggirh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggith_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggprh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggpth_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GGPRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggprh_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GGPTH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggpth_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGIRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GGIRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggirh_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGITH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GGITH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggith_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GGPRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        ggprh_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGPTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GGPTH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        ggpth_3.reset(new Opm::GroupParameter { std::move(prm) });
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
    ggprh_1->update(3, dt, input, simRes, st);
    ggpth_1->update(3, dt, input, simRes, st);
    ggirh_2->update(3, dt, input, simRes, st);
    ggith_2->update(3, dt, input, simRes, st);
    ggprh_3->update(3, dt, input, simRes, st);
    ggpth_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(ggprh_1->summaryKey()));
    BOOST_CHECK(st.has(ggpth_1->summaryKey()));
    BOOST_CHECK(st.has(ggirh_2->summaryKey()));
    BOOST_CHECK(st.has(ggith_2->summaryKey()));
    BOOST_CHECK(st.has(ggprh_3->summaryKey()));
    BOOST_CHECK(st.has(ggpth_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GGPRH:G_1"), 30.4  , 1.0e-10); // W_1 + W_2
    BOOST_CHECK_CLOSE(st.get("GGIRH:G_2"), 30.0e3, 1.0e-10); // W_6
    BOOST_CHECK_CLOSE(st.get("GGPRH:G_3"),  0.0  , 1.0e-10); // WCONPROD only

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("GGPTH:G_1"), 304.0  , 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGITH:G_2"), 300.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGPTH:G_3"),   0.0  , 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GWxH)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gwprh_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwpth_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwirh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwith_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwprh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwpth_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GWPRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwprh_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GWPTH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwpth_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWIRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GWIRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwirh_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWITH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GWITH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwith_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GWPRH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gwprh_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWPTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GWPTH" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gwpth_3.reset(new Opm::GroupParameter { std::move(prm) });
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
    gwprh_1->update(3, dt, input, simRes, st);
    gwpth_1->update(3, dt, input, simRes, st);
    gwirh_2->update(3, dt, input, simRes, st);
    gwith_2->update(3, dt, input, simRes, st);
    gwprh_3->update(3, dt, input, simRes, st);
    gwpth_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(gwprh_1->summaryKey()));
    BOOST_CHECK(st.has(gwpth_1->summaryKey()));
    BOOST_CHECK(st.has(gwirh_2->summaryKey()));
    BOOST_CHECK(st.has(gwith_2->summaryKey()));
    BOOST_CHECK(st.has(gwprh_3->summaryKey()));
    BOOST_CHECK(st.has(gwpth_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GWPRH:G_1"), 30.0, 1.0e-10); // W_1 + W_2
    BOOST_CHECK_CLOSE(st.get("GWIRH:G_2"), 30.0, 1.0e-10); // W_3
    BOOST_CHECK_CLOSE(st.get("GWPRH:G_3"),  0.0, 1.0e-10); // WCONPROD only

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("GWPTH:G_1"), 300.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWITH:G_2"), 300.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWPTH:G_3"),   0.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(GxRH)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto ggorh_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gglrh_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwcth_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggorh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gglrh_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwcth_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto ggorh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gglrh_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gwcth_3 = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGORH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GGORH" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        ggorh_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGLRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GGLRH" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gglrh_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWCTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GWCTH" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gwcth_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGORH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GGORH" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        ggorh_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGLRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GGLRH" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gglrh_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWCTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GWCTH" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gwcth_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGORH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GGORH" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        ggorh_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GGLRH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GGLRH" },
            Opm::GroupParameter::UnitString { "SM3/SM3" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gglrh_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GWCTH");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GWCTH" },
            Opm::GroupParameter::UnitString { "" },
            Opm::GroupParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        gwcth_3.reset(new Opm::GroupParameter { std::move(prm) });
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
    ggorh_1->update(3, dt, input, simRes, st);
    gglrh_1->update(3, dt, input, simRes, st);
    gwcth_1->update(3, dt, input, simRes, st);
    ggorh_2->update(3, dt, input, simRes, st);
    gglrh_2->update(3, dt, input, simRes, st);
    gwcth_2->update(3, dt, input, simRes, st);
    ggorh_3->update(3, dt, input, simRes, st);
    gglrh_3->update(3, dt, input, simRes, st);
    gwcth_3->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(ggorh_1->summaryKey()));
    BOOST_CHECK(st.has(gglrh_1->summaryKey()));
    BOOST_CHECK(st.has(gwcth_1->summaryKey()));
    BOOST_CHECK(st.has(ggorh_2->summaryKey()));
    BOOST_CHECK(st.has(gglrh_2->summaryKey()));
    BOOST_CHECK(st.has(gwcth_2->summaryKey()));
    BOOST_CHECK(st.has(ggorh_3->summaryKey()));
    BOOST_CHECK(st.has(gglrh_3->summaryKey()));
    BOOST_CHECK(st.has(gwcth_3->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("GGORH:G_1"), 30.4 / 30.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGLRH:G_1"), 30.4 / 60.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWCTH:G_1"), 30.0 / 60.2, 1.0e-10);

    // Flowing/producing ratios is zero in injection groups
    BOOST_CHECK_CLOSE(st.get("GGORH:G_2"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGLRH:G_2"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWCTH:G_2"), 0.0, 1.0e-10);

    // Flowing/producing ratios is zero in prediction groups
    BOOST_CHECK_CLOSE(st.get("GGORH:G_3"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GGLRH:G_3"), 0.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GWCTH:G_3"), 0.0, 1.0e-10);
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

//                      +-------+
//                      | FIELD |
//                      +---+---+
//                          |
//                  +-------+-------+
//                  |               |
//               +--+--+         +--+--+
//               |  G  |         | G_4 |
//               +--+--+         +--+--+
//                  |               |
//       +----------+            +--+--+
//       |          |            | G_3 |
//    +--+--+    +--+--+         +--+--+
//    | G_1 |    | G_2 |            |
//    +--+--+    +--+--+         +--+--+
//       |          |            | W_3 |
//    +--+--+    +--+--+         +-----+
//    | W_1 |    | W_2 |
//    +-----+    +-----+

BOOST_AUTO_TEST_CASE(GOPT)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "SUMMARY_EFF_FAC.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto gopr_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopt_1 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopr_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopt_2 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopr_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopt_3 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopr_4 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopt_4 = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopr_g = std::unique_ptr<Opm::SummaryParameter>{};
    auto gopt_g = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GOPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopr_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_1" },
            Opm::GroupParameter::Keyword    { "GOPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopt_1.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GOPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopr_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_2" },
            Opm::GroupParameter::Keyword    { "GOPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopt_2.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GOPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopr_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_3" },
            Opm::GroupParameter::Keyword    { "GOPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopt_3.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_4" },
            Opm::GroupParameter::Keyword    { "GOPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopr_4.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G_4" },
            Opm::GroupParameter::Keyword    { "GOPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopt_4.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPR");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G" },
            Opm::GroupParameter::Keyword    { "GOPR" },
            Opm::GroupParameter::UnitString { "SM3/DAY" },
            Opm::GroupParameter::Type       { Type::Rate },
            *eval
        }.validate();

        gopr_g.reset(new Opm::GroupParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("GOPT");

        auto prm = ::Opm::GroupParameter {
            Opm::GroupParameter::GroupName  { "G" },
            Opm::GroupParameter::Keyword    { "GOPT" },
            Opm::GroupParameter::UnitString { "SM3" },
            Opm::GroupParameter::Type       { Type::Total },
            *eval
        }.validate();

        gopt_g.reset(new Opm::GroupParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };
    gopr_1->update(1, cse.sched.seconds(1), input, simRes, st);
    gopt_1->update(1, cse.sched.seconds(1), input, simRes, st);
    gopr_2->update(1, cse.sched.seconds(1), input, simRes, st);
    gopt_2->update(1, cse.sched.seconds(1), input, simRes, st);
    gopr_3->update(1, cse.sched.seconds(1), input, simRes, st);
    gopt_3->update(1, cse.sched.seconds(1), input, simRes, st);
    gopr_4->update(1, cse.sched.seconds(1), input, simRes, st);
    gopt_4->update(1, cse.sched.seconds(1), input, simRes, st);
    gopr_g->update(1, cse.sched.seconds(1), input, simRes, st);
    gopt_g->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK_CLOSE(st.get("GOPR:G_1"), 10.0e3 * 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPR:G_2"), 50.0e3 * 0.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPR:G_3"), 25.0e3 * 0.3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPR:G_4"), 25.0e3 * 0.3 * 0.02, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("GOPR:G"),
                      10.0e3*1.0 + (50.0e3 * 0.2 * 0.01), 1.0e-10);

    // Cumulatives after 1st step
    {
        const auto ef_1 = 1.0;
        const auto ef_2 = 0.2 * 0.01;        // WEFAC W_2 * GEFAC G_2
        const auto ef_3 = 0.3 * 0.02 * 0.03; // WEFAC W_3 * GEFAC G_3 * GEFAC G_4

        BOOST_CHECK_CLOSE(st.get("GOPT:G_1"), ef_1 * 100.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G_2"), ef_2 * 500.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G_3"), ef_3 * 250.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G_4"), ef_3 * 250.0e3, 1.0e-10); // == G_3
        BOOST_CHECK_CLOSE(st.get("GOPT:G"),
                          ef_1*100.0e3 + ef_2*500.0e3, 1.0e-10); // == G_1 + G_2
    }

    auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    gopt_1->update(2, dt, input, simRes, st);
    gopt_2->update(2, dt, input, simRes, st);
    gopt_3->update(2, dt, input, simRes, st);
    gopt_4->update(2, dt, input, simRes, st);
    gopt_g->update(2, dt, input, simRes, st);

    // Cumulatives after 2nd step
    {
        const auto pt_1_init = 1.0               * 100.0e3;
        const auto pt_2_init = 0.2 * 0.01        * 500.0e3;
        const auto pt_3_init = 0.3 * 0.02 * 0.03 * 250.0e3;
        const auto pt_4_init = 0.3 * 0.02 * 0.03 * 250.0e3; // == pt_3_init
        const auto pt_g_init = pt_1_init + pt_2_init;

        const auto ef_1 = 1.0;
        const auto ef_2 = 0.2 * 0.01;        // WEFAC W_2 * GEFAC G_2
        const auto ef_3 = 0.3 * 0.02 * 0.04; // WEFAC W_3 * GEFAC G_3 * GEFAC G_4

        BOOST_CHECK_CLOSE(st.get("GOPT:G_1"), pt_1_init + (ef_1 * 100.0e3), 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G_2"), pt_2_init + (ef_2 * 500.0e3), 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G_3"), pt_3_init + (ef_3 * 250.0e3), 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G_4"), pt_4_init + (ef_3 * 250.0e3), 1.0e-10);
        BOOST_CHECK_CLOSE(st.get("GOPT:G"),
                          pt_g_init +
                          (ef_1 * 100.0e3) + (ef_2 * 500.0e3), 1.0e-10);
    }
}

BOOST_AUTO_TEST_SUITE_END() // Efficiency_Factors

BOOST_AUTO_TEST_SUITE_END() // Group_Parameters

// =====================================================================

BOOST_AUTO_TEST_SUITE(Field_Parameters)

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

    Opm::data::Well OP_3()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for producers
        xw.rates.set(r::oil, -50.0e3*sm3_pr_day());
        xw.rates.set(r::gas, -33.0e3*sm3_pr_day());
        xw.rates.set(r::wat, - 5.0e3*sm3_pr_day());

        xw.rates.set(r::polymer, -  12.34*sm3_pr_day());
        xw.rates.set(r::solvent, -1234.5*sm3_pr_day());

        xw.rates.set(r::dissolved_gas, -  30.0e3*sm3_pr_day());
        xw.rates.set(r::vaporized_oil, -1234.0*sm3_pr_day());

        xw.rates.set(r::reservoir_oil  , -45.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_gas  , - 1.0e3*rm3_pr_day());
        xw.rates.set(r::reservoir_water, - 4.9e3*rm3_pr_day());

        xw.rates.set(r::productivity_index_water, 20.0*sm3_pr_day_barsa());
        xw.rates.set(r::productivity_index_oil, 15.0*sm3_pr_day_barsa());
        xw.rates.set(r::productivity_index_gas, 12.5*sm3_pr_day_barsa());

        xw.rates.set(r::well_potential_water, 15.0e3*sm3_pr_day());
        xw.rates.set(r::well_potential_oil, 65.0e3*sm3_pr_day());
        xw.rates.set(r::well_potential_gas, 23.45e3*sm3_pr_day());

        xw.bhp = 75.57*Opm::unit::barsa;
        xw.thp = 45.67*Opm::unit::barsa;

        return xw;
    }

    Opm::data::Well OP_4()
    {
        using r = Opm::data::Rates::opt;
        auto xw = Opm::data::Well{};

        // Negative rate signs for producers
        xw.rates.set(r::oil, -1.0e3*sm3_pr_day());

        return xw;
    }

    Opm::data::Well OP_5()
    {
        // Not Flowing
        return {};
    }

    Opm::data::WellRates wellResults()
    {
        auto xw = Opm::data::WellRates{};

        xw["OP_1"] = OP_1();
        xw["OP_2"] = OP_2();
        xw["OP_3"] = OP_3();
        xw["OP_4"] = OP_4();
        xw["OP_5"] = OP_5();

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

BOOST_AUTO_TEST_CASE(FOPx)
{
    using Type = Opm::GroupParameter::Type;
    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fopr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fopt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fovpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fovpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto foprs = std::unique_ptr<Opm::SummaryParameter>{};
    auto fopts = std::unique_ptr<Opm::SummaryParameter>{};
    auto foprf = std::unique_ptr<Opm::SummaryParameter>{};
    auto foptf = std::unique_ptr<Opm::SummaryParameter>{};
    auto fopp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fopr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fopt.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOVPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOVPR" },
            Opm::FieldParameter::UnitString { "RM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fovpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOVPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOVPT" },
            Opm::FieldParameter::UnitString { "RM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fovpt.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPRS");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPRS" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        foprs.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPTS");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPTS" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fopts.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPRF");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPRF" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        foprf.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPTF");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPTF" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        foptf.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPP");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPP" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fopp.reset(new Opm::FieldParameter { std::move(prm) });
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
    fopr ->update(2, dt, input, simRes, st);
    fopt ->update(2, dt, input, simRes, st);
    fovpr->update(2, dt, input, simRes, st);
    fovpt->update(2, dt, input, simRes, st);
    foprs->update(2, dt, input, simRes, st);
    fopts->update(2, dt, input, simRes, st);
    foprf->update(2, dt, input, simRes, st);
    foptf->update(2, dt, input, simRes, st);
    fopp ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fopr ->summaryKey()));
    BOOST_CHECK(st.has(fopt ->summaryKey()));
    BOOST_CHECK(st.has(fovpr->summaryKey()));
    BOOST_CHECK(st.has(fovpt->summaryKey()));
    BOOST_CHECK(st.has(foprs->summaryKey()));
    BOOST_CHECK(st.has(fopts->summaryKey()));
    BOOST_CHECK(st.has(foprf->summaryKey()));
    BOOST_CHECK(st.has(foptf->summaryKey()));
    BOOST_CHECK(st.has(fopp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FOPR"), 60.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOVPR"), 75.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOPRS"), 2.234e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOPRF"), 57.766e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOPP"), 102.92e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FOPT"), 52.62e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOVPT"), 65.775e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOPTS"), 1.959218e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOPTF"), 50.660782e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FGPx)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fgpr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgpt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgvpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgvpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgprs = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgpts = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgprf = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgptf = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgpp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgpt.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGVPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGVPR" },
            Opm::FieldParameter::UnitString { "RM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgvpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGVPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGVPT" },
            Opm::FieldParameter::UnitString { "RM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgvpt.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPRS");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPRS" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgprs.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPTS");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPTS" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgpts.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPRF");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPRF" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgprf.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPTF");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPTF" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgptf.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPP");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPP" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgpp.reset(new Opm::FieldParameter { std::move(prm) });
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
    fgpr ->update(2, dt, input, simRes, st);
    fgpt ->update(2, dt, input, simRes, st);
    fgvpr->update(2, dt, input, simRes, st);
    fgvpt->update(2, dt, input, simRes, st);
    fgprs->update(2, dt, input, simRes, st);
    fgpts->update(2, dt, input, simRes, st);
    fgprf->update(2, dt, input, simRes, st);
    fgptf->update(2, dt, input, simRes, st);
    fgpp ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fgpr ->summaryKey()));
    BOOST_CHECK(st.has(fgpt ->summaryKey()));
    BOOST_CHECK(st.has(fgvpr->summaryKey()));
    BOOST_CHECK(st.has(fgvpt->summaryKey()));
    BOOST_CHECK(st.has(fgprs->summaryKey()));
    BOOST_CHECK(st.has(fgpts->summaryKey()));
    BOOST_CHECK(st.has(fgprf->summaryKey()));
    BOOST_CHECK(st.has(fgptf->summaryKey()));
    BOOST_CHECK(st.has(fgpp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FGPR"), 133.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGVPR"), 5.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGPRS"), 112.15e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGPRF"), 20.85e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGPP"), 146.9e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FGPT"), 116.641e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGVPT"), 4.385e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGPTS"), 98.35555e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGPTF"), 18.28545e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FWPx)
{
    using Type = ::Opm::FieldParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fwpr  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwpt  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwvpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwvpt = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwpp  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWPR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWPT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fwpt.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWVPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWVPR" },
            Opm::FieldParameter::UnitString { "RM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwvpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWVPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWVPT" },
            Opm::FieldParameter::UnitString { "RM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fwvpt.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWPP");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWPP" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwpp.reset(new Opm::FieldParameter { std::move(prm) });
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
    fwpr ->update(2, dt, input, simRes, st);
    fwpt ->update(2, dt, input, simRes, st);
    fwvpr->update(2, dt, input, simRes, st);
    fwvpt->update(2, dt, input, simRes, st);
    fwpp ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fwpr ->summaryKey()));
    BOOST_CHECK(st.has(fwpt ->summaryKey()));
    BOOST_CHECK(st.has(fwvpr->summaryKey()));
    BOOST_CHECK(st.has(fwvpt->summaryKey()));
    BOOST_CHECK(st.has(fwpp ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FWPR"), 55.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWVPR"), 53.9e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWPP"), 80.43e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FWPT"), 48.235e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWVPT"), 47.2703e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FOIx)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto foir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto foit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fovir = std::unique_ptr<Opm::SummaryParameter>{};
    auto fovit = std::unique_ptr<Opm::SummaryParameter>{};
    auto fopi  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOIR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        foir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOIT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        foit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOVIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOVIR" },
            Opm::FieldParameter::UnitString { "RM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fovir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOVIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOVIT" },
            Opm::FieldParameter::UnitString { "RM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fovit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPI");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPI" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fopi.reset(new Opm::FieldParameter { std::move(prm) });
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
    foir ->update(2, dt, input, simRes, st);
    foit ->update(2, dt, input, simRes, st);
    fovir->update(2, dt, input, simRes, st);
    fovit->update(2, dt, input, simRes, st);
    fopi ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(foir ->summaryKey()));
    BOOST_CHECK(st.has(foit ->summaryKey()));
    BOOST_CHECK(st.has(fovir->summaryKey()));
    BOOST_CHECK(st.has(fovit->summaryKey()));
    BOOST_CHECK(st.has(fopi ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FOIR"), 3.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOVIR"), 2.9, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOPI"), 12345.6, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FOIT"), 2631.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOVIT"), 2543.3, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FGIx)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fgir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgvir = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgvit = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgpi  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGIR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGIT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGVIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGVIR" },
            Opm::FieldParameter::UnitString { "RM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgvir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGVIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGVIT" },
            Opm::FieldParameter::UnitString { "RM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgvit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPI");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPI" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgpi.reset(new Opm::FieldParameter { std::move(prm) });
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
    fgir ->update(2, dt, input, simRes, st);
    fgit ->update(2, dt, input, simRes, st);
    fgvir->update(2, dt, input, simRes, st);
    fgvit->update(2, dt, input, simRes, st);
    fgpi ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fgir ->summaryKey()));
    BOOST_CHECK(st.has(fgit ->summaryKey()));
    BOOST_CHECK(st.has(fgvir->summaryKey()));
    BOOST_CHECK(st.has(fgvit->summaryKey()));
    BOOST_CHECK(st.has(fgpi ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FGIR"), 80.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGVIR"), 4.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGPI"), 86420.8, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FGIT"), 70.16e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGVIT"), 3.508e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FWIx)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fwir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwvir = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwvit = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwpi  = std::unique_ptr<Opm::SummaryParameter>{};
    auto flir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto flit  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fvir  = std::unique_ptr<Opm::SummaryParameter>{};
    auto fvit  = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWIR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWIT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fwit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWVIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWVIR" },
            Opm::FieldParameter::UnitString { "RM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwvir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWVIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWVIT" },
            Opm::FieldParameter::UnitString { "RM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fwvit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWPI");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWPI" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwpi.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FLIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FLIR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        flir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FLIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FLIT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        flit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FVIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FVIR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fvir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FVIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FVIT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fvit.reset(new Opm::FieldParameter { std::move(prm) });
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
    fwir ->update(2, dt, input, simRes, st);
    fwit ->update(2, dt, input, simRes, st);
    fwvir->update(2, dt, input, simRes, st);
    fwvit->update(2, dt, input, simRes, st);
    fwpi ->update(2, dt, input, simRes, st);
    flir ->update(2, dt, input, simRes, st);
    flit ->update(2, dt, input, simRes, st);
    fvir ->update(2, dt, input, simRes, st);
    fvit ->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fwir ->summaryKey()));
    BOOST_CHECK(st.has(fwit ->summaryKey()));
    BOOST_CHECK(st.has(fwvir->summaryKey()));
    BOOST_CHECK(st.has(fwvit->summaryKey()));
    BOOST_CHECK(st.has(fwpi ->summaryKey()));
    BOOST_CHECK(st.has(flir ->summaryKey()));
    BOOST_CHECK(st.has(flit ->summaryKey()));
    BOOST_CHECK(st.has(fvir ->summaryKey()));
    BOOST_CHECK(st.has(fvit ->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FWIR"), 20.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWVIR"), 19.0e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWPI"), 543.21e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FLIR"), 20.003e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FVIR"), 23.0029e3, 1.0e-10);

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FWIT"), 17.54e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWVIT"), 16.663e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FLIT"), 17.542631e6, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FVIT"), 20.1735433e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FxR)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fgor = std::unique_ptr<Opm::SummaryParameter>{};
    auto fglr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwct = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGOR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGOR" },
            Opm::FieldParameter::UnitString { "SM3/SM3" },
            Opm::FieldParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        fgor.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGLR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGLR" },
            Opm::FieldParameter::UnitString { "SM3/SM3" },
            Opm::FieldParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        fglr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWCT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWCT" },
            Opm::FieldParameter::UnitString { "" },
            Opm::FieldParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        fwct.reset(new Opm::FieldParameter { std::move(prm) });
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
    fgor->update(2, dt, input, simRes, st);
    fglr->update(2, dt, input, simRes, st);
    fwct->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fgor->summaryKey()));
    BOOST_CHECK(st.has(fglr->summaryKey()));
    BOOST_CHECK(st.has(fwct->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FGOR"), (100.0 + 33.0) / (10.0 + 50.0), 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGLR"), (100.0 + 33.0) / (60.0 + 55.0), 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWCT"), 55.0 / (60.0 + 55.0), 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Solvent)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fnir = std::unique_ptr<Opm::SummaryParameter>{};
    auto fnit = std::unique_ptr<Opm::SummaryParameter>{};
    auto fnpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fnpt = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FNIR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FNIR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fnir.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FNIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FNIT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fnit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FNPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FNPR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fnpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FNPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FNPT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fnpt.reset(new Opm::FieldParameter { std::move(prm) });
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
    fnir->update(2, dt, input, simRes, st);
    fnit->update(2, dt, input, simRes, st);
    fnpr->update(2, dt, input, simRes, st);
    fnpt->update(2, dt, input, simRes, st);

    BOOST_CHECK(st.has(fnir->summaryKey()));
    BOOST_CHECK(st.has(fnit->summaryKey()));
    BOOST_CHECK(st.has(fnpr->summaryKey()));
    BOOST_CHECK(st.has(fnpt->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FNIR"), 25.75, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FNPR"), 6666.6, 1.0e-10);  // 5432.1 + 1234.5

    // Constant rates for each of 877 days
    BOOST_CHECK_CLOSE(st.get("FNIT"), 22.58275e3, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FNPT"), 5.8466082e6, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(Active_Well_Types)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "FIRST_SIM.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fmwin = std::unique_ptr<Opm::SummaryParameter>{};
    auto fmwit = std::unique_ptr<Opm::SummaryParameter>{};
    auto fmwpr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fmwpt = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FMWIN");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FMWIN" },
            Opm::FieldParameter::UnitString { "" },
            Opm::FieldParameter::Type       { Type::Count },
            *eval
        }.validate();

        fmwin.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FMWIT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FMWIT" },
            Opm::FieldParameter::UnitString { "" },
            Opm::FieldParameter::Type       { Type::Count },
            *eval
        }.validate();

        fmwit.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FMWPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FMWPR" },
            Opm::FieldParameter::UnitString { "" },
            Opm::FieldParameter::Type       { Type::Count },
            *eval
        }.validate();

        fmwpr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FMWPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FMWPT" },
            Opm::FieldParameter::UnitString { "" },
            Opm::FieldParameter::Type       { Type::Count },
            *eval
        }.validate();

        fmwpt.reset(new Opm::FieldParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };

    {
        const auto dt = cse.sched.seconds(1);

        fmwin->update(1, dt, input, simRes, st);
        fmwit->update(1, dt, input, simRes, st);
        fmwpr->update(1, dt, input, simRes, st);
        fmwpt->update(1, dt, input, simRes, st);
    }

    BOOST_CHECK(st.has(fmwin->summaryKey()));
    BOOST_CHECK(st.has(fmwit->summaryKey()));
    BOOST_CHECK(st.has(fmwpr->summaryKey()));
    BOOST_CHECK(st.has(fmwpt->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FMWIN"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWIT"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPR"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPT"), 1.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);

        fmwin->update(2, dt, input, simRes, st);
        fmwit->update(2, dt, input, simRes, st);
        fmwpr->update(2, dt, input, simRes, st);
        fmwpt->update(2, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("FMWIN"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWIT"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPR"), 2.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPT"), 2.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(3) - cse.sched.seconds(2);

        fmwin->update(3, dt, input, simRes, st);
        fmwit->update(3, dt, input, simRes, st);
        fmwpr->update(3, dt, input, simRes, st);
        fmwpt->update(3, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("FMWIN"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWIT"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPR"), 2.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPT"), 2.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(4) - cse.sched.seconds(3);

        fmwin->update(4, dt, input, simRes, st);
        fmwit->update(4, dt, input, simRes, st);
        fmwpr->update(4, dt, input, simRes, st);
        fmwpt->update(4, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("FMWIN"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWIT"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPR"), 3.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPT"), 3.0, 1.0e-10);

    {
        const auto dt = cse.sched.seconds(5) - cse.sched.seconds(4);

        fmwin->update(5, dt, input, simRes, st);
        fmwit->update(5, dt, input, simRes, st);
        fmwpr->update(5, dt, input, simRes, st);
        fmwpt->update(5, dt, input, simRes, st);
    }

    BOOST_CHECK_CLOSE(st.get("FMWIN"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWIT"), 1.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FMWPR"), 3.0, 1.0e-10);  // New well OP_5 not flowing
    BOOST_CHECK_CLOSE(st.get("FMWPT"), 4.0, 1.0e-10);
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

//                      +-------+
//                      | FIELD |
//                      +-------+
//                          |
//            +-------------+-------------+
//            |             |             |
//         +-----+       +-----+       +-----+
//         | G_1 |       | G_2 |       | G_3 |
//         +-----+       +-----+       +-----+
//            |             |             |
//    +-------+         +-------+         +-------+
//    |       |         |       |         |       |
// +-----+ +-----+   +-----+ +-----+   +-----+ +-----+
// | W_1 | | W_2 |   | W_3 | | W_6 |   | W_4 | | W_5 |
// +-----+ +-----+   +-----+ +-----+   +-----+ +-----+

BOOST_AUTO_TEST_CASE(FOxH)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto foprh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fopth = std::unique_ptr<Opm::SummaryParameter>{};
    auto foirh = std::unique_ptr<Opm::SummaryParameter>{};
    auto foith = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPRH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        foprh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPTH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPTH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fopth.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOIRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOIRH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        foirh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOITH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOITH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        foith.reset(new Opm::FieldParameter { std::move(prm) });
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
    foprh->update(3, dt, input, simRes, st);
    fopth->update(3, dt, input, simRes, st);
    foirh->update(3, dt, input, simRes, st);
    foith->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(foprh->summaryKey()));
    BOOST_CHECK(st.has(fopth->summaryKey()));
    BOOST_CHECK(st.has(foirh->summaryKey()));
    BOOST_CHECK(st.has(foith->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FOPRH"), 30.2, 1.0e-10); // G_1 + G_2 + G_3
    BOOST_CHECK_CLOSE(st.get("FOIRH"),  0.0, 1.0e-10);

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("FOPTH"), 302.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FOITH"),   0.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FGxH)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fgprh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgpth = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgirh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fgith = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPRH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgprh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGPTH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGPTH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgpth.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGIRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGIRH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fgirh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGITH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGITH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fgith.reset(new Opm::FieldParameter { std::move(prm) });
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
    fgprh->update(3, dt, input, simRes, st);
    fgpth->update(3, dt, input, simRes, st);
    fgirh->update(3, dt, input, simRes, st);
    fgith->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(fgprh->summaryKey()));
    BOOST_CHECK(st.has(fgpth->summaryKey()));
    BOOST_CHECK(st.has(fgirh->summaryKey()));
    BOOST_CHECK(st.has(fgith->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FGPRH"), 30.4  , 1.0e-10); // G_1
    BOOST_CHECK_CLOSE(st.get("FGIRH"), 30.0e3, 1.0e-10); // G_2

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("FGPTH"), 304.0  , 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGITH"), 300.0e3, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FWxH)
{
    using Type = Opm::FieldParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fwprh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwpth = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwirh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwith = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWPRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWPRH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwprh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWPTH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWPTH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fwpth.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWIRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWIRH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fwirh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWITH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWITH" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fwith.reset(new Opm::FieldParameter { std::move(prm) });
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
    fwprh->update(3, dt, input, simRes, st);
    fwpth->update(3, dt, input, simRes, st);
    fwirh->update(3, dt, input, simRes, st);
    fwith->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(fwprh->summaryKey()));
    BOOST_CHECK(st.has(fwpth->summaryKey()));
    BOOST_CHECK(st.has(fwirh->summaryKey()));
    BOOST_CHECK(st.has(fwith->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FWPRH"), 30.0, 1.0e-10); // G_1
    BOOST_CHECK_CLOSE(st.get("FWIRH"), 30.0, 1.0e-10); // G_2

    // Constant rates for each of 10 days
    BOOST_CHECK_CLOSE(st.get("FWPTH"), 300.0, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWITH"), 300.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(FxRH)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "summary_deck.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fgorh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fglrh = std::unique_ptr<Opm::SummaryParameter>{};
    auto fwcth = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGORH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGORH" },
            Opm::FieldParameter::UnitString { "SM3/SM3" },
            Opm::FieldParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        fgorh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FGLRH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FGLRH" },
            Opm::FieldParameter::UnitString { "SM3/SM3" },
            Opm::FieldParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        fglrh.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FWCTH");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FWCTH" },
            Opm::FieldParameter::UnitString { "" },
            Opm::FieldParameter::Type       { Type::Ratio },
            *eval
        }.validate();

        fwcth.reset(new Opm::FieldParameter { std::move(prm) });
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
    fgorh->update(3, dt, input, simRes, st);
    fglrh->update(3, dt, input, simRes, st);
    fwcth->update(3, dt, input, simRes, st);

    BOOST_CHECK(st.has(fgorh->summaryKey()));
    BOOST_CHECK(st.has(fglrh->summaryKey()));
    BOOST_CHECK(st.has(fwcth->summaryKey()));

    BOOST_CHECK_CLOSE(st.get("FGORH"), 30.4 / 30.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FGLRH"), 30.4 / 60.2, 1.0e-10);
    BOOST_CHECK_CLOSE(st.get("FWCTH"), 30.0 / 60.2, 1.0e-10);
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

//                      +-------+
//                      | FIELD |
//                      +---+---+
//                          |
//                  +-------+-------+
//                  |               |
//               +--+--+         +--+--+
//               |  G  |         | G_4 |
//               +--+--+         +--+--+
//                  |               |
//       +----------+            +--+--+
//       |          |            | G_3 |
//    +--+--+    +--+--+         +--+--+
//    | G_1 |    | G_2 |            |
//    +--+--+    +--+--+         +--+--+
//       |          |            | W_3 |
//    +--+--+    +--+--+         +-----+
//    | W_1 |    | W_2 |
//    +-----+    +-----+

BOOST_AUTO_TEST_CASE(FOPT)
{
    using Type = Opm::GroupParameter::Type;

    const auto cse    = Setup{ "SUMMARY_EFF_FAC.DATA" };
    const auto rcache = ::Opm::out::RegionCache{};

    const auto input = ::Opm::SummaryParameter::InputData {
        cse.es, cse.sched, cse.es.getInputGrid(), rcache
    };

    auto fopr = std::unique_ptr<Opm::SummaryParameter>{};
    auto fopt = std::unique_ptr<Opm::SummaryParameter>{};
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPR");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPR" },
            Opm::FieldParameter::UnitString { "SM3/DAY" },
            Opm::FieldParameter::Type       { Type::Rate },
            *eval
        }.validate();

        fopr.reset(new Opm::FieldParameter { std::move(prm) });
    }
    {
        auto eval = ::Opm::SummaryHelpers::getParameterEvaluator("FOPT");

        auto prm = ::Opm::FieldParameter {
            Opm::FieldParameter::Keyword    { "FOPT" },
            Opm::FieldParameter::UnitString { "SM3" },
            Opm::FieldParameter::Type       { Type::Total },
            *eval
        }.validate();

        fopt.reset(new Opm::FieldParameter { std::move(prm) });
    }

    const auto& xw = wellResults();
    const auto& xs = singleResults();
    const auto& xr = regionResults();
    const auto& xb = blockResults();

    const auto simRes = ::Opm::SummaryParameter::SimulatorResults {
        xw, xs, xr, xb
    };

    auto st = ::Opm::SummaryState{ std::chrono::system_clock::now() };
    fopr->update(1, cse.sched.seconds(1), input, simRes, st);
    fopt->update(1, cse.sched.seconds(1), input, simRes, st);

    BOOST_CHECK_CLOSE(st.get("FOPR"),
                      (10.0e3 * 1.0) +
                      (50.0e3 * 0.2 * 0.01) +
                      (25.0e3 * 0.3 * 0.02 * 0.03), 1.0e-10);  // G + G_4

    // Cumulatives after 1st step
    {
        const auto ef_1 = 1.0;
        const auto ef_2 = 0.2 * 0.01;        // WEFAC W_2 * GEFAC G_2
        const auto ef_3 = 0.3 * 0.02 * 0.03; // WEFAC W_3 * GEFAC G_3 * GEFAC G_4

        BOOST_CHECK_CLOSE(st.get("FOPT"),
                          (ef_1 * 100.0e3) +
                          (ef_2 * 500.0e3) +
                          (ef_3 * 250.0e3), 1.0e-10); // == G + G_4
    }

    auto dt = cse.sched.seconds(2) - cse.sched.seconds(1);
    fopr->update(2, dt, input, simRes, st);
    fopt->update(2, dt, input, simRes, st);

    BOOST_CHECK_CLOSE(st.get("FOPR"),
                      (10.0e3 * 1.0) +
                      (50.0e3 * 0.2 * 0.01) +
                      (25.0e3 * 0.3 * 0.02 * 0.04), 1.0e-10);  // G + G_4

    // Cumulatives after 2nd step
    {
        const auto pt_1_init = 1.0               * 100.0e3;
        const auto pt_2_init = 0.2 * 0.01        * 500.0e3;
        const auto pt_4_init = 0.3 * 0.02 * 0.03 * 250.0e3;
        const auto pt_f_init = pt_1_init + pt_2_init + pt_4_init;

        const auto ef_1 = 1.0;
        const auto ef_2 = 0.2 * 0.01;        // WEFAC W_2 * GEFAC G_2
        const auto ef_3 = 0.3 * 0.02 * 0.04; // WEFAC W_3 * GEFAC G_3 * GEFAC G_4

        BOOST_CHECK_CLOSE(st.get("FOPT"),
                          pt_f_init +
                          (ef_1 * 100.0e3) +
                          (ef_2 * 500.0e3) +
                          (ef_3 * 250.0e3), 1.0e-10);
    }
}

BOOST_AUTO_TEST_SUITE_END() // Efficiency_Factors

BOOST_AUTO_TEST_SUITE_END() // Field_Parameters
