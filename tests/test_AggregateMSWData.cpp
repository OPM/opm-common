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

#include <opm/output/eclipse/SummaryState.hpp>

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

struct MockIH
{
    MockIH(const int numWells,
	   
	   const int nsegWell 	 =   1,  // E100
	   const int isegPerWell =   3,  // E100
	   const int rsegPerWell =  22,  // E100
	   const int ilbsPerWell = 146,  // E100
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
    this->nsegmx = this->value[Ix::NSEGMX] = 4;
    this->nisegz = this->value[Ix::NISEGZ] = isegPerWell;
    this->nrsegz = this->value[Ix::NRSEGZ] = rsegPerWell;
    this->nilbrz = this->value[Ix::NILBRZ] = ilbrPerWell;
}

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

WELSEGS
    'OP_1' 20. 0.25 1.0e-5 'ABS' 'HFA' 'HO' /
     2         2      1      1    20.25 0.5       0.3  0.00010 /
     3         3      2      1    20.25 0.5       0.3  0.00010 /
     4         4      2      3    20.25 0.5       0.3  0.00010 /
     /
COMPDAT
      'OP_1'  9  9   1   1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
      'OP_2'  9  9   2   2 'OPEN' 1*   46.825   0.311  4332.346 1*  1*  'X'  22.123 /
      'OP_1'  9  9   3   3 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
      'OP_1' 10  9   3   3 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
      'OP_1' 11  9   3   3 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
COMPSEGS
  'OP_1'/
     9    9     1     1   20      20.25 /
     9    9     3     1   20.25   20.5  /
    10    9     3     2   20.5    20.75 /
    11    9     3     2   20.75   21.0  /
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
        auto state = Opm::SummaryState{};

	state.add("SPR:OP_1:(1)",   235.);
	state.add("SPR:OP_1:(2)",   237.);
	state.add("SPR:OP_1:(3)",   239.);
	state.add("SPR:OP_1:(4)",   243.);

	state.add("SOFR:OP_1:(1)",   35.);
	state.add("SOFR:OP_1:(2)",   30.);
	state.add("SOFR:OP_1:(3)",   25.);
	state.add("SOFR:OP_1:(4)",   20.);
        return state;
    }


    /*Opm::data::WellRates well_rates_1()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::WellRates{};

        {

	    auto& s = xw["OP_1"].segments[1];
	    s.rates.set(o::wat, 1.0);
	    s.rates.set(o::oil, 2.0);
	    s.rates.set(o::gas, 3.0);
	    s.pressure = 235.;
		
	    auto& s2 = xw["OP_1"].segments[2];
	    //xw["OP_1"].segments.insert(std::make_pair(2, Segment());
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
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    Opm::EclipseState es;
    Opm::Schedule     sched;
};

// =====================================================================

BOOST_AUTO_TEST_SUITE(Aggregate_MSW)


BOOST_AUTO_TEST_CASE (Test_of_rseg_data)
{
    const auto simCase = SimulationCase{first_sim()};

    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep = std::size_t{2};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    //const auto xw   = well_rates_1();
    const auto smry = sim_state();
    auto msw = Opm::RestartIO::Helpers::AggregateMSWData{ih.value};

    msw.captureDeclaredMSWData(simCase.sched, rptStep, simCase.es.getUnits(),ih.value, 
			       simCase.es.getInputGrid(), smry);

    // rseg (OP_1) -- producer
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;


        const auto  i0 = 0*ih.nrsegz;
	const auto  i1 = 1*ih.nrsegz;
	const auto  i2 = 2*ih.nrsegz;
	const auto  i3 = 3*ih.nrsegz;
        const auto& rseg = msw.getRSeg();

        BOOST_CHECK_CLOSE(rseg[i0 + 11], 235., 1.0e-10);
        /*BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrRate], 2.0, 1.0e-10);
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
                          xwell[i0 + Ix::GasPrRate], 1.0e-10);*/
    }

    // XWEL (OP_2) -- water injector
/*    {
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
    
        // RSEG (OP_1) -- producer
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
*/
  
}

BOOST_AUTO_TEST_SUITE_END()
