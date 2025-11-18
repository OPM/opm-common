/*
  Copyright 2025 Equinor ASA.

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

#include "config.h"

#define BOOST_TEST_MODULE Summary_Output_GSATPROD

#include <boost/test/unit_test.hpp>

#include <opm/output/data/Groups.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/Inplace.hpp>
#include <opm/output/eclipse/Summary.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/io/eclipse/ERsm.hpp>
#include <opm/io/eclipse/ESmry.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <tests/WorkArea.hpp>

#include <cctype>
#include <memory>
#include <string>
#include <utility>

namespace {

    double day()
    {
        return 1*Opm::unit::day;
    }

    double sm3_pr_day()
    {
        return Opm::unit::cubic(Opm::unit::meter) / Opm::unit::day;
    }

    Opm::data::Wells wellSol()
    {
        auto xw = Opm::data::Wells{};

        auto& prod = xw["PROD"].rates;

        // Recall convention: Negative rates => production.
        prod.set(Opm::data::Rates::opt::oil,  -250*sm3_pr_day())
            .set(Opm::data::Rates::opt::wat,  -100*sm3_pr_day())
            .set(Opm::data::Rates::opt::gas, -2500*sm3_pr_day());

        return xw;
    }

    std::string toupper(std::string input)
    {
        for (auto& c : input) {
            const auto uc = std::toupper(static_cast<unsigned char>(c));
            c = static_cast<std::string::value_type>(uc);
        }

        return input;
    }

    struct Setup
    {
        Setup(std::string        case_name,
              const std::string& input)
            : Setup { std::move(case_name), Opm::Parser{}.parseString(input) }
        {}

        Setup(std::string case_name, const Opm::Deck& deck)
            : es       { deck }
            , schedule { deck, es, std::make_shared<Opm::Python>() }
            , config   { deck, schedule, es.fieldProps(), es.aquifer() }
            , name     { toupper(std::move(case_name)) }
            , ta       { "summary_test" }
        {}

        // ------------------------------------------------------------------------

        Opm::EclipseState es;
        Opm::Schedule schedule;
        Opm::SummaryConfig config;
        std::string name;
        WorkArea ta;
    };

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(No_Efficiency_Factor)

BOOST_AUTO_TEST_CASE(Satprod_Only)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
SCHEDULE
GRUPTREE
  G FIELD /
/
GSATPROD
  G 1000 500 10E3 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    // No dynamic simulator state in this test.
    const auto values = Opm::out::Summary::DynamicSimulatorState{};

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpr[0], 10.0e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 10.0e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 10.0e3f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*10.0e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*10.0e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*10.0e3f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopr[0], 1000.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 1000.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 1000.0f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*1000.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*1000.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*1000.0f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpr[0], 500.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 500.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 500.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*500.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*500.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*500.0f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_CASE(Satprod_And_Wells)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
SCHEDULE
GRUPTREE
  P FIELD /
  S FIELD /
/
GSATPROD
  S 1000 500 10E3 /
/
WELSPECS
  'PROD' 'P' 1 1 1* OIL /
/
COMPDAT
  'PROD' 1 1 1 2 OPEN 1* 1* 0.25 /
/
WCONPROD
  'PROD' OPEN LRAT 2000 1250 20E3 3000 1* 7.5 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    const auto xw = wellSol();
    auto values = Opm::out::Summary::DynamicSimulatorState{};

    values.well_solution = &xw;

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpr[0], 12.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 12.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 12.5e3f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*12.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*12.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*12.5e3f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopr[0], 1250.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 1250.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 1250.0f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*1250.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*1250.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*1250.0f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpr[0], 600.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 600.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 600.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*600.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*600.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*600.0f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // No_Efficiency_Factor

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Single_Level_EFac)

BOOST_AUTO_TEST_CASE(Group_Level_EFac_Affects_Well)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
SCHEDULE
GRUPTREE
  P FIELD /
  S FIELD /
/
GSATPROD
  S 1000 500 10E3 /
/
GEFAC
  P 0.5 /
/
WELSPECS
  'PROD' 'P' 1 1 1* OIL /
/
COMPDAT
  'PROD' 1 1 1 2 OPEN 1* 1* 0.25 /
/
WCONPROD
  'PROD' OPEN LRAT 2000 1250 20E3 3000 1* 7.5 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    const auto xw = wellSol();
    auto values = Opm::out::Summary::DynamicSimulatorState{};

    values.well_solution = &xw;

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpr[0], 11.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 11.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 11.25e3f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*11.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*11.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*11.25e3f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopr[0], 1125.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 1125.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 1125.0f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*1125.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*1125.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*1125.0f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpr[0], 550.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 550.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 550.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*550.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*550.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*550.0f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_CASE(Group_Level_EFact_Affects_Both)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
SCHEDULE
GRUPTREE
  PLAT FIELD /
  WELL PLAT /
  SAT PLAT /
/
GSATPROD
  SAT 1000 500 10E3 /
/
GEFAC
  PLAT 0.5 /
/
WELSPECS
  'PROD' 'WELL' 1 1 1* OIL /
/
COMPDAT
  'PROD' 1 1 1 2 OPEN 1* 1* 0.25 /
/
WCONPROD
  'PROD' OPEN LRAT 2000 1250 20E3 3000 1* 7.5 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    const auto xw = wellSol();
    auto values = Opm::out::Summary::DynamicSimulatorState{};

    values.well_solution = &xw;

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpr[0], 6.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 6.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 6.25e3f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*6.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*6.25e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*6.25e3f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopr[0], 625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 625.0f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*625.0f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpr[0], 300.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 300.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 300.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*300.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*300.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*300.0f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_CASE(Group_Level_EFact_Affects_Sat)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
SCHEDULE
GRUPTREE
  PLAT FIELD /
  WELL PLAT /
  COLL PLAT /
  SAT COLL /
/
GSATPROD
  SAT 1000 500 10E3 /
/
GEFAC
  COLL 0.5 /
/
WELSPECS
  'PROD' 'WELL' 1 1 1* OIL /
/
COMPDAT
  'PROD' 1 1 1 2 OPEN 1* 1* 0.25 /
/
WCONPROD
  'PROD' OPEN LRAT 2000 1250 20E3 3000 1* 7.5 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    const auto xw = wellSol();
    auto values = Opm::out::Summary::DynamicSimulatorState{};

    values.well_solution = &xw;

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpr[0], 7.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 7.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 7.5e3f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*7.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*7.5e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*7.5e3f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopr[0], 750.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 750.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 750.0f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*750.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*750.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*750.0f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpr[0], 350.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 350.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 350.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*350.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*350.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*350.0f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Level_EFac

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Level_EFac)

BOOST_AUTO_TEST_CASE(Top_And_Well)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
SCHEDULE
GRUPTREE
  PLAT FIELD /
  WELL PLAT /
  COLL PLAT /
  SAT COLL /
/
GSATPROD
  SAT 1000 500 10E3 /
/
GEFAC
  COLL 0.5 /
/
WELSPECS
  'PROD' 'WELL' 1 1 1* OIL /
/
COMPDAT
  'PROD' 1 1 1 2 OPEN 1* 1* 0.25 /
/
WEFAC
  PROD 0.25 /
/
WCONPROD
  'PROD' OPEN LRAT 2000 1250 20E3 3000 1* 7.5 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    const auto xw = wellSol();
    auto values = Opm::out::Summary::DynamicSimulatorState{};

    values.well_solution = &xw;

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpr[0], 5625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 5625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 5625.0f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*5625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*5625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*5625.0f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopr[0], 562.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 562.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 562.5f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*562.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*562.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*562.5f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpr[0], 275.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 275.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 275.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*275.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*275.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*275.0f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_CASE(All_Levels)
{
    auto cse = Setup { "SATPROD_ONLY", R"(RUNSPEC
DIMENS
 1 5 2 /
OIL
GAS
WATER
TABDIMS
/
GRID
DXV
 100 /
DYV
 5*100 /
DZV
 2*10 /
DEPTHZ
 12*2000 /
EQUALS
  PORO 0.3 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
FGPR
FGPT
FOPR
FOPT
FWPR
FWPT
GGPR
 COLL WELL /
GGPT
 COLL WELL /
GOPR
 COLL WELL /
GOPT
 COLL WELL /
GWPR
 COLL WELL /
GWPT
 COLL WELL /
SCHEDULE
GRUPTREE
  PLAT FIELD /
  WELL PLAT /
  COLL PLAT /
  SAT COLL /
/
GSATPROD
  SAT 1000 500 10E3 /
/
GEFAC
  PLAT 0.5 /
  COLL 0.75 /
/
WELSPECS
  'PROD' 'WELL' 1 1 1* OIL /
/
COMPDAT
  'PROD' 1 1 1 2 OPEN 1* 1* 0.25 /
/
WEFAC
  PROD 0.25 /
/
WCONPROD
  'PROD' OPEN LRAT 2000 1250 20E3 3000 1* 7.5 /
/
TSTEP
  5*1 /
END
)" };

    auto smry = Opm::out::Summary {
        cse.config, cse.es, cse.es.getInputGrid(), cse.schedule, cse.name
    };

    auto st = Opm::SummaryState {
        Opm::TimeService::now(),
        cse.es.runspec().udqParams().undefinedValue()
    };

    const auto xw = wellSol();
    auto values = Opm::out::Summary::DynamicSimulatorState{};

    values.well_solution = &xw;

    smry.eval(/* report_step = */ 0, /* secs_elapsed = */ 0.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 1, /* secs_elapsed = */ 1.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    smry.eval(/* report_step = */ 2, /* secs_elapsed = */ 2.0*day(), values, st);
    smry.add_timestep(st, /* report_step = */ 2, /* ministep_id = */ 2, /* isSubstep = */ false);

    smry.write();

    const auto res = Opm::EclIO::ESmry { cse.name };

    BOOST_CHECK_MESSAGE(res.hasKey("FGPR"), R"(Summary file must have "FGPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPR"), R"(Summary file must have "FOPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPR"), R"(Summary file must have "FWPR" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FGPT"), R"(Summary file must have "FGPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FOPT"), R"(Summary file must have "FOPT" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("FWPT"), R"(Summary file must have "FWPT" vector)");

    BOOST_CHECK_MESSAGE(res.hasKey("GGPR:COLL"), R"(Summary file must have "GGPR:COLL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GOPR:COLL"), R"(Summary file must have "GOPR:COLL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GWPR:COLL"), R"(Summary file must have "GWPR:COLL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GGPT:COLL"), R"(Summary file must have "GGPT:COLL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GOPT:COLL"), R"(Summary file must have "GOPT:COLL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GWPT:COLL"), R"(Summary file must have "GWPT:COLL" vector)");

    BOOST_CHECK_MESSAGE(res.hasKey("GGPR:WELL"), R"(Summary file must have "GGPR:WELL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GOPR:WELL"), R"(Summary file must have "GOPR:WELL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GWPR:WELL"), R"(Summary file must have "GWPR:WELL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GGPT:WELL"), R"(Summary file must have "GGPT:WELL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GOPT:WELL"), R"(Summary file must have "GOPT:WELL" vector)");
    BOOST_CHECK_MESSAGE(res.hasKey("GWPT:WELL"), R"(Summary file must have "GWPT:WELL" vector)");

    // --------------------------------------------------------------------
    // Field level
    {
        const auto& fgpr = res.get("FGPR");
        BOOST_REQUIRE_EQUAL(fgpr.size(), std::size_t{3});

        // 0.5*0.25*2500 (well) + 0.5*0.75*10e3 (sat)
        BOOST_CHECK_CLOSE(fgpr[0], 4062.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[1], 4062.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpr[2], 4062.5f, 1.0e-5f);
    }

    {
        const auto& fgpt = res.get("FGPT");
        BOOST_REQUIRE_EQUAL(fgpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fgpt[0], 0*4062.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[1], 1*4062.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fgpt[2], 2*4062.5f, 1.0e-5f);
    }

    {
        const auto& fopr = res.get("FOPR");
        BOOST_REQUIRE_EQUAL(fopr.size(), std::size_t{3});

        // 0.5*0.25*250 (well) + 0.5*0.75*1000 (sat)
        BOOST_CHECK_CLOSE(fopr[0], 406.25f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[1], 406.25f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopr[2], 406.25f, 1.0e-5f);
    }

    {
        const auto& fopt = res.get("FOPT");
        BOOST_REQUIRE_EQUAL(fopt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fopt[0], 0*406.25f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[1], 1*406.25f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fopt[2], 2*406.25f, 1.0e-5f);
    }

    {
        const auto& fwpr = res.get("FWPR");
        BOOST_REQUIRE_EQUAL(fwpr.size(), std::size_t{3});

        // 0.5*0.25*100 (well) + 0.5*0.75*500 (sat)
        BOOST_CHECK_CLOSE(fwpr[0], 200.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[1], 200.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpr[2], 200.0f, 1.0e-5f);
    }

    {
        const auto& fwpt = res.get("FWPT");
        BOOST_REQUIRE_EQUAL(fwpt.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(fwpt[0], 0*200.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[1], 1*200.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(fwpt[2], 2*200.0f, 1.0e-5f);
    }

    // --------------------------------------------------------------------
    // Group level (COLL)
    {
        const auto& ggpr = res.get("GGPR:COLL");
        BOOST_REQUIRE_EQUAL(ggpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(ggpr[0], 10.0e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpr[1], 10.0e3f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpr[2], 10.0e3f, 1.0e-5f);
    }

    {
        const auto& ggpt = res.get("GGPT:COLL");
        BOOST_REQUIRE_EQUAL(ggpt.size(), std::size_t{3});

        // 0.5*0.75*10e3
        BOOST_CHECK_CLOSE(ggpt[0], 0*3750.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpt[1], 1*3750.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpt[2], 2*3750.0f, 1.0e-5f);
    }

    {
        const auto& gopr = res.get("GOPR:COLL");
        BOOST_REQUIRE_EQUAL(gopr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(gopr[0], 1000.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopr[1], 1000.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopr[2], 1000.0f, 1.0e-5f);
    }

    {
        const auto& gopt = res.get("GOPT:COLL");
        BOOST_REQUIRE_EQUAL(gopt.size(), std::size_t{3});

        // 0.5*0.75*1000
        BOOST_CHECK_CLOSE(gopt[0], 0*375.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopt[1], 1*375.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopt[2], 2*375.0f, 1.0e-5f);
    }

    {
        const auto& gwpr = res.get("GWPR:COLL");
        BOOST_REQUIRE_EQUAL(gwpr.size(), std::size_t{3});

        BOOST_CHECK_CLOSE(gwpr[0], 500.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpr[1], 500.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpr[2], 500.0f, 1.0e-5f);
    }

    {
        const auto& gwpt = res.get("GWPT:COLL");
        BOOST_REQUIRE_EQUAL(gwpt.size(), std::size_t{3});

        // 0.5*0.75*500
        BOOST_CHECK_CLOSE(gwpt[0], 0*187.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpt[1], 1*187.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpt[2], 2*187.5f, 1.0e-5f);
    }

    // --------------------------------------------------------------------
    // Group level (WELL)
    {
        const auto& ggpr = res.get("GGPR:WELL");
        BOOST_REQUIRE_EQUAL(ggpr.size(), std::size_t{3});

        // 0.25*2500
        BOOST_CHECK_CLOSE(ggpr[0], 625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpr[1], 625.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpr[2], 625.0f, 1.0e-5f);
    }

    {
        const auto& ggpt = res.get("GGPT:WELL");
        BOOST_REQUIRE_EQUAL(ggpt.size(), std::size_t{3});

        // 0.5*0.25*2500
        BOOST_CHECK_CLOSE(ggpt[0], 0*312.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpt[1], 1*312.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(ggpt[2], 2*312.5f, 1.0e-5f);
    }

    {
        const auto& gopr = res.get("GOPR:WELL");
        BOOST_REQUIRE_EQUAL(gopr.size(), std::size_t{3});

        // 0.25*250
        BOOST_CHECK_CLOSE(gopr[0], 62.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopr[1], 62.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopr[2], 62.5f, 1.0e-5f);
    }

    {
        const auto& gopt = res.get("GOPT:WELL");
        BOOST_REQUIRE_EQUAL(gopt.size(), std::size_t{3});

        // 0.5*0.25*250
        BOOST_CHECK_CLOSE(gopt[0], 0*31.25f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopt[1], 1*31.25f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gopt[2], 2*31.25f, 1.0e-5f);
    }

    {
        const auto& gwpr = res.get("GWPR:WELL");
        BOOST_REQUIRE_EQUAL(gwpr.size(), std::size_t{3});

        // 0.25*100
        BOOST_CHECK_CLOSE(gwpr[0], 25.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpr[1], 25.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpr[2], 25.0f, 1.0e-5f);
    }

    {
        const auto& gwpt = res.get("GWPT:WELL");
        BOOST_REQUIRE_EQUAL(gwpt.size(), std::size_t{3});

        // 0.5*0.25*100
        BOOST_CHECK_CLOSE(gwpt[0], 0*12.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpt[1], 1*12.5f, 1.0e-5f);
        BOOST_CHECK_CLOSE(gwpt[2], 2*12.5f, 1.0e-5f);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Level_EFac
