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
#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

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
