/*
  Copyright 2019 Equinor
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

#define BOOST_TEST_MODULE Aggregate_Well_DataLGR

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/AggregateWellData.hpp>
#include <opm/output/eclipse/AggregateConnectionData.hpp>
#include <opm/output/eclipse/AggregateGroupData.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>


#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/io/eclipse/ERst.hpp>
#include <opm/io/eclipse/RestartFileView.hpp>
#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/state.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "tests/WorkArea.hpp"

namespace {

    struct MockIH
    {
        explicit MockIH(const int numWells,
                        const int iwelPerWell = 155,  // E100
                        const int swelPerWell = 122,  // E100
                        const int xwelPerWell = 130,  // E100
                        const int zwelPerWell =   3); // E100
        void add_icon_data(const int, const int,
                           const int, const int);
        void add_igr_data(const int, const int, const int,
                           const int, const int, const int);
        std::vector<int> value;

        using Sz = std::vector<int>::size_type;

        Sz nwells;
        Sz niwelz;
        Sz nswelz;
        Sz nxwelz;
        Sz nzwelz;
        Sz niconz;
        Sz nsconz;
        Sz nxconz;
        Sz ncwmax;
        Sz nwgmax;
        Sz ngmaxz;
        Sz nigrpz;
        Sz nsgrpz;
        Sz nxgrpz;
        Sz nzgrpz;

    };

    MockIH::MockIH(const int numWells,
                   const int iwelPerWell,
                   const int swelPerWell,
                   const int xwelPerWell,
                   const int zwelPerWell)
        : value(411, 0)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;

        this->nwells = this->value[Ix::NWELLS] = numWells;
        this->niwelz = this->value[Ix::NIWELZ] = iwelPerWell;
        this->nswelz = this->value[Ix::NSWELZ] = swelPerWell;
        this->nxwelz = this->value[Ix::NXWELZ] = xwelPerWell;
        this->nzwelz = this->value[Ix::NZWELZ] = zwelPerWell;
    }

    void MockIH::add_icon_data(const int entICON,  const int entSCON,
                               const int entXCON,  const int maxNumConn)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;
        this->niconz = this->value[Ix::NICONZ] = entICON;
        this->nsconz = this->value[Ix::NSCONZ] = entSCON;
        this->nxconz =  this->value[Ix::NXCONZ] = entXCON;
        this->ncwmax = this->value[Ix::NCWMAX] = maxNumConn;
    }


    void MockIH::add_igr_data(const int entIGR,  const int entSGR,
                              const int entXGR,  const int entZGR,
                              const int wellnumMaxGroup, const int maxGroupField)
    {
    using Ix = ::Opm::RestartIO::Helpers::VectorItems::intehead;
    this->nigrpz = this->value[Ix::NIGRPZ] = entIGR;
    this->nsgrpz = this->value[Ix::NSGRPZ] = entSGR;
    this->nxgrpz =  this->value[Ix::NXGRPZ] = entXGR;
    this->nzgrpz =  this->value[Ix::NZGRPZ] = entZGR;
    this->nwgmax = this->value[Ix::NWGMAX] = wellnumMaxGroup;
    this->ngmaxz = this->value[Ix::NGMAXZ] = maxGroupField;


    }

    struct SimulationCase
    {
        explicit SimulationCase(const Opm::Deck& deck)
            : es   { deck }
            , grid { es.getInputGrid() }
            , sched{ deck, es, std::make_shared<Opm::Python>() }
        {}

        // Order requirement: 'es' must be declared/initialised before 'sched'.
        // EclipseState calls routines that initialize EclipseGrid LGR cells.
        Opm::EclipseState es;
        Opm::EclipseGrid  grid;
        Opm::Schedule     sched;
    };

} // Anonymous namespace

// =====================================================================

BOOST_AUTO_TEST_SUITE(LGR_Aggregate_WD)

namespace {

Opm::Deck sim_default()
{
    // SPECASE DEFAULT FOR THE SAKE OF DEBUGGING - TO BE REMOVED
const auto sim_default = std::string {
    R"~(
RUNSPEC
TITLE
   SPE1 - CASE 1
DIMENS
   10 10 3 /
EQLDIMS
/
TABDIMS
/
OIL
GAS
WATER
DISGAS
FIELD
START
   1 'JAN' 2015 /
WELLDIMS
   2 3 1 2 /
UNIFOUT
GRID
INIT
NOECHO
DX
   	300*1000 /
DY
	300*1000 /
DZ
	100*20 100*30 100*50 /
TOPS
	100*8325 /
PORO
   	300*0.3 /
PERMX
	100*500 100*50 100*200 /
PERMY
	100*500 100*50 100*200 /
PERMZ
	100*500 100*50 100*200 /
ECHO
PROPS
PVTW
    	4017.55 1.038 3.22E-6 0.318 0.0 /
ROCK
	14.7 3E-6 /
SWOF
0.12	0    		 	1	0
0.18	4.64876033057851E-008	1	0
0.24	0.000000186		0.997	0
0.3	4.18388429752066E-007	0.98	0
0.36	7.43801652892562E-007	0.7	0
0.42	1.16219008264463E-006	0.35	0
0.48	1.67355371900826E-006	0.2	0
0.54	2.27789256198347E-006	0.09	0
0.6	2.97520661157025E-006	0.021	0
0.66	3.7654958677686E-006	0.01	0
0.72	4.64876033057851E-006	0.001	0
0.78	0.000005625		0.0001	0
0.84	6.69421487603306E-006	0	0
0.91	8.05914256198347E-006	0	0
1	0.00001			0	0 /
SGOF
0	0	1	0
0.001	0	1	0
0.02	0	0.997	0
0.05	0.005	0.980	0
0.12	0.025	0.700	0
0.2	0.075	0.350	0
0.25	0.125	0.200	0
0.3	0.190	0.090	0
0.4	0.410	0.021	0
0.45	0.60	0.010	0
0.5	0.72	0.001	0
0.6	0.87	0.0001	0
0.7	0.94	0.000	0
0.85	0.98	0.000	0
0.88	0.984	0.000	0 /
DENSITY
      	53.66 64.49 0.0533 /
PVDG
14.700	166.666	0.008000
264.70	12.0930	0.009600
514.70	6.27400	0.011200
1014.7	3.19700	0.014000
2014.7	1.61400	0.018900
2514.7	1.29400	0.020800
3014.7	1.08000	0.022800
4014.7	0.81100	0.026800
5014.7	0.64900	0.030900
9014.7	0.38600	0.047000 /
PVTO
0.0010	14.7	1.0620	1.0400 /
0.0905	264.7	1.1500	0.9750 /
0.1800	514.7	1.2070	0.9100 /
0.3710	1014.7	1.2950	0.8300 /
0.6360	2014.7	1.4350	0.6950 /
0.7750	2514.7	1.5000	0.6410 /
0.9300	3014.7	1.5650	0.5940 /
1.2700	4014.7	1.6950	0.5100
	9014.7	1.5790	0.7400 /
1.6180	5014.7	1.8270	0.4490
	9014.7	1.7370	0.6310 /
/
SOLUTION
EQUIL
	8400 4800 8450 0 8300 0 1 0 0 /
RSVD
8300 1.270
8450 1.270 /
SUMMARY
FOPR
WGOR
   'PROD'
/
FGOR
BPR
1  1  1 /
10 10 3 /
/
BGSAT
1  1  1 /
1  1  2 /
1  1  3 /
10 1  1 /
10 1  2 /
10 1  3 /
10 10 1 /
10 10 2 /
10 10 3 /
/
WBHP
  'INJ'
  'PROD'
/
WGIR
  'INJ'
  'PROD'
/
WGIT
  'INJ'
  'PROD'
/
WGPR
  'INJ'
  'PROD'
/
WGPT
  'INJ'
  'PROD'
/
WOIR
  'INJ'
  'PROD'
/
WOIT
  'INJ'
  'PROD'
/
WOPR
  'INJ'
  'PROD'
/
WOPT
  'INJ'
  'PROD'
/
WWIR
  'INJ'
  'PROD'
/
WWIT
  'INJ'
  'PROD'
/
WWPR
  'INJ'
  'PROD'
/
WWPT
  'INJ'
  'PROD'
/
SCHEDULE
RPTSCHED
	'PRES' 'SGAS' 'RS' 'WELLS' /
RPTRST
	'BASIC=1' /
DRSDT
 0 /
WELSPECS
	'PROD'	'G1'	10	10	8400	'OIL' /
	'INJ'	'G1'	1	1	8335	'GAS' /
/
COMPDAT
	'PROD'	10	10	1	3	'OPEN'	1*	1*	0.5 /
	'INJ'	1	1	2	3	'OPEN'	1*	1*	0.5 /
/
WCONPROD
	'PROD' 'OPEN' 'ORAT' 20000 4* 1000 /
/
WCONINJE
	'INJ'	'GAS'	'OPEN'	'RATE'	100000 1* 9014 /
/
TSTEP
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31
31 28 31 30 31 30 31 31 30 31 30 31 /
END
)~" };
    return Opm::Parser{}.parseString(sim_default);
}


    Opm::Deck simLGR_2lgrwell()
    {
        // TWO LGR Cells with wells in each LGR.
        // Mostly copy of tests/FIRST_SIM.DATA
        const auto input = std::string {
            R"~(
RUNSPEC
TITLE
   SPE1 - CASE 1
DIMENS
   3 1 1 /
EQLDIMS
/
TABDIMS
/
OIL
GAS
WATER
DISGAS
FIELD
START
   1 'JAN' 2015 /
WELLDIMS
   2 1 1 2 /
UNIFOUT
GRID
CARFIN
'LGR1'  1  1  1  1  1  1  3  3  1 /
ENDFIN
CARFIN
'LGR2'  3  3  1  1  1  1  3  3  1 /
ENDFIN
INIT
DX
   	3*2333 /
DY
	3*3500 /
DZ
	3*50 /
TOPS
	3*8325 /
PORO
   	3*0.3 /
PERMX
	3*500 /
PERMY
	3*250 /
PERMZ
	3*200 /
ECHO
PROPS
PVTW
    	4017.55 1.038 3.22E-6 0.318 0.0 /
ROCK
14.7 3E-6 /
SWOF
0.12	0    		 	1	0
0.18	4.64876033057851E-008	1	0
0.24	0.000000186		0.997	0
0.3	4.18388429752066E-007	0.98	0
0.36	7.43801652892562E-007	0.7	0
0.42	1.16219008264463E-006	0.35	0
0.48	1.67355371900826E-006	0.2	0
0.54	2.27789256198347E-006	0.09	0
0.6	2.97520661157025E-006	0.021	0
0.66	3.7654958677686E-006	0.01	0
0.72	4.64876033057851E-006	0.001	0
0.78	0.000005625		0.0001	0
0.84	6.69421487603306E-006	0	0
0.91	8.05914256198347E-006	0	0
1	0.00001			0	0 /
SGOF
0	0	1	0
0.001	0	1	0
0.02	0	0.997	0
0.05	0.005	0.980	0
0.12	0.025	0.700	0
0.2	0.075	0.350	0
0.25	0.125	0.200	0
0.3	0.190	0.090	0
0.4	0.410	0.021	0
0.45	0.60	0.010	0
0.5	0.72	0.001	0
0.6	0.87	0.0001	0
0.7	0.94	0.000	0
0.85	0.98	0.000	0
0.88	0.984	0.000	0 /
DENSITY
      	53.66 64.49 0.0533 /
PVDG
14.700	166.666	0.008000
264.70	12.0930	0.009600
514.70	6.27400	0.011200
1014.7	3.19700	0.014000
2014.7	1.61400	0.018900
2514.7	1.29400	0.020800
3014.7	1.08000	0.022800
4014.7	0.81100	0.026800
5014.7	0.64900	0.030900
6014.7	0.58700	0.035900
7014.7	0.45500	0.039900
8014.7	0.41200	0.044900
9014.7	0.38600	0.047000 /
PVTO
0.0010	14.7	1.0620	1.0400 /
0.0905	264.7	1.1500	0.9750 /
0.1800	514.7	1.2070	0.9100 /
0.3710	1014.7	1.2950	0.8300 /
0.6360	2014.7	1.4350	0.6950 /
0.7750	2514.7	1.5000	0.6410 /
0.9300	3014.7	1.5650	0.5940 /
1.2700	4014.7	1.6950	0.5100
         5014.7	1.6650	0.5500
		 6014.7	1.6250	0.6100
		 7014.7	1.6050	0.6500
		 8014.7	1.5850	0.6900
	    9014.7	1.5790	0.7400 /
1.6180	5014.7	1.8270	0.4490
        6014.7	1.8070	0.4890
		7014.7	1.770	0.5490
		8014.7	1.7570	0.5990
	    9014.7	1.7370	0.6310 /
/
SOLUTION
EQUIL
	8400 2800 8450 0 8300 0 1 0 0 /
RSVD
8300 1.270
8450 1.270 /
SUMMARY
FOPR
WGOR
   'PROD'
/
FGOR
WBHP
  'INJ'
  'PROD'
/
WGIR
  'INJ'
  'PROD'
/
WGIT
  'INJ'
  'PROD'
/
WGPR
  'INJ'
  'PROD'
/
WGPT
  'INJ'
  'PROD'
/
WOIR
  'INJ'
  'PROD'
/
WOIT
  'INJ'
  'PROD'
/
WOPR
  'INJ'
  'PROD'
/
WOPT
  'INJ'
  'PROD'
/
WWIR
  'INJ'
  'PROD'
/
WWIT
  'INJ'
  'PROD'
/
WWPR
  'INJ'
  'PROD'
/
WWPT
  'INJ'
  'PROD'
/
SCHEDULE
RPTSCHED
	'PRES' 'SGAS' 'RS' 'WELLS' /
RPTRST
	'BASIC=1' /
DRSDT
 0 /
WELSPECL
	'PROD'	'G1' 'LGR2'	3	3	8400	'OIL' /
	'INJ'	'G1' 'LGR1'	1	1	8335	'GAS' /
/
COMPDATL
	'PROD' 'LGR2'	3	3	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'  'LGR1'   1	1	1	1	'OPEN'	1*	1*	0.5 /
/
WCONPROD
-- Item #:1	2      3     4	   5  9
	'PROD' 'OPEN' 'ORAT' 20000 4* 1000 /
/

WCONINJE
-- Item #:1	 2	 3	 4	5      6  7
	'INJ'	'WATER'	'OPEN'	'RATE'	40000 1* 9014 /
/
TSTEP
0.1 1 31
/
END
)~" };

        return Opm::Parser{}.parseString(input);
    }


    Opm::Deck first_simLGR()
    {
        // Mostly copy of tests/FIRST_SIM.DATA
        const auto input = std::string {
            R"~(
RUNSPEC

TITLE
   LGR-PROBLEM

DIMENS
   3 1 1 /

EQLDIMS
/

TABDIMS
/

OIL
GAS
WATER


FIELD

START
   1 'JAN' 2015 /

WELLDIMS
   2 1 1 2 /

UNIFOUT

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  1  1  1  1  1  1  3  3  1 /
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  3  3  1  1  1  1  3  3  1 /
ENDFIN

INIT

DX
   	3*2333 /
DY
	3*3500 /
DZ
	3*50 /

TOPS
	3*8325 /

PORO
   	3*0.3 /

PERMX
	3*500 /

PERMY
	3*250 /

PERMZ

	3*200 /
ECHO
INIT
PROPS

PVDO
--Po    Bo     Vo
2500    1.26   0.5
3000    1.256  0.5
3500    1.252  0.5
4000    1.248  0.5
4500    1.245  0.5
5000    1.243  0.5
/
PVDG
--Pg    Bg     Vg
2500    0.98   0.1550
3000    0.74   0.1650
3500    0.65   0.170
4000    0.59   0.175
4500    0.54   0.180
5000    0.45   0.19
/

PVTW
4500         1.03  3.0E-5  0.4 /

ROCK
14.7 3E-6 /

SWFN
--Sw   Krw   Pc
0.2    0     6
0.25   0.005 5
0.3    0.01  4
0.35   0.02  3
0.40   0.03  2.4
0.45   0.04  1.9
0.5    0.055 1.4
0.55   0.08  1.0
0.6    0.11  0.7
0.65   0.17  0.4
0.7    0.23  0.25
0.75   0.32  0.1
1.0    1.0   0.0
/



SGFN
--Sg   Krg   Pcog
0      0     0
0.05   0     0.09
0.10   0.022 0.20
0.15   0.06  0.38
0.20   0.10  0.57
0.25   0.14  0.83
0.30   0.188 1.08
0.35   0.24  1.37
0.40   0.30  1.69
0.45   0.364 2
0.50   0.458 2.36
0.55   0.60  2.70
0.60   0.75  3
/


SOF3
--So   Krow   Krog
0.0    0      0
0.1    0.02   0
0.2    0.05   0
0.25   0.08   0.01
0.30   0.11   0.02
0.35   0.15   0.03
0.40   0.2    0.04
0.45   0.25   0.08
0.50   0.32   0.14
0.55   0.4    0.225
0.60   0.5    0.33
0.65   0.6    0.434
0.70   0.7    0.575
0.75   0.8    0.72
0.80   0.9    0.9
/

DENSITY
-- In FIELD units:
      	53.66 64.49 0.0533 /



SOLUTION

EQUIL
-- Item #: 1 2    3    4 5    6 7 8 9
	8400 4000 8700 0 8000 0 1 0 0 /



SUMMARY
-- 1a) Oil rate vs time
FOPR
-- Field Oil Production Rate

-- 1b) GOR vs time
WGOR
-- Well Gas-Oil Ratio
   'PROD'
/
-- Using FGOR instead of WGOR:PROD results in the same graph
FGOR

-- In order to compare Eclipse with Flow:
WBHP
  'INJ'
  'PROD'
/
WGIR
  'INJ'
  'PROD'
/
WGIT
  'INJ'
  'PROD'
/
WGPR
  'INJ'
  'PROD'
/
WGPT
  'INJ'
  'PROD'
/
WOIR
  'INJ'
  'PROD'
/
WOIT
  'INJ'
  'PROD'
/
WOPR
  'INJ'
  'PROD'
/
WOPT
  'INJ'
  'PROD'
/
WWIR
  'INJ'
  'PROD'
/
WWIT
  'INJ'
  'PROD'
/
WWPR
  'INJ'
  'PROD'
/
WWPT
  'INJ'
  'PROD'
/
SCHEDULE
RPTSCHED
	'PRES' 'SGAS'  'WELLS' /

RPTRST
	'BASIC=1' /

WELSPECL
-- Item #: 1	 2	3	4	5	 6 7
	'PROD'	'G1' 'LGR2'	3	3	1*	'OIL' /
	'INJ'	'G1' 'LGR1'	1	1	1*	'WATER' /
/
COMPDATL
-- Item #: 1	2	3	4	5	6	7	8	9 10
	'PROD' 'LGR2'	3	3	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'  'LGR1'   1	1	1	1	'OPEN'	1*	1*	0.5 /
/

WTEST
 'PROD' 1  PGD  3 2 /
/

WCONPROD
-- Item #:1	2      3     4	   5  9
	'PROD' 'OPEN' 'ORAT' 20000 4* 1000 /
/

WCONINJE
-- Item #:1	 2	 3	 4	5      6  7
	'INJ'	'WATER'	'OPEN'	'RATE'	40000 1* 9014 /
/

TSTEP
--Advance the simulater once a month for ONE years:
0.1 1 31 28 31
/

WELOPEN
      'PROD' 'STOP' /
/

TSTEP
--Advance the simulater once a month for ONE years:
30 31 30 31 31 30 31 30 31 31 28 31 30 31 30 31 31 30 31 30 31 31 28 31 30 31 30 31 31 30 31 30 31
/

END
)~" };

        return Opm::Parser{}.parseString(input);
    }


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
UNIFOUT
UNIFIN
DIMENS
 10 10 10 /
WELLDIMS
 6  20  1  6  /
TABDIMS
 1  1  15  15  2  15  /
FIELD
EQLDIMS
 1  /

GRID
DXV
10*100. /
DYV
10*100. /
DZV
10*100. /
TOPS
100*7000. /

PORO
1000*0.2 /

PERMX
1000*100. /

PERMY
1000*100. /

PERMZ
1000*10. /


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


REGIONS    ===========================================================


FIPNUM

  1000*1
/

EQLNUM

  1000*1
/


SOLUTION    ============================================================

EQUIL
7020.00 2700.00 7990.00  .00000 7200.00  .00000     0      0       5 /

RSVD       2 TABLES    3 NODES IN EACH           FIELD   12:00 17 AUG 83
   7000.0  1.0000
   7990.0  1.0000
/

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

WTEST
 'OP_1' 1  PGD  3 2 /
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

WELOPEN
      'OP_1' 'STOP' /
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
DATES             -- 8
 18  OCT 2014 /
/
WELSPECS
      'OP_6'       'OP'   9   9 1*     'OIL' 1*      1*  1*   1*  1*   1*  1*  /
/
COMPDAT
      'OP_6'  9  9   3  9 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/
WCONHIST
      'OP_6' 'OPEN' 'RESV' 275. 0.  34576.  /
/
WELOPEN
      'OP_5' 'SHUT'  /
/
TSTEP            -- 9
10 /
)~" };

        return Opm::Parser{}.parseString(input);
    }

    Opm::Deck wecon_etc_sim()
    {
        // Mostly copy of tests/FIRST_SIM.DATA
        const auto input = std::string {
            R"~(
RUNSPEC
OIL
GAS
WATER
DISGAS
UNIFOUT
UNIFIN
DIMENS
 10 10 10 /
WELLDIMS
 6  20  1  6  /
TABDIMS
 1  1  15  15  2  15  /
FIELD
EQLDIMS
 1  /

GRID
DXV
10*100. /
DYV
10*100. /
DZV
10*100. /
TOPS
100*7000. /

PORO
1000*0.2 /

PERMX
1000*100. /

PERMY
1000*100. /

PERMZ
1000*10. /


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


REGIONS    ===========================================================


FIPNUM

  1000*1
/

EQLNUM

  1000*1
/


SOLUTION    ============================================================

EQUIL
7020.00 2700.00 7990.00  .00000 7200.00  .00000     0      0       5 /

RSVD       2 TABLES    3 NODES IN EACH           FIELD   12:00 17 AUG 83
   7000.0  1.0000
   7990.0  1.0000
/

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

WTEST
 'OP_1' 1  PGD  3 2 /
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

WECON
 'OP_1'   1.234 12.345 0.87 210.98 1*  WELL   NO /
 'OP_3'   1* 1* 1* 0.0 0.0 +CON  YES 1* POTN  0.56 PLUG 1* 10.23 /
/

WGRUPCON
  OP_1 YES 123.456 GAS 0.75 /
  OP_3 NO 100.0 /
/

TSTEP            -- 3
10 /
)~" };
        return Opm::Parser{}.parseString(input);
    }

    Opm::Deck msw_sim(const std::string& fname)
    {
        return Opm::Parser{}.parseFile(fname);
    }

    Opm::SummaryState sim_stateLGR()
    {
        auto state = Opm::SummaryState{Opm::TimeService::now(), 0.0};

        state.update_well_var("PROD", "WOPR" ,    1.0);
        state.update_well_var("PROD", "WWPR" ,    2.0);
        state.update_well_var("PROD", "WGPR" ,    3.0);
        state.update_well_var("PROD", "WVPR" ,    4.0);
        state.update_well_var("PROD", "WOPT" ,   10.0);
        state.update_well_var("PROD", "WWPT" ,   20.0);
        state.update_well_var("PROD", "WGPT" ,   30.0);
        state.update_well_var("PROD", "WVPT" ,   40.0);
        state.update_well_var("PROD", "WWIR" ,    0.0);
        state.update_well_var("PROD", "WGIR" ,    0.0);
        state.update_well_var("PROD", "WWIT" ,    0.0);
        state.update_well_var("PROD", "WGIT" ,    0.0);
        state.update_well_var("PROD", "WVIT" ,    0.0);
        state.update_well_var("PROD", "WWCT" ,    0.625);
        state.update_well_var("PROD", "WGOR" ,  234.5);
        state.update_well_var("PROD", "WBHP" ,  314.15);
        state.update_well_var("PROD", "WTHP" ,  123.45);
        state.update_well_var("PROD", "WOPTH",  345.6);
        state.update_well_var("PROD", "WWPTH",  456.7);
        state.update_well_var("PROD", "WGPTH",  567.8);
        state.update_well_var("PROD", "WWITH",    0.0);
        state.update_well_var("PROD", "WGITH",    0.0);
        state.update_well_var("PROD", "WGVIR",    0.0);
        state.update_well_var("PROD", "WWVIR",    0.0);
        state.update_well_var("PROD", "WOPGR",    4.9);
        state.update_well_var("PROD", "WWPGR",    3.8);
        state.update_well_var("PROD", "WGPGR",    2.7);
        state.update_well_var("PROD", "WVPGR",    6.1);

        state.update_well_var("INJ", "WOPR" ,    0.0);
        state.update_well_var("INJ", "WWPR" ,    0.0);
        state.update_well_var("INJ", "WGPR" ,    0.0);
        state.update_well_var("INJ", "WVPR" ,    0.0);
        state.update_well_var("INJ", "WOPT" ,    0.0);
        state.update_well_var("INJ", "WWPT" ,    0.0);
        state.update_well_var("INJ", "WGPT" ,    0.0);
        state.update_well_var("INJ", "WVPT" ,    0.0);
        state.update_well_var("INJ", "WWIR" ,  100.0);
        state.update_well_var("INJ", "WGIR" ,  200.0);
        state.update_well_var("INJ", "WWIT" , 1000.0);
        state.update_well_var("INJ", "WGIT" , 2000.0);
        state.update_well_var("INJ", "WVIT" , 1234.5);
        state.update_well_var("INJ", "WWCT" ,    0.0);
        state.update_well_var("INJ", "WGOR" ,    0.0);
        state.update_well_var("INJ", "WBHP" ,  400.6);
        state.update_well_var("INJ", "WTHP" ,  234.5);
        state.update_well_var("INJ", "WOPTH",    0.0);
        state.update_well_var("INJ", "WWPTH",    0.0);
        state.update_well_var("INJ", "WGPTH",    0.0);
        state.update_well_var("INJ", "WWITH", 1515.0);
        state.update_well_var("INJ", "WGITH", 3030.0);
        state.update_well_var("INJ", "WGVIR", 1234.0);
        state.update_well_var("INJ", "WWVIR", 4321.0);
        state.update_well_var("INJ", "WOIGR",    4.9);
        state.update_well_var("INJ", "WWIGR",    3.8);
        state.update_well_var("INJ", "WGIGR",    2.7);
        state.update_well_var("INJ", "WVIGR",    6.1);

        return state;
    }

    Opm::SummaryState sim_state()
    {
        auto state = Opm::SummaryState{Opm::TimeService::now(), 0.0};

        state.update_well_var("OP_1", "WOPR" ,    1.0);
        state.update_well_var("OP_1", "WWPR" ,    2.0);
        state.update_well_var("OP_1", "WGPR" ,    3.0);
        state.update_well_var("OP_1", "WVPR" ,    4.0);
        state.update_well_var("OP_1", "WOPT" ,   10.0);
        state.update_well_var("OP_1", "WWPT" ,   20.0);
        state.update_well_var("OP_1", "WGPT" ,   30.0);
        state.update_well_var("OP_1", "WVPT" ,   40.0);
        state.update_well_var("OP_1", "WWIR" ,    0.0);
        state.update_well_var("OP_1", "WGIR" ,    0.0);
        state.update_well_var("OP_1", "WWIT" ,    0.0);
        state.update_well_var("OP_1", "WGIT" ,    0.0);
        state.update_well_var("OP_1", "WVIT" ,    0.0);
        state.update_well_var("OP_1", "WWCT" ,    0.625);
        state.update_well_var("OP_1", "WGOR" ,  234.5);
        state.update_well_var("OP_1", "WBHP" ,  314.15);
        state.update_well_var("OP_1", "WTHP" ,  123.45);
        state.update_well_var("OP_1", "WOPTH",  345.6);
        state.update_well_var("OP_1", "WWPTH",  456.7);
        state.update_well_var("OP_1", "WGPTH",  567.8);
        state.update_well_var("OP_1", "WWITH",    0.0);
        state.update_well_var("OP_1", "WGITH",    0.0);
        state.update_well_var("OP_1", "WGVIR",    0.0);
        state.update_well_var("OP_1", "WWVIR",    0.0);
        state.update_well_var("OP_1", "WOPGR",    4.9);
        state.update_well_var("OP_1", "WWPGR",    3.8);
        state.update_well_var("OP_1", "WGPGR",    2.7);
        state.update_well_var("OP_1", "WVPGR",    6.1);

        state.update_well_var("OP_2", "WOPR" ,    0.0);
        state.update_well_var("OP_2", "WWPR" ,    0.0);
        state.update_well_var("OP_2", "WGPR" ,    0.0);
        state.update_well_var("OP_2", "WVPR" ,    0.0);
        state.update_well_var("OP_2", "WOPT" ,    0.0);
        state.update_well_var("OP_2", "WWPT" ,    0.0);
        state.update_well_var("OP_2", "WGPT" ,    0.0);
        state.update_well_var("OP_2", "WVPT" ,    0.0);
        state.update_well_var("OP_2", "WWIR" ,  100.0);
        state.update_well_var("OP_2", "WGIR" ,  200.0);
        state.update_well_var("OP_2", "WWIT" , 1000.0);
        state.update_well_var("OP_2", "WGIT" , 2000.0);
        state.update_well_var("OP_2", "WVIT" , 1234.5);
        state.update_well_var("OP_2", "WWCT" ,    0.0);
        state.update_well_var("OP_2", "WGOR" ,    0.0);
        state.update_well_var("OP_2", "WBHP" ,  400.6);
        state.update_well_var("OP_2", "WTHP" ,  234.5);
        state.update_well_var("OP_2", "WOPTH",    0.0);
        state.update_well_var("OP_2", "WWPTH",    0.0);
        state.update_well_var("OP_2", "WGPTH",    0.0);
        state.update_well_var("OP_2", "WWITH", 1515.0);
        state.update_well_var("OP_2", "WGITH", 3030.0);
        state.update_well_var("OP_2", "WGVIR", 1234.0);
        state.update_well_var("OP_2", "WWVIR", 4321.0);
        state.update_well_var("OP_2", "WOIGR",    4.9);
        state.update_well_var("OP_2", "WWIGR",    3.8);
        state.update_well_var("OP_2", "WGIGR",    2.7);
        state.update_well_var("OP_2", "WVIGR",    6.1);

        state.update_well_var("OP_3", "WOPR" ,   11.0);
        state.update_well_var("OP_3", "WWPR" ,   12.0);
        state.update_well_var("OP_3", "WGPR" ,   13.0);
        state.update_well_var("OP_3", "WVPR" ,   14.0);
        state.update_well_var("OP_3", "WOPT" ,  110.0);
        state.update_well_var("OP_3", "WWPT" ,  120.0);
        state.update_well_var("OP_3", "WGPT" ,  130.0);
        state.update_well_var("OP_3", "WVPT" ,  140.0);
        state.update_well_var("OP_3", "WWIR" ,    0.0);
        state.update_well_var("OP_3", "WGIR" ,    0.0);
        state.update_well_var("OP_3", "WWIT" ,    0.0);
        state.update_well_var("OP_3", "WGIT" ,    0.0);
        state.update_well_var("OP_3", "WVIT" ,    0.0);
        state.update_well_var("OP_3", "WWCT" ,    0.0625);
        state.update_well_var("OP_3", "WGOR" , 1234.5);
        state.update_well_var("OP_3", "WBHP" ,  314.15);
        state.update_well_var("OP_3", "WTHP" ,  246.9);
        state.update_well_var("OP_3", "WOPTH", 2345.6);
        state.update_well_var("OP_3", "WWPTH", 3456.7);
        state.update_well_var("OP_3", "WGPTH", 4567.8);
        state.update_well_var("OP_3", "WWITH",    0.0);
        state.update_well_var("OP_3", "WGITH",    0.0);
        state.update_well_var("OP_3", "WGVIR",    0.0);
        state.update_well_var("OP_3", "WWVIR",   43.21);
        state.update_well_var("OP_3", "WOPGR",    49.0);
        state.update_well_var("OP_3", "WWPGR",    38.9);
        state.update_well_var("OP_3", "WGPGR",    27.8);
        state.update_well_var("OP_3", "WVPGR",    61.2);

        return state;
    }

    Opm::data::Wells well_rates_1()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::Wells{};

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
            auto& curr = xw["OP_1"].current_control;
            curr.isProducer = true;
            curr.prod = ::Opm::Well::ProducerCMode::GRAT;
        }

        {
            xw["OP_2"].bhp = 234.0;

            xw["OP_2"].rates.set(o::gas, 5.0);
            //xw["OP_2"].connections.emplace_back();

            //auto& c = xw["OP_2"].connections.back();
            //c.rates.set(o::gas, 4.0);
            auto& curr = xw["OP_2"].current_control;
            curr.isProducer = false;
            curr.inj = ::Opm::Well::InjectorCMode::RATE;
        }

        return xw;
    }

    Opm::data::Wells well_rates_2()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::Wells{};

        {
            xw["OP_1"].bhp = 150.0;  // Closed
            xw["OP_1"].dynamicStatus = ::Opm::Well::Status::STOP;

            xw["OP_1"].connections.emplace_back();
            auto& c = xw["OP_1"].connections.back();

            c.rates.set(o::wat, 1.0)
                   .set(o::oil, 2.0)
                   .set(o::gas, 3.0);

            auto& curr = xw["OP_1"].current_control;
            curr.isProducer = true;
            curr.prod = ::Opm::Well::ProducerCMode::NONE;
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

} // Anonymous namespace


// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (Declared_Well_DataLGR)
{
    const auto simCase = SimulationCase{simLGR_2lgrwell()};

    Opm::Action::State action_state;
    Opm::WellTestState wtest_state;

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};

    auto countWells = [&simCase](const std::string& lgr_tag) -> int {
        int num_filtered_wells = 0;
        for (const auto& well : simCase.sched.getWells(rptStep)) {
            if (well.get_lgr_well_tag().value_or("") == lgr_tag) {
                ++num_filtered_wells;
            }
        }
        return num_filtered_wells;
    };

    auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };
    // the original case has 25 entICON, 41 entSCON and 58 entWCON
    // however, OPM forces 26 and 42 when INTHEAD is created.
    ih.add_icon_data(26, 42 ,58 , 2);

    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{2});

    int num_lgr1 = countWells("LGR1");
    int num_lgr2 = countWells("LGR2");

    auto ih_lgr1 = MockIH {
        static_cast<int>(num_lgr1)
    };
    ih_lgr1.add_icon_data(26, 42 ,58 , 1);

    auto ih_lgr2 = MockIH {
        static_cast<int>(num_lgr2)
    };
    ih_lgr2.add_icon_data(26, 42 ,58 , 1);


    BOOST_CHECK_EQUAL(ih.nwells, ih_lgr1.nwells + ih_lgr2.nwells);

    const auto smry = sim_stateLGR();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    auto awd_lgr1 = Opm::RestartIO::Helpers::AggregateWellData{ih_lgr1.value};
    auto awd_lgr2 = Opm::RestartIO::Helpers::AggregateWellData{ih_lgr2.value};

    awd.captureDeclaredWellData(simCase.sched,
                            simCase.grid,
                            simCase.es.tracer(),
                            rptStep,
                            action_state,
                            wtest_state,
                            smry,
                            ih.value);


    awd_lgr1.captureDeclaredWellDataLGR(simCase.sched,
                                        simCase.grid,
                                        simCase.es.tracer(),
                                        rptStep,
                                        action_state,
                                        wtest_state,
                                        smry,
                                        ih.value,
                                        "LGR1");

    awd_lgr2.captureDeclaredWellDataLGR(simCase.sched,
                                        simCase.grid,
                                        simCase.es.tracer(),
                                        rptStep,
                                        action_state,
                                        wtest_state,
                                        smry,
                                        ih.value,
                                        "LGR2");


    // -------------------------- IWEL FOR GLOBAL WELLS --------------------------
    // GLOBAL WELLS
    // IWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 2); // LOCATED LGR2

    }
    // GLOBAL WELLS
    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // INJ -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // INJ -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // INJ #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 3); // INJ -> Injector
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LOCATED LGR1
    }
    // -------------------------- IWEL FOR LGR WELLS --------------------------
    // LGR02 WELL
    // IWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih_lgr2.niwelz;
        const auto& iwell = awd_lgr2.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 3); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LGR WELL LGR
    }
    // LGR02 WELL
    // LGWEL (PROD)
    {
        const auto& lgwel = awd_lgr2.getLGWell();
        BOOST_CHECK_EQUAL(lgwel[0] , 1); //
    }

    // -------------------------- IWEL FOR LGR WELLS --------------------------
    // LGR01 WELL
    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih_lgr1.niwelz;
        const auto& iwell = awd_lgr1.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 3); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 2); // LOCATED LGR2
    }
    // LGR01 WELL
    // LGWEL (INJ)
    {
        const auto& lgwel = awd_lgr1.getLGWell();
        BOOST_CHECK_EQUAL(lgwel[0] , 2);
    }

    // -------------------------- ZWEL FOR GLOBAL WELLS --------------------------
    // GLOBAL WELLS
    // ZWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 0*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD    ");
    }
    // ZWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i1 = 1*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i1 + Ix::WellName].c_str(), "INJ     ");
    }
    // -------------------------- ZWEL FOR LGR WELLS --------------------------
    // LGR02 WELL
    // ZWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 0*ih_lgr2.nzwelz;
        const auto& zwell = awd_lgr2.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD    ");
    }
    // LGR01 WELL
    // ZWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i1 = 0*ih_lgr1.nzwelz;
        const auto& zwell = awd_lgr1.getZWell();
        BOOST_CHECK_EQUAL(zwell[i1 + Ix::WellName].c_str(), "INJ     ");
    }


    auto conn_aggregator = Opm::RestartIO::Helpers::AggregateConnectionData(ih.value);
    auto xw = Opm::data::Wells {};
    conn_aggregator.captureDeclaredConnData(simCase.sched, simCase.es.getInputGrid(),
                                            simCase.es.getUnits(), xw,
                                            sim_stateLGR(), rptStep);


    auto conn_aggregator_lgr1 = Opm::RestartIO::Helpers::AggregateConnectionData(ih_lgr1.value);
    conn_aggregator_lgr1.captureDeclaredConnDataLGR(simCase.sched, simCase.es.getInputGrid(),
                                                    simCase.es.getUnits(), xw,
                                                    sim_stateLGR(), rptStep, "LGR1");


    auto conn_aggregator_lgr2 = Opm::RestartIO::Helpers::AggregateConnectionData(ih_lgr2.value);
    conn_aggregator_lgr2.captureDeclaredConnDataLGR(simCase.sched, simCase.es.getInputGrid(),
                                                    simCase.es.getUnits(), xw,
                                                    sim_stateLGR(), rptStep, "LGR2");

    // -------------------------- ICON FOR GLOBAL GRID --------------------------
    // ICON (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 0;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }
    // ICON (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 1;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    }
    // -------------------------- ICON FOR LGLS GRID --------------------------
    // ICON (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih_lgr2.niconz * ih_lgr2.ncwmax * 0;
        const auto& icon = conn_aggregator_lgr2.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }
    // ICON (INK)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        //const auto i0 = 1*.niconz;
        const auto i0 = ih_lgr1.niconz * ih_lgr1.ncwmax * 0;
        const auto& icon = conn_aggregator_lgr1.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    }

    // -------------------------- TESTING ROUTINES --------------------------
    // const double secs_elapsed = 10;
    // const auto ihw = Opm::RestartIO::Helpers::createInteHead(simCase.es, simCase.es.getInputGrid(), simCase.sched, secs_elapsed,
    //             rptStep, rptStep, rptStep);

    // auto group_aggregator1 = Opm::RestartIO::Helpers::AggregateGroupData(ihw);
    // const auto& units1    = simCase.es.getUnits();
    // group_aggregator1.captureDeclaredGroupData(simCase.sched, units1, rptStep, smry,
    //     ihw);
    // -------------------------- GROUP DATA FOR GLOBAL GRID --------------------------
    ih.add_igr_data(99,112, 181,
                    5,2, 2);
    auto group_aggregator = Opm::RestartIO::Helpers::AggregateGroupData(ih.value);
    const auto& units    = simCase.es.getUnits();
    group_aggregator.captureDeclaredGroupData(simCase.sched, units, rptStep, smry,
        ih.value);
    // -------------------------- GROUP DATA FOR LGR GRID LGR1--------------------------
    ih_lgr1.add_igr_data(99,112, 181,
                      5,2, 2);
    ih_lgr2.add_igr_data(99,112, 181,
                         5,2, 2);
    auto group_aggregator_lgr1 = Opm::RestartIO::Helpers::AggregateGroupData(ih_lgr1.value);
    auto group_aggregator_lgr2 = Opm::RestartIO::Helpers::AggregateGroupData(ih_lgr2.value);
    group_aggregator_lgr2.captureDeclaredGroupDataLGR(simCase.sched, units, rptStep, smry,
        ih.value,"LGR2");

    // -------------------------- IGR FOR GLOBAL GRID --------------------------
    // IGR (G1 GLOBAL)
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  2); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  2); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  2); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (FIELD GLOBAL)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  1); // Group FIELD - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group G1 - index of parent group (= 0 for FIELD)
    }


    // -------------------------- IGR FOR LGR GRID LGR2--------------------------
    // IGR (G1 GLOBAL)
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr2.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  1); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  2); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (FIELD GLOBAL)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr2.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  1); // Group FIELD - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group G1 - index of parent group (= 0 for FIELD)
    }

}


BOOST_AUTO_TEST_CASE (Declared_Well_DataDEFAULT)
{
    const auto simCase = SimulationCase{sim_default()};

    Opm::Action::State action_state;
    Opm::WellTestState wtest_state;

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};

    const double secs_elapsed = 100;

    const auto ih = Opm::RestartIO::Helpers::
        createInteHead(simCase.es, simCase.es.getInputGrid(), simCase.sched, secs_elapsed,
                    rptStep, rptStep, rptStep);

    const int nwells = 2;
    const int niwelz = niwelz;
    // auto ih = MockIH {
    //     static_cast<int>(simCase.sched.getWells(rptStep).size())
    // };
    // the original case has 25 entICON, 41 entSCON and 58 entWCON
    // however, OPM forces 26 and 42 when INTHEAD is created.
    // ih.add_icon_data(26, 42 ,58 , 2);


    const auto smry = sim_stateLGR();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih};

    awd.captureDeclaredWellData(simCase.sched,
                            simCase.grid,
                            simCase.es.tracer(),
                            rptStep,
                            action_state,
                            wtest_state,
                            smry,
                            ih);


    // -------------------------- IWEL FOR GLOBAL WELLS --------------------------
    // GLOBAL WELLS
    // IWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 2); // LOCATED LGR2

    }
    // GLOBAL WELLS
    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // INJ -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // INJ -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // INJ #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 3); // INJ -> Injector
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LOCATED LGR1
    }


    auto conn_aggregator = Opm::RestartIO::Helpers::AggregateConnectionData(ih);
    auto xw = Opm::data::Wells {};
    conn_aggregator.captureDeclaredConnData(simCase.sched, simCase.es.getInputGrid(),
                                            simCase.es.getUnits(), xw,
                                            sim_stateLGR(), rptStep);


    WorkArea work;
    {
        Opm::EclIO::OutputStream::Restart rstFile {
            Opm::EclIO::OutputStream::ResultSet {"./", "TEST"},
            rptStep,
            Opm::EclIO::OutputStream::Formatted {true},
            Opm::EclIO::OutputStream::Unified   {true}
        };

        const double secs_elapsed = 100;
        const double next_step_size = 10;

        const auto IH = Opm::RestartIO::Helpers::
            createInteHead(simCase.es, simCase.es.getInputGrid(), simCase.sched, secs_elapsed,
                            rptStep, rptStep, rptStep);

        const auto dh =
            Opm::RestartIO::Helpers::createDoubHead(simCase.es, simCase.sched,
                                                    rptStep, rptStep+1,
                                                    secs_elapsed, next_step_size);

        const auto& lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);

        rstFile.write("INTEHEAD", IH);
        rstFile.write("DOUBHEAD", dh);
        rstFile.write("LOGIHEAD", lh);
        {
            auto group_aggregator = Opm::RestartIO::Helpers::AggregateGroupData(IH);
            rstFile.write("IGRP", group_aggregator.getIGroup());
            rstFile.write("SGRP", group_aggregator.getSGroup());
            rstFile.write("XGRP", group_aggregator.getXGroup());
            rstFile.write("ZGRP", group_aggregator.getZGroup());
        }

        rstFile.write("IWEL", awd.getIWell());
        rstFile.write("SWEL", awd.getSWell());
        rstFile.write("XWEL", awd.getXWell());
        rstFile.write("ZWEL", awd.getZWell());
        {
            auto conn_aggregator = Opm::RestartIO::Helpers::AggregateConnectionData(IH);
            auto xw = Opm::data::Wells {};
            conn_aggregator.captureDeclaredConnData(simCase.sched, simCase.es.getInputGrid(),
                                                    simCase.es.getUnits(), xw,
                                                    sim_state(), rptStep);

            rstFile.write("ICON", conn_aggregator.getIConn());
            rstFile.write("SCON", conn_aggregator.getSConn());
            rstFile.write("XCON", conn_aggregator.getXConn());
        }
    }

    // // -------------------------- ICON FOR GLOBAL GRID --------------------------
    // // ICON (PROD)
    // {
    //     using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
    //     const auto i0 = ih.niconz * ih.ncwmax * 0;
    //     const auto& icon = conn_aggregator.getIConn();
    //     BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
    //     BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
    //     BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    // }
    // // ICON (PROD)
    // {
    //     using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
    //     const auto i0 = ih.niconz * ih.ncwmax * 1;
    //     const auto& icon = conn_aggregator.getIConn();
    //     BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // INJ    -> ICON
    //     BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
    //     BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    // }

}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (Declared_Well_Data)
{
    const auto simCase = SimulationCase{first_sim()};

    Opm::Action::State action_state;
    Opm::WellTestState wtest_state;

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{2});

    const auto smry = sim_state();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    auto tests = simCase.sched[rptStep];

    wtest_state.close_well("OP_1", Opm::WellTestConfig::Reason::PHYSICAL, 0);
    {
        auto tw = wtest_state.test_wells(simCase.sched[rptStep].wtest_config(), 86400 * 10);
        BOOST_CHECK(tw == std::vector<std::string>{"OP_1"});
    }

    awd.captureDeclaredWellData(simCase.sched,
                                simCase.es.tracer(),
                                rptStep,
                                action_state,
                                wtest_state,
                                smry,
                                ih.value);

    // IWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        namespace WTestReason = Opm::RestartIO::Helpers::VectorItems::IWell::Value;

        const auto start = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 9); // OP_1 -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 9); // OP_1 -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // OP_1/Head ->
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 2); // OP_1 #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // OP_1 -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::VFPTab], 0); // VFP defaulted -> 0
        BOOST_CHECK_EQUAL(iwell[start + Ix::WTestConfigReason], Opm::WTest::EclConfigReason::PHYSICAL * Opm::WTest::EclConfigReason::GCON * Opm::WTest::EclConfigReason::THPLimit);
        BOOST_CHECK_EQUAL(iwell[start + Ix::WTestRemaining], 3 + 1 - 1);  // Total + 1 - #attempt
        BOOST_CHECK_EQUAL(iwell[start + Ix::WTestCloseReason], Opm::WTest::EclCloseReason::PHYSICAL);
        // Completion order
        BOOST_CHECK_EQUAL(iwell[start + Ix::CompOrd], 0); // Track ordering (default)

        BOOST_CHECK_EQUAL(iwell[start + Ix::item18], -100); // M2 Magic
        BOOST_CHECK_EQUAL(iwell[start + Ix::item48], -  1); // M2 Magic
        BOOST_CHECK_EQUAL(iwell[start + Ix::item32],    7); // M2 Magic
    }

    // IWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 9); // OP_2 -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 9); // OP_2 -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 2); // OP_2/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // OP_2 #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 4); // OP_2 -> Gas Inj.
        BOOST_CHECK_EQUAL(iwell[start + Ix::VFPTab], 0); // VFP defaulted -> 0

        // Completion order
        BOOST_CHECK_EQUAL(iwell[start + Ix::CompOrd], 0); // Track ordering (default)

        BOOST_CHECK_EQUAL(iwell[start + Ix::item18], -100); // M2 Magic
        BOOST_CHECK_EQUAL(iwell[start + Ix::item48], -  1); // M2 Magic
        BOOST_CHECK_EQUAL(iwell[start + Ix::item32],    7); // M2 Magic
    }

    // SWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i0 = 0*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i0 + Ix::OilRateTarget], 20.0e3f, 1.0e-7f);

        // No WRAT limit
        BOOST_CHECK_CLOSE(swell[i0 + Ix::WatRateTarget], 1.0e20f, 1.0e-7f);

        // No GRAT limit
        BOOST_CHECK_CLOSE(swell[i0 + Ix::GasRateTarget], 1.0e20f, 1.0e-7f);

        // LRAT limit derived from ORAT + WRAT (= ORAT + 0.0)
        BOOST_CHECK_CLOSE(swell[i0 + Ix::LiqRateTarget], 1.0e20f, 1.0e-7f);

        // No direct limit, extract value from 'smry' (WVPR:OP_1)
        BOOST_CHECK_CLOSE(swell[i0 + Ix::ResVRateTarget], 1.0e20f, 1.0e-7f);

        // No THP limit
        BOOST_CHECK_CLOSE(swell[i0 + Ix::THPTarget] ,    0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::BHPTarget] , 1000.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::DatumDepth], 7050.00049f, 1.0e-7f);

        // Wtest
        BOOST_CHECK_CLOSE(swell[i0 + Ix::WTestInterval], 1, 1e-7);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::WTestStartupTime], 2, 1e-7);
    }

    // SWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i1 = 1*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i1 + Ix::THPTarget], 1.0e20f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::BHPTarget],  400.0f, 1.0e-7f);

        BOOST_CHECK_CLOSE(swell[i1 + Ix::DatumDepth], 7150.0f, 1.0e-7f);
    }

    // XWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto i0 = 0*ih.nxwelz;

        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::BHPTarget], 1000.0, 1.0e-10);
    }

    // XWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto i1 = 1*ih.nxwelz;

        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::BHPTarget], 400.0, 1.0e-10);
    }

    // ZWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;

        const auto i0 = 0*ih.nzwelz;

        const auto& zwell = awd.getZWell();

        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "OP_1    ");
    }

    // ZWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;

        const auto i1 = 1*ih.nzwelz;

        const auto& zwell = awd.getZWell();

        BOOST_CHECK_EQUAL(zwell[i1 + Ix::WellName].c_str(), "OP_2    ");
    }

    {
        WorkArea work;
        {
            Opm::EclIO::OutputStream::Restart rstFile {
                Opm::EclIO::OutputStream::ResultSet {"./", "TEST"},
                rptStep,
                Opm::EclIO::OutputStream::Formatted {false},
                Opm::EclIO::OutputStream::Unified   {true}
            };

            const double secs_elapsed = 100;
            const double next_step_size = 10;

            const auto IH = Opm::RestartIO::Helpers::
                createInteHead(simCase.es, simCase.es.getInputGrid(), simCase.sched, secs_elapsed,
                               rptStep, rptStep, rptStep);

            const auto dh =
                Opm::RestartIO::Helpers::createDoubHead(simCase.es, simCase.sched,
                                                        rptStep, rptStep+1,
                                                        secs_elapsed, next_step_size);

            const auto& lh = Opm::RestartIO::Helpers::createLogiHead(simCase.es);

            rstFile.write("INTEHEAD", IH);
            rstFile.write("DOUBHEAD", dh);
            rstFile.write("LOGIHEAD", lh);
            {
                auto group_aggregator = Opm::RestartIO::Helpers::AggregateGroupData(IH);
                rstFile.write("IGRP", group_aggregator.getIGroup());
                rstFile.write("SGRP", group_aggregator.getSGroup());
                rstFile.write("XGRP", group_aggregator.getXGroup());
                rstFile.write("ZGRP", group_aggregator.getZGroup());
            }

            rstFile.write("IWEL", awd.getIWell());
            rstFile.write("SWEL", awd.getSWell());
            rstFile.write("XWEL", awd.getXWell());
            rstFile.write("ZWEL", awd.getZWell());
            {
                auto conn_aggregator = Opm::RestartIO::Helpers::AggregateConnectionData(IH);
                auto xw = Opm::data::Wells {};
                conn_aggregator.captureDeclaredConnData(simCase.sched, simCase.es.getInputGrid(),
                                                        simCase.es.getUnits(), xw,
                                                        sim_state(), rptStep);

                rstFile.write("ICON", conn_aggregator.getIConn());
                rstFile.write("SCON", conn_aggregator.getSConn());
                rstFile.write("XCON", conn_aggregator.getXConn());
            }
        }

        auto rst_file = std::make_shared<Opm::EclIO::ERst>("TEST.UNRST");
        auto rst_view = std::make_shared<Opm::EclIO::RestartFileView>(std::move(rst_file), 1);
        auto rst_state = Opm::RestartIO::RstState::load(std::move(rst_view), simCase.es.runspec(), Opm::Parser{});

        {
            Opm::WellTestConfig wtest_config{rst_state, rptStep};
            Opm::WellTestState ws{simCase.sched.runspec().start_time(), rst_state};
            BOOST_CHECK(wtest_config.has("OP_1", Opm::WellTestConfig::Reason::PHYSICAL));
            BOOST_CHECK(wtest_config.has("OP_1", Opm::WellTestConfig::Reason::GROUP));
            BOOST_CHECK(wtest_config.has("OP_1", Opm::WellTestConfig::Reason::THP_DESIGN));

            BOOST_CHECK(ws.well_is_closed("OP_1"));
        }
    }

    // SWEL (OP_6)
    // Report Step 8: 2014-10-18 --> 2014-10-28
    const auto rptStep_8 = std::size_t{8};

    const auto ih_8 = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep_8).size())
    };

    BOOST_CHECK_EQUAL(ih_8.nwells, MockIH::Sz{6});

    //smry = sim_state();
    awd = Opm::RestartIO::Helpers::AggregateWellData{ih_8.value};
    awd.captureDeclaredWellData(simCase.sched,
                                simCase.es.tracer(),
                                rptStep_8,
                                action_state,
                                wtest_state,
                                smry,
                                ih_8.value);
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i1 = 4*ih_8.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i1 + Ix::OilRateTarget], 20000.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::WatRateTarget], 1.0e+20f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::GasRateTarget], 1.0e+20f, 1.0e-7f);


        const auto i2 = 5*ih_8.nswelz;
        BOOST_CHECK_CLOSE(swell[i2 + Ix::OilRateTarget], 275.f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::WatRateTarget], 0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::GasRateTarget], 34576.0f, 1.0e-7f);
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (WECON)
{
    const auto simCase = SimulationCase{wecon_etc_sim()};

    const auto action_state = Opm::Action::State{};
    const auto wtest_state = Opm::WellTestState{};

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{2};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{3});

    const auto smry = sim_state();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};

    awd.captureDeclaredWellData(simCase.sched,
                                simCase.es.tracer(),
                                rptStep,
                                action_state,
                                wtest_state,
                                smry,
                                ih.value);

    // IWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        namespace EconValue = Opm::RestartIO::Helpers::VectorItems::IWell::Value::EconLimit;

        const auto start = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::EconWorkoverProcedure],
                          EconValue::WOProcedure::StopOrShut);

        BOOST_CHECK_EQUAL(iwell[start + Ix::EconLimitEndRun],
                          EconValue::EndRun::No);

        BOOST_CHECK_EQUAL(iwell[start + Ix::EconLimitQuantity],
                          EconValue::Quantity::Rate);

        BOOST_CHECK_EQUAL(iwell[start + Ix::EconWorkoverProcedure_2],
                          EconValue::WOProcedure::StopOrShut);
    }

    // IWEL (OP_3)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        namespace EconValue = Opm::RestartIO::Helpers::VectorItems::IWell::Value::EconLimit;

        const auto start = 2*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::EconWorkoverProcedure],
                          EconValue::WOProcedure::ConAndBelow);

        BOOST_CHECK_EQUAL(iwell[start + Ix::EconLimitEndRun],
                          EconValue::EndRun::Yes);

        BOOST_CHECK_EQUAL(iwell[start + Ix::EconLimitQuantity],
                          EconValue::Quantity::Potential);

        BOOST_CHECK_EQUAL(iwell[start + Ix::EconWorkoverProcedure_2],
                          EconValue::WOProcedure::Plug);
    }

    // SWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i0 = 0*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i0 + Ix::EconLimitMinOil], 1.234f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::EconLimitMinGas], 12.345f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::EconLimitMaxWct], 0.87f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::EconLimitMaxGor], 210.98f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::EconLimitMaxWct_2], 0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::EconLimitMinLiq], 0.0f, 1.0e-7f);
    }

    // SWEL (OP_3)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i2 = 2*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i2 + Ix::EconLimitMinOil], 0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::EconLimitMinGas], 0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::EconLimitMaxWct], 1.0e+20f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::EconLimitMaxGor], 1.0e+20f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::EconLimitMaxWct_2], 0.56f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::EconLimitMinLiq], 10.23f, 1.0e-7f);
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (WGRUPCON)
{
    const auto simCase = SimulationCase{wecon_etc_sim()};

    const auto action_state = Opm::Action::State{};
    const auto wtest_state = Opm::WellTestState{};

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{2};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{3});

    const auto smry = sim_state();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};

    awd.captureDeclaredWellData(simCase.sched,
                                simCase.es.tracer(),
                                rptStep,
                                action_state,
                                wtest_state,
                                smry,
                                ih.value);

    // IWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        namespace GrupConValue = Opm::RestartIO::Helpers::VectorItems::IWell::Value::WGrupCon;

        const auto start = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::WGrupConControllable],
                          GrupConValue::Controllable::Yes);

        BOOST_CHECK_EQUAL(iwell[start + Ix::WGrupConGRPhase],
                          GrupConValue::GRPhase::Gas);
    }

    // IWEL (OP_3)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        namespace GrupConValue = Opm::RestartIO::Helpers::VectorItems::IWell::Value::WGrupCon;

        const auto start = 1*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::WGrupConControllable],
                          GrupConValue::Controllable::Yes);

        BOOST_CHECK_EQUAL(iwell[start + Ix::WGrupConGRPhase],
                          GrupConValue::GRPhase::Defaulted);
    }

    // IWEL (OP_3)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        namespace GrupConValue = Opm::RestartIO::Helpers::VectorItems::IWell::Value::WGrupCon;

        const auto start = 2*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::WGrupConControllable],
                          GrupConValue::Controllable::No);

        BOOST_CHECK_EQUAL(iwell[start + Ix::WGrupConGRPhase],
                          GrupConValue::GRPhase::Defaulted);
    }

    // SWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i0 = 0*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i0 + Ix::WGrupConGuideRate], 123.456f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i0 + Ix::WGrupConGRScaling], 0.75f, 1.0e-7f);
    }

    // SWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i1 = 1*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i1 + Ix::WGrupConGuideRate], -1.0e+20f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::WGrupConGRScaling], 1.0f, 1.0e-7f);
    }

    // SWEL (OP_3)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i2 = 2*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i2 + Ix::WGrupConGuideRate], 100.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i2 + Ix::WGrupConGRScaling], 1.0f, 1.0e-7f);
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (Declared_Well_Data_MSW_well_data)
{
    const auto simCase = SimulationCase{msw_sim("0A4_GRCTRL_LRAT_LRAT_GGR_BASE_MODEL2_MSW_ALL.DATA")};
    const auto rptStep = std::size_t{1};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    const auto smry = sim_state();

    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    awd.captureDeclaredWellData(simCase.sched,
                                simCase.es.tracer(),
                                rptStep,
                                Opm::Action::State{},
                                Opm::WellTestState{},
                                smry,
                                ih.value);

    // IWEL (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::MsWID] , 1); // PROD1  - first MSW well
        BOOST_CHECK_EQUAL(iwell[start + Ix::NWseg] , 12); // PROD1 - 12 segments
        BOOST_CHECK_EQUAL(iwell[start + Ix::MSW_PlossMod], 1); // PROD1 - HF- => 1
        BOOST_CHECK_EQUAL(iwell[start + Ix::MSW_MulPhaseMod] , 1); // PROD1 - HO => 1
    }

    // IWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::MsWID] , 2); // PROD2  - second MSW well
        BOOST_CHECK_EQUAL(iwell[start + Ix::NWseg] , 12); // PROD2 - 12 segments
        BOOST_CHECK_EQUAL(iwell[start + Ix::MSW_PlossMod], 0); // PROD2 - HFA => 0,
        BOOST_CHECK_EQUAL(iwell[start + Ix::MSW_MulPhaseMod] , 1); // PROD1 - HO => 0
    }

    // IWEL (PROD3)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 2*ih.niwelz;

        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::MsWID] , 3); // PROD3  - third MSW well
        BOOST_CHECK_EQUAL(iwell[start + Ix::NWseg] , 10); // PROD3 - 10 segments
        BOOST_CHECK_EQUAL(iwell[start + Ix::MSW_PlossMod], 2); // PROD3 - H-- => 2,
        BOOST_CHECK_EQUAL(iwell[start + Ix::MSW_MulPhaseMod] , 1); // PROD3 - HO => 0
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (Dynam_icWell_Data_Step1)
{
    const auto simCase = SimulationCase{first_sim()};

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    const auto xw   = well_rates_1();
    const auto smry = sim_state();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    Opm::WellTestState wtest_state;

    awd.captureDynamicWellData(simCase.sched, simCase.es.tracer(), rptStep, xw, smry);

    // IWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i0 = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();

        BOOST_CHECK_EQUAL(iwell[i0 + Ix::item9 ], iwell[i0 + Ix::ActWCtrl]);
        BOOST_CHECK_EQUAL(iwell[i0 + Ix::Status], Value::Open);
    }

    // IWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i1 = 1*ih.niwelz;

        const auto& iwell = awd.getIWell();

        //
        // These checks do not work because flow gives a well's status SHUT
        // when all the connections are shut (no flowing connections)
        // This needs to be corrected in flow

        BOOST_CHECK_EQUAL(iwell[i1 + Ix::item9 ], -1); // No flowing conns.
        BOOST_CHECK_EQUAL(iwell[i1 + Ix::Status], Value::Shut); // No flowing conns.
    }

    // XWEL (OP_1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i0 = 0*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::OilPrRate], 1.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrRate], 2.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrRate], 3.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::LiqPrRate], 1.0 + 2.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrRate], 4.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::TubHeadPr], 123.45, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::FlowBHP], 314.15 , 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatCut] ,   0.625, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GORatio], 234.5  , 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::OilPrTotal], 10.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrTotal], 20.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrTotal], 30.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrTotal], 40.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::item37],
                          xwell[i0 + Ix::WatPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::item38],
                          xwell[i0 + Ix::GasPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistOilPrTotal], 345.6, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistWatPrTotal], 456.7, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistGasPrTotal], 567.8, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistWatInjTotal], 0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistGasInjTotal], 0.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::PrimGuideRate], 4.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::PrimGuideRate], xwell[i0 + Ix::PrimGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrGuideRate], 3.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::WatPrGuideRate], xwell[i0 + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrGuideRate], 2.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::GasPrGuideRate], xwell[i0 + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrGuideRate], 6.1, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::VoidPrGuideRate], xwell[i0 + Ix::VoidPrGuideRate_2]);
    }

    // XWEL (OP_2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i1 = 1*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasPrRate], -200.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidPrRate], -1234.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::TubHeadPr], 234.5, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::FlowBHP], 400.6, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatInjTotal], 1000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasInjTotal], 2000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidInjTotal], 1234.5, 1.0e-10);

        // Bg = VGIR / GIR = 1234.0 / 200.0
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasFVF], 6.17, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::item38],
                          xwell[i1 + Ix::GasPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistOilPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatInjTotal], 1515.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasInjTotal], 3030.0, 1.0e-10);

        // Gas injector => primary guide rate == gas injection guide rate (with negative sign).
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::PrimGuideRate], -2.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::PrimGuideRate], xwell[i1 + Ix::PrimGuideRate_2]);

        // Injector => all phase production guide rates are zero
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::WatPrGuideRate], xwell[i1 + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::GasPrGuideRate], xwell[i1 + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::VoidPrGuideRate], xwell[i1 + Ix::VoidPrGuideRate_2]);
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (Dynamic_Well_Data_Step2)
{
    const auto simCase = SimulationCase{first_sim()};

    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep = std::size_t{2};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    const auto xw   = well_rates_2();
    const auto smry = sim_state();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    Opm::WellTestState wtest_state;

    awd.captureDynamicWellData(simCase.sched, simCase.es.tracer(), rptStep, xw, smry);

    // IWEL (OP_1) -- closed producer
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto i0 = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();

        BOOST_CHECK_EQUAL(iwell[i0 + Ix::item9] , 0);
        BOOST_CHECK_EQUAL(iwell[i0 + Ix::Status], 0);
    }

    // IWEL (OP_2) -- water injector
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i1 = 1*ih.niwelz;

        const auto& iwell = awd.getIWell();

        BOOST_CHECK_EQUAL(iwell[i1 + Ix::item9],
                          iwell[i1 + Ix::ActWCtrl]);
        BOOST_CHECK_EQUAL(iwell[i1 + Ix::Status], Value::Open);
    }

    // XWEL (OP_1) -- closed producer
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i0 = 0*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::OilPrRate], 1.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrRate], 2.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrRate], 3.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::LiqPrRate], 1.0 + 2.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrRate], 4.0, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::TubHeadPr], 123.45, 1.0e-10);
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

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistOilPrTotal], 345.6, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistWatPrTotal], 456.7, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i0 + Ix::HistGasPrTotal], 567.8, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::PrimGuideRate], 4.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::PrimGuideRate], xwell[i0 + Ix::PrimGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::WatPrGuideRate], 3.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::WatPrGuideRate], xwell[i0 + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::GasPrGuideRate], 2.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::GasPrGuideRate], xwell[i0 + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i0 + Ix::VoidPrGuideRate], 6.1, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i0 + Ix::VoidPrGuideRate], xwell[i0 + Ix::VoidPrGuideRate_2]);
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

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::TubHeadPr], 234.5, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::FlowBHP], 400.6, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatInjTotal], 1000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasInjTotal], 2000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidInjTotal], 1234.5, 1.0e-10);

        // Copy of WWIR
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::item37],
                          xwell[i1 + Ix::WatPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistOilPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatInjTotal], 1515.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasInjTotal], 3030.0, 1.0e-10);

        // WWVIR
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatVoidPrRate],
                          -4321.0, 1.0e-10);

        // Water injector => primary guide rate == water injection guide rate (with negative sign).
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::PrimGuideRate], -3.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::PrimGuideRate], xwell[i1 + Ix::PrimGuideRate_2]);

        // Injector => all phase production guide rates are zero
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::WatPrGuideRate], xwell[i1 + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::GasPrGuideRate], xwell[i1 + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::VoidPrGuideRate], xwell[i1 + Ix::VoidPrGuideRate_2]);
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

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::TubHeadPr], 246.9, 1.0e-10);
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

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::HistOilPrTotal], 2345.6, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::HistWatPrTotal], 3456.7, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i2 + Ix::HistGasPrTotal], 4567.8, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::PrimGuideRate], 49, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i2 + Ix::PrimGuideRate], xwell[i2 + Ix::PrimGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::WatPrGuideRate], 38.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i2 + Ix::WatPrGuideRate], xwell[i2 + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::GasPrGuideRate], 27.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i2 + Ix::GasPrGuideRate], xwell[i2 + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i2 + Ix::VoidPrGuideRate], 61.2, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i2 + Ix::VoidPrGuideRate], xwell[i2 + Ix::VoidPrGuideRate_2]);
    }
}

// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(WELL_POD)
{
    const auto simCase = SimulationCase{first_sim()};
    const auto& units = simCase.es.getUnits();

    // Report Step 2: 2011-01-20 --> 2013-06-15
    const auto rptStep = std::size_t{2};
    const auto sim_step = rptStep - 1;

    const auto sumState = Opm::SummaryState { Opm::TimeService::now(), 0.0 };

    const auto xw = well_rates_1();
    const auto ih =
        Opm::RestartIO::Helpers::createInteHead(simCase.es,
                                                simCase.grid,
                                                simCase.sched,
                                                0,
                                                sim_step,
                                                sim_step,
                                                sim_step);

    auto wellData = Opm::RestartIO::Helpers::AggregateWellData(ih);
    wellData.captureDeclaredWellData(simCase.sched, simCase.es.tracer(), sim_step,
                                     Opm::Action::State{}, Opm::WellTestState{},
                                     sumState, ih);

    wellData.captureDynamicWellData(simCase.sched, simCase.es.tracer(),
                                    sim_step, xw, sumState);

    auto connectionData = Opm::RestartIO::Helpers::AggregateConnectionData(ih);
    connectionData.captureDeclaredConnData(simCase.sched, simCase.grid, units,
                                           xw, sumState, sim_step);

    const auto& iwel = wellData.getIWell();
    const auto& swel = wellData.getSWell();
    const auto& xwel = wellData.getXWell();

    const auto& icon = connectionData.getIConn();
    const auto& scon = connectionData.getSConn();
    const auto& xcon = connectionData.getXConn();

    const auto header = Opm::RestartIO::RstHeader {
        simCase.es.runspec(), units, ih, std::vector<bool>(100), std::vector<double>(1000)
    };

    std::vector<Opm::RestartIO::RstWell> wells;

    auto zwel = std::vector<std::string>{};
    std::transform(wellData.getZWell().begin(), wellData.getZWell().end(),
                   std::back_inserter(zwel),
                   [](const auto& s8) { return s8.c_str(); });

    for (auto iw = 0; iw < header.num_wells; ++iw) {
        std::size_t zwel_offset = header.nzwelz * iw;
        std::size_t iwel_offset = header.niwelz * iw;
        std::size_t swel_offset = header.nswelz * iw;
        std::size_t xwel_offset = header.nxwelz * iw;
        std::size_t icon_offset = header.niconz * header.ncwmax * iw;
        std::size_t scon_offset = header.nsconz * header.ncwmax * iw;
        std::size_t xcon_offset = header.nxconz * header.ncwmax * iw;

        wells.emplace_back(units,
                           header,
                           "GROUP",
                           zwel.data() + zwel_offset,
                           iwel.data() + iwel_offset,
                           swel.data() + swel_offset,
                           xwel.data() + xwel_offset,
                           icon.data() + icon_offset,
                           scon.data() + scon_offset,
                           xcon.data() + xcon_offset);
    }

    // Well OP2
    const auto& well2 = wells[1];
    BOOST_CHECK_EQUAL(well2.k1k2.first, 1);
    BOOST_CHECK_EQUAL(well2.k1k2.second, 1);
    BOOST_CHECK_EQUAL(well2.ij[0], 8);
    BOOST_CHECK_EQUAL(well2.ij[1], 8);
    BOOST_CHECK_EQUAL(well2.name, "OP_2");

    BOOST_CHECK_EQUAL(well2.connections.size(), 1U);
    const auto& conn1 = well2.connections[0];
    BOOST_CHECK_EQUAL(conn1.ijk[0], 8);
    BOOST_CHECK_EQUAL(conn1.ijk[1], 8);
    BOOST_CHECK_EQUAL(conn1.ijk[2], 1);
}

BOOST_AUTO_TEST_SUITE_END()

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Extra_Effects)

namespace {

    SimulationCase wdfaccorCase()
    {
        return SimulationCase { Opm::Parser{}.parseString(R"~(
DIMENS
 10 10 10 /

START         -- 0
 19 JUN 2007 /

GRID

DXV
 10*100.0 /
DYV
 10*100.0 /
DZV
 10*10.0 /
DEPTHZ
121*2000.0 /

PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /

SCHEDULE

DATES        -- 1
 10  OKT 2008 /
/

WELSPECS
 'W1' 'G1'  3 3 2873.94 'WATER' 0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
 'W2' 'G2'  5 5 1       'OIL'   0.00 'STD' 'SHUT' 'NO' 0 'SEG' /
/

COMPDAT
 'W1'  3 3   1 1 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
 'W2'  3 3   2 2 'OPEN' 1*   32.948   0.311  3047.839 1*  1*  'X'  22.100 /
/

WDFACCOR
 'W2' 1.984e-7 -1.1045 0.0 /
/

WCONINJE
  'W1' 'WATER' 'OPEN' 'RATE' 4000.0 1* 850.0 /
/

WCONPROD
  'W2' 'OPEN' 'ORAT' 5000.0 4* 20.0 /
/

DATES        -- 2
 12  NOV 2008 /
/

END
)~") };
    }

    std::pair<Opm::data::Wells, Opm::SummaryState> dynamicStateWDFacCorr()
    {
        auto dyn_state = std::pair<Opm::data::Wells, Opm::SummaryState> {
            std::piecewise_construct,
            std::forward_as_tuple(),
            std::forward_as_tuple(Opm::TimeService::now(), 0.0)
        };

        using o = ::Opm::data::Rates::opt;

        auto& [xw, sum_state] = dyn_state;

        {
            const auto wname = std::string { "W2" };

            xw[wname].rates.set(o::wat, 1.0).set(o::oil, 2.0).set(o::gas, 3.0);
            xw[wname].bhp = 213.0*Opm::unit::barsa;
        }

        {
            const auto wname = std::string { "W1" };

            xw[wname].bhp = 234.0*Opm::unit::barsa;
            xw[wname].rates
                .set(o::wat, 5.0*Opm::unit::cubic(Opm::unit::meter)/Opm::unit::day)
                .set(o::oil, 0.0*Opm::unit::cubic(Opm::unit::meter)/Opm::unit::day)
                .set(o::gas, 0.0*Opm::unit::cubic(Opm::unit::meter)/Opm::unit::day);
        }

        return dyn_state;
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(WdFac_Correlation)
{
    const auto cse = wdfaccorCase();
    const auto& [xw, smry] = dynamicStateWDFacCorr();

    const auto ih = MockIH{ 2 };

    const auto action_state = Opm::Action::State{};
    const auto wtest_state = Opm::WellTestState{};

    const auto rptStep = std::size_t{1};

    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ ih.value };
    awd.captureDeclaredWellData(cse.sched,
                                cse.es.tracer(),
                                rptStep,
                                action_state,
                                wtest_state,
                                smry,
                                ih.value);

    // W1 (injector)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i1 = 0*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i1 + Ix::DFacCorrCoeffA], 0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::DFacCorrExpB]  , 0.0f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::DFacCorrExpC]  , 0.0f, 1.0e-7f);
    }

    // W2 (producer)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;

        const auto i1 = 1*ih.nswelz;

        const auto& swell = awd.getSWell();
        BOOST_CHECK_CLOSE(swell[i1 + Ix::DFacCorrCoeffA],  1.984e-7f, 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::DFacCorrExpB]  , -1.1045f  , 1.0e-7f);
        BOOST_CHECK_CLOSE(swell[i1 + Ix::DFacCorrExpC]  ,  0.0f     , 1.0e-7f);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Extra_Effects
