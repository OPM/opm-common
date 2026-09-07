/*
  Copyright 2018 Statoil ASA

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

#define BOOST_TEST_MODULE Aggregate_Group_Data

#include <opm/output/eclipse/AggregateGroupData.hpp>

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/TracerConfig.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

    struct SimulationCase
    {
        explicit SimulationCase(std::string_view deck)
            : SimulationCase { Opm::Parser{}.parseString(std::string {deck}) }
        {}

        explicit SimulationCase(const Opm::Deck& deck)
            : es    {deck}
            , grid  {deck}
            , sched {deck, es, std::make_shared<Opm::Python>()}
        {}

        // Order requirement: 'es' must be declared/initialised before 'sched'.
        Opm::EclipseState es;
        Opm::EclipseGrid grid;
        Opm::Schedule sched;
    };

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Aggregate_Group)

namespace {

struct MockIH
{
    MockIH(const int numWells,
           const int igrpPerGrp	 = 101,   // number of data elements per group in IGRP array
           const int sgrpPerGrp  = 112,   // number of data elements per group in SGRP array
           const int xgrpPerGrp  = 181,   // number of data elements per group in XGRP array
           const int zgrpPerGrp  =   5);  // number of data elements per group in XGRP array


    std::vector<int> value;

    using Sz = std::vector<int>::size_type;

    Sz nwells{};
    Sz nwgmax{};
    Sz ngmaxz{};
    Sz nigrpz{};
    Sz nsgrpz{};
    Sz nxgrpz{};
    Sz nzgrpz{};
};

MockIH::MockIH(const int numWells,
               const int igrpPerGrp,
               const int sgrpPerGrp,
               const int xgrpPerGrp,
               const int zgrpPerGrp)
    : value(411, 0)
{
    using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;

    this->nwells = this->value[Ix::NWELLS] = numWells;

    this->ngmaxz = this->value[Ix::NGMAXZ] = 5;
    this->nwgmax = this->value[Ix::NWGMAX] = 4;

    this->nigrpz = this->value[Ix::NIGRPZ] = igrpPerGrp;
    this->nsgrpz = this->value[Ix::NSGRPZ] = sgrpPerGrp;
    this->nxgrpz = this->value[Ix::NXGRPZ] = xgrpPerGrp;
    this->nzgrpz = this->value[Ix::NZGRPZ] = zgrpPerGrp;
}

Opm::Deck second_sim(std::string_view fname)
{
    return Opm::Parser {}.parseFile(std::string{fname});
}

Opm::Deck first_sim()
{
    // Mostly copy of tests/FIRST_SIM.DATA
    const std::string input = std::string {R"~(RUNSPEC
TITLE
2 PRODUCERS  AND INJECTORS, 2 WELL GROUPS AND ONE INTERMEDIATE GROUP LEVEL  BELOW THE FIELD LEVEL

DIMENS
  10  5  10  /

OIL
WATER
GAS
DISGAS

FIELD

TABDIMS
  1  1  15  15  2  15  /

EQLDIMS
  2  /

WELLDIMS
  4  20  4  2  /

UNIFIN
UNIFOUT

--FMTIN
--FMTOUT

START
1 'JAN' 2015 /

-- RPTRUNSP

GRID        =========================================================

--NOGGF
BOX
1 10 1 5 1 1 /

TOPS
50*7000 /

BOX
1 10  1 5 1 10 /

DXV
10*100 /
DYV
5*100  /
DZV
2*20 100 7*20 /

EQUALS
-- 'DX'     100  /
-- 'DY'     100  /
'PERMX'  50   /
'PERMZ'  5   /
-- 'DZ'     20   /
'PORO'   0.2  /
-- 'TOPS'   7000   1 10  1 5  1 1  /
-- 'DZ'     100    1 10  1 5  3 3  /
-- 'PORO'   0.0    1 10  1 5  3 3  /
/

COPY
PERMX PERMY /
/

RPTGRID
-- Report Levels for Grid Section Data
--
/

PROPS       ==========================================================

-- WATER RELATIVE PERMEABILITY AND CAPILLARY PRESSURE ARE TABULATED AS
-- A FUNCTION OF WATER SATURATION.
--
SWFN
--  SWAT   KRW   PCOW
0.12  0       0
1.0   0.00001 0  /

-- SIMILARLY FOR GAS
--
SGFN
--  SGAS   KRG   PCOG
0     0       0
0.02  0       0
0.05  0.005   0
0.12  0.025   0
0.2   0.075   0
0.25  0.125   0
0.3   0.19    0
0.4   0.41    0
0.45  0.6     0
0.5   0.72    0
0.6   0.87    0
0.7   0.94    0
0.85  0.98    0
1.0   1.0     0
/

-- OIL RELATIVE PERMEABILITY IS TABULATED AGAINST OIL SATURATION
-- FOR OIL-WATER AND OIL-GAS-CONNATE WATER CASES
--
SOF3
--  SOIL     KROW     KROG
0        0        0
0.18     0        0
0.28     0.0001   0.0001
0.38     0.001    0.001
0.43     0.01     0.01
0.48     0.021    0.021
0.58     0.09     0.09
0.63     0.2      0.2
0.68     0.35     0.35
0.76     0.7      0.7
0.83     0.98     0.98
0.86     0.997    0.997
0.879    1        1
0.88     1        1    /


-- PVT PROPERTIES OF WATER
--
--    REF. PRES. REF. FVF  COMPRESSIBILITY  REF VISCOSITY  VISCOSIBILITY
PVTW
4014.7     1.029        3.13D-6           0.31            0 /

-- ROCK COMPRESSIBILITY
--
--    REF. PRES   COMPRESSIBILITY
ROCK
14.7          3.0D-6          /

-- SURFACE DENSITIES OF RESERVOIR FLUIDS
--
--        OIL   WATER   GAS
DENSITY
49.1   64.79  0.06054  /

-- PVT PROPERTIES OF DRY GAS (NO VAPOURISED OIL)
-- WE WOULD USE PVTG TO SPECIFY THE PROPERTIES OF WET GAS
--
--   PGAS   BGAS   VISGAS
PVDG
14.7 166.666   0.008
264.7  12.093   0.0096
514.7   6.274   0.0112
1014.7   3.197   0.014
2014.7   1.614   0.0189
2514.7   1.294   0.0208
3014.7   1.080   0.0228
4014.7   0.811   0.0268
5014.7   0.649   0.0309
9014.7   0.386   0.047   /

-- PVT PROPERTIES OF LIVE OIL (WITH DISSOLVED GAS)
-- WE WOULD USE PVDO TO SPECIFY THE PROPERTIES OF DEAD OIL
--
-- FOR EACH VALUE OF RS THE SATURATION PRESSURE, FVF AND VISCOSITY
-- ARE SPECIFIED. FOR RS=1.27 AND 1.618, THE FVF AND VISCOSITY OF
-- UNDERSATURATED OIL ARE DEFINED AS A FUNCTION OF PRESSURE. DATA
-- FOR UNDERSATURATED OIL MAY BE SUPPLIED FOR ANY RS, BUT MUST BE
-- SUPPLIED FOR THE HIGHEST RS (1.618).
--
--   RS      POIL  FVFO  VISO
PVTO
0.001    14.7 1.062  1.04    /
0.0905  264.7 1.15   0.975   /
0.18    514.7 1.207  0.91    /
0.371  1014.7 1.295  0.83    /
0.636  2014.7 1.435  0.695   /
0.775  2514.7 1.5    0.641   /
0.93   3014.7 1.565  0.594   /
1.270  4014.7 1.695  0.51
5014.7 1.671  0.549
9014.7 1.579  0.74    /
1.618  5014.7 1.827  0.449
9014.7 1.726  0.605   /
/


RPTPROPS
-- PROPS Reporting Options
--
/

REGIONS    ===========================================================


FIPNUM
100*1
400*2
/

EQLNUM
100*1
400*2
/

RPTREGS
/

SOLUTION    ============================================================

EQUIL
7020.00 2700.00 7990.00  .00000 7020.00  .00000     0      0       5 /
7200.00 3700.00 7300.00  .00000 7000.00  .00000     1      0       5 /

RSVD       2 TABLES    3 NODES IN EACH           FIELD   12:00 17 AUG 83
7000.0  1.0000
7990.0  1.0000
/
7000.0  1.0000
7400.0  1.0000
/

RPTRST
-- Restart File Output Control
--
'BASIC=2' 'FLOWS' 'POT' 'PRES' /


SUMMARY      ===========================================================

FOPR
WOPR
/

FGPR
FWPR
FWIR
FWCT
FGOR

--RUNSUM

ALL

MSUMLINS
MSUMNEWT
SEPARATE

SCHEDULE     ===========================================================

DEBUG
1 3   /

DRSDT
1.0E20  /

RPTSCHED
'PRES'  'SWAT'  'SGAS'  'RESTART=1'  'RS'  'WELLS=2'  'SUMMARY=2'
'CPU=2' 'WELSPECS'   'NEWTON=2' /

NOECHO


ECHO

GRUPTREE
'GRP1' 'FIELD' /
'WGRP1' 'GRP1' /
'WGRP2' 'GRP1' /
/

WELSPECS
'PROD1' 'WGRP1' 1 5 7030 'OIL' 0.0  'STD'  'STOP'  /
'PROD2' 'WGRP2' 1 5 7030 'OIL' 0.0  'STD'  'STOP'  /
'WINJ1'  'WGRP1' 10 1 7030 'WAT' 0.0  'STD'  'STOP'   /
'WINJ2'  'WGRP2' 10 1 7030 'WAT' 0.0  'STD'  'STOP'   /
/

COMPDAT
'PROD1' 1 5 2 2   3*  0.2   3*  'X' /
'PROD1' 2 5 2 2   3*  0.2   3*  'X' /
'PROD1' 3 5 2 2   3*  0.2   3*  'X' /
'PROD2' 4 5 2 2   3*  0.2   3*  'X' /
'PROD2' 5 5 2 2   3*  0.2   3*  'X' /

'WINJ1' 10 1  9 9   3*  0.2   3*  'X' /
'WINJ1'   9 1  9 9   3*  0.2   3*  'X' /
'WINJ1'   8 1  9 9   3*  0.2   3*  'X' /
'WINJ2'   7 1  9 9   3*  0.2   3*  'X' /
'WINJ2'   6 1  9 9   3*  0.2   3*  'X' /
/

WCONPROD
'PROD1' 'OPEN' 'LRAT'  3*  1200  1*  2500  1*  /
'PROD2' 'OPEN' 'LRAT'  3*    800  1*  2500  1*  /
/

WCONINJE
'WINJ1' 'WAT' 'OPEN' 'BHP'  1*  1200  3500  1*  /
'WINJ2' 'WAT' 'OPEN' 'BHP'  1*    800  3500  1*  /
/

TUNING
/
/
/

LIFTOPT
  1 2 /

GLIFTOPT
   'WGRP1'  100 200 /
/

TSTEP
4
/

END
)~" };

    return Opm::Parser{}.parseString(input);
}

Opm::SummaryState sim_state()
{
    auto state = Opm::SummaryState {Opm::TimeService::now(), 0.0};

    state.update_group_var("GRP1", "GOPR",   235.);
    state.update_group_var("GRP1", "GGPR",   100237.);
    state.update_group_var("GRP1", "GWPR",   239.);
    state.update_group_var("GRP1", "GOPGR",  345.6);
    state.update_group_var("GRP1", "GWPGR",  456.7);
    state.update_group_var("GRP1", "GGPGR",  567.8);
    state.update_group_var("GRP1", "GVPGR",  678.9);
    state.update_group_var("GRP1", "GOIGR", 0.123);
    state.update_group_var("GRP1", "GWIGR", 1234.5);
    state.update_group_var("GRP1", "GGIGR", 2345.6);

    state.update_group_var("WGRP1", "GOPR",   23.);
    state.update_group_var("WGRP1", "GGPR",   50237.);
    state.update_group_var("WGRP1", "GWPR",   29.);
    state.update_group_var("WGRP1", "GOPGR",  456.7);
    state.update_group_var("WGRP1", "GWPGR",  567.8);
    state.update_group_var("WGRP1", "GGPGR",  678.9);
    state.update_group_var("WGRP1", "GVPGR",  789.1);
    state.update_group_var("WGRP1", "GOIGR", 1.23);
    state.update_group_var("WGRP1", "GWIGR", 2345.6);
    state.update_group_var("WGRP1", "GGIGR", 3456.7);

    state.update_group_var("WGRP2", "GOPR",   43.);
    state.update_group_var("WGRP2", "GGPR",   70237.);
    state.update_group_var("WGRP2", "GWPR",   59.);
    state.update_group_var("WGRP2", "GOPGR",  56.7);
    state.update_group_var("WGRP2", "GWPGR",  67.8);
    state.update_group_var("WGRP2", "GGPGR",  78.9);
    state.update_group_var("WGRP2", "GVPGR",  89.1);
    state.update_group_var("WGRP2", "GOIGR", 12.3);
    state.update_group_var("WGRP2", "GWIGR", 345.6);
    state.update_group_var("WGRP2", "GGIGR", 456.7);

    state.update("FOPR",   3456.);
    state.update("FGPR",   2003456.);
    state.update("FWPR",   5678.);

    return state;
}

Opm::SummaryState sim_state_2()
{
    auto state = Opm::SummaryState {Opm::TimeService::now(), 0.0};
    state.update_group_var("UPPER", "GMCTP", -1.);
    state.update_group_var("UPPER", "GMCTW",  0.);
    state.update_group_var("UPPER", "GMCTG",  0.);

    state.update_group_var("MOD4", "GMCTP",  1.);
    state.update_group_var("MOD4", "GMCTW",  3.);
    state.update_group_var("MOD4", "GMCTG",  0.);

    state.update_group_var("LOWER", "GMCTP", -1.);
    state.update_group_var("LOWER", "GMCTW",  0.);
    state.update_group_var("LOWER", "GMCTG",  0.);

    state.update_group_var("AQF", "GMCTP",  0.);
    state.update_group_var("AQF", "GMCTW",  0.);
    state.update_group_var("AQF", "GMCTG",  0.);

    state.update_group_var("MAIN", "GMCTP",  0.);
    state.update_group_var("MAIN", "GMCTW",  0.);
    state.update_group_var("MAIN", "GMCTG",  0.);

    state.update_group_var("NE", "GMCTP",  0.);
    state.update_group_var("NE", "GMCTW",  0.);
    state.update_group_var("NE", "GMCTG",  0.);

    state.update_group_var("NW", "GMCTP",  0.);
    state.update_group_var("NW", "GMCTW",  3.);
    state.update_group_var("NW", "GMCTG",  0.);

    state.update_group_var("SE", "GMCTP",  0.);
    state.update_group_var("SE", "GMCTW",  0.);
    state.update_group_var("SE", "GMCTG",  0.);

    state.update_group_var("CENTRAL", "GMCTP",  0.);
    state.update_group_var("CENTRAL", "GMCTW",  0.);
    state.update_group_var("CENTRAL", "GMCTG",  0.);

    state.update_well_var("OPL1", "WMCTL", -1.);
    state.update_well_var("OPL2", "WMCTL",  0.);
    state.update_well_var("OPL3", "WMCTL", -1.);
    state.update_well_var("OPL4", "WMCTL", -1.);
    state.update_well_var("OPL5", "WMCTL", -1.);
    state.update_well_var("OPU1", "WMCTL", -1.);
    state.update_well_var("OPU2", "WMCTL", -1.);
    state.update_well_var("OPU3", "WMCTL", -1.);
    state.update_well_var("OPU4", "WMCTL", -1.);
    state.update_well_var("OPU5", "WMCTL", -1.);
    state.update_well_var("OPU6", "WMCTL", -1.);
    state.update_well_var("OPU7", "WMCTL", -1.);
    state.update_well_var("WID1", "WMCTL", -1.);
    state.update_well_var("WID2", "WMCTL", -1.);
    state.update_well_var("WIL1", "WMCTL", -1.);
    state.update_well_var("WIL2", "WMCTL", -1.);
    state.update_well_var("WIU1", "WMCTL", -1.);
    state.update_well_var("WIU2", "WMCTL", -1.);
    state.update_well_var("WIU3", "WMCTL", -1.);
    state.update_well_var("WIU4", "WMCTL", -1.);

    return state;
}

Opm::SummaryState sim_state_3()
{
    auto state = Opm::SummaryState {Opm::TimeService::now(), 0.0};

    state.update("FMCTP", 0.0);  // FIELD: Production mode NONE
    state.update("FMCTW", 3.0);  // FIELD: Injection mode VREP for water
    state.update("FMCTG", 0.0);  // FIELD: Injection mode NONE for gas

    state.update_group_var("PLAT-A", "GMCTP", -1.0);
    state.update_group_var("PLAT-A", "GMCTW",  0.0);
    state.update_group_var("PLAT-A", "GMCTG",  0.0);

    state.update_group_var("M5S", "GMCTP",  1.0);
    state.update_group_var("M5S", "GMCTW",  3.0);
    state.update_group_var("M5S", "GMCTG",  0.0);

    state.update_group_var("M5N", "GMCTP", -1.0);
    state.update_group_var("M5N", "GMCTW",  0.0);
    state.update_group_var("M5N", "GMCTG",  0.0);

    state.update_group_var("B1", "GMCTP",  0.0);
    state.update_group_var("B1", "GMCTW",  0.0);
    state.update_group_var("B1", "GMCTG",  0.0);

    state.update_group_var("C1", "GMCTP",  0.0);
    state.update_group_var("C1", "GMCTW",  0.0);
    state.update_group_var("C1", "GMCTG",  0.0);

    state.update_group_var("F1", "GMCTP",  0.0);
    state.update_group_var("F1", "GMCTW",  0.0);
    state.update_group_var("F1", "GMCTG",  0.0);

    state.update_well_var("B-1H", "WMCTL", -1.0);
    state.update_well_var("B-2H", "WMCTL",  0.0);
    state.update_well_var("B-3H", "WMCTL", -1.0);
    state.update_well_var("G-3H", "WMCTL", -1.0);
    state.update_well_var("G-4H", "WMCTL", -1.0);
    state.update_well_var("C-1H", "WMCTL", -1.0);
    state.update_well_var("C-2H", "WMCTL", -1.0);
    state.update_well_var("F-1H", "WMCTL", -1.0);
    state.update_well_var("F-2H", "WMCTL", -1.0);

    return state;
}

} // Anonymous namespace

// test dimensions of multisegment data
BOOST_AUTO_TEST_CASE (Constructor)
{
    const auto ih = MockIH { 5 };

    const auto agrpd = Opm::RestartIO::Helpers::AggregateGroupData { ih.value };

    BOOST_CHECK_EQUAL(agrpd.getIGroup().size(), ih.ngmaxz * ih.nigrpz);
    BOOST_CHECK_EQUAL(agrpd.getSGroup().size(), ih.ngmaxz * ih.nsgrpz);
    BOOST_CHECK_EQUAL(agrpd.getXGroup().size(), ih.ngmaxz * ih.nxgrpz);
    BOOST_CHECK_EQUAL(agrpd.getZGroup().size(), ih.ngmaxz * ih.nzgrpz);
}

BOOST_AUTO_TEST_CASE (Declared_Group_Data)
{
    const auto simCase = SimulationCase {first_sim()};

    // Report Step 1: 2115-01-01 --> 2015-01-05
    const auto rptStep = std::size_t {1};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz {4});

    const auto smry = sim_state();

    auto agrpd = Opm::RestartIO::Helpers::AggregateGroupData {ih.value};
    agrpd.captureDeclaredGroupData(simCase.sched, simCase.es.tracer(), rptStep, smry, ih.value);

    // IGRP (PROD)
    {
        auto start = 0*ih.nigrpz;

        const auto& iGrp = agrpd.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  2); // Group GRP1 - Child group number one - equal to group WGRP1 (no 2)
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  3); // Group GRP1 - Child group number two - equal to group WGRP2 (no 3)
        BOOST_CHECK_EQUAL(iGrp[start + 4] ,  2); // Group GRP1 - No of child groups
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group GRP1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group GRP1 - Group level (FIELD level is 0)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  5); // Group GRP1 - index of parent group (= 0 for FIELD)

        start = 1*ih.nigrpz;
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group WGRP1 - Child well number one - equal to well PROD1 (no 1)
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  3); // Group WGRP1 - Child well number two - equal to well WINJ1 (no 3)
        BOOST_CHECK_EQUAL(iGrp[start + 4] ,  2); // Group WGRP1 - No of child wells
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group WGRP1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  2); // Group WGRP1 - Group level (FIELD level is 0)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  1); // Group GRP1 - index of parent group (= 0 for FIELD)

        start = (ih.ngmaxz-1)*ih.nigrpz;
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one - equal to group GRP1
        BOOST_CHECK_EQUAL(iGrp[start + 4] ,  1); // Group FIELD - No of child groups
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group FIELD - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group FIELD - Group level (FIELD level is 0)
    }

    // SGRP
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SGroup::prod_index;
        auto start = 1*ih.nsgrpz;

        const auto& sGrp = agrpd.getSGroup();
        BOOST_CHECK_CLOSE(sGrp[start + Ix::GLOMaxSupply], 100, 1e-6);
        BOOST_CHECK_CLOSE(sGrp[start + Ix::GLOMaxRate],   200, 1e-6);
    }

    // XGRP (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XGroup::index;
        auto start = 0*ih.nxgrpz;

        const auto& xGrp = agrpd.getXGroup();
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  235.); // Group GRP1 - GOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  239.); // Group GRP1 - GWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  100237.); // Group GRP1 - GGPR

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilPrGuideRate], 345.6, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::OilPrGuideRate],
                          xGrp[start + Ix::OilPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatPrGuideRate], 456.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatPrGuideRate],
                          xGrp[start + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasPrGuideRate], 567.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::GasPrGuideRate],
                          xGrp[start + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::VoidPrGuideRate], 678.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::VoidPrGuideRate],
                          xGrp[start + Ix::VoidPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilInjGuideRate], 0.123, 1.0e-10);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatInjGuideRate], 1234.5, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatInjGuideRate],
                          xGrp[start + Ix::WatInjGuideRate_2]);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasInjGuideRate], 2345.6, 1.0e-10);

        start = 1*ih.nxgrpz;
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  23.); // Group WGRP1 - GOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  29.); // Group WGRP1 - GWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  50237.); // Group WGRP1 - GGPR

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilPrGuideRate], 456.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::OilPrGuideRate],
                          xGrp[start + Ix::OilPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatPrGuideRate], 567.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatPrGuideRate],
                          xGrp[start + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasPrGuideRate], 678.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::GasPrGuideRate],
                          xGrp[start + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::VoidPrGuideRate], 789.1, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::VoidPrGuideRate],
                          xGrp[start + Ix::VoidPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilInjGuideRate], 1.23, 1.0e-10);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatInjGuideRate], 2345.6, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatInjGuideRate],
                          xGrp[start + Ix::WatInjGuideRate_2]);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasInjGuideRate], 3456.7, 1.0e-10);

        start = 2*ih.nxgrpz;
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  43.); // Group WGRP2 - GOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  59.); // Group WGRP2 - GWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  70237.); // Group WGRP2 - GGPR


        start = (ih.ngmaxz-1)*ih.nxgrpz;
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  3456.); // Group FIELD - FOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  5678.); // Group FIELD - FWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  2003456.); // Group FIELD - FGPR
    }

    // ZGRP (PROD)
    {
        auto start = 0*ih.nzgrpz;

        const auto& zGrp = agrpd.getZGroup();
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "GRP1    "); // Group GRP1 - GOPR

        start = 1*ih.nzgrpz;
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "WGRP1   "); // Group WGRP1 - GOPR

        start = 2*ih.nzgrpz;
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "WGRP2   "); // Group WGRP2 - GOPR

        start = (ih.ngmaxz-1)*ih.nzgrpz;
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "FIELD   "); // Group FIELD - FOPR
    }
}

// \todo Restore checks for IGRP[NWGMAX + 17]
BOOST_AUTO_TEST_CASE (Declared_Group_Data_2)
{
    namespace VI = ::Opm::RestartIO::Helpers::VectorItems;
    const auto simCase = SimulationCase {second_sim("MOD4_TEST_IGRP-DATA.DATA")};

    // Report Step 1:
    const auto rptStep = std::size_t {1};
    double secs_elapsed = 3.1536E07;

    const auto& es    = simCase.es;
    const auto& sched = simCase.sched;
    const auto& grid  = simCase.grid;

    const auto st = sim_state_2();

    const auto ih = Opm::RestartIO::Helpers::createInteHead(es, grid, sched, secs_elapsed,
                                                            rptStep, rptStep + 1, rptStep);

    auto agrpd = Opm::RestartIO::Helpers::AggregateGroupData(ih);
    agrpd.captureDeclaredGroupData(sched, es.tracer(), rptStep, st, ih);

    // IGRP (PROD)
    {
        auto start = 0*ih[VI::intehead::NIGRPZ];
        auto nwgmax = ih[VI::NWGMAX];

        const auto& iGrp = agrpd.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   2); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   0); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 39] ,   3); // groups sequence number in the external networt defined

        start = 1*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   0); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,  -1); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 39] ,   2); // groups sequence number in the external networt defined

        start = 2*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   2); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   2); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 39] ,   1); // groups sequence number in the external networt defined

        start = 3*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,  -1); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   1); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control

        start = 4*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   1); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   1); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control

        start = 5*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   1); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   2); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control

        start = 6*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   1); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,  -1); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control

        start = 7*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   1); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   2); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control

        start = 8*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   1); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   1); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,  -1); // group available for higher level gas injection control

        start = 9*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   0); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   0); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,   0); // group available for higher level gas injection control

        start = 10*ih[VI::intehead::NIGRPZ];
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax +  5] ,   0); // group available for higher level production control
        // BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 17] ,   0); // group available for higher level water injection control
        BOOST_CHECK_EQUAL(iGrp[start + nwgmax + 22] ,   0); // group available for higher level gas injection control
    }
}

BOOST_AUTO_TEST_CASE (GasLiftOtimisation)
{
    // Abridged, and amended, copy of opm-tests/model5/4_GLIFT_MODEL5.DATA
    const auto cse = SimulationCase { R"(
RUNSPEC

DIMENS
 20 30 10 /


OIL
WATER
GAS
DISGAS

METRIC

START
 01 'JAN' 2020 /

EQLDIMS
 1  100  25 /

TABDIMS
/

WELLDIMS
--max.well  max.con/well  max.grup  max.w/grup
 10         15            9          10   /

--FLOW   THP  WCT  GCT  ALQ  VFP
VFPPDIMS
  22     13   10   13    13   50  /

UNIFIN
UNIFOUT

GRID

DXV
 20*100
/

DYV
 30*100
/

DZV
 10*2
/

TOPS
  600*2000 /

PORO
 6000*0.28 /

PERMX
 6000*10000.0 /

PERMZ
 6000*1000.0 /

COPY
  PERMX PERMY /
/

PROPS

REGIONS

SOLUTION

EQUIL
-- Datum    P     woc     Pc   goc    Pc  Rsvd  Rvvd
 2000.00  195.0  2070     0.0  500.00  0.0   1   0   0 /

PBVD
  2000.00    75.00
  2150.00    75.00  /

------------------------------------------------------------------------------------------------
SCHEDULE
------------------------------------------------------------------------------------------------

--
--                                       FIELD
--                                         |
--                                       PLAT-A
--                          ---------------+---------------------
--                         |                                    |
--                        M5S                                  M5N
--                ---------+----------                     -----+-------
--               |                   |                    |            |
--              B1                  G1                   C1           F1
--           ----+------          ---+---              ---+---       ---+---
--          |    |     |         |      |             |      |      |      |
--        B-1H  B-2H  B-3H     G-3H    G-4H         C-1H   C-2H    F-1H   F-2H
--

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

WELSPECS
--WELL     GROUP  IHEEL JHEEL   DREF PHASE   DRAD INFEQ SIINS XFLOW PRTAB  DENS
 'B-1H'  'B1'     11    3      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'B-2H'  'B1'      4    7      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'B-3H'  'B1'     11   12      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'C-1H'  'C1'     13   20      1*   OIL     1*   1*   SHUT 1* 1* 1* /
 'C-2H'  'C1'     12   27      1*   OIL     1*   1*   SHUT 1* 1* 1* /
/

WELSPECS
 'F-1H'  'F1'   19    4      1*   WATER   1*   1*   SHUT 1* 1* 1* /
 'F-2H'  'F1'   19   12      1*   WATER   1*   1*   SHUT 1* 1* 1* /
 'G-3H'  'G1'   19   21      1*   WATER   1*   1*   SHUT 1* 1* 1* /
 'G-4H'  'G1'   19   25      1*   WATER   1*   1*   SHUT 1* 1* 1* /
/

COMPDAT
--WELL      I   J    K1   K2 OP/SH  SATN    TRAN    WBDIA    KH     SKIN DFACT   DIR    PEQVR
 'B-1H'    11   3    1    5   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'B-2H'     4   7    1    6   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'B-3H'    11  12    1    4   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'C-1H'    13  20    1    4   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'C-2H'    12  27    1    5   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
/

COMPDAT
 'F-1H'    19   4    6   10   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'F-2H'    19  12    6   10   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'G-3H'    19  21    6   10   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
 'G-4H'    19  25    6   10   OPEN    1*      1*    0.216    1*        0    1*    Z       1* /
/

WCONPROD
--  Well_name  Status  Ctrl  Orate   Wrate  Grate Lrate   RFV  FBHP   THP  VFP Glift
   'B-1H'      OPEN    ORAT  4000.0  1*     1*    6000.0  1*   100.0  1*   0   1*  /
   'B-2H'      SHUT    ORAT  4000.0  1*     1*    6000.0  1*   100.0  1*   0   1*  /
   'B-3H'      OPEN    ORAT  4000.0  1*     1*    6000.0  1*   100.0  1*   0   1*  /
   'C-1H'      OPEN    ORAT  4000.0  1*     1*    6000.0  1*   100.0  1*   0   1*  /
   'C-2H'      SHUT    ORAT  4000.0  1*     1*    6000.0  1*   100.0  1*   0   1*  /
/

GCONINJE
 'FIELD'   'WATER'    'VREP'  3*      1.020    'NO'  5* /
/

GCONPROD
 'PLAT-A' ORAT 10000 /
/

WCONINJE
-- Well_name    Type    Status  Ctrl    SRate1  Rrate   BHP     THP     VFP
  'F-1H'        WATER   OPEN    GRUP    4000    1*      225.0    1*      1*     /
  'F-2H'        WATER   OPEN    GRUP    4000    1*      225.0    1*      1*     /
  'G-3H'        WATER   OPEN    GRUP    4000    1*      225.0    1*      1*     /
  'G-4H'        WATER   OPEN    GRUP    4000    1*      225.0    1*      1*     /
/

-- Turns on gas lift optimization
LIFTOPT
 12500 5E-3 0.0 YES /

-- Group lift gas limits for gas lift optimization
GLIFTOPT
 'PLAT-A' 12345 /  --
 'M5S' 1* 12345 /
 'M5N' -1.0 0.0 /
 'B1' 0.0 1.0E-20 /
 'G1' -12.345 0.99E-20 /
 'C1' 1.0E-8 1.0E-8 /
/

GCONPROD
 'PLAT-A' ORAT 10000 /
/

DATES
 1 FEB 2020 /
 1 MAR 2020 /
 1 APR 2020 /
/

END
)" };

    const auto rptStep = std::size_t {1};
    double secs_elapsed = 3.1536E07;

    const auto& es    = cse.es;
    const auto& sched = cse.sched;
    const auto& grid  = cse.grid;

    const auto st = sim_state_3();

    const auto ih = Opm::RestartIO::Helpers::createInteHead(es, grid, sched, secs_elapsed,
                                                            rptStep, rptStep + 1, rptStep);

    auto agrpd = Opm::RestartIO::Helpers::AggregateGroupData(ih);
    agrpd.captureDeclaredGroupData(sched, es.tracer(), rptStep, st, ih);

    const auto& sgrp = agrpd.getSGroup();
    const auto& zgrp = agrpd.getZGroup();

    namespace VI = ::Opm::RestartIO::Helpers::VectorItems;
    using namespace std::string_literals;

    using Ix = VI::SGroup::prod_index;

    auto requireGroup = [&zgrp, &ih](const int groupID, const std::string& name)
    {
        BOOST_REQUIRE_EQUAL(zgrp[groupID*ih[VI::intehead::NZGRPZ] + 0].c_str(), name);
    };

    auto sgrpValue = [&ih, &sgrp](const int groupID, const int item)
    {
        return sgrp[groupID*ih[VI::intehead::NSGRPZ] + item];
    };

    // PLAT-A
    {
        const auto groupID = 2;
        requireGroup(groupID, "PLAT-A  "s);

        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxSupply),  12345.0f, 1.0e-5f);
        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxRate)  , -   10.0f, 1.0e-5f); // Defaulted -> no limit
    }

    // M5S
    {
        const auto groupID = 1;
        requireGroup(groupID, "M5S     "s);

        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxSupply), -   10.0f, 1.0e-5f); // Defaulted -> no limit
        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxRate)  ,  12345.0f, 1.0e-5f);
    }

    // M5N
    {
        const auto groupID = 3;
        requireGroup(groupID, "M5N     "s);

        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxSupply), -   10.0f, 1.0e-5f); // Negative -> defaulted
        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxRate)  ,   1.0e-6f, 1.0e-5f); // 0.0 -> small
    }

    // B1
    {
        const auto groupID = 6;
        requireGroup(groupID, "B1      "s);

        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxSupply), 1.0e-6f, 1.0e-5f); // 0.0 -> small
        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxRate)  , 1.0e-20f, 1.0e-5f); // >= threshold -> preserve
    }

    // G1
    {
        const auto groupID = 7;
        requireGroup(groupID, "G1      "s);

        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxSupply), -10.0f, 1.0e-5f); // Negative -> defaulted
        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxRate)  ,  1.0e-6f, 1.0e-5f); // < threshold -> small
    }

    // C1
    {
        const auto groupID = 4;
        requireGroup(groupID, "C1      "s);

        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxSupply), 1.0e-8f, 1.0e-5f); // >= threshold -> preserve
        BOOST_CHECK_CLOSE(sgrpValue(groupID, Ix::GLOMaxRate)  , 1.0e-8f, 1.0e-5f); // >= threshold -> preserve
    }
}

BOOST_AUTO_TEST_SUITE_END()

// =====================================================================

BOOST_AUTO_TEST_SUITE(Tracer_Values)

namespace {
    // Note: We intentionally omit concentrations ([FG]T[IR]C*) here.
    // While very useful for engineers, those values don't appear in
    // the restart file to which this set of unit tests apply.
    //
    // This function sets up a dynamic state for group (and field) level
    // tracer related summary vectors.  We have the following tracers
    //
    //    TRACER
    //    SEA  WAT  /
    //    OCE  GAS  /
    //    OCF  GAS  /
    //    SEB  WAT  /
    //    /
    //
    // with or without temperature ("HEA") and the following group tree
    //
    //    'PLAT_A' FIELD
    //    INJE     'PLAT_A'
    //    PROD     'PLAT_A'
    //    'INJE-G' INJE
    //    'INJE-W' INJE
    //
    // Note as well that the synthetic values here don't make physical
    // sense and have no internal consistency; they are purely for
    // testing purposes and to be distinct at each level and quantity.
    Opm::SummaryState dynamicState()
    {
        auto ret = Opm::SummaryState { Opm::TimeService::now(), 0.0 };

        // Field level values
        {
            ret.update("FTIRSEA", 112233.44);
            ret.update("FTITSEA", 11223344.55);
            ret.update("FTPRSEA", 2233.44);
            ret.update("FTPTSEA", 223344.55);

            ret.update("FTIRSEB", 111222.333);
            ret.update("FTITSEB", 111222333.444555);
            ret.update("FTPRSEB", 222333.444);
            ret.update("FTPTSEB", 222333444.555);

            ret.update("FTIROCF", 111.222);
            ret.update("FTIRFOCF", 111.0);
            ret.update("FTIRSOCF", 11.202);
            ret.update("FTPROCF", 22.33);
            ret.update("FTPRFOCF", 22.0);
            ret.update("FTPRSOCF", 0.33);
            ret.update("FTPTOCF", 223.34455);
            ret.update("FTPTFOCF", 3.34455);
            ret.update("FTPTSOCF", 22.0);

            ret.update("FTIROCE", 11.1222);
            ret.update("FTIRFOCE", 11.11);
            ret.update("FTIRSOCE", 11.1212);
            ret.update("FTITOCE", 1.122);
            ret.update("FTITFOCE", 1.1);
            ret.update("FTITSOCE", 0.022);
            ret.update("FTPROCE", 2.233);
            ret.update("FTPRFOCE", 2.2);
            ret.update("FTPRSOCE", 2.033);
            ret.update("FTPTOCE", 2233.4455);
            ret.update("FTPTFOCE", 2200.44);
            ret.update("FTPTSOCE", 33.0055);

            ret.update("FTIRHEA", 11122.2);
            ret.update("FTITHEA", 0.1122);
            ret.update("FTPRHEA", 0.2233);
            ret.update("FTPTHEA", 22.334455);
        }

        // PLAT_A group level values
        {
            ret.update_group_var("PLAT_A", "GTIRSEA", 2000.0);
            ret.update_group_var("PLAT_A", "GTITSEA", 2200.0);
            ret.update_group_var("PLAT_A", "GTPRSEA", 2220.0);
            ret.update_group_var("PLAT_A", "GTPTSEA", 2222.0);

            ret.update_group_var("PLAT_A", "GTIRSEB", 2.0);
            ret.update_group_var("PLAT_A", "GTITSEB", 2.2);
            ret.update_group_var("PLAT_A", "GTPRSEB", 2.22);
            ret.update_group_var("PLAT_A", "GTPTSEB", 2.222);

            ret.update_group_var("PLAT_A", "GTIROCF", 22.0);
            ret.update_group_var("PLAT_A", "GTIRFOCF", 22.0);
            ret.update_group_var("PLAT_A", "GTITOCF", 22.2);
            ret.update_group_var("PLAT_A", "GTITFOCF", 20.02);
            ret.update_group_var("PLAT_A", "GTITSOCF", 2.24);
            ret.update_group_var("PLAT_A", "GTPROCF", 22.22);
            ret.update_group_var("PLAT_A", "GTPRFOCF", 22.02);
            ret.update_group_var("PLAT_A", "GTPRSOCF", 0.2);
            ret.update_group_var("PLAT_A", "GTPTOCF", 22.222);
            ret.update_group_var("PLAT_A", "GTPTFOCF", 22.202);
            ret.update_group_var("PLAT_A", "GTPTSOCF", 24.02);

            ret.update_group_var("PLAT_A", "GTIROCE", 222.0);
            ret.update_group_var("PLAT_A", "GTIRSOCE", 202.02);
            ret.update_group_var("PLAT_A", "GTITOCE", 222.2);
            ret.update_group_var("PLAT_A", "GTITFOCE", 200.002);
            ret.update_group_var("PLAT_A", "GTITSOCE", 2.202);
            ret.update_group_var("PLAT_A", "GTPROCE", 222.22);
            ret.update_group_var("PLAT_A", "GTPRFOCE", 202.0);
            ret.update_group_var("PLAT_A", "GTPRSOCE", 20.22);
            ret.update_group_var("PLAT_A", "GTPTOCE", 222.222);
            ret.update_group_var("PLAT_A", "GTPTFOCE", 222.202);
            ret.update_group_var("PLAT_A", "GTPTSOCE", 242.242);

            ret.update_group_var("PLAT_A", "GTIRHEA", 22222.0);
            ret.update_group_var("PLAT_A", "GTITHEA", 22222.2);
            ret.update_group_var("PLAT_A", "GTPRHEA", 22222.22);
            ret.update_group_var("PLAT_A", "GTPTHEA", 22222.222);
        }

        // INJE group level values
        {
            ret.update_group_var("INJE", "GTIRSEA", 3000.0);
            ret.update_group_var("INJE", "GTITSEA", 3300.0);
            ret.update_group_var("INJE", "GTPRSEA", 3330.0);
            ret.update_group_var("INJE", "GTPTSEA", 3333.0);

            ret.update_group_var("INJE", "GTIRSEB", 3.0);
            ret.update_group_var("INJE", "GTITSEB", 3.3);
            ret.update_group_var("INJE", "GTPRSEB", 3.33);
            ret.update_group_var("INJE", "GTPTSEB", 3.333);

            ret.update_group_var("INJE", "GTIROCF", 33.0);
            ret.update_group_var("INJE", "GTIRFOCF", 3.3);
            ret.update_group_var("INJE", "GTIRSOCF", 30.7);
            ret.update_group_var("INJE", "GTITOCF", 33.3);
            ret.update_group_var("INJE", "GTITFOCF", 33.303);
            ret.update_group_var("INJE", "GTPROCF", 33.33);
            ret.update_group_var("INJE", "GTPRFOCF", 30.03);
            ret.update_group_var("INJE", "GTPRSOCF", 43.34);
            ret.update_group_var("INJE", "GTPTOCF", 33.333);
            ret.update_group_var("INJE", "GTPTFOCF", 33.003);
            ret.update_group_var("INJE", "GTPTSOCF", 0.33);

            ret.update_group_var("INJE", "GTIROCE", 333.0);
            ret.update_group_var("INJE", "GTIRFOCE", 303.0);
            ret.update_group_var("INJE", "GTIRSOCE", 30.3);
            ret.update_group_var("INJE", "GTITOCE", 333.3);
            ret.update_group_var("INJE", "GTITFOCE", 343.43);
            ret.update_group_var("INJE", "GTITSOCE", 334.3);
            ret.update_group_var("INJE", "GTPROCE", 333.33);
            ret.update_group_var("INJE", "GTPRSOCE", 303.03);
            ret.update_group_var("INJE", "GTPTOCE", 333.333);
            ret.update_group_var("INJE", "GTPTFOCE", 334.334);
            ret.update_group_var("INJE", "GTPTSOCE", 343.5);

            ret.update_group_var("INJE", "GTIRHEA", 33333.0);
            ret.update_group_var("INJE", "GTITHEA", 33333.3);
            ret.update_group_var("INJE", "GTPRHEA", 33333.33);
            ret.update_group_var("INJE", "GTPTHEA", 33333.333);
        }

        // PROD group level values
        {
            ret.update_group_var("PROD", "GTIRSEA", 4000.0);
            ret.update_group_var("PROD", "GTITSEA", 4400.0);
            ret.update_group_var("PROD", "GTPRSEA", 4440.0);
            ret.update_group_var("PROD", "GTPTSEA", 4444.0);

            ret.update_group_var("PROD", "GTIRSEB", 4.0);
            ret.update_group_var("PROD", "GTITSEB", 4.4);
            ret.update_group_var("PROD", "GTPRSEB", 4.44);
            ret.update_group_var("PROD", "GTPTSEB", 4.444);

            ret.update_group_var("PROD", "GTIROCF", 44.0);
            ret.update_group_var("PROD", "GTIRFOCF", 40.0);
            ret.update_group_var("PROD", "GTIRSOCF", 44.4);
            ret.update_group_var("PROD", "GTITOCF", 44.4);
            ret.update_group_var("PROD", "GTITFOCF", 44.44);
            ret.update_group_var("PROD", "GTITSOCF", 45.54);
            ret.update_group_var("PROD", "GTPROCF", 44.44);
            ret.update_group_var("PROD", "GTPRFOCF", 40.04);
            ret.update_group_var("PROD", "GTPRSOCF", 44.56);
            ret.update_group_var("PROD", "GTPTOCF", 44.444);
            ret.update_group_var("PROD", "GTPTFOCF", 44.434);
            ret.update_group_var("PROD", "GTPTSOCF", 44.454);

            ret.update_group_var("PROD", "GTIROCE", 444.0);
            ret.update_group_var("PROD", "GTIRFOCE", 404.0);
            ret.update_group_var("PROD", "GTIRSOCE", 440.4);
            ret.update_group_var("PROD", "GTITOCE", 444.4);
            ret.update_group_var("PROD", "GTITFOCE", 404.505);
            ret.update_group_var("PROD", "GTITSOCE", 444.4567);
            ret.update_group_var("PROD", "GTPROCE", 444.44);
            ret.update_group_var("PROD", "GTPRFOCE", 444.54);
            ret.update_group_var("PROD", "GTPRSOCE", 404.04);
            ret.update_group_var("PROD", "GTPTOCE", 444.444);
            ret.update_group_var("PROD", "GTPTFOCE", 454.454);
            ret.update_group_var("PROD", "GTPTSOCE", 434.434);

            ret.update_group_var("PROD", "GTIRHEA", 44444.0);
            ret.update_group_var("PROD", "GTITHEA", 44444.4);
            ret.update_group_var("PROD", "GTPRHEA", 44444.44);
            ret.update_group_var("PROD", "GTPTHEA", 44444.444);
        }

        // INJE-G group level values
        {
            ret.update_group_var("INJE-G", "GTIRSEA", 5000.0);
            ret.update_group_var("INJE-G", "GTITSEA", 5500.0);
            ret.update_group_var("INJE-G", "GTPRSEA", 5550.0);
            ret.update_group_var("INJE-G", "GTPTSEA", 5555.0);

            ret.update_group_var("INJE-G", "GTIRSEB", 5.0);
            ret.update_group_var("INJE-G", "GTITSEB", 5.5);
            ret.update_group_var("INJE-G", "GTPRSEB", 5.55);
            ret.update_group_var("INJE-G", "GTPTSEB", 5.555);

            ret.update_group_var("INJE-G", "GTIROCF", 55.0);
            ret.update_group_var("INJE-G", "GTIRFOCF", 55.055);
            ret.update_group_var("INJE-G", "GTIRSOCF", 55.55055);
            ret.update_group_var("INJE-G", "GTITOCF", 55.5);
            ret.update_group_var("INJE-G", "GTITFOCF", 55.55);
            ret.update_group_var("INJE-G", "GTITSOCF", 55.678);
            ret.update_group_var("INJE-G", "GTPROCF", 55.55);
            ret.update_group_var("INJE-G", "GTPRFOCF", 50.05);
            ret.update_group_var("INJE-G", "GTPRSOCF", 56.65);
            ret.update_group_var("INJE-G", "GTPTOCF", 55.555);
            ret.update_group_var("INJE-G", "GTPTFOCF", 55.585);
            ret.update_group_var("INJE-G", "GTPTSOCF", 55.575);

            ret.update_group_var("INJE-G", "GTIROCE", 555.0);
            ret.update_group_var("INJE-G", "GTIRFOCE", 505.05);
            ret.update_group_var("INJE-G", "GTITOCE", 555.5);
            ret.update_group_var("INJE-G", "GTITFOCE", 555.555);
            ret.update_group_var("INJE-G", "GTITSOCE", 565.565);
            ret.update_group_var("INJE-G", "GTPROCE", 555.55);
            ret.update_group_var("INJE-G", "GTPRFOCE", 555.505);
            ret.update_group_var("INJE-G", "GTPRSOCE", 505.565);
            ret.update_group_var("INJE-G", "GTPTOCE", 555.555);
            ret.update_group_var("INJE-G", "GTPTFOCE", 565.5656);
            ret.update_group_var("INJE-G", "GTPTSOCE", 545.5454);

            ret.update_group_var("INJE-G", "GTIRHEA", 55555.0);
            ret.update_group_var("INJE-G", "GTITHEA", 55555.5);
            ret.update_group_var("INJE-G", "GTPRHEA", 55555.55);
            ret.update_group_var("INJE-G", "GTPTHEA", 55555.555);
        }

        // INJE-W group level values
        {
            ret.update_group_var("INJE-W", "GTIRSEA", 6000.0);
            ret.update_group_var("INJE-W", "GTITSEA", 6600.0);
            ret.update_group_var("INJE-W", "GTPRSEA", 6660.0);
            ret.update_group_var("INJE-W", "GTPTSEA", 6666.0);

            ret.update_group_var("INJE-W", "GTIRSEB", 6.0);
            ret.update_group_var("INJE-W", "GTITSEB", 6.6);
            ret.update_group_var("INJE-W", "GTPRSEB", 6.66);
            ret.update_group_var("INJE-W", "GTPTSEB", 6.666);

            ret.update_group_var("INJE-W", "GTIROCF", 66.0);
            ret.update_group_var("INJE-W", "GTIRSOCF", 66.33);
            ret.update_group_var("INJE-W", "GTITOCF", 66.6);
            ret.update_group_var("INJE-W", "GTITFOCF", 66.678);
            ret.update_group_var("INJE-W", "GTITSOCF", 66.9876);
            ret.update_group_var("INJE-W", "GTPROCF", 66.66);
            ret.update_group_var("INJE-W", "GTPTOCF", 66.666);
            ret.update_group_var("INJE-W", "GTPTFOCF", 66.006);
            ret.update_group_var("INJE-W", "GTPTSOCF", 66.007);

            ret.update_group_var("INJE-W", "GTIROCE", 666.0);
            ret.update_group_var("INJE-W", "GTIRFOCE", 606.06);
            ret.update_group_var("INJE-W", "GTIRSOCE", 660.066);
            ret.update_group_var("INJE-W", "GTITOCE", 666.6);
            ret.update_group_var("INJE-W", "GTITFOCE", 666.5678);
            ret.update_group_var("INJE-W", "GTITSOCE", 666.9876);
            ret.update_group_var("INJE-W", "GTPROCE", 666.66);
            ret.update_group_var("INJE-W", "GTPRFOCE", 666.666);
            ret.update_group_var("INJE-W", "GTPRSOCE", 666.066);
            ret.update_group_var("INJE-W", "GTPTOCE", 666.666);
            ret.update_group_var("INJE-W", "GTPTFOCE", 666.006);
            ret.update_group_var("INJE-W", "GTPTSOCE", 666.007);

            ret.update_group_var("INJE-W", "GTIRHEA", 66666.0);
            ret.update_group_var("INJE-W", "GTITHEA", 66666.6);
            ret.update_group_var("INJE-W", "GTPRHEA", 66666.66);
            ret.update_group_var("INJE-W", "GTPTHEA", 66666.666);
        }

        return ret;
    }

    namespace VI = Opm::RestartIO::Helpers::VectorItems;

    std::tuple<int, int, double> timePoint(const SimulationCase& cse)
    {
        constexpr auto report_step = 7;
        constexpr auto sim_step = report_step - 1;
        const auto simTime = cse.sched.seconds(report_step);

        BOOST_CHECK_CLOSE(simTime, (1 + 2 + 3 + 4 + 5 + 10 + 15) * 86'400.0, 1.0e-7);

        return {report_step, sim_step, simTime};
    }

    std::vector<int> intehead(const SimulationCase& cse)
    {
        const auto [report_step, sim_step, simTime] = timePoint(cse);

        return Opm::RestartIO::Helpers::createInteHead
            (cse.es, cse.grid, cse.sched, simTime,
             report_step, // Should really be number of timesteps
             report_step, sim_step);
    }

    Opm::RestartIO::Helpers::AggregateGroupData
    groupData(const SimulationCase&    cse,
              const std::vector<int>&  ih,
              const Opm::SummaryState& smry)
    {
        auto gd = Opm::RestartIO::Helpers::AggregateGroupData{ih};

        const auto tp = timePoint(cse);
        const auto sim_step = std::get<1>(tp);

        gd.captureDeclaredGroupData(cse.sched, cse.es.tracer(),
                                    sim_step, smry, ih);

        return gd;
    }
}

BOOST_AUTO_TEST_SUITE(Isothermal)

namespace {
    SimulationCase defineCase()
    {
        return SimulationCase { R"(RUNSPEC
DIMENS
  5 5 2 /
OIL
GAS
WATER
DISGAS
TABDIMS
/
WELLDIMS
  3 2 5 3 / -- Item 3 (NGMAX) must be at least 5 for this test.
EQLDIMS
  3 1 1 /
TRACERS
--  oil  water  gas  env
     1*  3      2    1*   /
GRID
DXV
  5*100 /
DYV
  5*100 /
DZV
  5 10 /
DEPTHZ
  36*2000.0 /
EQUALS
  PORO 0.25 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  852.6 1014.3 0.83 /
TRACER
SEA  WAT  /
OCE  GAS  /
OCF  GAS  /
SEB  WAT  /
/
TVDPFSEA
1000   0.0
5000   0.0 /
TVDPFSEB
1000   0.0
5000   0.0 /
TBLKFOCE
1.0 2.0 3.0 /
TBLKFOCF
1.0 2.0 3.0 /
SCHEDULE
GRUPTREE
  'PLAT_A' FIELD /
  INJE     'PLAT_A' /
  PROD     'PLAT_A' /
  'INJE-G' INJE /
  'INJE-W' INJE /
/
WELSPECS
  IW 'INJE-W' 1 1 2010.0 'WATER' /
  IG 'INJE-G' 1 5 2010.0 'GAS' /
  P  'PROD'   5 3 2002.5 'LIQ' /
/
COMPDAT
  IW 1 1 2 2 'OPEN' 2* 0.5 /
  IG 1 5 1 1 'OPEN' 2* 0.5 /
  P  5 3 1 2 'OPEN' 2* 0.5 /
/
WTRACER
  IW SEA 0.123 /
  IG OCE 0.456 /
/
WCONINJE
  IW 'WATER' 'OPEN' RATE 12345.6 1* 512.256 /
  IG 'GAS' 'OPEN' RATE 678910.1112 1* 256.128 /
/
WCONPROD
  P 'OPEN' LRAT 3* 100E3 1* 25.125 /
/
TSTEP
  1 2 3 4 5 10 15 4*20 /
END
)"
        };
    }

    // 3 Water + 2 Gas*(free + solution)
    constexpr auto tracerStride = std::size_t{7};
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(InteHEAD_Allocation_Sizes)
{
    const auto cse = defineCase();
    const auto ih  = intehead(cse);

    const auto expectNumTracers = [&trcCount = cse.es.runspec().tracers()]()
    {
        return trcCount.water_tracers()
             + trcCount.oil_tracers()
             + trcCount.gas_tracers();
    }();

    BOOST_CHECK_EQUAL(expectNumTracers, 5);

    const auto expectNumTrcElems = [&trcCount = cse.es.runspec().tracers()]()
    {
        return trcCount.water_tracers()
             + 0*trcCount.oil_tracers()
             + 2*trcCount.gas_tracers();
    }();

    BOOST_CHECK_EQUAL(expectNumTrcElems, static_cast<int>(tracerStride));

    BOOST_CHECK_EQUAL(ih[VI::intehead::NGRP], 5);
    BOOST_CHECK_EQUAL(ih[VI::intehead::NWGMAX], 5);
    BOOST_CHECK_EQUAL(ih[VI::intehead::NGMAXZ], 6);
    BOOST_CHECK_EQUAL(ih[VI::intehead::NXGRPZ], static_cast<int>(VI::XGroup::TracerOffset) + 4*expectNumTrcElems);
}

BOOST_AUTO_TEST_CASE(Field)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    // FIELD group is *last* in XGRP.
    const auto grpID = ih[VI::intehead::NGMAXZ] - 1;

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto fti_rate = xgrptrc.subspan(0*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(fti_rate[0], 112233.44, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(fti_rate[1], 11.11, 1.0e-6);      // FOCE
    BOOST_CHECK_CLOSE(fti_rate[2], 11.1212, 1.0e-6);    // SOCE
    BOOST_CHECK_CLOSE(fti_rate[3], 111.0, 1.0e-6);      // FOCF
    BOOST_CHECK_CLOSE(fti_rate[4], 11.202, 1.0e-6);     // SOCF
    BOOST_CHECK_CLOSE(fti_rate[5], 111222.333, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(fti_rate[6], 0.0, 1.0e-6);        // Unused

    // Production rates
    const auto ftp_rate = xgrptrc.subspan(1*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(ftp_rate[0], 2233.44, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(ftp_rate[1], 2.2, 1.0e-6);        // FOCE
    BOOST_CHECK_CLOSE(ftp_rate[2], 2.033, 1.0e-6);      // SOCE
    BOOST_CHECK_CLOSE(ftp_rate[3], 22.0, 1.0e-6);       // FOCF
    BOOST_CHECK_CLOSE(ftp_rate[4], 0.33, 1.0e-6);       // SOCF
    BOOST_CHECK_CLOSE(ftp_rate[5], 222333.444, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(ftp_rate[6], 0.0, 1.0e-6);        // Unused

    // Total injected volume
    const auto fti_vol = xgrptrc.subspan(2*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(fti_vol[0], 11223344.55, 1.0e-6);      // SEA
    BOOST_CHECK_CLOSE(fti_vol[1], 1.1, 1.0e-6);              // FOCE
    BOOST_CHECK_CLOSE(fti_vol[2], 0.022, 1.0e-6);            // SOCE
    BOOST_CHECK_CLOSE(fti_vol[3], 0.0, 1.0e-6);              // FOCF
    BOOST_CHECK_CLOSE(fti_vol[4], 0.0, 1.0e-6);              // SOCF
    BOOST_CHECK_CLOSE(fti_vol[5], 111222333.444555, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(fti_vol[6], 0.0, 1.0e-6);              // Unused

    // Total produced volume
    const auto ftp_vol = xgrptrc.subspan(3*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(ftp_vol[0], 223344.55, 1.0e-6);     // SEA
    BOOST_CHECK_CLOSE(ftp_vol[1], 2200.44, 1.0e-6);       // FOCE
    BOOST_CHECK_CLOSE(ftp_vol[2], 33.0055, 1.0e-6);       // SOCE
    BOOST_CHECK_CLOSE(ftp_vol[3], 3.34455, 1.0e-6);       // FOCF
    BOOST_CHECK_CLOSE(ftp_vol[4], 22.0, 1.0e-6);          // SOCF
    BOOST_CHECK_CLOSE(ftp_vol[5], 222333444.555, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(ftp_vol[6], 0.0, 1.0e-6);           // Unused
}

BOOST_AUTO_TEST_CASE(Plat_A)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 0;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "PLAT_A  ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 2000.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gti_rates[1], 0.0, 1.0e-6);    // FOCE
    BOOST_CHECK_CLOSE(gti_rates[2], 202.02, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 22.0, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_rates[4], 0.0, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 2.0, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gti_rates[6], 0.0, 1.0e-6);    // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 2220.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 202.0, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[2], 20.22, 1.0e-6);  // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 22.02, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[4], 0.2, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 2.22, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_rates[6], 0.0, 1.0e-6);    // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 2200.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_vol[1], 200.002, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[2], 2.202, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 20.02, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_vol[4], 2.24, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 2.2, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_vol[6], 0.0, 1.0e-6);     // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 2222.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 222.202, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[2], 242.242, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 22.202, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[4], 24.02, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 2.222, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_vol[6], 0.0, 1.0e-6);     // Unused
}

BOOST_AUTO_TEST_CASE(Inje)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 1;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "INJE    ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 3000.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gti_rates[1], 303, 1.0e-6);    // FOCE
    BOOST_CHECK_CLOSE(gti_rates[2], 30.3, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 3.3, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gti_rates[4], 30.7, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 3.0, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gti_rates[6], 0.0, 1.0e-6);    // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 3330.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 0.0, 1.0e-6);    // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[2], 303.03, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 30.03, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[4], 43.34, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 3.33, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_rates[6], 0.0, 1.0e-6);    // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 3300.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gti_vol[1], 343.43, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[2], 334.3, 1.0e-6);  // SOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 33.303, 1.0e-6); // FOCF
    BOOST_CHECK_CLOSE(gti_vol[4], 0.0, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 3.3, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gti_vol[6], 0.0, 1.0e-6);    // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 3333.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 334.334, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[2], 343.5, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 33.003, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[4], 0.33, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 3.333, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_vol[6], 0.0, 1.0e-6);     // Unused
}

BOOST_AUTO_TEST_CASE(Prod)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 2;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "PROD    ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 4000.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gti_rates[1], 404.0, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gti_rates[2], 440.4, 1.0e-6);  // SOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 40.0, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_rates[4], 44.4, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 4.0, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gti_rates[6], 0.0, 1.0e-6);    // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 4440.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 444.54, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[2], 404.04, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 40.04, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[4], 44.56, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 4.44, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_rates[6], 0.0, 1.0e-6);    // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 4400.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gti_vol[1], 404.505, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gti_vol[2], 444.4567, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 44.44, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gti_vol[4], 45.54, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 4.4, 1.0e-6);      // SEB
    BOOST_CHECK_CLOSE(gti_vol[6], 0.0, 1.0e-6);      // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 4444.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 454.454, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[2], 434.434, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 44.434, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[4], 44.454, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 4.444, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_vol[6], 0.0, 1.0e-6);     // Unused
}

BOOST_AUTO_TEST_CASE(Inje_G)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 3;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "INJE-G  ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 5000.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gti_rates[1], 505.05, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gti_rates[2], 0.0, 1.0e-6);      // SOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 55.055, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_rates[4], 55.55055, 1.0e-6); // SOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 5.0, 1.0e-6);      // SEB
    BOOST_CHECK_CLOSE(gti_rates[6], 0.0, 1.0e-6);      // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 5550.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 555.505, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[2], 505.565, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 50.05, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[4], 56.65, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 5.55, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gtp_rates[6], 0.0, 1.0e-6);     // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 5500.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_vol[1], 555.555, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[2], 565.565, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 55.55, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_vol[4], 55.678, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 5.5, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_vol[6], 0.0, 1.0e-6);     // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 5555.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 565.5656, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[2], 545.5454, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 55.585, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[4], 55.575, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 5.555, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gtp_vol[6], 0.0, 1.0e-6);      // Unused
}

BOOST_AUTO_TEST_CASE(Inje_W)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 4;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "INJE-W  ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates.
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 6000.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_rates[1], 606.06, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gti_rates[2], 660.066, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 0.0, 1.0e-6);     // FOCF
    BOOST_CHECK_CLOSE(gti_rates[4], 66.33, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 6.0, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_rates[6], 0.0, 1.0e-6);     // Unused

    // Production rates.
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 6660.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 666.666, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[2], 666.066, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 0.0, 1.0e-6);     // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[4], 0.0, 1.0e-6);     // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 6.66, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gtp_rates[6], 0.0, 1.0e-6);     // Unused

    // Total injected volumes.
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 6600.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gti_vol[1], 666.5678, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[2], 666.9876, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 66.678, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_vol[4], 66.9876, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 6.6, 1.0e-6);      // SEB
    BOOST_CHECK_CLOSE(gti_vol[6], 0.0, 1.0e-6);      // Unused

    // Total produced volumes.
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 6666.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 666.006, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[2], 666.007, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 66.006, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[4], 66.007, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 6.666, 1.0e-6);   // SEB
    BOOST_CHECK_CLOSE(gtp_vol[6], 0.0, 1.0e-6);     // Unused
}

BOOST_AUTO_TEST_SUITE_END() // Isothermal

// ---------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Thermal)

namespace {
    SimulationCase defineCase()
    {
        return SimulationCase { R"(RUNSPEC
DIMENS
  5 5 2 /
OIL
GAS
WATER
DISGAS
TEMP
TABDIMS
/
WELLDIMS
  3 2 5 3 / -- Item 3 (NGMAX) must be at least 5 for this test.
EQLDIMS
  3 1 1 /
TRACERS
--  oil  water  gas  env
     1*  3      2    1*   /
GRID
DXV
  5*100 /
DYV
  5*100 /
DZV
  5 10 /
DEPTHZ
  36*2000.0 /
EQUALS
  PORO 0.25 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  852.6 1014.3 0.83 /
TRACER
SEA  WAT  /
OCE  GAS  /
OCF  GAS  /
SEB  WAT  /
/
TVDPFSEA
1000   0.0
5000   0.0 /
TVDPFSEB
1000   0.0
5000   0.0 /
TBLKFOCE
1.0 2.0 3.0 /
TBLKFOCF
1.0 2.0 3.0 /
TEMPI
  50*42.0 /
SCHEDULE
GRUPTREE
  'PLAT_A' FIELD /
  INJE     'PLAT_A' /
  PROD     'PLAT_A' /
  'INJE-G' INJE /
  'INJE-W' INJE /
/
WELSPECS
  IW 'INJE-W' 1 1 2010.0 'WATER' /
  IG 'INJE-G' 1 5 2010.0 'GAS' /
  P  'PROD'   5 3 2002.5 'LIQ' /
/
COMPDAT
  IW 1 1 2 2 'OPEN' 2* 0.5 /
  IG 1 5 1 1 'OPEN' 2* 0.5 /
  P  5 3 1 2 'OPEN' 2* 0.5 /
/
WTRACER
  IW SEA 0.123 /
  IG OCE 0.456 /
/
WCONINJE
  IW 'WATER' 'OPEN' RATE 12345.6 1* 512.256 /
  IG 'GAS' 'OPEN' RATE 678910.1112 1* 256.128 /
/
WCONPROD
  P 'OPEN' LRAT 3* 100E3 1* 25.125 /
/
TSTEP
  1 2 3 4 5 10 15 4*20 /
END
)"
        };
    }

    // 3 Water + 2 Gas*(free + solution) + Temperature.
    constexpr auto tracerStride = std::size_t{8};
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(InteHEAD_Allocation_Sizes)
{
    const auto cse = defineCase();
    const auto ih  = intehead(cse);

    const auto expectNumTracers = [&trcCount = cse.es.runspec().tracers()]()
    {
        return trcCount.water_tracers()
             + trcCount.oil_tracers()
             + trcCount.gas_tracers()
             + 1; // Temperature
    }();

    BOOST_CHECK_EQUAL(expectNumTracers, 6);

    const auto expectNumTrcElems = [&trcCount = cse.es.runspec().tracers()]()
    {
        return trcCount.water_tracers()
             + 0*trcCount.oil_tracers()
             + 2*trcCount.gas_tracers()
             + 1; // Temperature
    }();

    BOOST_CHECK_EQUAL(expectNumTrcElems, static_cast<int>(tracerStride));

    BOOST_CHECK_EQUAL(ih[VI::intehead::NGRP], 5);
    BOOST_CHECK_EQUAL(ih[VI::intehead::NWGMAX], 5);
    BOOST_CHECK_EQUAL(ih[VI::intehead::NGMAXZ], 6);
    BOOST_CHECK_EQUAL(ih[VI::intehead::NXGRPZ], static_cast<int>(VI::XGroup::TracerOffset) + 4*expectNumTrcElems);
}

BOOST_AUTO_TEST_CASE(Field)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    // FIELD group is *last* in XGRP.
    const auto grpID = ih[VI::intehead::NGMAXZ] - 1;

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto fti_rate = xgrptrc.subspan(0*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(fti_rate[0], 11122.2, 1.0e-6);    // HEA
    BOOST_CHECK_CLOSE(fti_rate[1], 112233.44, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(fti_rate[2], 11.11, 1.0e-6);      // FOCE
    BOOST_CHECK_CLOSE(fti_rate[3], 11.1212, 1.0e-6);    // SOCE
    BOOST_CHECK_CLOSE(fti_rate[4], 111.0, 1.0e-6);      // FOCF
    BOOST_CHECK_CLOSE(fti_rate[5], 11.202, 1.0e-6);     // SOCF
    BOOST_CHECK_CLOSE(fti_rate[6], 111222.333, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(fti_rate[7], 0.0, 1.0e-6);        // Unused

    // Production rates
    const auto ftp_rate = xgrptrc.subspan(1*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(ftp_rate[0], 0.2233, 1.0e-6);     // HEA
    BOOST_CHECK_CLOSE(ftp_rate[1], 2233.44, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(ftp_rate[2], 2.2, 1.0e-6);        // FOCE
    BOOST_CHECK_CLOSE(ftp_rate[3], 2.033, 1.0e-6);      // SOCE
    BOOST_CHECK_CLOSE(ftp_rate[4], 22.0, 1.0e-6);       // FOCF
    BOOST_CHECK_CLOSE(ftp_rate[5], 0.33, 1.0e-6);       // SOCF
    BOOST_CHECK_CLOSE(ftp_rate[6], 222333.444, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(ftp_rate[7], 0.0, 1.0e-6);        // Unused

    // Total injected volume
    const auto fti_vol = xgrptrc.subspan(2*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(fti_vol[0], 0.1122, 1.0e-6);           // HEA
    BOOST_CHECK_CLOSE(fti_vol[1], 11223344.55, 1.0e-6);      // SEA
    BOOST_CHECK_CLOSE(fti_vol[2], 1.1, 1.0e-6);              // FOCE
    BOOST_CHECK_CLOSE(fti_vol[3], 0.022, 1.0e-6);            // SOCE
    BOOST_CHECK_CLOSE(fti_vol[4], 0.0, 1.0e-6);              // FOCF
    BOOST_CHECK_CLOSE(fti_vol[5], 0.0, 1.0e-6);              // SOCF
    BOOST_CHECK_CLOSE(fti_vol[6], 111222333.444555, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(fti_vol[7], 0.0, 1.0e-6);              // Unused

    // Total produced volume
    const auto ftp_vol = xgrptrc.subspan(3*tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(ftp_vol[0], 22.334455, 1.0e-6);     // HEA
    BOOST_CHECK_CLOSE(ftp_vol[1], 223344.55, 1.0e-6);     // SEA
    BOOST_CHECK_CLOSE(ftp_vol[2], 2200.44, 1.0e-6);       // FOCE
    BOOST_CHECK_CLOSE(ftp_vol[3], 33.0055, 1.0e-6);       // SOCE
    BOOST_CHECK_CLOSE(ftp_vol[4], 3.34455, 1.0e-6);       // FOCF
    BOOST_CHECK_CLOSE(ftp_vol[5], 22.0, 1.0e-6);          // SOCF
    BOOST_CHECK_CLOSE(ftp_vol[6], 222333444.555, 1.0e-6); // SEB
    BOOST_CHECK_CLOSE(ftp_vol[7], 0.0, 1.0e-6);           // Unused
}

BOOST_AUTO_TEST_CASE(Plat_A)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 0;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "PLAT_A  ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 22222.0, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_rates[1], 2000.0, 1.0e-6); // SEA
    BOOST_CHECK_CLOSE(gti_rates[2], 0.0, 1.0e-6);    // FOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 202.02, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_rates[4], 22.0, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 0.0, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_rates[6], 2.0, 1.0e-6);    // SEB
    BOOST_CHECK_CLOSE(gti_rates[7], 0.0, 1.0e-6);    // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 22222.22, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 2220.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gtp_rates[2], 202.0, 1.0e-6);    // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 20.22, 1.0e-6);    // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[4], 22.02, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 0.2, 1.0e-6);      // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[6], 2.22, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_rates[7], 0.0, 1.0e-6);      // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 22222.2, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_vol[1], 2200.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_vol[2], 200.002, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 2.202, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gti_vol[4], 20.02, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 2.24, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_vol[6], 2.2, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_vol[7], 0.0, 1.0e-6);     // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 22222.222, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 2222.0, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(gtp_vol[2], 222.202, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 242.242, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[4], 22.202, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 24.02, 1.0e-6);     // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[6], 2.222, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_vol[7], 0.0, 1.0e-6);       // Unused
}

BOOST_AUTO_TEST_CASE(Inje)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 1;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "INJE    ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 33333.0, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_rates[1], 3000.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_rates[2], 303, 1.0e-6);     // FOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 30.3, 1.0e-6);    // SOCE
    BOOST_CHECK_CLOSE(gti_rates[4], 3.3, 1.0e-6);     // FOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 30.7, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_rates[6], 3.0, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_rates[7], 0.0, 1.0e-6);     // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 33333.33, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 3330.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gtp_rates[2], 0.0, 1.0e-6);      // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 303.03, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[4], 30.03, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 43.34, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[6], 3.33, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_rates[7], 0.0, 1.0e-6);      // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 33333.3, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_vol[1], 3300.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_vol[2], 343.43, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 334.3, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gti_vol[4], 33.303, 1.0e-6);  // FOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 0.0, 1.0e-6);     // SOCF
    BOOST_CHECK_CLOSE(gti_vol[6], 3.3, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_vol[7], 0.0, 1.0e-6);     // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 33333.333, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 3333.0, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(gtp_vol[2], 334.334, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 343.5, 1.0e-6);     // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[4], 33.003, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 0.33, 1.0e-6);      // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[6], 3.333, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_vol[7], 0.0, 1.0e-6);       // Unused
}

BOOST_AUTO_TEST_CASE(Prod)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 2;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "PROD    ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 44444.0, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_rates[1], 4000.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_rates[2], 404.0, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 440.4, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gti_rates[4], 40.0, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 44.4, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_rates[6], 4.0, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_rates[7], 0.0, 1.0e-6);     // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 44444.44, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 4440.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gtp_rates[2], 444.54, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 404.04, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[4], 40.04, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 44.56, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[6], 4.44, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_rates[7], 0.0, 1.0e-6);      // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 44444.4, 1.0e-6);  // HEA
    BOOST_CHECK_CLOSE(gti_vol[1], 4400.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gti_vol[2], 404.505, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 444.4567, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_vol[4], 44.44, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 45.54, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gti_vol[6], 4.4, 1.0e-6);      // SEB
    BOOST_CHECK_CLOSE(gti_vol[7], 0.0, 1.0e-6);      // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 44444.444, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 4444.0, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(gtp_vol[2], 454.454, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 434.434, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[4], 44.434, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 44.454, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[6], 4.444, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_vol[7], 0.0, 1.0e-6);       // Unused
}

BOOST_AUTO_TEST_CASE(Inje_G)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 3;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "INJE-G  ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 55555.0, 1.0e-6);  // HEA
    BOOST_CHECK_CLOSE(gti_rates[1], 5000.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gti_rates[2], 505.05, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 0.0, 1.0e-6);      // SOCE
    BOOST_CHECK_CLOSE(gti_rates[4], 55.055, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 55.55055, 1.0e-6); // SOCF
    BOOST_CHECK_CLOSE(gti_rates[6], 5.0, 1.0e-6);      // SEB
    BOOST_CHECK_CLOSE(gti_rates[7], 0.0, 1.0e-6);      // Unused

    // Production rates
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 55555.55, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 5550.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gtp_rates[2], 555.505, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 505.565, 1.0e-6);  // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[4], 50.05, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 56.65, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[6], 5.55, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_rates[7], 0.0, 1.0e-6);      // Unused

    // Total injected volumes
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 55555.5, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_vol[1], 5500.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_vol[2], 555.555, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 565.565, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_vol[4], 55.55, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 55.678, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gti_vol[6], 5.5, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_vol[7], 0.0, 1.0e-6);     // Unused

    // Total produced volumes
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 55555.555, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 5555.0, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(gtp_vol[2], 565.5656, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 545.5454, 1.0e-6);  // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[4], 55.585, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 55.575, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[6], 5.555, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_vol[7], 0.0, 1.0e-6);       // Unused
}

BOOST_AUTO_TEST_CASE(Inje_W)
{
    const auto cse  = defineCase();
    const auto ih   = intehead(cse);
    const auto smry = dynamicState();

    const auto gd = groupData(cse, ih, smry);
    const auto xgsz = ih[VI::intehead::NXGRPZ];

    BOOST_REQUIRE_MESSAGE(static_cast<std::size_t>(xgsz) > VI::XGroup::TracerOffset,
                          "XGRP must allocate space for tracer concentrations");

    const auto grpID = 4;

    {
        const auto zgrp = std::span{gd.getZGroup()}
            .subspan(grpID * ih[VI::intehead::NZGRPZ],
                     ih[VI::intehead::NZGRPZ]);

        BOOST_CHECK_EQUAL(zgrp[0].c_str(), "INJE-W  ");
    }

    const auto xgrptrc = std::span {gd.getXGroup()}
        .subspan(grpID * xgsz, xgsz)
        .subspan(VI::XGroup::TracerOffset);

    BOOST_REQUIRE_EQUAL(xgrptrc.size(), 4 * tracerStride);

    // Injection rates.
    const auto gti_rates = xgrptrc.subspan(0 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_rates[0], 66666.0, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gti_rates[1], 6000.0, 1.0e-6);  // SEA
    BOOST_CHECK_CLOSE(gti_rates[2], 606.06, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gti_rates[3], 660.066, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_rates[4], 0.0, 1.0e-6);     // FOCF
    BOOST_CHECK_CLOSE(gti_rates[5], 66.33, 1.0e-6);   // SOCF
    BOOST_CHECK_CLOSE(gti_rates[6], 6.0, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gti_rates[7], 0.0, 1.0e-6);     // Unused

    // Production rates.
    const auto gtp_rates = xgrptrc.subspan(1 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_rates[0], 66666.66, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_rates[1], 6660.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gtp_rates[2], 666.666, 1.0e-6);  // FOCE
    BOOST_CHECK_CLOSE(gtp_rates[3], 666.066, 1.0e-6);  // SOCE
    BOOST_CHECK_CLOSE(gtp_rates[4], 0.0, 1.0e-6);      // FOCF
    BOOST_CHECK_CLOSE(gtp_rates[5], 0.0, 1.0e-6);      // SOCF
    BOOST_CHECK_CLOSE(gtp_rates[6], 6.66, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_rates[7], 0.0, 1.0e-6);      // Unused

    // Total injected volumes.
    const auto gti_vol = xgrptrc.subspan(2 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gti_vol[0], 66666.6, 1.0e-6);  // HEA
    BOOST_CHECK_CLOSE(gti_vol[1], 6600.0, 1.0e-6);   // SEA
    BOOST_CHECK_CLOSE(gti_vol[2], 666.5678, 1.0e-6); // FOCE
    BOOST_CHECK_CLOSE(gti_vol[3], 666.9876, 1.0e-6); // SOCE
    BOOST_CHECK_CLOSE(gti_vol[4], 66.678, 1.0e-6);   // FOCF
    BOOST_CHECK_CLOSE(gti_vol[5], 66.9876, 1.0e-6);  // SOCF
    BOOST_CHECK_CLOSE(gti_vol[6], 6.6, 1.0e-6);      // SEB
    BOOST_CHECK_CLOSE(gti_vol[7], 0.0, 1.0e-6);      // Unused

    // Total produced volumes.
    const auto gtp_vol = xgrptrc.subspan(3 * tracerStride, tracerStride);
    BOOST_CHECK_CLOSE(gtp_vol[0], 66666.666, 1.0e-6); // HEA
    BOOST_CHECK_CLOSE(gtp_vol[1], 6666.0, 1.0e-6);    // SEA
    BOOST_CHECK_CLOSE(gtp_vol[2], 666.006, 1.0e-6);   // FOCE
    BOOST_CHECK_CLOSE(gtp_vol[3], 666.007, 1.0e-6);   // SOCE
    BOOST_CHECK_CLOSE(gtp_vol[4], 66.006, 1.0e-6);    // FOCF
    BOOST_CHECK_CLOSE(gtp_vol[5], 66.007, 1.0e-6);    // SOCF
    BOOST_CHECK_CLOSE(gtp_vol[6], 6.666, 1.0e-6);     // SEB
    BOOST_CHECK_CLOSE(gtp_vol[7], 0.0, 1.0e-6);       // Unused
}

BOOST_AUTO_TEST_SUITE_END() // Thermal

BOOST_AUTO_TEST_SUITE_END() // Tracer_Values
