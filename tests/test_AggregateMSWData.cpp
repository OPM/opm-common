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

#define BOOST_TEST_MODULE Aggregate_Well_Data
#include <opm/output/eclipse/AggregateMSWData.hpp>

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/AggregateWellData.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <cstddef>

struct MockIH
{
    MockIH(const int numWells,
	   
	   const int nsegWell 	 =   2,  // E100
	   const int isegPerWell =  22,  // E100
	   const int rsegPerWell = 146,  // E100
	   const int ilbsPerWell =   5,  // E100
	   const int ilbrPerWell =  10); // E100
    

    std::vector<int> value;

    using Sz = std::vector<int>::size_type;

    Sz nwells;
    
    Sz nsegwl;
    Sz nswlmx;
    Sz nsegmx;
    Sz nlbrmx;
    Sz nisegz;
    Sz nrsegz;
    Sz nilbrz;
};

MockIH::MockIH(const int numWells,
	       const int nsegWell,
	       const int isegPerWell,
	       const int rsegPerWell,
	       const int ilbsPerWell,
	       const int ilbrPerWell )
    : value(411, 0)
{
    using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;

    this->nwells = this->value[Ix::NWELLS] = numWells;
    
    this->nsegwl = this->value[Ix::NSEGWL] = nsegWell;
    this->nswlmx = this->value[Ix::NSWLMX] = 2;
    this->nsegmx = this->value[Ix::NSEGMX] = 32;
    this->nisegz = this->value[Ix::NISEGZ] = isegPerWell;
    this->nrsegz = this->value[Ix::NRSEGZ] = rsegPerWell;
    this->nlbrmx = this->value[Ix::NLBRMX] = ilbsPerWell;
    this->nilbrz = this->value[Ix::NILBRZ] = ilbrPerWell;
}

namespace {
    Opm::Deck first_sim()
    {
        // Mostly copy of tests/FIRST_SIM.DATA
        const auto input = std::string {
            R"~(
RUNSPEC

TITLE
 TWO MULTI-LATERAL WELLS; PRODUCER AND INJECTOR - MULTI-SEGMENT BRANCHES

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
 3  20  1  3  /

WSEGDIMS
 2  32  5  /

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
 'PORO'   0.0    1 10  1 5  3 3  /
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
--  SWAT   KRW   PCOW
SWFN

    0.12  0       0
    1.0   0.00001 0  /

-- SIMILARLY FOR GAS
--
--  SGAS   KRG   PCOG
SGFN

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
--  SOIL     KROW     KROG
SOF3

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
 
RPTSOL
-- 
-- Initialisation Print Output
-- 
'PRES' 'SOIL' 'SWAT' 'SGAS' 'RS' 'RESTART=1' 'FIP=2' 'EQUIL' 'RSVD' /

SUMMARY      ===========================================================

FOPR

WOPR
 'PROD'
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

WELSPECS
 'PROD' 'G' 1 5 7030 'OIL' 0.0  'STD'  'STOP'  /
 'WINJ' 'G' 10 1 7030 'WAT' 0.0  'STD'  'STOP'   /
/

COMPDAT

 'PROD' 1 5 2 2   3*  0.2   3*  'X' /
 'PROD' 2 5 2 2   3*  0.2   3*  'X' /
 'PROD' 3 5 2 2   3*  0.2   3*  'X' /
 'PROD' 4 5 2 2   3*  0.2   3*  'X' /
 'PROD' 5 5 2 2   3*  0.2   3*  'X' /

 'PROD' 1 5 5 5   3*  0.2   3*  'X' /
 'PROD' 2 5 5 5   3*  0.2   3*  'X' /
 'PROD' 3 5 5 5   3*  0.2   3*  'X' /
 'PROD' 4 5 5 5   3*  0.2   3*  'X' /
 'PROD' 5 5 5 5   3*  0.2   3*  'X' /

 'PROD' 1 5 6 6   3*  0.2   3*  'X' /
 'PROD' 2 5 6 6   3*  0.2   3*  'X' /
 'PROD' 3 5 6 6   3*  0.2   3*  'X' /
 'PROD' 4 5 6 6   3*  0.2   3*  'X' /
 'PROD' 5 5 6 6   3*  0.2   3*  'X' /

 'PROD' 1 5 8 8   3*  0.2   3*  'X' /
 'PROD' 2 5 8 8   3*  0.2   3*  'X' /
 'PROD' 3 5 8 8   3*  0.2   3*  'X' /
 'PROD' 4 5 8 8   3*  0.2   3*  'X' /
 'PROD' 5 5 8 8   3*  0.2   3*  'X' /

 'WINJ' 10 1 1 1   3*  0.2   3*  'X' /
 'WINJ'   9 1 1 1   3*  0.2   3*  'X' /
 'WINJ'   8 1 1 1   3*  0.2   3*  'X' /
 'WINJ'   7 1 1 1   3*  0.2   3*  'X' /
 'WINJ'   6 1 1 1   3*  0.2   3*  'X' /

 'WINJ' 10 1  9 9   3*  0.2   3*  'X' /
 'WINJ'   9 1  9 9   3*  0.2   3*  'X' /
 'WINJ'   8 1  9 9   3*  0.2   3*  'X' /
 'WINJ'   7 1  9 9   3*  0.2   3*  'X' /
 'WINJ'   6 1  9 9   3*  0.2   3*  'X' /
/

WELSEGS

-- Name    Dep 1   Tlen 1  Vol 1
  'PROD'   7010      10    0.31   'INC' /

-- First   Last   Branch   Outlet  Length   Depth  Diam  Ruff  Area  Vol
-- Seg     Seg    Num      Seg              Chang
-- Main Stem
    2       12     1        1         20     20    0.2   1.E-3  1*   1* /
-- Top Branch
    13      13     2        2         50      0    0.2   1.E-3  1*   1* /
    14      17     2        13       100      0    0.2   1.E-3  1*   1* /
-- Middle Branch
    18      18     3        9         50      0    0.2   1.E-3  1*   1* /
    19      22     3        18       100      0    0.2   1.E-3  1*   1* /
-- Bottom Branch
    23      23     4        12        50      0    0.2   1.E-3  1*   1* /
    24      27     4        23       100      0    0.2   1.E-3  1*   1* /
-- Lower Middle Branch
    28      28     5        10         50      0    0.2   1.E-3  1*   1* /
    29      32     5        28       100      0    0.2   1.E-3  1*   1* /
 /

COMPSEGS

-- Name
  'PROD' /

-- I  J  K  Brn  Start   End     Dirn   End
--          No   Length  Length  Penet  Range
-- Top Branch
  1  5  2  2         30      130     'X'    3* /
  2  5  2  2        130      230     'X'    3* /
  3  5  2  2        230      330     'X'    3* /
  4  5  2  2        330      430     'X'    3* /
  5  5  2  2        430      530     'X'    3* /
-- Middle Branch
  1  5  5  3        170      270     'X'    3* /
  2  5  5  3        270      370     'X'    3* /
  3  5  5  3        370      470     'X'    3* /
  4  5  5  3        470      570     'X'    3* /
  5  5  5  3        570      670     'X'    3* /
-- Lower Middle Branch
  1  5  6  5        170      270     'X'    3* /
  2  5  6  5        270      370     'X'    3* /
  3  5  6  5        370      470     'X'    3* /
  4  5  6  5        470      570     'X'    3* /
  5  5  6  5        570      670     'X'    3* /
-- Bottom Branch
  1  5  8  4        230      330     'X'    3* /
  2  5  8  4        330      430     'X'    3* /
  3  5  8  4        430      530     'X'    3* /
  4  5  8  4        530      630     'X'    3* /
  5  5  8  4        630      730     'X'    3* /
 /
WELSEGS

-- Name    Dep 1   Tlen 1  Vol 1
  'WINJ'   7010      10    0.31   'INC' /

-- First   Last   Branch   Outlet  Length   Depth  Diam  Ruff  Area  Vol
-- Seg     Seg    Num      Seg              Chang
-- Main Stem
    2       14     1        1         20     20    0.2   1.E-3  1*   1* /
-- Top Branch
    15      15     2        2         50      0    0.2   1.E-3  1*   1* /
    16      19     2        15       100      0    0.2   1.E-3  1*   1* /
-- Bottom Branch
    20      20     3        14        50      0    0.2   1.E-3  1*   1* /
    21      24     3        20       100      0    0.2   1.E-3  1*   1* /
 /

COMPSEGS

-- Name
  'WINJ' /

-- I  J  K  Brn  Start   End     Dirn   End
--          No   Length  Length  Penet  Range
-- Top Branch
  10  1  1  2         30      130     'X'    3* /
    9  1  1  2        130      230     'X'    3* /
    8  1  1  2        230      330     'X'    3* /
    7  1  1  2        330      430     'X'    3* /
    6  1  1  2        430      530     'X'    3* /
-- Bottom Branch
  10  1  9  3        270      370     'X'    3* /
    9  1  9  3        370      470     'X'    3* /
    8  1  9  3        470      570     'X'    3* /
    7  1  9  3        570      670     'X'    3* /
    6  1  9  3        670      770     'X'    3* /
 /


WCONPROD
 'PROD' 'OPEN' 'LRAT'  3*  4000  1*  1000  1*  /
 /

WCONINJE
 'WINJ' 'WAT' 'OPEN' 'RESV'  1*  2000  3500  1*  /
 /


TUNING
 /
 /
 /

TSTEP
 2 2
/


END

)~" };

        return Opm::Parser{}.parseString(input);
    }

        Opm::SummaryState sim_state()
    {
        auto state = Opm::SummaryState{};

	state.add("SPR:PROD:1",   235.);
	state.add("SPR:PROD:2",   237.);
	state.add("SPR:PROD:3",   239.);
	state.add("SPR:PROD:4",   243.);

	state.add("SOFR:PROD:1",   35.);
	state.add("SOFR:PROD:2",   30.);
	state.add("SOFR:PROD:3",   25.);
	state.add("SOFR:PROD:4",   20.);
	
	state.add("SGFR:PROD:1",   25.E3);
	state.add("SGFR:PROD:2",   20.E3);
	state.add("SGFR:PROD:3",   15.E3);
	state.add("SGFR:PROD:4",   10.E3);

	state.add("SWFR:PROD:1",   5.);
	state.add("SWFR:PROD:2",   3.);
	state.add("SWFR:PROD:3",   2.);
	state.add("SWFR:PROD:4",   1.);

        return state;
    }


    /*Opm::data::WellRates well_rates_1()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::WellRates{};

        {

	    auto& s = xw["PROD"].segments[1];
	    s.rates.set(o::wat, 1.0);
	    s.rates.set(o::oil, 2.0);
	    s.rates.set(o::gas, 3.0);
	    s.pressure = 235.;
		
	    auto& s2 = xw["PROD"].segments[2];
	    //xw["PROD"].segments.insert(std::make_pair(2, Segment());
	    s2.rates.set(o::wat, 0.5);
	    s2.rates.set(o::oil, 1.0);
	    s2.rates.set(o::gas, 2.0);
	    s2.pressure = 225.;

        }

        {
            xw["OP_2"].bhp = 234.0;

            xw["OP_2"].rates.set(o::gas, 5.0);
            xw["OP_2"].connections.emplace_back();
        }

        return xw;
    }*/

}

struct SimulationCase
{
    explicit SimulationCase(const Opm::Deck& deck)
        : es   { deck }
        , sched{ deck, es }
        //, wr {deck, es, sched}
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    Opm::EclipseState es;
    Opm::Schedule     sched;
    Opm::data::WellRates wr;
};

// =====================================================================
/*
BOOST_AUTO_TEST_SUITE(Aggregate_MSW)


BOOST_AUTO_TEST_CASE (Test_of_rseg_data)
{
    std::cout << "before construct SimulationCase" << std::endl;
    const auto simCase = SimulationCase{first_sim()};
    std::cout << "after construct SimulationCase" << std::endl;
    // Report Step 2: 2011-01-20 --> 2013-06-15
    //const auto rptStep = std::size_t {2};
    const std::size_t rptStep = 1; 

    std::cout << "before getWells" << std::endl;
    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };
    std::cout << "After initialised - nlbrm: " << ih.nlbrmx <<  std::endl;
    std::cout << "After initialised - nilbrz: " << ih.nilbrz <<  std::endl;
    const auto nw = simCase.sched.getWells(rptStep).size();
    std::cout << "nwells =" << nw << std::endl;
    const auto nw_ih = ih.nwells;
    std::cout << "nwells_ih =" << nw_ih << std::endl;
    std::cout << "after getWells" << std::endl;
    const auto wr = simCase.wr;

    //const auto xw   = well_rates_1();
    std::cout << "before construct smry" << std::endl;
    const auto smry = sim_state();
    std::cout << "after construct smry" << std::endl;
    std::cout << "before construct AggregateMSWData" << std::endl;
    auto msw = Opm::RestartIO::Helpers::AggregateMSWData{ih.value};
    std::cout << "after construct AggregateMSWData" << std::endl;
    std::cout << "before captureDeclaredMSWData" << std::endl;
    std::cout << "before cdMSWD - nlbrm: " << ih.nlbrmx <<  std::endl;
    std::cout << "before cdMSWD - nilbrz: " << ih.nilbrz <<  std::endl;
    msw.captureDeclaredMSWData(simCase.sched, rptStep, simCase.es.getUnits(),ih.value, 
			       simCase.es.getInputGrid(), smry, wr);
     std::cout << "after captureDeclaredMSWData" << std::endl; 
    // rseg (PROD) -- producer
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;


        const auto  i0 = 0*ih.nrsegz;
	const auto  i1 = 1*ih.nrsegz;
	const auto  i2 = 2*ih.nrsegz;
	const auto  i3 = 3*ih.nrsegz;
	std::cout << "before getRSeg" << std::endl;
        const auto& rseg = msw.getRSeg();
	std::cout << "after getRSeg" << std::endl;

        BOOST_CHECK_CLOSE(rseg[i0 + 11], 235., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i1 + 11], 237., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i2 + 11], 239., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i3 + 11], 243., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i0 +  8],  35., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i1 +  8],  30., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i2 +  8],  25., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i3 +  8],  20., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i  +  8],  20., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i3 +  8],  20., 1.0e-10);
	BOOST_CHECK_CLOSE(rseg[i3 +  8],  20., 1.0e-10); 
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrRate], 2.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrRate], 3.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::LiqPrRate], 1.0 + 2.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrRate], 4.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::FlowBHP], 314.15, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatCut] , 0.625, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GORatio], 234.5, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::OilPrTotal], 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrTotal], 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrTotal], 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrTotal], 40.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::item37],
                          xwell[i0 + Ix::WatPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::item38],
                          xwell[i0 + Ix::GasPrRate], 1.0e-10);
    }  

    // XWEL (OP_2) -- water injector
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i1 = 1*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatPrRate], -100.0, 1.0e-10);

        // Copy of WWIR
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::LiqPrRate],
                          xwell[i1 + Ix::WatPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::FlowBHP], 400.6, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatInjTotal], 1000.0, 1.0e-10);

        // Copy of WWIR
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::item37],
                          xwell[i1 + Ix::WatPrRate], 1.0e-10);

        // Copy of WWIT
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::item82],
                          xwell[i1 + Ix::WatInjTotal], 1.0e-10);

        // WWVIR
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatVoidPrRate],
                          -4321.0, 1.0e-10);
    }

    // XWEL (OP_3) -- producer
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i2 = 2*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::OilPrRate], 11.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatPrRate], 12.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GasPrRate], 13.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::LiqPrRate], 11.0 + 12.0, 1.0e-10); // LPR
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::VoidPrRate], 14.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::FlowBHP], 314.15, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatCut] , 0.0625, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GORatio], 1234.5, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::OilPrTotal], 110.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatPrTotal], 120.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GasPrTotal], 130.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::VoidPrTotal], 140.0, 1.0e-10);

        // Copy of WWPR
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::item37],
                          xwell[i2 + Ix::WatPrRate], 1.0e-10);

        // Copy of WGPR
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::item38],
                          xwell[i2 + Ix::GasPrRate], 1.0e-10);
    }
    
        // RSEG (PROD) -- producer
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i2 = 2*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::OilPrRate], 11.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatPrRate], 12.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GasPrRate], 13.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::LiqPrRate], 11.0 + 12.0, 1.0e-10); // LPR
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::VoidPrRate], 14.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::FlowBHP], 314.15, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatCut] , 0.0625, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GORatio], 1234.5, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::OilPrTotal], 110.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatPrTotal], 120.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GasPrTotal], 130.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::VoidPrTotal], 140.0, 1.0e-10);

        // Copy of WWPR
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::item37],
                          xwell[i2 + Ix::WatPrRate], 1.0e-10);

        // Copy of WGPR
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::item38],
                          xwell[i2 + Ix::GasPrRate], 1.0e-10);
    }
  
}

BOOST_AUTO_TEST_SUITE_END()
*/