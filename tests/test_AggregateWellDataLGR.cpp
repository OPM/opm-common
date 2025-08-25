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

    Opm::Deck msw_sim(const std::string& fname)
    {
        return Opm::Parser{}.parseFile(fname);
    }

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

    Opm::Deck simLGR_1global2lgrwellmixed()
    {
        // ONE LGR Cells with wells with 1 Global Well and 2 LGR Wells in the same cell
        // 1 Global Well and 1 LGR well in G1 - 1 LGR well in G2
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
    3 2 2 3 /
    UNIFOUT
    GRID
    CARFIN
    'LGR1'  1  1  1  1  1  1  3  3  1 2/
    ENDFIN
    CARFIN
    'LGR2'  3  3  1  1  1  1  3  3  1 2/
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
        'PROD1'	'G1' 'LGR1'	3	3	8400	'OIL' /
        'PROD2'	'G2' 'LGR2'	1	1	8400	'OIL' /
    /
    WELSPECS
        'INJ'	'G1' 	2	1	8335	'GAS' /
    /
    COMPDATL
        'PROD1' 'LGR1'	3	3	1	1	'OPEN'	1*	1*	0.5 /
        'PROD2' 'LGR2'	1	1	1	1	'OPEN'	1*	1*	0.5 /
    /
    COMPDAT
        'INJ'    2	1	1	1	'OPEN'	1*	1*	0.5 /
    /
    WCONPROD
        'PROD1' 'OPEN' 'ORAT' 20000 4* 1000 /
        'PROD2' 'OPEN' 'ORAT' 20000 4* 2000 /
    /
    WCONINJE
        'INJ'	'GAS'	'OPEN'	'RATE'	100000 1* 5014 /
    /
    TSTEP
    0.1 1 31
    /
    END
            )~" };
        return Opm::Parser{}.parseString(input);
    }

    Opm::Deck simLGR_1global2lgrwell()
    {
        // ONE LGR Cells with wells with 1 Global Well and 2 LGR Wells in the same cell
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
  3 2 2 3 /
UNIFOUT
GRID
CARFIN
'LGR2'  3  3  1  1  1  1  3  3  1 2/
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
	'PROD1'	'G1' 'LGR2'	3	3	8400	'OIL' /
	'PROD2'	'G1' 'LGR2'	1	1	8400	'OIL' /
/
WELSPECS
	'INJ'	'G1' 	1	1	8335	'GAS' /
/
COMPDATL
	'PROD1' 'LGR2'	3	3	1	1	'OPEN'	1*	1*	0.5 /
	'PROD2' 'LGR2'	1	1	1	1	'OPEN'	1*	1*	0.5 /
/
COMPDAT
	'INJ'    1	1	1	1	'OPEN'	1*	1*	0.5 /
/
WCONPROD
	'PROD1' 'OPEN' 'ORAT' 20000 4* 1000 /
	'PROD2' 'OPEN' 'ORAT' 20000 4* 2000 /
/
WCONINJE
	'INJ'	'GAS'	'OPEN'	'RATE'	100000 1* 5014 /
/
TSTEP
0.1 1 31
/
END
)~" };

        return Opm::Parser{}.parseString(input);
    }


    Opm::Deck simLGR_2lgrwell()
    {
        // TWO LGR Cells with wells in each LGR.
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

    Opm::data::Wells well_ratesLGR()
    {
        using o = ::Opm::data::Rates::opt;

        auto xw = ::Opm::data::Wells{};

        {
            xw["PROD"].rates
                .set(o::wat, 1.0)
                .set(o::oil, 2.0)
                .set(o::gas, 3.0);

            xw["PROD"].connections.emplace_back();
            auto& c = xw["PROD"].connections.back();

            c.rates.set(o::wat, 1.0)
                   .set(o::oil, 2.0)
                   .set(o::gas, 3.0);
            auto& curr = xw["PROD"].current_control;
            curr.isProducer = true;
            curr.prod = ::Opm::Well::ProducerCMode::GRAT;
        }

        {
            xw["INJ"].bhp = 234.0;

            xw["INJ"].rates.set(o::gas, 5.0);
            //xw["OP_2"].connections.emplace_back();

            //auto& c = xw["OP_2"].connections.back();
            //c.rates.set(o::gas, 4.0);
            auto& curr = xw["INJ"].current_control;
            curr.isProducer = false;
            curr.inj = ::Opm::Well::InjectorCMode::RATE;
        }

        return xw;
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

} // Anonymous namespace


// --------------------------------------------------------------------

BOOST_AUTO_TEST_CASE (Declared_Well_Data2LGRWells)
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
    group_aggregator_lgr1.captureDeclaredGroupDataLGR(simCase.sched, units, rptStep, smry,
        ih.value,"LGR1");
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

    // -------------------------- IGR FOR LGR GRID LGR1--------------------------
    // IGR (G1 LGR1)
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr1.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  1); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  2); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (FIELD LGR1)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr1.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  1); // Group FIELD - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group G1 - index of parent group (= 0 for FIELD)
    }

    // -------------------------- IGR FOR LGR GRID LGR2--------------------------
    // IGR (G1 LGR2)
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
    // IGR (FIELD LGR2)
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

BOOST_AUTO_TEST_CASE (Declared_Well_Data3Wells1G2LGR)
{
    const auto simCase = SimulationCase{simLGR_1global2lgrwell()};

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

    ih.add_icon_data(26, 42 ,58 , 3);
    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{3});



    int num_lgr2 = countWells("LGR2");
    auto ih_lgr2 = MockIH {
        static_cast<int>(num_lgr2)
    };
    ih_lgr2.add_icon_data(26, 42 ,58 , 1);


    const auto smry = sim_stateLGR();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    auto awd_lgr2 = Opm::RestartIO::Helpers::AggregateWellData{ih_lgr2.value};

    awd.captureDeclaredWellData(simCase.sched,
                            simCase.grid,
                            simCase.es.tracer(),
                            rptStep,
                            action_state,
                            wtest_state,
                            smry,
                            ih.value);


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
    // IWEL (PROD1)
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
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LOCATED LGR2 (LGR2 is the only LGR well in this case)
    }
    // IWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LOCATED LGR2 (LGR2 is the only LGR well in this case)
    }
    // GLOBAL WELLS
    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 2*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // INJ -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // INJ -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // INJ #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 4); // INJ -> Injector
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 0); // GLOBAL WELL (no LGR well)
    }
    // -------------------------- IWEL FOR LGR WELLS --------------------------
    // LGR02 WELL
    // IWEL (PROD1)
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
    // IWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*ih_lgr2.niwelz;
        const auto& iwell = awd_lgr2.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 2); // LGR WELL LGR
    }

    // LGR02 WELL
    // LGWEL (PROD)
    {
        const auto& lgwel = awd_lgr2.getLGWell();
        BOOST_CHECK_EQUAL(lgwel[0] , 1); //
        BOOST_CHECK_EQUAL(lgwel[1] , 2); //
    }


    // -------------------------- ZWEL FOR GLOBAL WELLS --------------------------
    // GLOBAL WELLS
    // ZWEL (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 0*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD1   ");
    }
    // ZWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 1*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD2   ");
    }
    // ZWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i1 = 2*ih.nzwelz;
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
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD1   ");
    }
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 1*ih_lgr2.nzwelz;
        const auto& zwell = awd_lgr2.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD2   ");
    }

    auto conn_aggregator = Opm::RestartIO::Helpers::AggregateConnectionData(ih.value);
    auto xw = Opm::data::Wells {};
    conn_aggregator.captureDeclaredConnData(simCase.sched, simCase.es.getInputGrid(),
                                            simCase.es.getUnits(), xw,
                                            sim_stateLGR(), rptStep);

    auto conn_aggregator_lgr2 = Opm::RestartIO::Helpers::AggregateConnectionData(ih_lgr2.value);
    conn_aggregator_lgr2.captureDeclaredConnDataLGR(simCase.sched, simCase.es.getInputGrid(),
                                                    simCase.es.getUnits(), xw,
                                                    sim_stateLGR(), rptStep, "LGR2");

    // -------------------------- ICON FOR GLOBAL GRID --------------------------
    // ICON (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 0;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }
    // ICON (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 1;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    }
    // ICON (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 2;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    }
    // -------------------------- ICON FOR LGRS GRID --------------------------
    // ICON (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih_lgr2.niconz * ih_lgr2.ncwmax * 0;
        const auto& icon = conn_aggregator_lgr2.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }
    // ICON (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih_lgr2.niconz * ih_lgr2.ncwmax * 1;
        const auto& icon = conn_aggregator_lgr2.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }

    // -------------------------- GROUP DATA FOR GLOBAL GRID --------------------------
    ih.add_igr_data(100,112, 181,
                    5,3, 3);
    auto group_aggregator = Opm::RestartIO::Helpers::AggregateGroupData(ih.value);
    const auto& units    = simCase.es.getUnits();
    group_aggregator.captureDeclaredGroupData(simCase.sched, units, rptStep, smry,
        ih.value);
    // -------------------------- GROUP DATA FOR LGR GRID LGR1--------------------------
    auto gp = simCase.sched.restart_groups(rptStep);

    ih_lgr2.add_igr_data(100,112, 181,
                         5,3, 2);
    auto group_aggregator_lgr2 = Opm::RestartIO::Helpers::AggregateGroupData(ih_lgr2.value);
    group_aggregator_lgr2.captureDeclaredGroupDataLGR(simCase.sched, units, rptStep, smry,
        ih.value,"LGR2");


    // IGRP allocation is different for LGRs. GLOBAL use the nwgmax of the global grid,
    // while LGRs use the nwgmax of the LGR grid.
    // however inside the IGRP the ngwgmax used to count is the same the same as for GLOBAL GRID even for LGRs.
    // -------------------------- IGR FOR GLOBAL GRID --------------------------
    // IGR (G1 GLOBAL)
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  2); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2],   3); // Group G1 - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  3); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  3); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (EMPTY GLOBAL GROUP)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  0); // Group EMPTY - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group EMPTY - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2],   0); // Group EMPTY - Child group number three

        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  0); // Group EMPTY - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group EMPTY - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group EMPTY - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group EMPTY - index of parent group (= 0 for FIELD)
    }

    // IGR (FIELD GLOBAL)
    {
        auto start = 2*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group FIELD - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group FIELD - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group G1 - index of parent group (= 0 for FIELD)
    }

    // -------------------------- IGR FOR LGR GRID LGR2--------------------------
    // IGR (G1 LGR2)
    // NWGMAX FOR LGR GRID IS THE SAME AS THE GLOBAL GRID
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr2.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  2); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group G1 - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  2); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  2); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (FIELD LGR2)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr2.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group FIELD - Child group number three

        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group FIELD - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group FIELD - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group FIELD - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group FIELD - index of parent group (= 0 for FIELD)
    }

}


BOOST_AUTO_TEST_CASE (Declared_Well_Data3MixedGroupsWells)
{
    const auto simCase = SimulationCase{simLGR_1global2lgrwellmixed()};

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

    ih.add_icon_data(26, 42 ,58 , 3);
    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{3});


    int num_lgr1 = countWells("LGR1");
    int num_lgr2 = countWells("LGR2");

    auto ih_lgr1 = MockIH {
        static_cast<int>(num_lgr1)
    };
    auto ih_lgr2 = MockIH {
        static_cast<int>(num_lgr2)
    };
    ih_lgr1.add_icon_data(26, 42 ,58 , 1);
    ih_lgr2.add_icon_data(26, 42 ,58 , 1);


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
    // IWEL (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LOCATED LGR2 (LGR2 is the only LGR well in this case)
    }
    // IWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 1*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 2); // LOCATED LGR2 (LGR2 is the only LGR well in this case)
    }
    // GLOBAL WELLS
    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 2*ih.niwelz;
        const auto& iwell = awd.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 2); // INJ -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // INJ -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // INJ/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // INJ #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 4); // INJ -> Injector
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 0); // GLOBAL WELL (no LGR well)
    }
    // -------------------------- IWEL FOR LGR WELLS --------------------------
    // LGR01 WELL
    // IWEL (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih_lgr1.niwelz;
        const auto& iwell = awd_lgr1.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 3); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] ,3); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 1); // LGR WELL LGR
    }

    // LGR02 WELL
    // IWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

        const auto start = 0*ih_lgr2.niwelz;
        const auto& iwell = awd_lgr2.getIWell();
        BOOST_CHECK_EQUAL(iwell[start + Ix::IHead] , 1); // PROD -> I
        BOOST_CHECK_EQUAL(iwell[start + Ix::JHead] , 1); // PROD -> J
        BOOST_CHECK_EQUAL(iwell[start + Ix::FirstK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::LastK], 1); // PROD/Head -> K
        BOOST_CHECK_EQUAL(iwell[start + Ix::NConn] , 1); // PROD #Compl
        BOOST_CHECK_EQUAL(iwell[start + Ix::WType] , 1); // PROD -> Producer
        BOOST_CHECK_EQUAL(iwell[start + Ix::LGRIndex] , 2); // LGR WELL LGR
    }
    // INJ IS NOT IN LGRL
    // IWEL (PROD2)
    // -------------------------- LGWEL --------------------------
    // LGR01 WELL
    // LGWEL (PROD1)
    {
        const auto& lgwel = awd_lgr1.getLGWell();
        BOOST_CHECK_EQUAL(lgwel[0] , 1); //
        // UKNOWN lgrwll[1]
        //        BOOST_CHECK_EQUAL(lgwel[1] , 2); //
    }


    // LGR02 WELL
    // LGWEL (PROD2)
    {
        const auto& lgwel = awd_lgr2.getLGWell();
        BOOST_CHECK_EQUAL(lgwel[0] , 2); //
        // UKNOWN lgrwll[1]
        //         BOOST_CHECK_EQUAL(lgwel[1] , 2); //
    }


    // -------------------------- ZWEL FOR GLOBAL WELLS --------------------------
    // GLOBAL WELLS
    // ZWEL (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 0*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD1   ");
    }
    // ZWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 1*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD2   ");
    }
    // ZWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i1 = 2*ih.nzwelz;
        const auto& zwell = awd.getZWell();
        BOOST_CHECK_EQUAL(zwell[i1 + Ix::WellName].c_str(), "INJ     ");
    }
    // -------------------------- ZWEL FOR LGR WELLS --------------------------
    // LGR01 WELL
    // ZWEL (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 0*ih_lgr1.nzwelz;
        const auto& zwell = awd_lgr1.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD1   ");
    }
    // LGR01 WELL
    // ZWEL (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;
        const auto i0 = 0*ih_lgr1.nzwelz;
        const auto& zwell = awd_lgr2.getZWell();
        BOOST_CHECK_EQUAL(zwell[i0 + Ix::WellName].c_str(), "PROD2   ");
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
    // ICON (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 0;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }
    // ICON (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 1;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    }
    // ICON (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih.niconz * ih.ncwmax * 2;
        const auto& icon = conn_aggregator.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 2); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // INJ    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // INJ    -> ICON
    }
    // -------------------------- ICON FOR LGRS GRID --------------------------
    // ICON (PROD1)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih_lgr1.niconz * ih_lgr1.ncwmax * 0;
        const auto& icon = conn_aggregator_lgr1.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ], 3); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }
    // ICON (PROD2)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;
        const auto i0 = ih_lgr2.niconz * ih_lgr2.ncwmax * 0;
        const auto& icon = conn_aggregator_lgr2.getIConn();
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellI] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellJ] , 1); // PROD    -> ICON
        BOOST_CHECK_EQUAL(icon[i0 + Ix::CellK] , 1); // PROD    -> ICON
    }

    // -------------------------- GROUP DATA FOR GLOBAL GRID --------------------------
    ih.add_igr_data(100,112, 181,
                    5,3, 3);
    auto group_aggregator = Opm::RestartIO::Helpers::AggregateGroupData(ih.value);
    const auto& units    = simCase.es.getUnits();
    group_aggregator.captureDeclaredGroupData(simCase.sched, units, rptStep, smry,
        ih.value);
    // -------------------------- GROUP DATA FOR LGR GRID LGR1--------------------------
    ih_lgr1.add_igr_data(100,112, 181,
        5,3, 2);
    auto group_aggregator_lgr1 = Opm::RestartIO::Helpers::AggregateGroupData(ih_lgr1.value);
    group_aggregator_lgr1.captureDeclaredGroupDataLGR(simCase.sched, units, rptStep, smry,
    ih.value,"LGR1");
    // -------------------------- GROUP DATA FOR LGR GRID LGR2--------------------------
    ih_lgr2.add_igr_data(100,112, 181,
                         5,3, 2);
    auto group_aggregator_lgr2 = Opm::RestartIO::Helpers::AggregateGroupData(ih_lgr2.value);
    group_aggregator_lgr2.captureDeclaredGroupDataLGR(simCase.sched, units, rptStep, smry,
        ih.value,"LGR2");
    // IGRP allocation is different for LGRs. GLOBAL use the nwgmax of the global grid,
    // while LGRs use the nwgmax of the LGR grid.
    // however inside the IGRP the ngwgmax used to count is the same the same as for GLOBAL GRID even for LGRs.
    // -------------------------- IGR FOR GLOBAL GRID --------------------------
    // IGR (G1 GLOBAL)
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  3); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2],   0); // Group G1 - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  2); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  3); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (G2 GLOBAL)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  2); // Group G2 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group G2 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2],   0); // Group G2 - Child group number three

        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group G2 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G2 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G2 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  3); // Group G2 - index of parent group (= 0 for FIELD)
    }

    // IGR (FIELD GLOBAL)
    {
        auto start = 2*ih.nigrpz;
        const auto& iGrp = group_aggregator.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  2); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group FIELD - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  2); // Group FIELD - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group FIELD - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group FIELD - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group FIELD - index of parent group (= 0 for FIELD)
    }

    // -------------------------- IGR FOR LGR GRID LGR1--------------------------
    // IGR (G1 from LGR1)
    // NWGMAX FOR LGR GRID IS THE SAME AS THE GLOBAL GRID
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr1.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G1 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group G1 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group G1 - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group G1 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G1 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  2); // Group G1 - index of parent group (= 0 for FIELD)
    }
    // IGR (FIELD from LGR1)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr1.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group FIELD - Child group number three

        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group FIELD - Num of elements in group
                                                            // MUST BE FILTERED BY LGR

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group FIELD - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group FIELD - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group FIELD - index of parent group (= 0 for FIELD)
    }

    // -------------------------- IGR FOR LGR GRID LGR2--------------------------
    // IGR (G2 from LGR2)
    // NWGMAX FOR LGR GRID IS THE SAME AS THE GLOBAL GRID
    {
        auto start = 0*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr2.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group G2 - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group G2 - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group G2 - Child group number three
        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group G2 - Num of elements in group

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group G2 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group G2 - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  2); // Group G2 - index of parent group (= 0 for FIELD)
    }
    // IGR (FIELD from LGR2)
    {
        auto start = 1*ih.nigrpz;
        const auto& iGrp = group_aggregator_lgr2.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  0); // Group FIELD - Child group number two
        BOOST_CHECK_EQUAL(iGrp[start + 2] ,  0); // Group FIELD - Child group number three

        BOOST_CHECK_EQUAL(iGrp[start + 3] ,  1); // Group FIELD - Num of elements in group
                                                            // MUST BE FILTERED BY LGR

        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group FIELD - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group FIELD - Group level (FIELD level is 0)
        // Group G1 - INDEX 1 - Group FIELD INDEX 2
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  0); // Group FIELD - index of parent group (= 0 for FIELD)
    }

}


BOOST_AUTO_TEST_CASE (Declared_WellDyamicDataLGR)
{
    const auto simCase = SimulationCase{msw_sim("LGR_BASESIM2WELLS.DATA")};
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


    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    const auto ih_lgr1 = MockIH {
        static_cast<int>(countWells("LGR1"))
    };

    const auto ih_lgr2 = MockIH {
        static_cast<int>(countWells("LGR2"))
    };

    const auto xw   = well_ratesLGR();
    const auto smry = sim_stateLGR();
    auto awd = Opm::RestartIO::Helpers::AggregateWellData{ih.value};
    auto awd_lgr1 = Opm::RestartIO::Helpers::AggregateWellData{ih_lgr1.value};
    auto awd_lgr2 = Opm::RestartIO::Helpers::AggregateWellData{ih_lgr2.value};


    Opm::WellTestState wtest_state;

    awd.captureDynamicWellData(simCase.sched, simCase.es.tracer(), rptStep, xw, smry);
    awd_lgr1.captureDynamicWellDataLGR(simCase.sched, simCase.es.tracer(), rptStep, xw, smry, "LGR1");
    awd_lgr2.captureDynamicWellDataLGR(simCase.sched, simCase.es.tracer(), rptStep, xw, smry, "LGR2");

    // IWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i0 = 0*ih.niwelz;

        const auto& iwell = awd.getIWell();

        BOOST_CHECK_EQUAL(iwell[i0 + Ix::item9 ], iwell[i0 + Ix::ActWCtrl]);
        BOOST_CHECK_EQUAL(iwell[i0 + Ix::Status], Value::Open);
    }

    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i1 = 1*ih.niwelz;

        const auto& iwell = awd.getIWell();

        BOOST_CHECK_EQUAL(iwell[i1 + Ix::item9 ], -1);
        BOOST_CHECK_EQUAL(iwell[i1 + Ix::Status], Value::Shut); // No flowing conns.
    }

    // IWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i0 = 0*ih_lgr2.niwelz;

        const auto& iwell = awd_lgr2.getIWell();

        BOOST_CHECK_EQUAL(iwell[i0 + Ix::item9 ], iwell[i0 + Ix::ActWCtrl]);
        BOOST_CHECK_EQUAL(iwell[i0 + Ix::Status], Value::Open);
    }

    // IWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;
        using Value = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::Status;

        const auto i1 = 0*ih_lgr1.niwelz;

        const auto& iwell = awd_lgr1.getIWell();

        BOOST_CHECK_EQUAL(iwell[i1 + Ix::item9 ], -1);
        BOOST_CHECK_EQUAL(iwell[i1 + Ix::Status], Value::Shut); // No flowing conns.
    }

    // XWEL (PROD)
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

    // XWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i1 = 1*ih.nxwelz;
        const auto& xwell = awd.getXWell();

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasPrRate], -200.0, 1.0e-10);
        // no need to test this
        // BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidPrRate], -1234.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::TubHeadPr], 234.5, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::FlowBHP], 400.6, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatInjTotal], 1000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasInjTotal], 2000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidInjTotal], 1234.5, 1.0e-10);

        // Bg = VGIR / GIR = 1234.0 / 200.0
        // no need to test this
        //BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasFVF], 6.17, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::item38],
                          xwell[i1 + Ix::GasPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistOilPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatInjTotal], 1515.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasInjTotal], 3030.0, 1.0e-10);

        // Gas injector => primary guide rate == gas injection guide rate (with negative sign).
        // no need to test this
        //BOOST_CHECK_CLOSE(xwell[i1 + Ix::PrimGuideRate], -2.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::PrimGuideRate], xwell[i1 + Ix::PrimGuideRate_2]);

        // Injector => all phase production guide rates are zero
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::WatPrGuideRate], xwell[i1 + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::GasPrGuideRate], xwell[i1 + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidPrGuideRate], 0.0, 1.0e-10);
        BOOST_CHECK_EQUAL(xwell[i1 + Ix::VoidPrGuideRate], xwell[i1 + Ix::VoidPrGuideRate_2]);
    }

    // XWEL (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i0 = 0*ih_lgr2.nxwelz;
        const auto& xwell = awd_lgr2.getXWell();

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

    // XWEL (INJ)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

        const auto  i1 = 0*ih.nxwelz;
        const auto& xwell = awd_lgr1.getXWell();

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasPrRate], -200.0, 1.0e-10);
        // no need to test this
        // BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidPrRate], -1234.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::TubHeadPr], 234.5, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::FlowBHP], 400.6, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::WatInjTotal], 1000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasInjTotal], 2000.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::VoidInjTotal], 1234.5, 1.0e-10);

        // Bg = VGIR / GIR = 1234.0 / 200.0
        // no need to test this
        //BOOST_CHECK_CLOSE(xwell[i1 + Ix::GasFVF], 6.17, 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::item38],
                          xwell[i1 + Ix::GasPrRate], 1.0e-10);

        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistOilPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasPrTotal] ,    0.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistWatInjTotal], 1515.0, 1.0e-10);
        BOOST_CHECK_CLOSE(xwell[i1 + Ix::HistGasInjTotal], 3030.0, 1.0e-10);

        // Gas injector => primary guide rate == gas injection guide rate (with negative sign).
        // no need to test this
        //BOOST_CHECK_CLOSE(xwell[i1 + Ix::PrimGuideRate], -2.7, 1.0e-10);
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

BOOST_AUTO_TEST_SUITE_END()     // Extra_Effects
