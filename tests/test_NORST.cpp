/*
  Copyright 2026 SINTEF Digital.

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

#define BOOST_TEST_MODULE NORST_Test

#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/OutputStream.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>

#include <opm/output/eclipse/AggregateAquiferData.hpp>
#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>
#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Groups.hpp>
#include <opm/output/data/Wells.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <tests/WorkArea.hpp>

namespace {

struct Setup
{
    Opm::EclipseState es;
    const Opm::EclipseGrid& grid;
    Opm::Schedule schedule;
    Opm::SummaryConfig summary_config;

    explicit Setup(const std::string& path)
        : Setup { Opm::Parser{}.parseFile(path) }
    {}

    explicit Setup(const Opm::Deck& deck)
        : es             { deck }
        , grid           { es.getInputGrid() }
        , schedule       { deck, es, std::make_shared<Opm::Python>() }
        , summary_config { deck, schedule, es.fieldProps(), es.aquifer() }
    {
        es.getIOConfig().setEclCompatibleRST(false);
    }
};

std::unique_ptr<Opm::EclIO::RestartFileView>
openRestart(const std::string& filename, const int report_step)
{
    auto rst = std::make_shared<Opm::EclIO::ERst>(filename);
    return std::make_unique<Opm::EclIO::RestartFileView>(std::move(rst), report_step);
}

Opm::data::Solution makeSolution(const int numCells)
{
    using measure = Opm::UnitSystem::measure;

    auto sol = Opm::data::Solution {
        { "PRESSURE", Opm::data::CellData { measure::pressure, {}, Opm::data::TargetType::RESTART_SOLUTION } },
        { "SWAT",     Opm::data::CellData { measure::identity, {}, Opm::data::TargetType::RESTART_SOLUTION } },
    };

    sol.data<double>("PRESSURE").assign(numCells, 250.0);
    sol.data<double>("SWAT").assign(numCells, 0.3);

    return sol;
}

void writeRestartFile(const Setup&       setup,
                      const std::string& outputDir,
                      const std::string& caseName)
{
    namespace OS = Opm::EclIO::OutputStream;

    const auto seqnum      = 1;
    const auto numCells    = setup.grid.getNumActive();
    auto       aquiferData = std::optional<Opm::RestartIO::Helpers::AggregateAquiferData>{};
    auto       action_state = Opm::Action::State{};
    auto       wtest_state  = Opm::WellTestState{};
    auto       sumState     = Opm::SummaryState{ Opm::TimeService::now(), 0.0 };
    const auto udqState     = Opm::UDQState{ 1 };

    Opm::RestartValue restartValue { makeSolution(numCells), {}, {}, {} };

    auto rstFile = OS::Restart {
        OS::ResultSet{ outputDir, caseName }, seqnum,
        OS::Formatted{ false }, OS::Unified{ true }
    };

    Opm::RestartIO::save(rstFile, seqnum, 100.0,
                         restartValue,
                         setup.es, setup.grid, setup.schedule,
                         action_state, wtest_state,
                         sumState, udqState,
                         aquiferData, false);
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(NORST)

// A standard full restart file must not be flagged as graphics-only.
BOOST_AUTO_TEST_CASE(isGraphicsOnly_Full_Restart_Returns_False)
{
    const auto rst = openRestart("SPE1_TESTCASE.UNRST", 10);
    BOOST_CHECK_MESSAGE(!rst->isGraphicsOnly(),
        "Full restart file must not be detected as graphics-only");
}

// OPM-written NORST=0 restart must not be flagged as graphics-only.
BOOST_AUTO_TEST_CASE(isGraphicsOnly_OPM_NORST0_Returns_False)
{
    WorkArea work_area("test_NORST0");
    work_area.copyIn("BASE_SIM.DATA");

    const Setup setup { "BASE_SIM.DATA" };
    writeRestartFile(setup, work_area.currentWorkingDirectory(), "BASE_SIM");

    const auto rst = openRestart("BASE_SIM.UNRST", 1);
    BOOST_CHECK_MESSAGE(!rst->isGraphicsOnly(),
        "NORST=0 restart must not be detected as graphics-only");
}

// OPM-written NORST=2 restart must be flagged as graphics-only.
BOOST_AUTO_TEST_CASE(isGraphicsOnly_OPM_NORST2_Returns_True)
{
    WorkArea work_area("test_NORST2");
    work_area.copyIn("NORST_SIM.DATA");

    const Setup setup { "NORST_SIM.DATA" };
    writeRestartFile(setup, work_area.currentWorkingDirectory(), "NORST_SIM");

    const auto rst = openRestart("NORST_SIM.UNRST", 1);
    BOOST_CHECK_MESSAGE(rst->isGraphicsOnly(),
        "NORST=2 restart must be detected as graphics-only");
}

// Loading a graphics-only restart must throw a descriptive error.
BOOST_AUTO_TEST_CASE(Load_Throws_On_Graphics_Only)
{
    WorkArea work_area("test_NORST_load");
    work_area.copyIn("NORST_SIM.DATA");

    const Setup setup { "NORST_SIM.DATA" };
    writeRestartFile(setup, work_area.currentWorkingDirectory(), "NORST_SIM");

    Opm::Action::State action_state;
    auto sumState = Opm::SummaryState { Opm::TimeService::now(), 0.0 };

    const std::vector<Opm::RestartKey> solution_keys {
        { "PRESSURE", Opm::UnitSystem::measure::pressure },
    };

    BOOST_CHECK_THROW(
        Opm::RestartIO::load("NORST_SIM.UNRST", 1,
                             action_state, sumState,
                             solution_keys,
                             setup.es, setup.grid, setup.schedule, {}),
        std::runtime_error
    );
}

BOOST_AUTO_TEST_SUITE_END()
