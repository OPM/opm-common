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

#include <config.h>

#define BOOST_TEST_MODULE Summary_Dynamic_Connection_Vectors

#include <boost/test/unit_test.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/Summary.hpp>

#include <opm/io/eclipse/ESmry.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <tests/WorkArea.hpp>

#include <cctype>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

using namespace Opm;
using rt = data::Rates::opt;

namespace {

double sm3_pr_day()
{
    return unit::cubic(unit::meter) / unit::day;
}

double cp_rm3_per_db()
{
    return prefix::centi * unit::Poise * unit::cubic(unit::meter)
        / (unit::day * unit::barsa);
}

double operator ""_degC(long double t)
{
    return UnitSystem{}
        .to_si(UnitSystem::measure::temperature, static_cast<double>(t));
}

std::string toUpper(std::string input)
{
    for (auto& c : input) {
        const auto uc = std::toupper(static_cast<unsigned char>(c));
        c = static_cast<std::string::value_type>(uc);
    }

    return input;
}

std::string makeDeck(const bool geomech)
{
    const auto mech = geomech ? std::string{"MECH\nFRAC\n"} : std::string{};

    return std::string { R"(RUNSPEC
DIMENS
 4 1 1 /
OIL
GAS
WATER
)" } + mech + std::string { R"(TABDIMS
/
GRID
DXV
 4*100 /
DYV
 100 /
DZV
 10 /
TOPS
 4*2000 /
EQUALS
  PORO 0.30 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  800 1000 1.05 /
SUMMARY
CTFAC
  W_3 /
/
CWIRFRAC
  W_3 /
/
CFCFFVIR
  W_3 /
/
CFCFVIR
  W_3 /
/
SCHEDULE
WELSPECS
  'W_3' 'FIELD' 3 1 1* 'WATER' 7* /
/
COMPDAT
  W_3 0 0 1 1 2* 1* 2* 0.7 /
/
WCONINJH
  W_3 WATER OPEN 30.0 2.1 2.2 /
/
TSTEP
 2*1 /
END
)" };
}

data::Wells makeWells(const bool includeDynamicConn)
{
    auto wells = data::Wells {};

    data::Rates wellRates{};
    wellRates.set(rt::wat, 30.0 / unit::day)
             .set(rt::oil,  0.0 / unit::day)
             .set(rt::gas,  0.0 / unit::day);

    data::Rates connRates{};
    connRates.set(rt::wat, 30.0 / unit::day)
             .set(rt::wat_frac, 0.618 / unit::day);

    const auto baseFiltrate = data::ConnectionFiltrate {
        .rate = 0.1 * sm3_pr_day(),
        .total = 1.0 * unit::cubic(unit::meter),
        .skin_factor = 1.0,
        .thickness = 0.01 * unit::meter,
        .perm = 1.0 * prefix::milli * unit::darcy,
        .poro = 0.2,
        .radius = 0.05 * unit::meter,
        .area_of_flow = 10.0 * unit::square(unit::meter),
        .flow_factor = 0.75,
        .fracture_rate = 0.025 * sm3_pr_day()
    };

    const auto dynamicFiltrate = data::ConnectionFiltrate {
        .rate = 0.2 * sm3_pr_day(),
        .total = 1.0 * unit::cubic(unit::meter),
        .skin_factor = 1.0,
        .thickness = 0.01 * unit::meter,
        .perm = 1.0 * prefix::milli * unit::darcy,
        .poro = 0.2,
        .radius = 0.05 * unit::meter,
        .area_of_flow = 10.0 * unit::square(unit::meter),
        .flow_factor = 0.75,
        .fracture_rate = 0.05 * sm3_pr_day()
    };

    auto baseConn = data::Connection {
        .index = 2,
        .rates = connRates,
        .pressure = 1.0 * unit::barsa,
        .reservoir_rate = 100.0 * sm3_pr_day(),
        .cell_pressure = 100.0,
        .cell_saturation_water = 0.2,
        .cell_saturation_gas = 0.1,
        .effective_Kh = 10.0,
        .trans_factor = 444.555 * cp_rm3_per_db(),
        .d_factor = 0.0,
        .compact_mult = 1.0,
        .lgr_grid = 0,
        .filtrate = baseFiltrate
    };

    auto dynamicConn = baseConn;
    dynamicConn.index = 3; // (4,1,1) in a 4x1x1 grid
    dynamicConn.trans_factor = 654.321 * cp_rm3_per_db();
    dynamicConn.filtrate = dynamicFiltrate;

    auto well = data::Well {
        .rates = wellRates,
        .bhp = 2.1 * unit::barsa,
        .thp = 2.2 * unit::barsa,
        .temperature = 10.11_degC,
        .control = 3,
        .efficiency_scaling_factor = 1.0,
        .filtrate = {},
        .dynamicStatus = ::Opm::Well::Status::OPEN,
        .connections = { baseConn },
        .segments = {},
        .current_control = data::CurrentControl {},
        .guide_rates = data::GuideRateValue {},
        .limits = data::WellControlLimits {}
    };

    if (includeDynamicConn) {
        well.connections.push_back(dynamicConn);
    }

    well.current_control.isProducer = false;
    well.current_control.inj = ::Opm::Well::InjectorCMode::BHP;

    wells.insert_or_assign("W_3", std::move(well));

    return wells;
}

struct Setup
{
    explicit Setup(std::string caseName, const bool geomech)
        : deck     { Parser{}.parseString(makeDeck(geomech)) }
        , es       { deck }
        , schedule { deck, es, std::make_shared<Python>() }
        , config   { deck, schedule, es.fieldProps(), es.aquifer() }
        , name     { toUpper(std::move(caseName)) }
        , ta       { "summary_test" }
    {}

    Deck deck;
    EclipseState es;
    Schedule schedule;
    SummaryConfig config;
    std::string name;
    WorkArea ta;
};

EclIO::ESmry runCase(const bool geomech)
{
    auto cfg = Setup { geomech ? "dynconn_geomech" : "dynconn_nongeomech", geomech };

    auto writer = out::Summary {
        cfg.config, cfg.es, cfg.es.getInputGrid(), cfg.schedule, cfg.name
    };

    auto st = SummaryState {
        TimeService::now(), cfg.es.runspec().udqParams().undefinedValue()
    };

    auto values = out::Summary::DynamicSimulatorState {};
    auto wells = makeWells(/* includeDynamicConn = */ geomech);
    values.well_solution = &wells;

    writer.eval(/* sim_step = */ 0, /* secs_elapsed = */ 0.0 * unit::day, values, st);
    writer.add_timestep(st, /* report_step = */ 0, /* ministep_id = */ 0, /* isSubstep = */ false);

    // Global index 3 => (4,1,1) in a 4x1x1 grid.
    writer.recordNewDynamicWellConns({ { "W_3", { std::size_t {3} } } });

    writer.eval(/* sim_step = */ 1, /* secs_elapsed = */ 1.0 * unit::day, values, st);
    writer.add_timestep(st, /* report_step = */ 1, /* ministep_id = */ 1, /* isSubstep = */ false);

    writer.write();

    auto smry = EclIO::ESmry { cfg.name };

    // Note: We load all vectors eagerly because the WorkArea and its
    // associated output files will go away at the end of this function.
    smry.loadData();

    return smry;
}

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Summary_Dynamic_Connection_Vectors)

namespace {
    std::string connVectorKey(std::string_view vector, const int I = 4)
    {
        return fmt::format("{}:W_3:{},1,1", vector, I);
    }
}

BOOST_AUTO_TEST_CASE(Geomech_Activates_Dynamic_Connection_Vectors)
{
    const auto smry = runCase(/* geomech = */ true);

    const auto ctfac_key = connVectorKey("CTFAC");
    const auto cwirfrac_key = connVectorKey("CWIRFRAC");
    const auto cfcffvir_key = connVectorKey("CFCFFVIR");
    const auto cfcfvir_key = connVectorKey("CFCFVIR");

    {
        BOOST_REQUIRE_MESSAGE(smry.hasKey(ctfac_key),
                              "Expected dynamic key " << ctfac_key);

        const auto& ctfac = smry.get(ctfac_key);

        BOOST_REQUIRE_EQUAL(ctfac.size(), std::size_t{2});

        BOOST_CHECK_CLOSE(ctfac[0],   0.0f  , 1.0e-4f);
        BOOST_CHECK_CLOSE(ctfac[1], 654.321f, 1.0e-4f);
    }

    {
        BOOST_REQUIRE_MESSAGE(smry.hasKey(cwirfrac_key),
                              "Expected dynamic key " << cwirfrac_key);

        const auto& cwirfrac = smry.get(cwirfrac_key);

        BOOST_REQUIRE_EQUAL(cwirfrac.size(), std::size_t{2});

        BOOST_CHECK_CLOSE(cwirfrac[0], 0.0f  , 1.0e-4f);
        BOOST_CHECK_CLOSE(cwirfrac[1], 0.618f, 1.0e-4f);
    }

    {
        BOOST_REQUIRE_MESSAGE(smry.hasKey(cfcffvir_key),
                              "Expected dynamic key " << cfcffvir_key);

        const auto& cfcffvir = smry.get(cfcffvir_key);

        BOOST_REQUIRE_EQUAL(cfcffvir.size(), std::size_t{2});

        BOOST_CHECK_CLOSE(cfcffvir[0], 0.0f , 1.0e-4f);
        BOOST_CHECK_CLOSE(cfcffvir[1], 0.05f, 1.0e-4f);
    }

    {
        BOOST_REQUIRE_MESSAGE(smry.hasKey(cfcfvir_key),
                              "Expected dynamic key " << cfcfvir_key);

        const auto& cfcfvir = smry.get(cfcfvir_key);

        BOOST_REQUIRE_EQUAL(cfcfvir.size(), std::size_t{2});

        BOOST_CHECK_CLOSE(cfcfvir[0], 0.0f , 1.0e-4f);
        BOOST_CHECK_CLOSE(cfcfvir[1], 0.25f, 1.0e-4f);
    }
}

BOOST_AUTO_TEST_CASE(NonGeomech_Does_Not_Activate_Dynamic_Connection_Vectors)
{
    const auto smry = runCase(/* geomech = */ false);

    BOOST_CHECK(smry.hasKey(connVectorKey("CTFAC", 3)));

    BOOST_CHECK(!smry.hasKey(connVectorKey("CTFAC")));
    BOOST_CHECK(!smry.hasKey(connVectorKey("CWIRFRAC")));
    BOOST_CHECK(!smry.hasKey(connVectorKey("CFCFFVIR")));
    BOOST_CHECK(!smry.hasKey(connVectorKey("CFCFVIR")));
}

BOOST_AUTO_TEST_SUITE_END()
