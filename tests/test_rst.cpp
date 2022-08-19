/*
  Copyright 2020 Equinor ASA

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

#define BOOST_TEST_MODULE test_rst

#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/OutputStream.hpp>
#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>

#include <opm/io/eclipse/rst/connection.hpp>
#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/segment.hpp>
#include <opm/io/eclipse/rst/state.hpp>
#include <opm/io/eclipse/rst/well.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/AggregateWellData.hpp>
#include <opm/output/eclipse/AggregateConnectionData.hpp>
#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <tests/WorkArea.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
    struct SimulationCase
    {
        explicit SimulationCase(const Opm::Deck& deck)
            : es    { deck }
            , grid  { deck }
            , sched { deck, es, std::make_shared<Opm::Python>() }
        {}

        // Order requirement: 'es' must be declared/initialised before 'sched'.
        Opm::EclipseState es;
        Opm::EclipseGrid  grid;
        Opm::Schedule     sched;
        Opm::Parser       parser;
    };

    Opm::Deck first_sim()
    {
        // Mostly copy of tests/FIRST_SIM.DATA
        const auto input = std::string {
            R"~(
RUNSPEC
OIL
GAS
WATER
DISGAS
VAPOIL
UNIFOUT
UNIFIN
DIMENS
 10 10 10 /

GRID
DXV
10*0.25 /
DYV
10*0.25 /
DZV
10*0.25 /
TOPS
100*0.25 /

PORO
1000*0.2 /
PERMX
1000*1 /
PERMY
1000*0.1 /
PERMZ
1000*0.01 /

SOLUTION


START             -- 0
1 NOV 1979 /

SCHEDULE
RPTRST
BASIC=1
/
DATES             -- 1
 10  OKT 2008 /
/
WELSPECS
      'OP_1'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
      'OP_2'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
      'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
      'OP_2'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
      'OP_1'  9  9   3   3 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
      'OP_1' 'OPEN' 'ORAT' 20000  4* 1000 /
/
WCONINJE
      'OP_2' 'GAS' 'OPEN' 'RATE' 100 200 400 /
/

DATES             -- 2
 20  JAN 2011 /
/
WELSPECS
      'OP_3'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
      'OP_3'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
      'OP_3' 'OPEN' 'ORAT' 20000  4* 1000 /
/
WCONINJE
      'OP_2' 'WATER' 'OPEN' 'RATE' 100 200 400 /
/

DATES             -- 3
 15  JUN 2013 /
/
COMPDAT
      'OP_2'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
      'OP_1'  9  9   7  7 'SHUT' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

DATES             -- 4
 22  APR 2014 /
/
WELSPECS
      'OP_4'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
      'OP_4'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
      'OP_3'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
      'OP_4' 'OPEN' 'ORAT' 20000  4* 1000 /
/

DATES             -- 5
 30  AUG 2014 /
/
WELSPECS
      'OP_5'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
      'OP_5'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
      'OP_5' 'OPEN' 'ORAT' 20000  4* 1000 /
/

DATES             -- 6
 15  SEP 2014 /
/
WCONPROD
      'OP_3' 'SHUT' 'ORAT' 20000  4* 1000 /
/

DATES             -- 7
 9  OCT 2014 /
/
WELSPECS
      'OP_6'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
      'OP_6'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONPROD
      'OP_6' 'OPEN' 'ORAT' 20000  4* 1000 /
/
TSTEP            -- 8
10 /
)~" };

        return Opm::Parser{}.parseString(input);
    }

    void writeRstFile(const SimulationCase& simCase,
                      const std::string&    baseName,
                      const std::size_t     rptStep)
    {
        const auto& units    = simCase.es.getUnits();
        const auto  sim_step = rptStep - 1;

        const auto sumState     = Opm::SummaryState { Opm::TimeService::now() };
        const auto action_state = Opm::Action::State{};
        const auto wtest_state  = Opm::WellTestState{};

        const auto ih = Opm::RestartIO::Helpers::
            createInteHead(simCase.es, simCase.grid, simCase.sched,
                           0, sim_step, sim_step, sim_step);

        const auto lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);
        const auto dh = Opm::RestartIO::Helpers::
            createDoubHead(simCase.es, simCase.sched,
                           sim_step, sim_step + 1, 0, 0);

        auto wellData = Opm::RestartIO::Helpers::AggregateWellData(ih);
        wellData.captureDeclaredWellData(simCase.sched, simCase.es.tracer(),
                                         sim_step, action_state,
                                         wtest_state, sumState, ih);
        wellData.captureDynamicWellData(simCase.sched, simCase.es.tracer(),
                                        sim_step, {}, sumState);

        auto connectionData = Opm::RestartIO::Helpers::AggregateConnectionData(ih);
        connectionData.captureDeclaredConnData(simCase.sched, simCase.grid, units,
                                               {}, sumState, sim_step);

        auto groupData = Opm::RestartIO::Helpers::AggregateGroupData(ih);
        groupData.captureDeclaredGroupData(simCase.sched, units, sim_step, sumState, ih);

        const auto outputDir = std::string { "./" };

        Opm::EclIO::OutputStream::Restart rstFile {
            Opm::EclIO::OutputStream::ResultSet {outputDir, baseName},
            static_cast<int>(rptStep),
            Opm::EclIO::OutputStream::Formatted {false},
            Opm::EclIO::OutputStream::Unified {true}
        };

        rstFile.write("INTEHEAD", ih);
        rstFile.write("DOUBHEAD", dh);
        rstFile.write("LOGIHEAD", lh);

        rstFile.write("IGRP", groupData.getIGroup());
        rstFile.write("SGRP", groupData.getSGroup());
        rstFile.write("XGRP", groupData.getXGroup());
        rstFile.write("ZGRP", groupData.getZGroup());

        rstFile.write("IWEL", wellData.getIWell());
        rstFile.write("SWEL", wellData.getSWell());
        rstFile.write("XWEL", wellData.getXWell());
        rstFile.write("ZWEL", wellData.getZWell());

        rstFile.write("ICON", connectionData.getIConn());
        rstFile.write("SCON", connectionData.getSConn());
        rstFile.write("XCON", connectionData.getXConn());
    }

    Opm::RestartIO::RstState
    loadRestart(const SimulationCase& simCase,
                const std::string&    baseName,
                const std::size_t     rptStep)
    {
        auto rstFile = std::make_shared<Opm::EclIO::ERst>(baseName + ".UNRST");
        auto rstView = std::make_shared<Opm::EclIO::RestartFileView>
            (std::move(rstFile), rptStep);

        return Opm::RestartIO::RstState::
            load(std::move(rstView), simCase.es.runspec(), simCase.parser);
    }

    Opm::RestartIO::RstState
    makeRestartState(const SimulationCase& simCase,
                     const std::string&    baseName,
                     const std::size_t     rptStep,
                     const std::string&    workArea)
    {
        // Constructor changes working directory of current process
        WorkArea work_area{workArea};

        writeRstFile(simCase, baseName, rptStep);
        return loadRestart(simCase, baseName, rptStep);
    }
} // Anonymous namespace

// =====================================================================

BOOST_AUTO_TEST_CASE(group_test)
{
    const auto simCase = SimulationCase{first_sim()};
    const auto& units = simCase.es.getUnits();
    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep = std::size_t{2};
    const auto sim_step = rptStep - 1;
    Opm::SummaryState sumState(Opm::TimeService::now());

    const auto ih = Opm::RestartIO::Helpers::createInteHead(simCase.es,
                                                            simCase.grid,
                                                            simCase.sched,
                                                            0,
                                                            sim_step,
                                                            sim_step,
                                                            sim_step);

    const auto lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);

    const auto dh = Opm::RestartIO::Helpers::createDoubHead(simCase.es,
                                                            simCase.sched,
                                                            sim_step,
                                                            sim_step+1,
                                                            0, 0);

    auto groupData = Opm::RestartIO::Helpers::AggregateGroupData(ih);
    groupData.captureDeclaredGroupData(simCase.sched, units, sim_step, sumState, ih);

    const auto& igrp = groupData.getIGroup();
    const auto& sgrp = groupData.getSGroup();
    const auto& xgrp = groupData.getXGroup();
    const auto& zgrp8 = groupData.getZGroup();

    Opm::UnitSystem unit_system(Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC);
    std::vector<std::string> zgrp;
    for (const auto& s8: zgrp8)
        zgrp.push_back(s8.c_str());

    Opm::RestartIO::RstHeader header(simCase.es.runspec(), unit_system,ih,lh,dh);
    for (int ig=0; ig < header.ngroup; ig++) {
        std::size_t zgrp_offset = ig * header.nzgrpz;
        std::size_t igrp_offset = ig * header.nigrpz;
        std::size_t sgrp_offset = ig * header.nsgrpz;
        std::size_t xgrp_offset = ig * header.nxgrpz;

        Opm::RestartIO::RstGroup group(unit_system,
                                       header,
                                       zgrp.data() + zgrp_offset,
                                       igrp.data() + igrp_offset,
                                       sgrp.data() + sgrp_offset,
                                       xgrp.data() + xgrp_offset);
    }
}

BOOST_AUTO_TEST_CASE(State_test)
{
    const auto simCase = SimulationCase{first_sim()};

    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep  = std::size_t{4};
    const auto baseName = std::string { "TEST_UDQRST" };

    const auto state =
        makeRestartState(simCase, baseName, rptStep,
                         "test_rstate");

    const auto& well = state.get_well("OP_3");
    BOOST_CHECK_THROW(well.segment(10), std::invalid_argument);




}
