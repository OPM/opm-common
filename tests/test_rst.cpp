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

#include <vector>

#include <opm/output/eclipse/WriteRestartHelpers.hpp>
#include <opm/output/eclipse/AggregateWellData.hpp>
#include <opm/output/eclipse/AggregateConnectionData.hpp>
#include <opm/output/eclipse/AggregateGroupData.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/io/eclipse/rst/connection.hpp>
#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/segment.hpp>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/state.hpp>

namespace {
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

SOLUTION
RESTART
FIRST_SIM 1/


START             -- 0
1 NOV 1979 /

SCHEDULE
SKIPREST
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

    Opm::SummaryState sim_state()
    {
        auto state = Opm::SummaryState{std::chrono::system_clock::now()};

        state.update("WOPR:OP_1" ,    1.0);
        state.update("WWPR:OP_1" ,    2.0);
        state.update("WGPR:OP_1" ,    3.0);
        state.update("WVPR:OP_1" ,    4.0);
        state.update("WOPT:OP_1" ,   10.0);
        state.update("WWPT:OP_1" ,   20.0);
        state.update("WGPT:OP_1" ,   30.0);
        state.update("WVPT:OP_1" ,   40.0);
        state.update("WWIR:OP_1" ,    0.0);
        state.update("WGIR:OP_1" ,    0.0);
        state.update("WWIT:OP_1" ,    0.0);
        state.update("WGIT:OP_1" ,    0.0);
        state.update("WWCT:OP_1" ,    0.625);
        state.update("WGOR:OP_1" ,  234.5);
        state.update("WBHP:OP_1" ,  314.15);
        state.update("WOPTH:OP_1",  345.6);
        state.update("WWPTH:OP_1",  456.7);
        state.update("WGPTH:OP_1",  567.8);
        state.update("WWITH:OP_1",    0.0);
        state.update("WGITH:OP_1",    0.0);
        state.update("WGVIR:OP_1",    0.0);
        state.update("WWVIR:OP_1",    0.0);

        state.update("WOPR:OP_2" ,    0.0);
        state.update("WWPR:OP_2" ,    0.0);
        state.update("WGPR:OP_2" ,    0.0);
        state.update("WVPR:OP_2" ,    0.0);
        state.update("WOPT:OP_2" ,    0.0);
        state.update("WWPT:OP_2" ,    0.0);
        state.update("WGPT:OP_2" ,    0.0);
        state.update("WVPT:OP_2" ,    0.0);
        state.update("WWIR:OP_2" ,  100.0);
        state.update("WGIR:OP_2" ,  200.0);
        state.update("WWIT:OP_2" , 1000.0);
        state.update("WGIT:OP_2" , 2000.0);
        state.update("WWCT:OP_2" ,    0.0);
        state.update("WGOR:OP_2" ,    0.0);
        state.update("WBHP:OP_2" ,  400.6);
        state.update("WOPTH:OP_2",    0.0);
        state.update("WWPTH:OP_2",    0.0);
        state.update("WGPTH:OP_2",    0.0);
        state.update("WWITH:OP_2", 1515.0);
        state.update("WGITH:OP_2", 3030.0);
        state.update("WGVIR:OP_2", 1234.0);
        state.update("WWVIR:OP_2", 4321.0);

        state.update("WOPR:OP_3" ,   11.0);
        state.update("WWPR:OP_3" ,   12.0);
        state.update("WGPR:OP_3" ,   13.0);
        state.update("WVPR:OP_3" ,   14.0);
        state.update("WOPT:OP_3" ,  110.0);
        state.update("WWPT:OP_3" ,  120.0);
        state.update("WGPT:OP_3" ,  130.0);
        state.update("WVPT:OP_3" ,  140.0);
        state.update("WWIR:OP_3" ,    0.0);
        state.update("WGIR:OP_3" ,    0.0);
        state.update("WWIT:OP_3" ,    0.0);
        state.update("WGIT:OP_3" ,    0.0);
        state.update("WWCT:OP_3" ,    0.0625);
        state.update("WGOR:OP_3" , 1234.5);
        state.update("WBHP:OP_3" ,  314.15);
        state.update("WOPTH:OP_3", 2345.6);
        state.update("WWPTH:OP_3", 3456.7);
        state.update("WGPTH:OP_3", 4567.8);
        state.update("WWITH:OP_3",    0.0);
        state.update("WGITH:OP_3",    0.0);
        state.update("WGVIR:OP_3",    0.0);
        state.update("WWVIR:OP_3",   43.21);

        return state;
    }

    Opm::data::WellRates well_rates_1()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::WellRates{};

        {
            xw["OP_1"].rates
                .set(o::wat, 1.0)
                .set(o::oil, 2.0)
                .set(o::gas, 3.0);

            xw["OP_1"].connections.emplace_back();
            auto& c = xw["OP_1"].connections.back();

            c.rates.set(o::wat, 1.0)
                   .set(o::oil, 2.0)
                   .set(o::gas, 3.0);
        }

        {
            xw["OP_2"].bhp = 234.0;

            xw["OP_2"].rates.set(o::gas, 5.0);
            xw["OP_2"].connections.emplace_back();
        }

        return xw;
    }

    Opm::data::WellRates well_rates_2()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::WellRates{};

        {
            xw["OP_1"].bhp = 150.0;  // Closed
        }

        {
            xw["OP_2"].bhp = 234.0;

            xw["OP_2"].rates.set(o::wat, 5.0);
            xw["OP_2"].connections.emplace_back();

            auto& c = xw["OP_2"].connections.back();
            c.rates.set(o::wat, 5.0);
        }

        return xw;
    }


} // namespace

struct SimulationCase
{
    explicit SimulationCase(const Opm::Deck& deck)
        : es   { deck }
        , grid { deck }
        , sched{ deck, es }
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    Opm::EclipseState es;
    Opm::EclipseGrid  grid;
    Opm::Schedule     sched;
};

// =====================================================================
BOOST_AUTO_TEST_CASE(group_test) {
    const auto simCase = SimulationCase{first_sim()};
    const auto& units = simCase.es.getUnits();
    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep = std::size_t{2};
    const auto sim_step = rptStep - 1;
    Opm::SummaryState sumState(std::chrono::system_clock::now());

    const auto ih = Opm::RestartIO::Helpers::createInteHead(simCase.es,
                                                            simCase.grid,
                                                            simCase.sched,
                                                            0,
                                                            sim_step,
                                                            sim_step);

    const auto lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);

    const auto dh = Opm::RestartIO::Helpers::createDoubHead(simCase.es,
                                                            simCase.sched,
                                                            sim_step,
                                                            0, 0);

    auto groupData = Opm::RestartIO::Helpers::AggregateGroupData(ih);
    groupData.captureDeclaredGroupData(simCase.sched, units, sim_step, sumState, ih);

    const auto& igrp = groupData.getIGroup();
    const auto& sgrp = groupData.getSGroup();
    const auto& xgrp = groupData.getXGroup();
    const auto& zgrp = groupData.getZGroup();


    Opm::RestartIO::Header header(ih,lh,dh);
    for (int ig=0; ig < header.ngroup; ig++) {
        std::size_t zgrp_offset = ig * header.nzgrpz;
        std::size_t igrp_offset = ig * header.nigrpz;
        std::size_t sgrp_offset = ig * header.nsgrpz;
        std::size_t xgrp_offset = ig * header.nxgrpz;

        Opm::RestartIO::Group group(header,
                                    zgrp.data() + zgrp_offset,
                                    igrp.data() + igrp_offset,
                                    sgrp.data() + sgrp_offset,
                                    xgrp.data() + xgrp_offset);
    }


}

BOOST_AUTO_TEST_CASE(state_test) {
    const auto simCase = SimulationCase{first_sim()};
    const auto& units = simCase.es.getUnits();
    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep = std::size_t{2};
    const auto sim_step = rptStep - 1;
    Opm::SummaryState sumState(std::chrono::system_clock::now());

    const auto ih = Opm::RestartIO::Helpers::createInteHead(simCase.es,
                                                            simCase.grid,
                                                            simCase.sched,
                                                            0,
                                                            sim_step,
                                                            sim_step);

    const auto lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);

    const auto dh = Opm::RestartIO::Helpers::createDoubHead(simCase.es,
                                                            simCase.sched,
                                                            sim_step,
                                                            0, 0);

    auto wellData = Opm::RestartIO::Helpers::AggregateWellData(ih);
    wellData.captureDeclaredWellData(simCase.sched, units, sim_step, sumState, ih);
    wellData.captureDynamicWellData(simCase.sched, sim_step, {} , sumState);

    auto connectionData = Opm::RestartIO::Helpers::AggregateConnectionData(ih);
    connectionData.captureDeclaredConnData(simCase.sched, simCase.grid, units, {} , sim_step);

    auto groupData = Opm::RestartIO::Helpers::AggregateGroupData(ih);
    groupData.captureDeclaredGroupData(simCase.sched, units, sim_step, sumState, ih);

    const auto& iwel = wellData.getIWell();
    const auto& swel = wellData.getSWell();
    const auto& xwel = wellData.getXWell();
    const auto& zwel = wellData.getZWell();

    const auto& icon = connectionData.getIConn();
    const auto& scon = connectionData.getSConn();
    const auto& xcon = connectionData.getXConn();

    const auto& zgrp = groupData.getZGroup();
    const auto& igrp = groupData.getIGroup();
    const auto& sgrp = groupData.getSGroup();
    const auto& xgrp = groupData.getXGroup();

    Opm::RestartIO::state state(ih, lh, dh,
                                zgrp, igrp, sgrp, xgrp,
                                zwel, iwel, swel, xwel,
                                icon, scon, xcon);
}
